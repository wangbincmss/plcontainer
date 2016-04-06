/* a POC to implement pl that uses containers and is language agnostic */

#include <string.h>

#include "postgres.h"

#include "common/comm_channel.h"
#include "common/messages/messages.h"
#include "fmgr.h"
#include "message_fns.h"


#include "executor/spi_priv.h"
#include "lib/stringinfo.h"
#include "nodes/makefuncs.h"
#include "parser/parse_type.h"
#include "sqlhandler.h"
#include "funcapi.h"
#include "miscadmin.h"

#include "access/xact.h"
#include "commands/trigger.h"
#include "utils/memutils.h"
#include "utils/palloc.h"
#include "utils/lsyscache.h"
#include "utils/portal.h"

#include "containers.h"
#include "postgres.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

MemoryContext pl_container_caller_context;

PG_FUNCTION_INFO_V1(plcontainer_call_handler);

/* entrypoint for all plcontainer procedures */
Datum plcontainer_call_handler(PG_FUNCTION_ARGS);

static Datum plcontainer_call_hook(PG_FUNCTION_ARGS);
static Datum plcontainer_return_result(plcontainer_result  resmsg,
                                       FunctionCallInfo    fcinfo,
                                       plcProcInfo        *pinfo);
static Datum get_array_datum(plcArray *arr, plcTypeInfo *ret_type);
//void get_tuple_store( MemoryContext oldContext, MemoryContext messageContext,
//        ReturnSetInfo *rsinfo,plcontainer_result res, int *isNull );
static void plcontainer_exception_do(error_message);
static void plcontainer_sql_do(sql_msg msg, plcConn* conn);
static void plcontainer_log_do(log_message);

Datum plcontainer_call_handler(PG_FUNCTION_ARGS) {
    Datum datumreturn;
    int ret;

    /* save caller's context */
    pl_container_caller_context = CurrentMemoryContext;

    /* Create a new memory context and switch to it */
    ret = SPI_connect();
    if (ret != SPI_OK_CONNECT)
        elog(ERROR, "[plcontainer] SPI connect error: %d (%s)", ret,
             SPI_result_code_string(ret));

    datumreturn = plcontainer_call_hook(fcinfo);

    /* Return to old memory context */
    ret = SPI_finish();
    if (ret != SPI_OK_FINISH)
        elog(ERROR, "[plcontainer] SPI finish error: %d (%s)", ret,
             SPI_result_code_string(ret));

    return datumreturn;
}

static Datum plcontainer_call_hook(PG_FUNCTION_ARGS) {
    Datum result = (Datum) 0;

    /* TODO: handle trigger requests as well */
    if (CALLED_AS_TRIGGER(fcinfo)) {
        elog(ERROR, "triggers aren't supported");
    } else {
        char        *name;
        callreq      req;
        plcConn     *conn;
        plcProcInfo *pinfo;
        int          message_type;
        int          shared = 0;

        /* By default we return NULL */
        fcinfo->isnull = true;

        pinfo = get_proc_info(fcinfo);
        req = plcontainer_create_call(fcinfo, pinfo);
        name = parse_container_meta(req->proc.src, &shared);
        conn = find_container(name);
        if (conn == NULL) {
            conn = start_container(name, shared);
        }

        plcontainer_channel_send(conn, (message)req);

        pfree(name);
        do {
            int     res = 0;
            message answer;

            res = plcontainer_channel_receive(conn, &answer);
            if (res < 0) {
                elog(ERROR, "Error receiving data from the client, %d", res);
                break;
            }

            message_type = answer->msgtype;
            switch (message_type) {
                case MT_RESULT:
                    result = plcontainer_return_result((plcontainer_result)answer, fcinfo, pinfo);
                    break;
                case MT_EXCEPTION:
                    plcontainer_exception_do((error_message)answer);
                    PG_RETURN_NULL();
                    break;
                case MT_SQL:
                    plcontainer_sql_do((sql_msg)answer, conn);
                    break;
                case MT_LOG:
                    plcontainer_log_do((log_message)answer);
                    break;
                default:
                    elog(ERROR, "Received unhandled message with type id %d "
                         "from client", message_type);
                    break;
            }
        } while (message_type == MT_SQL || message_type == MT_LOG);
    }

    return result;
}

/*
 * Processing client results message
 */
static Datum plcontainer_return_result(plcontainer_result  resmsg,
                                       FunctionCallInfo    fcinfo,
                                       plcProcInfo        *pinfo) {
    Datum        result = (Datum) 0;

    if (resmsg->rows == 1 && resmsg->cols == 1) {
        if (resmsg->data[0][0].isnull == 0) {
            fcinfo->isnull = false;
            if (pinfo->rettype.type != PLC_DATA_ARRAY) {
                /*
                 * handle non array scalars
                 */
                 switch (resmsg->types[0].type) {
                     case PLC_DATA_TEXT:
                         result   = OidFunctionCall1(pinfo->rettype.input,
                                        CStringGetDatum(resmsg->data[0][0].value));
                         break;
                     case PLC_DATA_INT1:
                         result = BoolGetDatum(*((bool*)resmsg->data[0][0].value));
                         break;
                     case PLC_DATA_INT2:
                         result = Int16GetDatum(*((int16*)resmsg->data[0][0].value));
                         break;
                     case PLC_DATA_INT4:
                         result = Int32GetDatum(*((int32*)resmsg->data[0][0].value));
                         break;
                     case PLC_DATA_INT8:
                         result = Int64GetDatum(*((int64*)resmsg->data[0][0].value));
                         break;
                     case PLC_DATA_FLOAT4:
                         result = Float4GetDatum(*((float4*)resmsg->data[0][0].value));
                         break;
                     case PLC_DATA_FLOAT8:
                         result = Float8GetDatum(*((float8*)resmsg->data[0][0].value));
                         break;
                     case PLC_DATA_ARRAY:
                         // We should never get here
                         elog(FATAL, "Array processing should be in a different branch");
                         break;
                     case PLC_DATA_UDT:
                     case PLC_DATA_RECORD:
                     default:
                          elog(ERROR, "Data type not handled yet: %d", resmsg->types[0].type);
                          break;
                 }
            } else {
                /*
                 * handle arrays
                 */
                 result = get_array_datum((plcArray*)resmsg->data[0][0].value,
                                          &pinfo->rettype);
            }
        }
    } else if (resmsg->rows > 1) {
        elog(ERROR, "Set-returning functions sare not supported yet");
    } else if (resmsg->cols > 1) {
        elog(ERROR, "Functions returning multiple columns are not supported yet");
    }
    return result;
}

static Datum get_array_datum(plcArray *arr, plcTypeInfo *ret_type)
{
    Datum         dvalue;
    Datum	     *elems;
    ArrayType    *array = NULL;
    int          *lbs = NULL;
    int           i;
    MemoryContext oldContext;

    lbs = (int*)palloc(arr->meta->ndims * sizeof(int));
    for (i = 0; i < arr->meta->ndims; i++)
        lbs[i] = 1;

    elems = palloc(sizeof(Datum) * arr->meta->size);
    for (i = 0; i < arr->meta->size; i++) {
        elems[i] = Int64GetDatum( ((long long*)arr->data)[i] );
    }

    oldContext = MemoryContextSwitchTo(pl_container_caller_context);
    array = construct_md_array(elems,
                               arr->nulls,
                               arr->meta->ndims,
                               arr->meta->dims,
                               lbs,
                               ret_type->subTypes[0].typeOid,
                               ret_type->subTypes[0].typlen,
                               ret_type->subTypes[0].typbyval,
                               ret_type->subTypes[0].typalign);

    dvalue = PointerGetDatum(array);
    MemoryContextSwitchTo(oldContext);

    pfree(lbs);
    pfree(elems);

    return dvalue;
}

/*
void get_tuple_store( MemoryContext oldContext, MemoryContext messageContext,
        ReturnSetInfo *rsinfo, plcontainer_result res, int *isNull )
{
    AttInMetadata      *attinmeta;
    Tuplestorestate    *tupstore = tuplestore_begin_heap(true, false, work_mem);
    TupleDesc          tupdesc;
    HeapTuple    typetup,
                 tuple;
    Form_pg_type type;
    Oid          typeOid;
    int32        typeMod;

    char **values;
    int i,j;

     * TODO: Returning tuple, you will not have any tuple description for the
     * function returning setof record. This needs to be fixed *
     * get the requested return tuple description *
    if (rsinfo->expectedDesc != NULL)
        tupdesc = CreateTupleDescCopy(rsinfo->expectedDesc);
    else {
        elog(ERROR, "Functions returning 'record' type are not supported yet");
        *isNull = TRUE;
        return;
    }

    for (j = 0; j < res->cols; j++) {
        parseTypeString(res->types[j], &typeOid, &typeMod);
        typetup = SearchSysCache(TYPEOID, typeOid, 0, 0, 0);

        if (!HeapTupleIsValid(typetup)) {
            MemoryContextSwitchTo(oldContext);
            MemoryContextDelete(messageContext);
            elog(FATAL, "[plcontainer] Invalid heaptuple at result return");
            // This won`t run
            *isNull=TRUE;
        }

        type = (Form_pg_type)GETSTRUCT(typetup);

        strcpy(tupdesc->attrs[j]->attname.data, res->names[j]);
        tupdesc->attrs[j]->atttypid = typeOid;
        ReleaseSysCache(typetup);
    }

    attinmeta = TupleDescGetAttInMetadata(tupdesc);

    * OK, go to work *
    rsinfo->returnMode = SFRM_Materialize;
    MemoryContextSwitchTo(oldContext);

     *
     * SFRM_Materialize mode expects us to return a NULL Datum. The actual
     * tuples are in our tuplestore and passed back through
     * rsinfo->setResult. rsinfo->setDesc is set to the tuple description
     * that we actually used to build our tuples with, so the caller can
     * verify we did what it was expecting.
     *
    rsinfo->setDesc = tupdesc;

    for (i=0; i<res->rows;i++){

        values = palloc(sizeof(char *)* res->cols);
        for (j=0; j< res->cols;j++){
            values[j] = res->data[i][j].value;
        }

        * construct the tuple *
        tuple = BuildTupleFromCStrings(attinmeta, values);
        pfree(values);

        * switch to appropriate context while storing the tuple *
        oldContext = MemoryContextSwitchTo(messageContext);

        * now store it *
        tuplestore_puttuple(tupstore, tuple);

        MemoryContextSwitchTo(oldContext);
    }
    rsinfo->setResult = tupstore;
    MemoryContextSwitchTo(oldContext);
    MemoryContextDelete(messageContext);

    *isNull = TRUE;
}
*/

static void plcontainer_log_do(log_message log) {
    int level = DEBUG1;

    if (log == NULL)
        return;
    switch (log->level) {
    case 1:
        level = DEBUG5;
        break;
    case 2:
        level = INFO;
        break;
    default:
        level = INFO;
    }

    elog(level, "[%s] -  %s ", log->category, log->message);
}

static void plcontainer_sql_do(sql_msg msg, plcConn* conn) {
    message res;
    res = handle_sql_message(msg);
    if (res != NULL)
        plcontainer_channel_send(conn, (message)res);
}

static void plcontainer_exception_do(error_message msg) {
    elog(ERROR, "exception occurred: \n %s \n %s", msg->message, msg->stacktrace);
}

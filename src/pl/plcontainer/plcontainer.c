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

static void plcontainer_exception_do(error_message);
static void plcontainer_sql_do(sql_msg msg, plcConn* conn);
static void  plcontainer_log_do(log_message);
static Datum plcontainer_call_hook(PG_FUNCTION_ARGS);
void get_tuple_store( MemoryContext oldContext, MemoryContext messageContext,
        ReturnSetInfo *rsinfo,plcontainer_result res, int *isNull );
Datum get_array_datum(plcontainer_result res, plcTypeInfo *ret_type, int col, int *isNull);
void perm_fmgr_info(Oid functionId, FmgrInfo *finfo);

Datum plcontainer_call_handler(PG_FUNCTION_ARGS) {
    Datum datumreturn;
    int ret;

    /* save caller's context */
    pl_container_caller_context = CurrentMemoryContext;

    ret = SPI_connect();
    if (ret != SPI_OK_CONNECT)
        elog(ERROR, "[plcontainer] SPI connect error: %d (%s)", ret,
             SPI_result_code_string(ret));

    datumreturn = plcontainer_call_hook(fcinfo);

    ret = SPI_finish();
    if (ret != SPI_OK_FINISH)
        elog(ERROR, "[plcontainer] SPI finish error: %d (%s)", ret,
             SPI_result_code_string(ret));

    return datumreturn;
}

static Datum plcontainer_call_hook(PG_FUNCTION_ARGS) {
    char        *name;
    callreq      req;
    int          message_type;
    plcConn     *conn;
    plcProcInfo *pinfo;
    int          shared = 0;

    /* TODO: handle trigger requests as well */
    if (CALLED_AS_TRIGGER(fcinfo)) {
        elog(ERROR, "triggers aren't supported");
    }

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
        int res = 0;
        message       answer;
        MemoryContext messageContext;
        MemoryContext oldContext;

        messageContext = AllocSetContextCreate(
            CurrentMemoryContext, "PL message context", 128000, 32, 65536);

        oldContext = MemoryContextSwitchTo(messageContext);

        res = plcontainer_channel_receive(conn, &answer);
        if (res < 0) {
            elog(ERROR, "Error receiving data from the client, %d", res);
            break;
        }

        message_type = answer->msgtype;
        switch (message_type) {
            case MT_RESULT:
                // handle elsewhere
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
            case MT_TUPLRES:
                break;
            default:
                // lets first switch back to the old context and handle the error.
                MemoryContextSwitchTo(oldContext);
                MemoryContextDelete(messageContext);
                /* pljelog(FATAL, "[plj core] received: unhandled message with type
                 * id %d", message_type); */
                // this wont run.
                PG_RETURN_NULL();
        }

        // we do not try to free messages anymore, we switch context and delete
        // the old one

        /*
         * here is how to escape from the loop
         */
        if (message_type == MT_RESULT) {
            plcontainer_result res = (plcontainer_result)answer;

            HeapTuple    typetup;
            Form_pg_type type;
            Oid          typeOid;
            Datum        rawDatum;

            /*
             * see if we can return right now
             */
            if (res->rows == 0) {
                MemoryContextSwitchTo(oldContext);
                MemoryContextDelete(messageContext);
                PG_RETURN_VOID();
            } else if (res->rows == 1 && res->cols == 1 && pinfo->rettype.type != PLC_DATA_ARRAY) {

               /*
                * handle non array and scalars
                */
                Datum    ret;
                if (res->data[0][0].isnull == 1) {
                    MemoryContextSwitchTo(oldContext);
                    MemoryContextDelete(messageContext);
                    PG_RETURN_NULL();
                }
                fcinfo->isnull=0;

                /*
                 * use the return type provided by the function
                 * to figure out the method to use to return
                 * the value
                 */
                typeOid = pinfo->rettype.typeOid;
                typetup = SearchSysCache(TYPEOID, typeOid, 0, 0, 0);
                if (!HeapTupleIsValid(typetup)) {
                    MemoryContextSwitchTo(oldContext);
                    MemoryContextDelete(messageContext);
                    elog(FATAL, "[plcontainer] Invalid heaptuple at result return");
                    // This won`t run
                    PG_RETURN_NULL();
                }

                type = (Form_pg_type)GETSTRUCT(typetup);

                switch (res->types[0].type) {
                    case PLC_DATA_TEXT:
                    case PLC_DATA_ARRAY:
                        /* TODO: temporary solution for array to make the result be cstring */
                        rawDatum = CStringGetDatum(res->data[0][0].value);
                        ret      = OidFunctionCall1(type->typinput, rawDatum);
                        break;
                    case PLC_DATA_INT1:
                        ret = BoolGetDatum(*((bool*)res->data[0][0].value));
                        break;
                    case PLC_DATA_INT2:
                        ret = Int16GetDatum(*((int16*)res->data[0][0].value));
                        break;
                    case PLC_DATA_INT4:
                        ret = Int32GetDatum(*((int32*)res->data[0][0].value));
                        break;
                    case PLC_DATA_INT8:
                        ret = Int64GetDatum(*((int64*)res->data[0][0].value));
                        break;
                    case PLC_DATA_FLOAT4:
                        ret = Float4GetDatum(*((float4*)res->data[0][0].value));
                        break;
                    case PLC_DATA_FLOAT8:
                        ret = Float8GetDatum(*((float8*)res->data[0][0].value));
                        break;
                    case PLC_DATA_UDT:
                    case PLC_DATA_RECORD:
                    default:
                         elog(ERROR, "Data Type not handled: %d", res->types[0].type);
                         fcinfo->isnull=1;
                         ret = (Datum)0;

                }
                ReleaseSysCache(typetup);

                MemoryContextSwitchTo(oldContext);
                MemoryContextDelete(messageContext);

                return ret;
            } else {
                /*
                 * here is where we handle tuples, and other non-scalar types
                 */
                Datum result;

                int isNull = FALSE;

                if ( fcinfo->flinfo->fn_retset ) {
                    // we have rows and columns
                    /*
                    ReturnSetInfo      *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
                    TODO: Not working yet. Plus we can have set-returning function with
                    a single column, not returning records
                    get_tuple_store(oldContext, messageContext, rsinfo, res, &isNull);
                    */
                    PG_RETURN_NULL();
                } else {
                    /* TODO: We get here if it is not set-returning function,
                       but in fact we can return more than just a single array! */
                    result = get_array_datum(res, &pinfo->rettype, 0, &isNull);
                    fcinfo->isnull = isNull;

                    MemoryContextSwitchTo(oldContext);
                    MemoryContextDelete(messageContext);

                    return result;
                }
            }
        }

        MemoryContextSwitchTo(oldContext);
        MemoryContextDelete(messageContext);

        if (message_type == MT_EXCEPTION) {
            elog(ERROR, "Exception caught: %s", ((error_message)answer)->message);
            PG_RETURN_NULL();
        }
    } while (1);

    PG_RETURN_NULL();
}

Datum get_array_datum(plcontainer_result res, plcTypeInfo *ret_type, int col,  int *isNull)
{
    bool        typbyval;
    char        typalign;
    Oid         typeOid;
    int16       typlen;
    char        typdelim;
    Oid         typinput,
                typelem;
    FmgrInfo    inputproc;
    Datum      *dvalues = NULL;
    Datum       dvalue;
    ArrayType  *array = NULL;
    int         ndims;
    int        *dims = NULL;
    int        *lbs = NULL;
    bool       *nulls = NULL;
    bool        have_nulls = FALSE;
    int         i;
    plcArray   *arr;

    arr = (plcArray*)res->data[0][col].value;
    ndims = arr->meta->ndims;
    dims = (int*)palloc(ndims * sizeof(int));
    lbs = (int*)palloc(ndims * sizeof(int));
    for (i = 0; i < ndims; i++) {
        dims[i] = arr->meta->dims[i];
        lbs[i] = 1;
    }

    /*
    * get the type of the column from the result
    */
    typeOid = get_element_type(ret_type->typeOid);

    get_type_io_data(typeOid, IOFunc_input,
                    &typlen,   &typbyval, &typalign,
                    &typdelim, &typelem,  &typinput);

    /*
     * get the input proc
     */
    perm_fmgr_info(typinput, &inputproc);

    dvalues = (Datum *) palloc(arr->meta->size * sizeof(Datum));
    nulls = (bool *) palloc(arr->meta->size * sizeof(bool));

    for(i = 0; i < arr->meta->size; i++){
        if (arr->nulls[i]){
            nulls[i] = TRUE;
            have_nulls |= TRUE;
        } else {
            nulls[i] = FALSE;
            dvalues[i] = FunctionCall3(&inputproc,
                                       CStringGetDatum(((char**)arr->data)[i]),
                                       (Datum) 0,
                                       Int32GetDatum(-1));
        }
    }

    if (!have_nulls) {
        array = construct_md_array((Datum *)dvalues, NULL, ndims, dims, lbs,
                                    typeOid, typlen, typbyval, typalign);
    } else {
        array = construct_md_array((Datum *)dvalues, (bool *)nulls, ndims, dims, lbs,
                                    typeOid, typlen, typbyval, typalign);
    }

    dvalue = PointerGetDatum(array);
    pfree(nulls);
    pfree(dvalues);

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

void
perm_fmgr_info(Oid functionId, FmgrInfo *finfo)
{
    fmgr_info_cxt(functionId, finfo, TopMemoryContext);
    finfo->fn_mcxt = pl_container_caller_context;
    finfo->fn_expr = (Node *) NULL;
}
void
plcontainer_log_do(log_message log) {
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

void plcontainer_sql_do(sql_msg msg, plcConn* conn) {
    message res;
    res = handle_sql_message(msg);
    if (res != NULL)
        plcontainer_channel_send(conn, (message)res);
}

void plcontainer_exception_do(error_message msg) {
    elog(ERROR, "exception occurred: \n %s \n %s", msg->message, msg->stacktrace);
}

/* Postgres Headers */
#include "postgres.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "commands/trigger.h"

/* PLContainer Headers */
#include "common/comm_channel.h"
#include "common/messages/messages.h"
#include "message_fns.h"
#include "sqlhandler.h"
#include "containers.h"
#include "plc_typeio.h"
#include "plcontainer.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(plcontainer_call_handler);

static Datum plcontainer_call_hook(PG_FUNCTION_ARGS);
static Datum plcontainer_process_result(plcontainer_result  resmsg,
                                        FunctionCallInfo    fcinfo,
                                        plcProcInfo        *pinfo);
static void plcontainer_process_exception(error_message);
static void plcontainer_process_sql(sql_msg msg, plcConn* conn);
static void plcontainer_process_log(log_message);

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
                    result = plcontainer_process_result((plcontainer_result)answer, fcinfo, pinfo);
                    break;
                case MT_EXCEPTION:
                    plcontainer_process_exception((error_message)answer);
                    PG_RETURN_NULL();
                    break;
                case MT_SQL:
                    plcontainer_process_sql((sql_msg)answer, conn);
                    break;
                case MT_LOG:
                    plcontainer_process_log((log_message)answer);
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
static Datum plcontainer_process_result(plcontainer_result  resmsg,
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

/*
 * Processing client log message
 */
static void plcontainer_process_log(log_message log) {
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

/*
 * Processing client SQL query message
 */
static void plcontainer_process_sql(sql_msg msg, plcConn* conn) {
    message res;
    res = handle_sql_message(msg);
    if (res != NULL)
        plcontainer_channel_send(conn, (message)res);
}

/*
 * Processing client exception message
 */
static void plcontainer_process_exception(error_message msg) {
    elog(ERROR, "exception occurred: \n %s \n %s", msg->message, msg->stacktrace);
}

/**
 * SQL message handler implementation.
 *
 */

#include "postgres.h"
#include "executor/spi.h"

#include "common/comm_utils.h"
#include "common/comm_channel.h"
#include "plc_typeio.h"
#include "sqlhandler.h"

int msg_handler_init = 0;

typedef struct {
    int   event_type;
    char *desc;
    message (*handler)(sql_msg);
} * plcontainer_handler;

static plcontainer_handler handlertab = NULL;

static message handle_invalid_message(sql_msg msg);
static message handle_statement_message(sql_msg msg);
static plcontainer_result create_sql_result(void);

static message handle_invalid_message(sql_msg msg) {
    elog(ERROR, "[plcontainer core] invalid message type: %d", msg->sqltype);
    return NULL;
}

static message handle_statement_message(sql_msg msg) {
    int retval;

    //elog(DEBUG1, "Runing statement: %s",
    //     ((sql_msg_statement)msg)->statement);
    retval = SPI_exec(((sql_msg_statement)msg)->statement, 0);
    switch (retval) {
        case SPI_OK_SELECT:
        case SPI_OK_INSERT_RETURNING:
        case SPI_OK_DELETE_RETURNING:
        case SPI_OK_UPDATE_RETURNING:
            /* some data was returned back */
            return (message)create_sql_result();
            break;
        default:
            lprintf(ERROR, "cannot handle non-select sql at the moment");
            break;
    }

    /* we shouldn't get here */
    abort();
}

static plcontainer_result create_sql_result() {
    plcontainer_result  result;
    int                 i, j;
    SPITupleTable      *res_tuptable;
    plcTypeInfo        *resTypes;

    res_tuptable = SPI_tuptable;
    SPI_tuptable = NULL;

    result          = palloc(sizeof(*result));
    result->msgtype = MT_RESULT;
    result->cols    = res_tuptable->tupdesc->natts;
    result->rows    = SPI_processed;
    result->types   = palloc(result->cols * sizeof(*result->types));
    result->names   = palloc(result->cols * sizeof(*result->names));
    resTypes        = palloc(result->cols * sizeof(plcTypeInfo));
    for (j = 0; j < result->cols; j++) {
        fill_type_info(NULL, res_tuptable->tupdesc->attrs[j]->atttypid, &resTypes[j]);
        copy_type_info(&result->types[j], &resTypes[j]);
        result->names[j] = SPI_fname(res_tuptable->tupdesc, j + 1);
    }

    if (result->rows == 0) {
        result->data = NULL;
    } else {
        bool  isnull;
        Datum origval;

        result->data = palloc(sizeof(*result->data) * result->rows);
        for (i = 0; i < result->rows; i++) {
            result->data[i] = palloc(result->cols * sizeof(*result->data[i]));
            for (j = 0; j < result->cols; j++) {
                origval = heap_getattr(res_tuptable->vals[i],
                                       j + 1,
                                       res_tuptable->tupdesc,
                                       &isnull);
                if (isnull) {
                    result->data[i][j].isnull = 1;
                    result->data[i][j].value = NULL;
                } else {
                    result->data[i][j].isnull = 0;
                    result->data[i][j].value = resTypes[j].outfunc(origval, &resTypes[j]);
                }
            }
        }
    }

    for (i = 0; i < result->cols; i++) {
        free_type_info(&resTypes[i]);
    }
    pfree(resTypes);

    return result;
}

static void message_handler_init() {
    int i;

    //elog(DEBUG1, "[pl-j - sql] initializing handler table");

    handlertab = malloc(SQL_TYPE_MAX * sizeof(*handlertab));

    for (i = 0; i < SQL_TYPE_MAX; i++) {
        handlertab[i].desc    = "Invalid or not handled type";
        handlertab[i].handler = handle_invalid_message;
    }

    handlertab[SQL_TYPE_STATEMENT].desc    = "statement message handler";
    handlertab[SQL_TYPE_STATEMENT].handler = handle_statement_message;

    //elog(DEBUG1, "[plcontainer - sql] init done");
    msg_handler_init = 1;
}

message handle_sql_message(sql_msg msg) {
    int msg_type = msg->sqltype;

    if (!msg_handler_init)
        message_handler_init();

    if (msg_type > SQL_TYPE_MAX)
        return handle_invalid_message(msg);

    //elog(DEBUG1, "calling handler: %d", msg_type);
    //elog(DEBUG1, "desc: %s", handlertab[msg_type].desc);
    return handlertab[msg_type].handler(msg);
}

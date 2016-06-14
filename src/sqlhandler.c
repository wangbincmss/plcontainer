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

static plcMsgResult *create_sql_result(void);

static plcMsgResult *create_sql_result() {
    plcMsgResult  *result;
    int            i, j;
    SPITupleTable *res_tuptable;
    plcTypeInfo   *resTypes;

    res_tuptable = SPI_tuptable;
    SPI_tuptable = NULL;

    result          = palloc(sizeof(plcMsgResult));
    result->msgtype = MT_RESULT;
    result->cols    = res_tuptable->tupdesc->natts;
    result->rows    = SPI_processed;
    result->types   = palloc(result->cols * sizeof(*result->types));
    result->names   = palloc(result->cols * sizeof(*result->names));
    result->exception_callback = NULL;
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

plcMessage *handle_sql_message(plcMsgSQL *msg) {
    int retval;

    retval = SPI_exec(msg->statement, 0);
    switch (retval) {
        case SPI_OK_SELECT:
        case SPI_OK_INSERT_RETURNING:
        case SPI_OK_DELETE_RETURNING:
        case SPI_OK_UPDATE_RETURNING:
            /* some data was returned back */
            return (plcMessage*)create_sql_result();
            break;
        default:
            lprintf(ERROR, "cannot handle non-select sql at the moment");
            break;
    }

    /* we shouldn't get here */
    abort();
}
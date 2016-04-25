#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "pycall.h"
#include "pyerror.h"
#include "pyconversions.h"

#include <Python.h>

static plcontainer_result receive_from_backend();

static plcontainer_result receive_from_backend() {
    message  resp;
    int      res = 0;
    plcConn *conn = plcconn_global;

    res = plcontainer_channel_receive(conn, &resp);
    if (res < 0) {
        lprintf (ERROR, "Error receiving data from the backend, %d", res);
        return NULL;
    }

    switch (resp->msgtype) {
       case MT_CALLREQ:
          handle_call((callreq)resp, conn);
          free_callreq((callreq)resp, 0);
          return receive_from_backend();
       case MT_RESULT:
           break;
       default:
           lprintf(WARNING, "didn't receive result back %c", resp->msgtype);
           return NULL;
    }
    return (plcontainer_result)resp;
}

/* plpy methods */
PyObject *plpy_execute(PyObject *self UNUSED, PyObject *pyquery) {
    int                 i, j;
    sql_msg_statement   msg;
    plcontainer_result  resp;
    PyObject           *pyresult,
                       *pydict,
                       *pyval;
    plcPyResult        *result;
    plcConn            *conn = plcconn_global;

    if (!PyString_Check(pyquery)) {
        raise_execution_error(conn, "plpy expected the query string");
        return NULL;
    }

    /* If the execution was terminated we don't need to proceed with SPI */
    if (plc_is_execution_terminated != 0) {
        return NULL;
    }

    msg            = malloc(sizeof(*msg));
    msg->msgtype   = MT_SQL;
    msg->sqltype   = SQL_TYPE_STATEMENT;
    msg->statement = PyString_AsString(pyquery);

    plcontainer_channel_send(conn, (message)msg);

    /* we don't need it anymore */
    free(msg);

    resp = receive_from_backend();
    if (resp == NULL) {
        raise_execution_error(conn, "Error receiving data from backend");
        return NULL;
    }

    result = plc_init_result_conversions(resp);

    /* convert the result set into list of dictionaries */
    pyresult = PyList_New(result->res->rows);
    if (pyresult == NULL) {
        raise_execution_error(conn, "Cannot allocate new list object in Python");
        free_result(resp);
        plc_free_result_conversions(result);
        return NULL;
    }

    for (j = 0; j < result->res->cols; j++) {
        if (result->inconv[j].inputfunc == NULL) {
            raise_execution_error(conn, "Type %d is not yet supported by Python container",
                                  (int)result->res->types[j].type);
            free_result(resp);
            plc_free_result_conversions(result);
            return NULL;
        }
    }

    for (i = 0; i < result->res->rows; i++) {
        pydict = PyDict_New();

        for (j = 0; j < result->res->cols; j++) {
            pyval = result->inconv[j].inputfunc(result->res->data[i][j].value);

            if (PyDict_SetItemString(pydict, result->res->names[j], pyval) != 0) {
                raise_execution_error(conn, "Error setting result dictionary element",
                                      (int)result->res->types[j].type);
                free_result(resp);
                plc_free_result_conversions(result);
                return NULL;
            }
        }

        if (PyList_SetItem(pyresult, i, pydict) != 0) {
            raise_execution_error(conn, "Error setting result list element",
                                  (int)result->res->types[j].type);
            free_result(resp);
            plc_free_result_conversions(result);
            return NULL;
        }
    }

    free_result(resp);
    plc_free_result_conversions(result);

    return pyresult;
}
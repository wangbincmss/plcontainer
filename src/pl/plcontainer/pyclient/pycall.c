#include <stdlib.h>
#include <string.h>

#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "common/comm_connectivity.h"
#include "pycall.h"
#include "pyerror.h"
#include "pyconversions.h"

#include <Python.h>
/*
  Resources:

  1. https://docs.python.org/2/c-api/reflection.html
 */

static char *create_python_func(callreq req);
static PyObject *plpy_execute(PyObject *self UNUSED, PyObject *pyquery);
static PyObject *arguments_to_pytuple (plcConn *conn, plcPyFunction *pyfunc);
static int process_call_results(plcConn *conn, PyObject *retval, plcPyFunction *pyfunc);
static plcontainer_result receive_from_backend();

static PyMethodDef moddef[] = {
    {"execute", plpy_execute, METH_O, NULL}, {NULL},
};

static plcConn* plcconn;

void python_init() {
    PyObject *plpymod, *mainmod;

    Py_SetProgramName("PythonContainer");
    Py_Initialize();

    /* create the plpy module */
    plpymod = Py_InitModule("plpy", moddef);

    mainmod = PyImport_ImportModule("__main__");
    PyModule_AddObject(mainmod, "plpy", plpymod);
    Py_DECREF(mainmod);
}

void handle_call(callreq req, plcConn *conn) {
    char          *func;
    PyObject      *val = NULL;
    PyObject      *retval = NULL;
    PyObject      *dict = NULL;
    PyObject      *args = NULL;
    plcPyFunction *pyfunc = NULL;

    /*
     * Keep our connection for future calls from Python back to us.
     */
    plcconn = conn;

    /* import __main__ to get the builtin functions */
    val = PyImport_ImportModule("__main__");
    if (val == NULL) {
        raise_execution_error(conn, "Cannot import '__main__' module in Python");
        return;
    }

    dict = PyModule_GetDict(val);
    dict = PyDict_Copy(dict);
    if (dict == NULL) {
        raise_execution_error(conn, "Cannot get '__main__' module contents in Python");
        return;
    }

    /* wrap the input in a function and evaluate the result */
    func = create_python_func(req);

    /* the function will be in the dictionary because it was wrapped with "def proc.name:... " */
    val = PyRun_String(func, Py_single_input, dict, dict);
    if (val == NULL) {
        raise_execution_error(conn, "Cannot compile function in Python");
        return;
    }
    Py_DECREF(val);
    free(func);

    /* get the function from the global dictionary */
    val = PyDict_GetItemString(dict, req->proc.name);
    Py_INCREF(val);
    if (!PyCallable_Check(val)) {
        raise_execution_error(conn, "Object produced by function is not callable");
        return;
    }

    pyfunc = plc_py_init_function(req);
    args = arguments_to_pytuple(conn, pyfunc);
    if (args == NULL) {
        plc_py_free_function(pyfunc);
        return;
    }

    /* call the function */
    retval = PyObject_Call(val, args, NULL);
    if (retval == NULL) {
        plc_py_free_function(pyfunc);
        raise_execution_error(conn, "Function produced NULL output");
        return;
    }
    process_call_results(conn, retval, pyfunc);

    plc_py_free_function(pyfunc);
    Py_XDECREF(args);
    Py_XDECREF(dict);
    Py_XDECREF(val);
    Py_XDECREF(retval);
    return;
}

static char *create_python_func(callreq req) {
    int         i, plen;
    const char *sp;
    char *      mrc, *mp;
    size_t      mlen, namelen;
    const char *src, *name;

    name = req->proc.name;
    src  = req->proc.src;

    /*
     * room for function source, the def statement, and the function call.
     *
     * note: we need to allocate strlen(src) * 2 since we replace
     * newlines with newline followed by tab (i.e. "\n\t")
     */
    namelen = strlen(name);
    /* source */
    mlen = sizeof("def ") + namelen + sizeof("():\n\t") + (strlen(src) * 2);
    /* function delimiter*/
    mlen += sizeof("\n\n");
    /* room for n-1 commas and the n argument names */
    mlen += req->nargs - 1;
    for (i = 0; i < req->nargs; i++) {
        mlen += strlen(req->args[i].name);
    }
    mlen += 1; /* null byte */

    mrc  = malloc(mlen);
    plen = snprintf(mrc, mlen, "def %s(", name);
    assert(plen >= 0 && ((size_t)plen) < mlen);

    sp = src;
    mp = mrc + plen;

    for (i = 0; i < req->nargs; i++) {
        if (i > 0)
            mp += snprintf(mp, mlen - (mp - mrc), ",%s", req->args[i].name);
        else
            mp += snprintf(mp, mlen - (mp - mrc), "%s", req->args[i].name);
    }

    mp += snprintf(mp, mlen - (mp - mrc), "):\n\t");
    /* replace newlines with newline+tab */
    while (*sp != '\0') {
        if (*sp == '\r' && *(sp + 1) == '\n')
            sp++;

        if (*sp == '\n' || *sp == '\r') {
            *mp++ = '\n';
            *mp++ = '\t';
            sp++;
        } else
            *mp++ = *sp++;
    }
    /* finish the function definition with 2 newlines */
    *mp++ = '\n';
    *mp++ = '\n';
    *mp++ = '\0';

    assert(mp <= mrc + mlen);

    return mrc;
}

static plcontainer_result receive_from_backend() {
    message resp;
    int     res = 0;

    res = plcontainer_channel_receive(plcconn, &resp);
    if (res < 0) {
        lprintf (ERROR, "Error receiving data from the backend, %d", res);
        return NULL;
    }

    switch (resp->msgtype) {
       case MT_CALLREQ:
          handle_call((callreq)resp, plcconn);
          free_callreq((callreq)resp);
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

static PyObject *plpy_execute(PyObject *self UNUSED, PyObject *pyquery) {
    int                 i, j;
    sql_msg_statement   msg;
    plcontainer_result  resp;
    PyObject           *pyresult,
                       *pydict,
                       *pyval;
    plcPyResult        *result;

    if (!PyString_Check(pyquery)) {
        raise_execution_error(plcconn, "plpy expected the query string");
        return NULL;
    }

    msg            = malloc(sizeof(*msg));
    msg->msgtype   = MT_SQL;
    msg->sqltype   = SQL_TYPE_STATEMENT;
    msg->statement = PyString_AsString(pyquery);

    plcontainer_channel_send(plcconn, (message)msg);

    /* we don't need it anymore */
    free(msg);

    resp = receive_from_backend();
    if (resp == NULL) {
        raise_execution_error(plcconn, "Error receiving data from backend");
        return NULL;
    }

    result = plc_init_result_conversions(resp);

    /* convert the result set into list of dictionaries */
    pyresult = PyList_New(result->res->rows);
    if (pyresult == NULL) {
        raise_execution_error(plcconn, "Cannot allocate new list object in Python");
        free_result(resp);
        plc_free_result_conversions(result);
        return NULL;
    }

    for (j = 0; j < result->res->cols; j++) {
        if (result->inconv[j].inputfunc == NULL) {
            raise_execution_error(plcconn, "Type %d is not yet supported by Python container",
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
                raise_execution_error(plcconn, "Error setting result dictionary element",
                                      (int)result->res->types[j].type);
                free_result(resp);
                plc_free_result_conversions(result);
                return NULL;
            }
        }

        if (PyList_SetItem(pyresult, i, pydict) != 0) {
            raise_execution_error(plcconn, "Error setting result list element",
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

static PyObject *arguments_to_pytuple (plcConn *conn, plcPyFunction *pyfunc) {
    PyObject *args;
    int i;

    args = PyTuple_New(pyfunc->nargs);
    for (i = 0; i < pyfunc->nargs; i++) {
        PyObject *arg = NULL;

        if (pyfunc->call->args[i].data.isnull) {
            Py_INCREF(Py_None);
            arg = Py_None;
        } else {
            if (pyfunc->args[i].conv.inputfunc == NULL) {
                raise_execution_error(conn,
                                      "Parameter '%s' type %d is not supported",
                                      pyfunc->args[i].name,
                                      pyfunc->args[i].type);
                return NULL;
            }
            arg = pyfunc->args[i].conv.inputfunc(pyfunc->call->args[i].data.value);
        }
        if (arg == NULL) {
            raise_execution_error(conn,
                                  "Converting parameter '%s' to Python type failed",
                                  pyfunc->args[i].name);
            return NULL;
        }

        if (PyTuple_SetItem(args, i, arg) != 0) {
            raise_execution_error(conn,
                                  "Setting Python list element %d for argument '%s' has failed",
                                  i, pyfunc->args[i].name);
            return NULL;
        }
    }
    return args;
}

static int process_call_results(plcConn *conn, PyObject *retval, plcPyFunction *pyfunc) {
    plcontainer_result res;

    /* allocate a result */
    res          = malloc(sizeof(*res));
    res->msgtype = MT_RESULT;
    res->names   = malloc(sizeof(*res->names));
    res->types   = malloc(sizeof(*res->types));
    res->rows = res->cols = 1;
    res->data    = malloc(sizeof( *res->data) * res->rows);
    res->data[0] = malloc(sizeof(**res->data) * res->cols);
    plc_py_copy_type(&res->types[0], &pyfunc->res);
    res->names[0] = pyfunc->res.name;

    if (retval == Py_None) {
        res->data[0][0].isnull = 1;
        res->data[0][0].value = NULL;
        Py_DECREF(Py_None);
    } else {
        int ret = 0;
        res->data[0][0].isnull = 0;
        if (pyfunc->res.conv.outputfunc == NULL) {
            raise_execution_error(plcconn,
                                  "Type %d is not yet supported by Python container",
                                  (int)res->types[0].type);
            free_result(res);
            return -1;
        }
        ret = pyfunc->res.conv.outputfunc(retval, &res->data[0][0].value);
        if (ret != 0) {
            raise_execution_error(plcconn,
                                  "Exception raised converting function output to function output type %d",
                                  (int)res->types[0].type);
            free_result(res);
            return -1;
        }
    }

    /* send the result back */
    plcontainer_channel_send(conn, (message)res);

    free_result(res);

    return 0;
}
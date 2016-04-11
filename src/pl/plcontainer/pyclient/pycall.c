#include <stdlib.h>
#include <string.h>

#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "common/comm_connectivity.h"
#include "pycall.h"
#include "pyerror.h"
#include "pyconversions.h"
#include "pylogging.h"
#include "pyspi.h"

#include <Python.h>
/*
  Resources:

  1. https://docs.python.org/2/c-api/reflection.html
 */

static char *create_python_func(callreq req);
static PyObject *arguments_to_pytuple (plcConn *conn, plcPyFunction *pyfunc);
static int process_call_results(plcConn *conn, PyObject *retval, plcPyFunction *pyfunc);
static int fill_rawdata(rawdata *res, plcConn *conn, PyObject *retval, plcPyFunction *pyfunc);

static PyMethodDef moddef[] = {
    /*
     * logging methods
     */
    {"debug",   plpy_debug,   METH_VARARGS, NULL},
    {"log",     plpy_log,     METH_VARARGS, NULL},
    {"info",    plpy_info,    METH_VARARGS, NULL},
    {"notice",  plpy_notice,  METH_VARARGS, NULL},
    {"warning", plpy_warning, METH_VARARGS, NULL},
    {"error",   plpy_error,   METH_VARARGS, NULL},
    {"fatal",   plpy_fatal,   METH_VARARGS, NULL},

    /*
     * query execution
     */
    {"execute", plpy_execute, METH_O,      NULL},

    {NULL, NULL, 0, NULL}
};

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
    plcconn_global = conn;

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
    plc_is_execution_terminated = 0;
    retval = PyObject_Call(val, args, NULL);
    if (retval == NULL) {
        plc_py_free_function(pyfunc);
        raise_execution_error(conn, "Function produced NULL output");
        return;
    }

    if (plc_is_execution_terminated == 0) {
        process_call_results(conn, retval, pyfunc);
    }

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
    int                retcode = 0;

    /* allocate a result */
    res           = malloc(sizeof(str_plcontainer_result));
    res->msgtype  = MT_RESULT;
    res->names    = malloc(1 * sizeof(char*));
    res->names[0] = pyfunc->res.name;
    res->types    = malloc(1 * sizeof(plcType));
    res->data     = NULL;
    plc_py_copy_type(&res->types[0], &pyfunc->res);

    /* Now we support only functions returning single column */
    res->cols = 1;

    if (pyfunc->retset) {
        int       i      = 0;
        int       len    = 0;
        PyObject *retobj = retval;

        if (PySequence_Check(retval)) {
            len = PySequence_Length(retval);
        } else {
            PyObject *iter = PyObject_GetIter(retval);
            PyObject *obj  = NULL;

            if (retobj == NULL) {
                raise_execution_error(conn,
                                      "Cannot get iterator out of the returned object");
                free_result(res);
                return -1;
            }

            retobj = PyList_New(0);
            obj = PyIter_Next(iter);
            while (obj != NULL && retcode == 0) {
                len += 1;
                retcode = PyList_Append(retobj, obj);
                obj = PyIter_Next(iter);
            }

            if (retcode < 0) {
                raise_execution_error(conn, "Error receiving result data from Python iterator");
                free_result(res);
                return -1;
            }
        }

        res->rows = len;
        res->data = malloc(res->rows * sizeof(rawdata*));
        for(i = 0; i < len && retcode == 0; i++) {
            res->data[i] = malloc(res->cols * sizeof(rawdata));
            retcode = fill_rawdata(&res->data[i][0], conn, PySequence_GetItem(retobj, i), pyfunc);
        }
    } else {
        res->rows = 1;
        res->data    = malloc(res->rows * sizeof(rawdata*));
        res->data[0] = malloc(res->cols * sizeof(rawdata));
        retcode = fill_rawdata(&res->data[0][0], conn, retval, pyfunc);
    }

    /* If the output operation succeeded we send the result back */
    if (retcode == 0) {
        plcontainer_channel_send(conn, (message)res);
    }

    free_result(res);

    return retcode;
}

static int fill_rawdata(rawdata *res, plcConn *conn, PyObject *retval, plcPyFunction *pyfunc) {
    if (retval == Py_None) {
        res->isnull = 1;
        res->value  = NULL;
    } else {
        int ret = 0;
        res->isnull = 0;
        if (pyfunc->res.conv.outputfunc == NULL) {
            raise_execution_error(conn,
                                  "Type %d is not yet supported by Python container",
                                  (int)pyfunc->res.type);
            return -1;
        }
        ret = pyfunc->res.conv.outputfunc(retval, &res->value, &pyfunc->res);
        if (ret != 0) {
            raise_execution_error(conn,
                                  "Exception raised converting function output to function output type %d",
                                  (int)pyfunc->res.type);
            return -1;
        }
    }
    return 0;
}
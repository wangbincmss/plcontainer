#include <stdlib.h>
#include <string.h>

#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "common/comm_connectivity.h"
#include "pycall.h"

#include <Python.h>
/*
  Resources:

  1. https://docs.python.org/2/c-api/reflection.html
 */

static char *create_python_func(callreq req);
static PyObject *plpy_execute(PyObject *self UNUSED, PyObject *pyquery);
static PyObject *arguments_to_pytuple (callreq req);
static int process_call_results(plcConn *conn, PyObject *retval, plcDatatype type);
static long long python_get_longlong(PyObject *obj);
static double python_get_double(PyObject *obj);
static char *get_python_error();
static void raise_execution_error (plcConn *conn, const char *format, ...);

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
    char            *func;
    PyObject        *val = NULL;
    PyObject        *retval = NULL;
    PyObject        *dict = NULL;
    PyObject        *args = NULL;

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

    args = arguments_to_pytuple(req);
    if (args == NULL) {
        return;
    }

    /* call the function */
    retval = PyObject_Call(val, args, NULL);
    if (retval == NULL) {
        raise_execution_error(conn, "Function produced NULL output");
        return;
    }
    process_call_results(conn, retval, req->retType);

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

/* plpy methods */

static PyObject *plpy_execute(PyObject *self UNUSED, PyObject *pyquery) {
    int                 i, j;
    int                 res = 0;
    sql_msg_statement   msg;
    plcontainer_result  result;
    message             resp;
    PyObject           *pyresult,
                       *pydict,
                       *pyval;

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

receive:
    res = plcontainer_channel_receive(plcconn, &resp);
    if (res < 0) {
        lprintf (ERROR, "Error receiving data from the backend, %d", res);
        return NULL;
    }

    switch (resp->msgtype) {
       case MT_CALLREQ:
          handle_call((callreq)resp, plcconn);
          free_callreq((callreq)resp);
          goto receive;
       case MT_RESULT:
           break;
       default:
           lprintf(WARNING, "didn't receive result back %c", resp->msgtype);
           return NULL;
    }

    result = (plcontainer_result)resp;

    /* convert the result set into list of dictionaries */
    pyresult = PyList_New(result->rows);
    if (pyresult == NULL) {
        return NULL;
    }

    for (i = 0; i < result->rows; i++) {
        pydict = PyDict_New();

        for (j = 0; j < result->cols; j++) {
            switch (result->types[j]) {
                case PLC_DATA_TEXT:
                    pyval = PyString_FromStringAndSize((char *)(result->data[i][j].value+4),
                                                      *((int*)result->data[i][j].value));
                    break;
                case PLC_DATA_INT1:
                case PLC_DATA_INT2:
                case PLC_DATA_INT4:
                case PLC_DATA_INT8:
                case PLC_DATA_FLOAT4:
                case PLC_DATA_FLOAT8:
                case PLC_DATA_ARRAY:
                case PLC_DATA_RECORD:
                case PLC_DATA_UDT:
                default:
                    raise_execution_error(plcconn, "Type %d is not yet supported by Python container",
                                          (int)result->types[j]);
                    return NULL;
                /*
                case 'i':
                    pyval = PyLong_FromString(result->data[i][j].value, NULL, 0);
                    break;
                case 'f':
                    tmp   = PyString_FromString(result->data[i][j].value);
                    pyval = PyFloat_FromString(tmp, NULL);
                    Py_DECREF(tmp);
                    break;
                case 't':
                    pyval = PyString_FromString(result->data[i][j].value);
                    break;
                default:
                    PyErr_Format(PyExc_TypeError, "unknown type %d",
                                 result->types[i]);
                    return NULL;
                */
            }

            if (PyDict_SetItemString(pydict, result->names[j], pyval) != 0) {
                return NULL;
            }
        }

        if (PyList_SetItem(pyresult, i, pydict) != 0) {
            return NULL;
        }
    }

    free_result(result);

    return pyresult;
}

static PyObject *arguments_to_pytuple (callreq req) {
    PyObject *args;
    int i;

    args = PyTuple_New(req->nargs);
    for (i = 0; i < req->nargs; i++) {
        PyObject *arg = NULL;

        if (req->args[i].data.isnull) {
            Py_INCREF(Py_None);
            arg = Py_None;
        } else {
            switch (req->args[i].type) {
                case PLC_DATA_INT1:
                    arg = PyLong_FromLong( (long) *((char*)req->args[i].data.value) );
                    break;
                case PLC_DATA_INT2:
                    arg = PyLong_FromLong( (long) *((short*)req->args[i].data.value) );
                    break;
                case PLC_DATA_INT4:
                    arg = PyLong_FromLong( (long) *((int*)req->args[i].data.value) );
                    break;
                case PLC_DATA_INT8:
                    arg = PyLong_FromLongLong( *((long long*)req->args[i].data.value) );
                    break;
                case PLC_DATA_FLOAT4:
                    arg = PyFloat_FromDouble( (double) *((float*)req->args[i].data.value) );
                    break;
                case PLC_DATA_FLOAT8:
                    arg = PyFloat_FromDouble( *((double*)req->args[i].data.value) );
                    break;
                case PLC_DATA_TEXT:
                    arg = PyString_FromStringAndSize((char *)(req->args[i].data.value+4),
                                                     *((int*)req->args[i].data.value));
                    break;
                case PLC_DATA_ARRAY:
                case PLC_DATA_RECORD:
                case PLC_DATA_UDT:
                default:
                    raise_execution_error(plcconn,
                                          "Type %d is not yet supported by Python container",
                                          (int)req->args[i].type);
                    return NULL;
            }
        }
        if (arg == NULL) {
            raise_execution_error(plcconn,
                                  "Converting parameter '%s' to Python type failed",
                                  req->args[i].name);
            return NULL;
        }

        if (PyTuple_SetItem(args, i, arg) != 0) {
            raise_execution_error(plcconn,
                                  "Setting Python list element %d for argument '%s' has failed",
                                  i, req->args[i].name);
            return NULL;
        }
    }
    return args;
}

static long long python_get_longlong(PyObject *obj) {
    long long ll = 0;
    if (PyInt_Check(obj))
        ll = (long long)PyInt_AsLong(obj);
    else if (PyLong_Check(obj))
        ll = (long long)PyLong_AsLongLong(obj);
    else if (PyFloat_Check(obj))
        ll = (long long)PyFloat_AsDouble(obj);
    else {
        raise_execution_error(plcconn,
                              "Function expects integer return type but got '%s'",
                              PyString_AsString(PyObject_Str(obj)));
    }
    return ll;
}

static double python_get_double(PyObject *obj) {
    double d = 0;
    if (PyInt_Check(obj))
        d = (double)PyInt_AsLong(obj);
    else if (PyLong_Check(obj))
        d = (double)PyLong_AsLongLong(obj);
    else if (PyFloat_Check(obj))
        d = (double)PyFloat_AsDouble(obj);
    else {
        raise_execution_error(plcconn,
                              "Function expects floating point return type but got '%s'",
                              PyString_AsString(PyObject_Str(obj)));
    }
    return d;
}

static int process_call_results(plcConn *conn, PyObject *retval, plcDatatype type) {
    char  *txt;
    int    len = 0;
    plcontainer_result res;

    /* allocate a result */
    res          = malloc(sizeof(*res));
    res->msgtype = MT_RESULT;
    res->names   = malloc(sizeof(*res->types));
    res->names   = malloc(sizeof(*res->names));
    res->types   = malloc(sizeof(*res->types));
    res->rows = res->cols = 1;
    res->data    = malloc(sizeof( *res->data) * res->rows);
    res->data[0] = malloc(sizeof(**res->data) * res->cols);
    res->types[0] = type;
    res->names[0] = strdup("result");

    if (retval == Py_None) {
        res->data[0][0].isnull = 1;
        res->data[0][0].value = NULL;
        Py_DECREF(Py_None);
    } else {
        res->data[0][0].isnull = 0;
        switch (type) {
            case PLC_DATA_INT1:
                res->data[0][0].value = malloc(1);
                *((char*)res->data[0][0].value) = (char)python_get_longlong(retval);
                break;
            case PLC_DATA_INT2:
                res->data[0][0].value = malloc(2);
                *((short*)res->data[0][0].value) = (short)python_get_longlong(retval);
                break;
            case PLC_DATA_INT4:
                res->data[0][0].value = malloc(4);
                *((int*)res->data[0][0].value) = (int)python_get_longlong(retval);
                break;
            case PLC_DATA_INT8:
                res->data[0][0].value = malloc(8);
                *((long long*)res->data[0][0].value) = (long long)python_get_longlong(retval);
                break;
            case PLC_DATA_FLOAT4:
                res->data[0][0].value = malloc(4);
                *((float*)res->data[0][0].value) = (float)python_get_double(retval);
                break;
            case PLC_DATA_FLOAT8:
                res->data[0][0].value = malloc(8);
                *((double*)res->data[0][0].value) = (double)python_get_double(retval);
                break;
            case PLC_DATA_TEXT:
                txt = PyString_AsString(PyObject_Str(retval));
                len = strlen(txt);
                res->data[0][0].value = (char*)malloc(len + 5);
                memcpy(res->data[0][0].value, &len, 4);
                memcpy(res->data[0][0].value+4, txt, len+1);
                break;
            case PLC_DATA_ARRAY:
            case PLC_DATA_RECORD:
            case PLC_DATA_UDT:
            default:
                raise_execution_error(plcconn,
                                      "Type %d is not yet supported by Python container",
                                      (int)type);
                free_result(res);
                return -1;
        }
    }

    /* send the result back */
    plcontainer_channel_send(conn, (message)res);

    free_result(res);

    return 0;
}

static char *get_python_error() {
    // Python equivilant:
    // import traceback, sys
    // return "".join(traceback.format_exception(sys.exc_type,
    //    sys.exc_value, sys.exc_traceback))

    PyObject *type, *value, *traceback;
    PyObject *tracebackModule;
    char *chrRetval = "";

    if (PyErr_Occurred()) {
        PyErr_Fetch(&type, &value, &traceback);

        tracebackModule = PyImport_ImportModule("traceback");
        if (tracebackModule != NULL)
        {
            PyObject *tbList, *emptyString, *strRetval;

            tbList = PyObject_CallMethod(
                tracebackModule,
                "format_exception",
                "OOO",
                type,
                value == NULL ? Py_None : value,
                traceback == NULL ? Py_None : traceback);

            emptyString = PyString_FromString("");
            strRetval = PyObject_CallMethod(emptyString, "join",
                "O", tbList);

            chrRetval = strdup(PyString_AsString(strRetval));

            Py_DECREF(tbList);
            Py_DECREF(emptyString);
            Py_DECREF(strRetval);
            Py_DECREF(tracebackModule);
        } else {
            PyObject *strRetval;

            strRetval = PyObject_Str(value);
            chrRetval = strdup(PyString_AsString(strRetval));

            Py_DECREF(strRetval);
        }

        Py_DECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
    }

    return chrRetval;
}

static void raise_execution_error (plcConn *conn, const char *format, ...) {
    va_list        args;
    error_message  err;
    char          *msg;
    int            len, res;

    if (format == NULL) {
        lprintf(FATAL, "Error message cannot be NULL");
        return;
    }

    va_start(args, format);
    len = 100 + 2 * strlen(format);
    msg = (char*)malloc(len + 1);
    res = vsnprintf(msg, len, format, args);
    if (res < 0 || res >= len) {
        lprintf(FATAL, "Error formatting error message string");
    } else {
        /* an exception to be thrown */
        err             = malloc(sizeof(*err));
        err->msgtype    = MT_EXCEPTION;
        err->message    = msg;
        err->stacktrace = get_python_error();

        /* send the result back */
        plcontainer_channel_send(conn, (message)err);
    }

    /* free the objects */
    free(err);
    free(msg);
}
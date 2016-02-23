#include <stdlib.h>
#include <string.h>

#include "common/comm_channel.h"
#include "common/comm_logging.h"
#include "common/libpq-mini.h"
#include "pycall.h"

#include <Python.h>
/*
  Resources:

  1. https://docs.python.org/2/c-api/reflection.html
 */

static char *create_python_func(callreq req);
static PyObject *plpy_execute(PyObject *self UNUSED, PyObject *pyquery);

static PyMethodDef moddef[] = {
    {"execute", plpy_execute, METH_O, NULL}, {NULL},
};

static PGconn_min* pqconn;

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

void handle_call(callreq req, PGconn_min* conn) {
    int              i;
    int                 returnedNull = false;
    char *           func, *txt;
    plcontainer_result res;
    error_message    err;
    PyObject *       exc, *val, *retval, *tb, *str, *dict, *args;

    pqconn = conn;

    /* import __main__ to get the builtin functions */
    val = PyImport_ImportModule("__main__");
    if (val == NULL) {
        goto error;
    }
    dict = PyModule_GetDict(val);
    dict = PyDict_Copy(dict);

    /* wrap the input in a function and evaluate the result */
    func = create_python_func(req);
    /* the function will be in the dictionary because it was wrapped with "def proc.name:... " */
    val  = PyRun_String(func, Py_single_input, dict, dict);
    if (val == NULL) {
        goto error;
    }
    Py_DECREF(val);
    free(func);

    /* get the function from the global dictionary */
    val = PyDict_GetItemString(dict, req->proc.name);
    Py_INCREF(val);
    if (!PyCallable_Check(val)) {
        lprintf(FATAL, "object not callable");
    }

    /* create the argument list */
    args = PyTuple_New(req->nargs);
    for (i = 0; i < req->nargs; i++) {
        PyObject *arg;
        /*
        *  Use \N as null
        */
        if ( strcmp( req->args[i].value, "\\N" ) == 0 ) {
            Py_INCREF(Py_None);
            arg = Py_None;
        } else {
            if ( strcmp(req->args[i].type, "bool") == 0 ) {
                if( strcmp(req->args[i].value,"t") == 0 ) {
                    Py_INCREF(Py_True);
                    arg = Py_True;
                }else{
                    Py_INCREF(Py_False);
                    arg = Py_False;
                }
            } else if (strcmp(req->args[i].type, "varchar") == 0) {
                arg = PyString_FromString((char *)req->args[i].value);
            } else if (strcmp(req->args[i].type, "text") == 0) {
                arg = PyString_FromString((char *)req->args[i].value);
            } else if (strcmp(req->args[i].type, "int2") == 0) {
                arg = PyLong_FromString((char *)req->args[i].value, NULL, 0);
            } else if (strcmp(req->args[i].type, "int4") == 0) {
                arg = PyLong_FromString((char *)req->args[i].value, NULL, 0);
            } else if (strcmp(req->args[i].type, "int8") == 0) {
                arg = PyLong_FromString((char *)req->args[i].value, NULL, 0);
            } else if (strcmp(req->args[i].type, "float4") == 0) {
                arg = PyFloat_FromString(PyString_FromString((char *)req->args[i].value), NULL);
            } else if (strcmp(req->args[i].type, "float8") == 0) {
                arg = PyFloat_FromString(PyString_FromString((char *)req->args[i].value), NULL);

            } else {
                lprintf(ERROR, "unknown type %s", req->args[i].type);
            }
        }
        if (arg == NULL) {
            goto error;
        }

        if (PyTuple_SetItem(args, i, arg) != 0) {
            goto error;
        }
    }

    /* call the function */
    retval = PyObject_Call(val, args, NULL);
    if (retval == NULL) {
        goto error;
    }
    if (retval == Py_None){
        returnedNull=true;
        txt = NULL;
    } else {
        val = PyObject_Str(retval);
        txt = PyString_AsString(val);
        if (txt == NULL) {
            goto error;
        }
    }

    /* release all references */
    Py_DECREF(args);
    Py_DECREF(dict);


    /* allocate a result */
    res          = pmalloc(sizeof(*res));
    res->msgtype = MT_RESULT;
    res->names   = pmalloc(sizeof(*res->types));
    res->names   = pmalloc(sizeof(*res->names));
    res->types   = pmalloc(sizeof(*res->types));
    res->data    = pmalloc(sizeof(*res->data));
    res->data[0] = pmalloc(sizeof(*res->data[0]));

    res->rows = res->cols = 1;

    /* use strdup since free_result assumes values are on the heap */
    res->names[0] = pstrdup("result");
    res->types[0] = pstrdup("text");

    res->data[0]->isnull = returnedNull;
    res->data[0]->value  = txt;

    /* send the result back */
    plcontainer_channel_send((message)res, conn);

    free_result(res);

    /* free the result object and the python value */
    Py_DECREF(retval);
    Py_DECREF(val);
    return;

error:
    PyErr_Fetch(&exc, &val, &tb);
    str = PyObject_Str(val);
    if (str == NULL) {
        lprintf(FATAL, "cannot convert error to string");
    }

    /* an exception was thrown */
    err             = pmalloc(sizeof(*err));
    err->msgtype    = MT_EXCEPTION;
    err->message    = PyString_AsString(str);
    err->stacktrace = "";

    /* send the result back */
    plcontainer_channel_send((message)err, conn);

    /* free the objects */
    free(err);
    Py_DECREF(str);
}

static char * create_python_func(callreq req) {
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

    mrc  = pmalloc(mlen);
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

static PyObject * plpy_execute(PyObject *self UNUSED, PyObject *pyquery) {
    int               i, j;
    sql_msg_statement msg;
    plcontainer_result  result;
    message           resp;
    PyObject *        pyresult, *pydict, *pyval, *tmp;

    if (!PyString_Check(pyquery)) {
        PyErr_SetString(PyExc_TypeError, "expected the query string");
        return NULL;
    }

    msg            = pmalloc(sizeof(*msg));
    msg->msgtype   = MT_SQL;
    msg->sqltype   = SQL_TYPE_STATEMENT;
    msg->statement = PyString_AsString(pyquery);

    plcontainer_channel_send((message)msg, pqconn);

    /* we don't need it anymore */
    pfree(msg);


receive:
    resp = plcontainer_channel_receive(pqconn);

    switch (resp->msgtype) {
       case MT_CALLREQ:
          lprintf(DEBUG1, "receives call req %c", resp->msgtype);
          handle_call((callreq)resp, pqconn);
          free_callreq((callreq)resp);
          goto receive;

       case MT_RESULT:
           break;
       default:
           lprintf(DEBUG1, "didn't receive result back %c", resp->msgtype);
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
            /* just look at the first char, hacky */
            switch (result->types[j][0]) {
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
                PyErr_Format(PyExc_TypeError, "unknown type %s",
                             result->types[i]);
                return NULL;
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

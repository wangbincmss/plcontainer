#include <stdlib.h>
#include <string.h>

#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "common/comm_connectivity.h"
#include "pycall.h"

#include <Python.h>

static char *get_python_error();

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

void raise_execution_error (plcConn *conn, const char *format, ...) {
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
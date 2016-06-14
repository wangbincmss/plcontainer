#include <stdlib.h>
#include <string.h>

#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "common/comm_connectivity.h"
#include "pycall.h"
#include "pyerror.h"

#include <Python.h>

static char *get_python_error();

/* Stack of error messages obtained when no connectivity was available */
plcMsgError *plcLastErrMessage = NULL;

static char *get_python_error() {
    // Python equivilant:
    // import traceback, sys
    // return "".join(traceback.format_exception(sys.exc_type,
    //    sys.exc_value, sys.exc_traceback))

    PyObject *type, *value, *traceback;
    PyObject *tracebackModule;
    char *chrRetval = NULL;

    if (PyErr_Occurred()) {
        PyErr_Fetch(&type, &value, &traceback);

        tracebackModule = PyImport_ImportModule("traceback");
        if (tracebackModule != NULL) {
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

void raise_execution_error (const char *format, ...) {
    char *msg   = NULL;
    char *stack = NULL;

    /* First send the message saved if there is any */
    plc_raise_delayed_error();

    if (format == NULL) {
        msg = strdup("Error message cannot be NULL in raise_execution_error()");
    } else {
        va_list args;
        int     len, res;

        va_start(args, format);
        len = 100 + 2 * strlen(format);
        msg = (char*)malloc(len + 1);
        res = vsnprintf(msg, len, format, args);
        if (res < 0 || res >= len) {
            msg = strdup("Error formatting error message string in raise_execution_error()");
        }
    }
    stack = get_python_error();

    if (plcLastErrMessage == NULL && plc_is_execution_terminated == 0) {
        plcMsgError *err;

        /* an exception to be thrown */
        err             = malloc(sizeof(plcMsgError));
        err->msgtype    = MT_EXCEPTION;
        err->message    = msg;
        err->stacktrace = stack;

        /* When no connection available - keep the error message in stack */
        plcLastErrMessage = err;
        plc_raise_delayed_error();
    } else {
        lprintf(WARNING, "Cannot send second subsequent error message to client:");
        lprintf(WARNING, msg);
        free(msg);
        if (stack != NULL) {
            free(stack);
        }
    }
}

void plc_raise_delayed_error() {
    if (plcLastErrMessage != NULL) {
        if (plc_is_execution_terminated == 0 &&
                plcconn_global != NULL &&
                plc_sending_data == 0) {
            plcontainer_channel_send(plcconn_global, (plcMessage*)plcLastErrMessage);
            free_error(plcLastErrMessage);
            plcLastErrMessage = NULL;
            plc_is_execution_terminated = 1;
        }
    }
}

void *plc_error_callback() {
    plcMsgError *msg = plcLastErrMessage;
    plcLastErrMessage = NULL;
    return (void*)msg;
}
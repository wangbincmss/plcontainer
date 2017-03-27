/*------------------------------------------------------------------------------
 *
 *
 * Copyright (c) 2016, Pivotal.
 *
 *------------------------------------------------------------------------------
 */

#ifndef PLC_PYLOGGING_H
#define PLC_PYLOGGING_H

#include <Python.h>

PyObject *plpy_debug(PyObject *self, PyObject *args);
PyObject *plpy_log(PyObject *self, PyObject *args);
PyObject *plpy_info(PyObject *self, PyObject *args);
PyObject *plpy_notice(PyObject *self, PyObject *args);
PyObject *plpy_warning(PyObject *self, PyObject *args);
PyObject *plpy_error(PyObject *self, PyObject *args);
PyObject *plpy_fatal(PyObject *self, PyObject *args);

#endif /* PLC_PYLOGGING_H */

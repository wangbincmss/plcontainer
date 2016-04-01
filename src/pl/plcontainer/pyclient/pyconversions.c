#include "pyconversions.h"
#include "common/messages/messages.h"
#include "common/comm_utils.h"
#include <Python.h>

static PyObject *plc_pyobject_from_int1(char *input);
static PyObject *plc_pyobject_from_int2(char *input);
static PyObject *plc_pyobject_from_int4(char *input);
static PyObject *plc_pyobject_from_int8(char *input);
static PyObject *plc_pyobject_from_float4(char *input);
static PyObject *plc_pyobject_from_float8(char *input);
static PyObject *plc_pyobject_from_text(char *input);
static PyObject *plc_pyobject_from_text_ptr(char *input);
static PyObject *plc_pyobject_from_array_dim(plcArray *arr, plcPyInputFunc infunc,
                    int *idx, int *ipos, char **pos, int vallen, int dim);
static PyObject *plc_pyobject_from_array (char *input);

static int plc_pyobject_as_int1(PyObject *input, char **output);
static int plc_pyobject_as_int2(PyObject *input, char **output);
static int plc_pyobject_as_int4(PyObject *input, char **output);
static int plc_pyobject_as_int8(PyObject *input, char **output);
static int plc_pyobject_as_float4(PyObject *input, char **output);
static int plc_pyobject_as_float8(PyObject *input, char **output);
static int plc_pyobject_as_text(PyObject *input, char **output);
static int plc_pyobject_as_array(PyObject *input, char **output);

static int plc_get_type_length(plcDatatype dt);
static plcPyInputFunc plc_get_input_function(plcDatatype dt);
static plcPyOutputFunc plc_get_output_function(plcDatatype dt);

static PyObject *plc_pyobject_from_int1(char *input) {
    return PyLong_FromLong( (long) *input );
}

static PyObject *plc_pyobject_from_int2(char *input) {
    return PyLong_FromLong( (long) *((short*)input) );
}

static PyObject *plc_pyobject_from_int4(char *input) {
    return PyLong_FromLong( (long) *((int*)input) );
}

static PyObject *plc_pyobject_from_int8(char *input) {
    return PyLong_FromLongLong( (long long) *((long long*)input) );
}

static PyObject *plc_pyobject_from_float4(char *input) {
    return PyFloat_FromDouble( (double) *((float*)input) );
}

static PyObject *plc_pyobject_from_float8(char *input) {
    return PyFloat_FromDouble( *((double*)input) );
}

static PyObject *plc_pyobject_from_text(char *input) {
    return PyString_FromString( input );
}

static PyObject *plc_pyobject_from_text_ptr(char *input) {
    return PyString_FromString( *((char**)input) );
}

static PyObject *plc_pyobject_from_array_dim(plcArray *arr,
                                             plcPyInputFunc infunc,
                                             int *idx,
                                             int *ipos,
                                             char **pos,
                                             int vallen,
                                             int dim) {
    PyObject *res = NULL;
    if (dim == arr->meta->ndims) {
        if (arr->nulls[*ipos] != 0) {
            res = Py_None;
            Py_INCREF(Py_None);
        } else {
            res = infunc(*pos);
            *ipos += 1;
            *pos = *pos + vallen;
        }
    } else {
        res = PyList_New(arr->meta->dims[dim]);
        for (idx[dim] = 0; idx[dim] < arr->meta->dims[dim]; idx[dim]++) {
            PyObject *obj;
            obj = plc_pyobject_from_array_dim(arr, infunc, idx, ipos, pos, vallen, dim+1);
            if (obj == NULL) {
                Py_DECREF(res);
                return NULL;
            }
            PyList_SetItem(res, idx[dim], obj);
        }
    }
    return res;
}

static PyObject *plc_pyobject_from_array (char *input) {
    plcArray *arr = (plcArray*)input;
    PyObject *res = NULL;

    if (arr->meta->ndims == 0) {
        res = PyList_New(0);
    } else {
        int  *idx;
        int  ipos;
        char *pos;
        int   vallen = 0;
        plcPyInputFunc infunc;

        idx = malloc(sizeof(int) * arr->meta->ndims);
        memset(idx, 0, sizeof(int) * arr->meta->ndims);
        ipos = 0;
        pos = arr->data;
        vallen = plc_get_type_length(arr->meta->type);
        infunc = plc_get_input_function(arr->meta->type);
        if (arr->meta->type == PLC_DATA_TEXT)
            infunc = plc_pyobject_from_text_ptr;
        res = plc_pyobject_from_array_dim(arr, infunc, idx, &ipos, &pos, vallen, 0);
    }

    return res;
}

static int plc_pyobject_as_int1(PyObject *input, char **output) {
    int res = 0;
    char *out = (char*)malloc(1);
    *output = out;
    if (PyLong_Check(input))
        *out = (char)PyLong_AsLongLong(input);
    else if (PyInt_Check(input))
        *out = (char)PyInt_AsLong(input);
    else if (PyFloat_Check(input))
        *out = (char)PyFloat_AsDouble(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_int2(PyObject *input, char **output) {
    int res = 0;
    char *out = (char*)malloc(2);
    *output = out;
    if (PyLong_Check(input))
        *((short*)out) = (short)PyLong_AsLongLong(input);
    else if (PyInt_Check(input))
        *((short*)out) = (short)PyInt_AsLong(input);
    else if (PyFloat_Check(input))
        *((short*)out) = (short)PyFloat_AsDouble(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_int4(PyObject *input, char **output) {
    int res = 0;
    char *out = (char*)malloc(4);
    *output = out;
    if (PyLong_Check(input))
        *((int*)out) = (int)PyLong_AsLongLong(input);
    else if (PyInt_Check(input))
        *((int*)out) = (int)PyInt_AsLong(input);
    else if (PyFloat_Check(input))
        *((int*)out) = (int)PyFloat_AsDouble(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_int8(PyObject *input, char **output) {
    int res = 0;
    char *out = (char*)malloc(8);
    *output = out;
    if (PyLong_Check(input))
        *((long long*)out) = (long long)PyLong_AsLongLong(input);
    else if (PyInt_Check(input))
        *((long long*)out) = (long long)PyInt_AsLong(input);
    else if (PyFloat_Check(input))
        *((long long*)out) = (long long)PyFloat_AsDouble(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_float4(PyObject *input, char **output) {
    int res = 0;
    char *out = (char*)malloc(4);
    *output = out;
    if (PyFloat_Check(input))
        *((float*)out) = (float)PyFloat_AsDouble(input);
    else if (PyLong_Check(input))
        *((float*)out) = (float)PyLong_AsLongLong(input);
    else if (PyInt_Check(input))
        *((float*)out) = (float)PyInt_AsLong(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_float8(PyObject *input, char **output) {
    int res = 0;
    char *out = (char*)malloc(8);
    *output = out;
    if (PyFloat_Check(input))
        *((double*)out) = (double)PyFloat_AsDouble(input);
    else if (PyLong_Check(input))
        *((double*)out) = (double)PyLong_AsLongLong(input);
    else if (PyInt_Check(input))
        *((double*)out) = (double)PyInt_AsLong(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_text(PyObject *input, char **output) {
    int res = 0;
    PyObject *obj;
    obj = PyObject_Str(input);
    if (obj != NULL) {
        *output = strdup(PyString_AsString(obj));
        Py_DECREF(obj);
    } else {
        res = -1;
    }
    return res;
}

static int plc_pyobject_as_array(PyObject *input, char **output) {
    // TODO: Implement it in a right way
    return plc_pyobject_as_text(input, output);
}

static int plc_get_type_length(plcDatatype dt) {
    int res = 0;
    switch (dt) {
        case PLC_DATA_INT1:
            res = 1;
            break;
        case PLC_DATA_INT2:
            res = 2;
            break;
        case PLC_DATA_INT4:
            res = 4;
            break;
        case PLC_DATA_INT8:
            res = 8;
            break;
        case PLC_DATA_FLOAT4:
            res = 4;
            break;
        case PLC_DATA_FLOAT8:
            res = 8;
            break;
        case PLC_DATA_TEXT:
            res = 8;
            break;
        case PLC_DATA_ARRAY:
        case PLC_DATA_RECORD:
        case PLC_DATA_UDT:
        default:
            lprintf(ERROR, "Type %d cannot be passed plc_get_type_length function",
                    (int)dt);
            break;
    }
    return res;
}

static plcPyInputFunc plc_get_input_function(plcDatatype dt) {
    plcPyInputFunc res = NULL;
    switch (dt) {
        case PLC_DATA_INT1:
            res = plc_pyobject_from_int1;
            break;
        case PLC_DATA_INT2:
            res = plc_pyobject_from_int2;
            break;
        case PLC_DATA_INT4:
            res = plc_pyobject_from_int4;
            break;
        case PLC_DATA_INT8:
            res = plc_pyobject_from_int8;
            break;
        case PLC_DATA_FLOAT4:
            res = plc_pyobject_from_float4;
            break;
        case PLC_DATA_FLOAT8:
            res = plc_pyobject_from_float8;
            break;
        case PLC_DATA_TEXT:
            res = plc_pyobject_from_text;
            break;
        case PLC_DATA_ARRAY:
            res = plc_pyobject_from_array;
            break;
        case PLC_DATA_RECORD:
        case PLC_DATA_UDT:
        default:
            lprintf(ERROR, "Type %d cannot be passed plc_get_input_function function",
                    (int)dt);
            break;
    }
    return res;
}

static plcPyOutputFunc plc_get_output_function(plcDatatype dt) {
    plcPyOutputFunc res = NULL;
    switch (dt) {
        case PLC_DATA_INT1:
            res = plc_pyobject_as_int1;
            break;
        case PLC_DATA_INT2:
            res = plc_pyobject_as_int2;
            break;
        case PLC_DATA_INT4:
            res = plc_pyobject_as_int4;
            break;
        case PLC_DATA_INT8:
            res = plc_pyobject_as_int8;
            break;
        case PLC_DATA_FLOAT4:
            res = plc_pyobject_as_float4;
            break;
        case PLC_DATA_FLOAT8:
            res = plc_pyobject_as_float8;
            break;
        case PLC_DATA_TEXT:
            res = plc_pyobject_as_text;
            break;
        case PLC_DATA_ARRAY:
            res = plc_pyobject_as_array;
            break;
        case PLC_DATA_RECORD:
        case PLC_DATA_UDT:
        default:
            lprintf(ERROR, "Type %d cannot be passed plc_get_output_function function",
                    (int)dt);
            break;
    }
    return res;
}

plcPyCallReq *plc_init_call_conversions(callreq call) {
    plcPyCallReq *rescall;
    int i;

    rescall = (plcPyCallReq*)malloc(sizeof(plcPyCallReq));
    rescall->call = call;

    /* Currently supporting only single output parameter */
    rescall->inconv = (plcPyTypeConverter*)malloc(call->nargs * sizeof(plcPyTypeConverter));
    rescall->outconv = (plcPyTypeConverter*)malloc(1 * sizeof(plcPyTypeConverter));

    /* Get input convertion function for each of the arguments */
    for (i = 0; i < call->nargs; i++) {
        rescall->inconv[i].outputfunc = NULL;
        rescall->inconv[i].inputfunc = plc_get_input_function(call->args[i].type);
    }

    /* Get output convertion function for output argument */
    rescall->outconv[0].inputfunc = NULL;
    rescall->outconv[0].outputfunc = plc_get_output_function(call->retType.type);

    return rescall;
}

plcPyResult *plc_init_result_conversions(plcontainer_result res) {
    plcPyResult *pyres = NULL;
    int i;

    pyres = (plcPyResult*)malloc(sizeof(plcPyResult));
    pyres->res = res;
    pyres->inconv = (plcPyTypeConverter*)malloc(res->cols * sizeof(plcPyTypeConverter));

    for (i = 0; i < res->cols; i++)
        pyres->inconv[i].inputfunc = plc_get_input_function(res->types[i]);

    return pyres;
}

void plc_free_call_conversions(plcPyCallReq *req) {
    free(req->inconv);
    free(req->outconv);
    free(req);
}

void plc_free_result_conversions(plcPyResult *res) {
    free(res->inconv);
    free(res);
}
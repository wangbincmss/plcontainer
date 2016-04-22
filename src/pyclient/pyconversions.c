#include "pyconversions.h"
#include "pycall.h"
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

static int plc_pyobject_as_int1(PyObject *input, char **output, plcPyType *type);
static int plc_pyobject_as_int2(PyObject *input, char **output, plcPyType *type);
static int plc_pyobject_as_int4(PyObject *input, char **output, plcPyType *type);
static int plc_pyobject_as_int8(PyObject *input, char **output, plcPyType *type);
static int plc_pyobject_as_float4(PyObject *input, char **output, plcPyType *type);
static int plc_pyobject_as_float8(PyObject *input, char **output, plcPyType *type);
static int plc_pyobject_as_text(PyObject *input, char **output, plcPyType *type);
static int plc_pyobject_as_array(PyObject *input, char **output, plcPyType *type);

static void plc_pyobject_iter_free (plcIterator *iter);
static rawdata *plc_pyobject_as_array_next (plcIterator *iter);

static plcPyInputFunc plc_get_input_function(plcDatatype dt);
static plcPyOutputFunc plc_get_output_function(plcDatatype dt);

static PyObject *plc_pyobject_from_int1(char *input) {
    return PyInt_FromLong( (long) *input );
}

static PyObject *plc_pyobject_from_int2(char *input) {
    return PyInt_FromLong( (long) *((short*)input) );
}

static PyObject *plc_pyobject_from_int4(char *input) {
    return PyInt_FromLong( (long) *((int*)input) );
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

static int plc_pyobject_as_int1(PyObject *input, char **output, plcPyType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(1);
    *output = out;
    if (PyInt_Check(input))
        *out = (char)PyInt_AsLong(input);
    else if (PyLong_Check(input))
        *out = (char)PyLong_AsLongLong(input);
    else if (PyFloat_Check(input))
        *out = (char)PyFloat_AsDouble(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_int2(PyObject *input, char **output, plcPyType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(2);
    *output = out;
    if (PyInt_Check(input))
        *((short*)out) = (short)PyInt_AsLong(input);
    else if (PyLong_Check(input))
        *((short*)out) = (short)PyLong_AsLongLong(input);
    else if (PyFloat_Check(input))
        *((short*)out) = (short)PyFloat_AsDouble(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_int4(PyObject *input, char **output, plcPyType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(4);
    *output = out;
    if (PyInt_Check(input))
        *((int*)out) = (int)PyInt_AsLong(input);
    else if (PyLong_Check(input))
        *((int*)out) = (int)PyLong_AsLongLong(input);
    else if (PyFloat_Check(input))
        *((int*)out) = (int)PyFloat_AsDouble(input);
    else
        res = -1;
    return res;
}

static int plc_pyobject_as_int8(PyObject *input, char **output, plcPyType *type UNUSED) {
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

static int plc_pyobject_as_float4(PyObject *input, char **output, plcPyType *type UNUSED) {
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

static int plc_pyobject_as_float8(PyObject *input, char **output, plcPyType *type UNUSED) {
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

static int plc_pyobject_as_text(PyObject *input, char **output, plcPyType *type UNUSED) {
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

static void plc_pyobject_iter_free (plcIterator *iter) {
    plcArrayMeta *meta;
    plcPyArrMeta *pymeta;
    meta = (plcArrayMeta*)iter->meta;
    pymeta = (plcPyArrMeta*)iter->payload;
    pfree(meta->dims);
    pfree(pymeta->dims);
    pfree(iter->meta);
    pfree(iter->payload);
    pfree(iter->position);
    return;
}

static rawdata *plc_pyobject_as_array_next (plcIterator *iter) {
    plcPyArrMeta    *meta;
    plcPyArrPointer *ptrs;
    rawdata         *res;
    PyObject        *obj;
    int              ptr;

    meta = (plcPyArrMeta*)iter->payload;
    ptrs = (plcPyArrPointer*)iter->position;
    res  = (rawdata*)pmalloc(sizeof(rawdata));

    ptr = meta->ndims - 1;
    obj = PySequence_GetItem(ptrs[ptr].obj, ptrs[ptr].pos);
    if (obj == NULL || obj == Py_None) {
        res->isnull = 1;
        res->value = NULL;
    } else {
        res->isnull = 0;
        meta->outputfunc(obj, &res->value, meta->type);
    }

    while (ptr >= 0) {
        ptrs[ptr].pos += 1;
        /* If we finished up iterating over this dimension */
        if (ptrs[ptr].pos == meta->dims[ptr]) {
            Py_DECREF(ptrs[ptr].obj);
            ptrs[ptr].obj = NULL;
            ptrs[ptr].pos = 0;
            ptr -= 1;
        }
        /* If we found the "next" dimension to iterate over */
        else if (ptrs[ptr].pos < meta->dims[ptr]) {
            ptr += 1;
            while (ptr < meta->ndims) {
                ptrs[ptr].obj = PySequence_GetItem(ptrs[ptr-1].obj, ptrs[ptr-1].pos);
                Py_INCREF(ptrs[ptr].obj);
                ptrs[ptr].pos = 0;
                ptr += 1;
            }
            break;
        }
    }

    return res;
}

static int plc_pyobject_as_array(PyObject *input, char **output, plcPyType *type) {
    plcPyArrMeta    *meta;
    plcArrayMeta    *arrmeta;
    PyObject        *obj;
    plcIterator     *iter;
    size_t           dims[PLC_MAX_ARRAY_DIMS];
    PyObject        *stack[PLC_MAX_ARRAY_DIMS];
    int              ndims = 0;
    int              res = 0;
    int              i = 0;
    plcPyArrPointer *ptrs;

    /* We allow only lists to be returned as arrays */
    if (PySequence_Check(input)) {
        obj = input;
        /* We want to iterate through all iterable objects except by strings */
        while (obj != NULL && PySequence_Check(obj) && !PyString_Check(obj)) {
            int len = PySequence_Length(obj);
            if (len < 0) {
                *output = NULL;
                return -1;
            }
            dims[ndims] = len;
            stack[ndims] = obj;
            Py_INCREF(stack[ndims]);
            ndims += 1;
            if (dims[ndims-1] > 0)
                obj = PySequence_GetItem(obj, 0);
            else
                break;
        }

        /* Allocate the iterator */
        iter = (plcIterator*)pmalloc(sizeof(plcIterator));

        /* Initialize metas */
        arrmeta = (plcArrayMeta*)pmalloc(sizeof(plcArrayMeta));
        arrmeta->ndims = ndims;
        arrmeta->dims = (int*)pmalloc(ndims * sizeof(int));
        arrmeta->size = (ndims == 0) ? 0 : 1;
        arrmeta->type = type->subTypes[0].type;

        meta = (plcPyArrMeta*)pmalloc(sizeof(plcPyArrMeta));
        meta->ndims = ndims;
        meta->dims  = (size_t*)pmalloc(ndims * sizeof(size_t));
        meta->outputfunc = plc_get_output_function(type->subTypes[0].type);
        meta->type = &type->subTypes[0];

        for (i = 0; i < ndims; i++) {
            meta->dims[i] = dims[i];
            arrmeta->dims[i] = (int)dims[i];
            arrmeta->size *= (int)dims[i];
        }

        iter->meta = arrmeta;
        iter->payload = (char*)meta;

        /* Initializing initial position */
        ptrs = (plcPyArrPointer*)pmalloc(ndims * sizeof(plcPyArrPointer));
        for (i = 0; i < ndims; i++) {
            ptrs[i].pos = 0;
            ptrs[i].obj = stack[i];
        }
        iter->position = (char*)ptrs;

        /* Initializing "data" */
        iter->data = (char*)input;

        /* Initializing "next" and "cleanup" functions */
        iter->next = plc_pyobject_as_array_next;
        iter->cleanup = plc_pyobject_iter_free;

        *output = (char*)iter;
    } else {
        *output = NULL;
        res = -1;
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

static void plc_parse_type(plcPyType *pytype, plcType *type) {
    int i = 0;
    //pytype->name = strdup(type->name); TODO: implement type name to support UDTs
    pytype->name = strdup("results");
    pytype->type = type->type;
    pytype->nSubTypes = type->nSubTypes;
    pytype->conv.inputfunc  = plc_get_input_function(pytype->type);
    pytype->conv.outputfunc = plc_get_output_function(pytype->type);
    if (pytype->nSubTypes > 0) {
        pytype->subTypes = (plcPyType*)malloc(pytype->nSubTypes * sizeof(plcPyType));
        for (i = 0; i < type->nSubTypes; i++)
            plc_parse_type(&pytype->subTypes[i], &type->subTypes[i]);
    } else {
        pytype->subTypes = NULL;
    }
}

plcPyFunction *plc_py_init_function(callreq call) {
    plcPyFunction *res;
    int i;

    res = (plcPyFunction*)malloc(sizeof(plcPyFunction));
    res->call = call;
    res->proc.src  = strdup(call->proc.src);
    res->proc.name = strdup(call->proc.name);
    res->nargs = call->nargs;
    res->retset = call->retset;
    res->args = (plcPyType*)malloc(res->nargs * sizeof(plcPyType));
    res->objectid = call->objectid;

    for (i = 0; i < res->nargs; i++)
        plc_parse_type(&res->args[i], &call->args[i].type);

    plc_parse_type(&res->res, &call->retType);

    return res;
}

plcPyResult *plc_init_result_conversions(plcontainer_result res) {
    plcPyResult *pyres = NULL;
    int i;

    pyres = (plcPyResult*)malloc(sizeof(plcPyResult));
    pyres->res = res;
    pyres->inconv = (plcPyTypeConv*)malloc(res->cols * sizeof(plcPyTypeConv));

    for (i = 0; i < res->cols; i++)
        pyres->inconv[i].inputfunc = plc_get_input_function(res->types[i].type);

    return pyres;
}

static void plc_py_free_type(plcPyType *type) {
    int i = 0;
    for (i = 0; i < type->nSubTypes; i++)
        plc_py_free_type(&type->subTypes[i]);
    if (type->nSubTypes > 0)
        free(type->subTypes);
    return;
}

void plc_py_free_function(plcPyFunction *func) {
    int i = 0;
    for (i = 0; i < func->nargs; i++)
        plc_py_free_type(&func->args[i]);
    plc_py_free_type(&func->res);
    free(func->args);
    free(func);
}

void plc_free_result_conversions(plcPyResult *res) {
    free(res->inconv);
    free(res);
}

void plc_py_copy_type(plcType *type, plcPyType *pytype) {
    type->type = pytype->type;
    type->nSubTypes = pytype->nSubTypes;
    if (type->nSubTypes > 0) {
        int i = 0;
        type->subTypes = (plcType*)pmalloc(type->nSubTypes * sizeof(plcType));
        for (i = 0; i < type->nSubTypes; i++)
            plc_py_copy_type(&type->subTypes[i], &pytype->subTypes[i]);
    } else {
        type->subTypes = NULL;
    }
}
/* Greenplum headers */
#include "postgres.h"
#include "utils/array.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "utils/lsyscache.h"
#include "access/transam.h"
#include "utils/typcache.h"

#include "plcontainer.h"
#include "plc_typeio.h"
#include "message_fns.h"
#include "common/comm_utils.h"

static char *plc_datum_as_int1(Datum input, plcTypeInfo *type);
static char *plc_datum_as_int2(Datum input, plcTypeInfo *type);
static char *plc_datum_as_int4(Datum input, plcTypeInfo *type);
static char *plc_datum_as_int8(Datum input, plcTypeInfo *type);
static char *plc_datum_as_float4(Datum input, plcTypeInfo *type);
static char *plc_datum_as_float8(Datum input, plcTypeInfo *type);
static char *plc_datum_as_float8_numeric(Datum input, plcTypeInfo *type);
static char *plc_datum_as_text(Datum input, plcTypeInfo *type);
static char *plc_datum_as_array(Datum input, plcTypeInfo *type);
static void plc_backend_array_free(plcIterator *iter);
static rawdata *plc_backend_array_next(plcIterator *self);
static char *plc_datum_as_udt(Datum input, plcTypeInfo *type);

static Datum plc_datum_from_int1(char *input, plcTypeInfo *type);
static Datum plc_datum_from_int2(char *input, plcTypeInfo *type);
static Datum plc_datum_from_int4(char *input, plcTypeInfo *type);
static Datum plc_datum_from_int8(char *input, plcTypeInfo *type);
static Datum plc_datum_from_float4(char *input, plcTypeInfo *type);
static Datum plc_datum_from_float8(char *input, plcTypeInfo *type);
static Datum plc_datum_from_float8_numeric(char *input, plcTypeInfo *type);
static Datum plc_datum_from_text(char *input, plcTypeInfo *type);
static Datum plc_datum_from_text_ptr(char *input, plcTypeInfo *type);
static Datum plc_datum_from_array(char *input, plcTypeInfo *type);
static Datum plc_datum_from_udt(char *input, plcTypeInfo *type);

void fill_type_info(Oid typeOid, plcTypeInfo *type, bool issubtype) {
    HeapTuple     typeTup;
    Form_pg_type  typeStruct;
    char          dummy_delim;

    typeTup = SearchSysCache(TYPEOID, typeOid, 0, 0, 0);
    if (!HeapTupleIsValid(typeTup))
        elog(ERROR, "cache lookup failed for type %u", typeOid);

    typeStruct = (Form_pg_type)GETSTRUCT(typeTup);
    ReleaseSysCache(typeTup);

    type->typeOid = typeOid;
    type->output  = typeStruct->typoutput;
    type->input   = typeStruct->typinput;
    get_type_io_data(typeOid, IOFunc_input,
                     &type->typlen, &type->typbyval, &type->typalign,
                     &dummy_delim,
                     &type->typioparam, &type->input);
    type->typmod = typeStruct->typtypmod;
    type->nSubTypes = 0;
    type->subTypes = NULL;
    type->typelem = typeStruct->typelem;

    type->is_rowtype = false;
    type->attisdropped = false;
    type->typ_relid = InvalidOid;
    type->typrel_xmin = InvalidTransactionId;
    ItemPointerSetInvalid(&type->typrel_tid);

    switch(typeOid) {
        case BOOLOID:
            type->type = PLC_DATA_INT1;
            type->outfunc = plc_datum_as_int1;
            type->infunc = plc_datum_from_int1;
            break;
        case INT2OID:
            type->type = PLC_DATA_INT2;
            type->outfunc = plc_datum_as_int2;
            type->infunc = plc_datum_from_int2;
            break;
        case INT4OID:
            type->type = PLC_DATA_INT4;
            type->outfunc = plc_datum_as_int4;
            type->infunc = plc_datum_from_int4;
            break;
        case INT8OID:
            type->type = PLC_DATA_INT8;
            type->outfunc = plc_datum_as_int8;
            type->infunc = plc_datum_from_int8;
            break;
        case FLOAT4OID:
            type->type = PLC_DATA_FLOAT4;
            type->outfunc = plc_datum_as_float4;
            type->infunc = plc_datum_from_float4;
            break;
        case FLOAT8OID:
            type->type = PLC_DATA_FLOAT8;
            type->outfunc = plc_datum_as_float8;
            type->infunc = plc_datum_from_float8;
            break;
        case NUMERICOID:
            type->type = PLC_DATA_FLOAT8;
            type->outfunc = plc_datum_as_float8_numeric;
            type->infunc = plc_datum_from_float8_numeric;
            break;
        /* All the other types are passed through in-out functions to translate
         * them to text before sending and after receiving */
        default:
            type->type = PLC_DATA_TEXT;
            type->outfunc = plc_datum_as_text;
            if (!issubtype) {
                type->infunc = plc_datum_from_text;
            } else {
                type->infunc = plc_datum_from_text_ptr;
            }
            break;
    }

    /* Processing arrays here */
    if (typeStruct->typelem != 0 && typeStruct->typoutput == ARRAY_OUT_OID) {
        type->type = PLC_DATA_ARRAY;
        type->outfunc = plc_datum_as_array;
        type->infunc = plc_datum_from_array;
        type->nSubTypes = 1;
        type->subTypes = (plcTypeInfo*)plc_top_alloc(sizeof(plcTypeInfo));
        fill_type_info(typeStruct->typelem, &type->subTypes[0], true);
    }

    /* Processing composite types - only first level is supported */
    if (!issubtype && typeStruct->typtype == TYPTYPE_COMPOSITE) {
        int i;

        type->is_rowtype = true;
        type->type = PLC_DATA_UDT;
        type->outfunc = plc_datum_as_udt;
        type->infunc  = plc_datum_from_udt;
        type->tupleDesc = lookup_rowtype_tupdesc(type->typeOid, type->typmod);
        type->nSubTypes = type->tupleDesc->natts;

        // Allocate memory for this number of arguments
        type->subTypes = (plcTypeInfo*)plc_top_alloc(type->nSubTypes * sizeof(plcTypeInfo));
        memset(type->subTypes, 0, type->nSubTypes * sizeof(plcTypeInfo));

        // Fill all the subtypes
        for (i = 0; i < type->tupleDesc->natts; i++) {
            type->subTypes[i].attisdropped = type->tupleDesc->attrs[i]->attisdropped;
            if (!type->subTypes[i].attisdropped) {
                fill_type_info(type->tupleDesc->attrs[i]->atttypid, &type->subTypes[i], true);
            }
        }
    }
}

void copy_type_info(plcType *type, plcTypeInfo *ptype) {
    type->type = ptype->type;
    type->nSubTypes = 0;
    if (ptype->nSubTypes > 0) {
        int i, j;

        for (i = 0; i < ptype->nSubTypes; i++) {
            if (!ptype->subTypes[i].attisdropped) {
                type->nSubTypes += 1;
            }
        }

        type->subTypes = (plcType*)pmalloc(type->nSubTypes * sizeof(plcType));
        for (i = 0, j = 0; i < ptype->nSubTypes; i++) {
            if (!ptype->subTypes[i].attisdropped) {
                copy_type_info(&type->subTypes[j], &ptype->subTypes[i]);
                j += 1;
            }
        }
    } else {
        type->subTypes = NULL;
    }
}

void free_type_info(plcTypeInfo *types, int ntypes) {
    int i = 0;
    // typleDesc is already freed at this moment in calling function
    for (i = 0; i < ntypes; i++) {
        if (types->nSubTypes > 0)
            free_type_info(types->subTypes, types->nSubTypes);
    }
    pfree(types);
}

static char *plc_datum_as_int1(Datum input, plcTypeInfo *type UNUSED) {
    char *out = (char*)pmalloc(1);
    *((char*)out) = DatumGetBool(input);
    return out;
}

static char *plc_datum_as_int2(Datum input, plcTypeInfo *type UNUSED) {
    char *out = (char*)pmalloc(2);
    *((int16*)out) = DatumGetInt16(input);
    return out;
}

static char *plc_datum_as_int4(Datum input, plcTypeInfo *type UNUSED) {
    char *out = (char*)pmalloc(4);
    *((int32*)out) = DatumGetInt32(input);
    return out;
}

static char *plc_datum_as_int8(Datum input, plcTypeInfo *type UNUSED) {
    char *out = (char*)pmalloc(8);
    *((int64*)out) = DatumGetInt64(input);
    return out;
}

static char *plc_datum_as_float4(Datum input, plcTypeInfo *type UNUSED) {
    char *out = (char*)pmalloc(4);
    *((float4*)out) = DatumGetFloat4(input);
    return out;
}

static char *plc_datum_as_float8(Datum input, plcTypeInfo *type UNUSED) {
    char *out = (char*)pmalloc(8);
    *((float8*)out) = DatumGetFloat8(input);
    return out;
}

static char *plc_datum_as_float8_numeric(Datum input, plcTypeInfo *type UNUSED) {
    char *out = (char*)pmalloc(8);
    /* Numeric is casted to float8 which causes precision lost */
    Datum fdatum = DirectFunctionCall1(numeric_float8, input);
    *((float8*)out) = DatumGetFloat8(fdatum);
    return out;
}

static char *plc_datum_as_text(Datum input, plcTypeInfo *type) {
    return DatumGetCString(OidFunctionCall3(type->output,
                                            input,
                                            type->typelem,
                                            type->typmod));
}

static char *plc_datum_as_array(Datum input, plcTypeInfo *type) {
    ArrayType    *array = DatumGetArrayTypeP(input);
    plcIterator  *iter;
    plcArrayMeta *meta;
    int           i;

    iter = (plcIterator*)palloc(sizeof(plcIterator));
    meta = (plcArrayMeta*)palloc(sizeof(plcArrayMeta));
    iter->meta = meta;

    meta->type = type->subTypes[0].type;
    meta->ndims = ARR_NDIM(array);
    meta->dims = (int*)palloc(meta->ndims * sizeof(int));
    iter->position = (char*)palloc(sizeof(int) * meta->ndims * 2 + 2);
    ((plcTypeInfo**)iter->position)[0] = type;
    meta->size = meta->ndims > 0 ? 1 : 0;
    for (i = 0; i < meta->ndims; i++) {
        meta->dims[i] = ARR_DIMS(array)[i];
        meta->size *= ARR_DIMS(array)[i];
        ((int*)iter->position)[i + 2] = ARR_LBOUND(array)[i];
        ((int*)iter->position)[i + meta->ndims + 2] = ARR_LBOUND(array)[i];
    }
    iter->data = (char*)array;
    iter->next = plc_backend_array_next;
    iter->cleanup = plc_backend_array_free;

    return (char*)iter;
}

static void plc_backend_array_free(plcIterator *iter) {
    plcArrayMeta *meta;
    meta = (plcArrayMeta*)iter->meta;
    if (meta->ndims > 0) {
        pfree(meta->dims);
    }
    pfree(iter->meta);
    pfree(iter->position);
    return;
}

static rawdata *plc_backend_array_next(plcIterator *self) {
    plcArrayMeta *meta;
    ArrayType    *array;
    plcTypeInfo  *typ;
    plcTypeInfo  *subtyp;
    int          *lbounds;
    int          *pos;
    int           dim;
    bool          isnull = 0;
    Datum         el;
    rawdata      *res;

    res     = palloc(sizeof(rawdata));
    meta    = (plcArrayMeta*)self->meta;
    array   = (ArrayType*)self->data;
    typ     = ((plcTypeInfo**)self->position)[0];
    subtyp  = &typ->subTypes[0];
    lbounds = (int*)self->position + 2;
    pos     = (int*)self->position + 2 + meta->ndims;

    el = array_ref(array, meta->ndims, pos, typ->typlen,
                   subtyp->typlen, subtyp->typbyval,
                   subtyp->typalign, &isnull);
    if (isnull) {
        res->isnull = 1;
        res->value  = NULL;
    } else {
        res->isnull = 0;
        res->value = subtyp->outfunc(el, subtyp);
    }

    dim     = meta->ndims - 1;
    while (dim >= 0 && pos[dim]-lbounds[dim] < meta->dims[dim]) {
        pos[dim] += 1;
        if (pos[dim]-lbounds[dim] >= meta->dims[dim]) {
            pos[dim] = lbounds[dim];
            dim -= 1;
        } else {
            break;
        }
    }

    return res;
}

/*
HeapTupleData rec_data;
rec_data.t_len = HeapTupleHeaderGetDatumLength(rec_header);
ItemPointerSetInvalid(&(rec_data.t_self));
rec_data.t_tableOid = InvalidOid;
rec_data.t_data = rec_header;
vattr = heap_getattr(&rec_data, (i + 1), type->tupleDesc, &is_null);
*/
static char *plc_datum_as_udt(Datum input, plcTypeInfo *type) {
    HeapTupleHeader rec_header;
    plcUDT         *res;
    int             i, j;
    int             nNonDropped = 0;

    res = pmalloc(sizeof(plcUDT));
    rec_header = DatumGetHeapTupleHeader(input);
    for (i = 0; i < type->nSubTypes; i++) {
        if (!type->subTypes[i].attisdropped) {
            nNonDropped += 1;
        }
    }

    res->nargs = nNonDropped;
    res->types = pmalloc(res->nargs * sizeof(plcType));
    res->nulls = pmalloc(res->nargs * sizeof(char));
    res->data  = pmalloc(res->nargs * sizeof(char*));

    for (i = 0, j = 0; i < type->nSubTypes; i++) {
        Datum  vattr;
        bool   is_null;

        if (!type->subTypes[i].attisdropped) {
            copy_type_info(&res->types[j], type);
            vattr = GetAttributeByNum(rec_header, (i + 1), &is_null);
            if (is_null) {
                res->nulls[j] = true;
                res->data[j] = NULL;
            } else {
                res->nulls[j] = false;
                res->data[j] = type->subTypes[i].outfunc(vattr, &type->subTypes[i]);
            }
            j += 1;
        }
    }

    return (char*)res;
}

static Datum plc_datum_from_int1(char *input, plcTypeInfo *type UNUSED) {
    return BoolGetDatum(*((bool*)input));
}

static Datum plc_datum_from_int2(char *input, plcTypeInfo *type UNUSED) {
    return Int16GetDatum(*((int16*)input));
}

static Datum plc_datum_from_int4(char *input, plcTypeInfo *type UNUSED) {
    return Int32GetDatum(*((int32*)input));
}

static Datum plc_datum_from_int8(char *input, plcTypeInfo *type UNUSED) {
    return Int64GetDatum(*((int64*)input));
}

static Datum plc_datum_from_float4(char *input, plcTypeInfo *type UNUSED) {
    return Float4GetDatum(*((float4*)input));
}

static Datum plc_datum_from_float8(char *input, plcTypeInfo *type UNUSED) {
    return Float8GetDatum(*((float8*)input));
}

static Datum plc_datum_from_float8_numeric(char *input, plcTypeInfo *type UNUSED) {
    Datum fdatum = Float8GetDatum(*((float8*)input));
    return DirectFunctionCall1(float8_numeric, fdatum);
}

static Datum plc_datum_from_text(char *input, plcTypeInfo *type) {
    return OidFunctionCall3(type->input,
                            CStringGetDatum(input),
                            type->typelem,
                            type->typmod);
}

static Datum plc_datum_from_text_ptr(char *input, plcTypeInfo *type) {
    return OidFunctionCall3(type->input,
                            CStringGetDatum( *((char**)input) ),
                            type->typelem,
                            type->typmod);
}

static Datum plc_datum_from_array(char *input, plcTypeInfo *type) {
    Datum         dvalue;
    Datum         *elems;
    ArrayType    *array = NULL;
    int          *lbs = NULL;
    int           i;
    MemoryContext oldContext;
    plcArray     *arr;
    char         *ptr;
    int           len;
    plcTypeInfo  *subType;

    arr = (plcArray*)input;
    subType = &type->subTypes[0];
    lbs = (int*)palloc(arr->meta->ndims * sizeof(int));
    for (i = 0; i < arr->meta->ndims; i++)
        lbs[i] = 1;

    elems = palloc(arr->meta->size * sizeof(Datum));
    ptr = arr->data;
    len = plc_get_type_length(subType->type);
    for (i = 0; i < arr->meta->size; i++) {
        if (arr->nulls[i] == 0) {
            elems[i] = subType->infunc(ptr, subType);
        } else {
            elems[i] = (Datum)0;
        }
        ptr += len;
    }

    oldContext = MemoryContextSwitchTo(pl_container_caller_context);
    array = construct_md_array(elems,
                               arr->nulls,
                               arr->meta->ndims,
                               arr->meta->dims,
                               lbs,
                               subType->typeOid,
                               subType->typlen,
                               subType->typbyval,
                               subType->typalign);

    dvalue = PointerGetDatum(array);
    MemoryContextSwitchTo(oldContext);

    pfree(lbs);
    pfree(elems);

    return dvalue;
}

static Datum plc_datum_from_udt(char *input, plcTypeInfo *type) {
    HeapTuple      tuple;
    Datum         *values;
    bool          *nulls;
    volatile int   i, j;
    MemoryContext  oldContext;
    plcUDT        *udt  = (plcUDT*)input;

    /* Build tuple */
    values = palloc(sizeof(Datum) * type->nSubTypes);
    nulls = palloc(sizeof(bool) * type->nSubTypes);
    for (i = 0, j = 0; i < type->nSubTypes; ++i) {
        if (!type->subTypes[i].attisdropped) {
            if (udt->nulls[j]) {
                nulls[i] = true;
                values[i] = (Datum) 0;
            } else {
                nulls[i] = false;
                values[i] = type->subTypes[j].infunc(udt->data[j], &type->subTypes[j]);
            }
            j += 1;
        }
    }

    oldContext = MemoryContextSwitchTo(pl_container_caller_context);
    tuple = heap_form_tuple(type->tupleDesc, values, nulls);
    MemoryContextSwitchTo(oldContext);

    pfree(values);
    pfree(nulls);

    return HeapTupleGetDatum(tuple);
}

/*
void plc_datum_from_tuple( MemoryContext oldContext, MemoryContext messageContext,
        ReturnSetInfo *rsinfo, plcontainer_result res, int *isNull )
{
    AttInMetadata      *attinmeta;
    Tuplestorestate    *tupstore = tuplestore_begin_heap(true, false, work_mem);
    TupleDesc          tupdesc;
    HeapTuple    typetup,
                 tuple;
    Form_pg_type type;
    Oid          typeOid;
    int32        typeMod;

    char **values;
    int i,j;

     * TODO: Returning tuple, you will not have any tuple description for the
     * function returning setof record. This needs to be fixed *
     * get the requested return tuple description *
    if (rsinfo->expectedDesc != NULL)
        tupdesc = CreateTupleDescCopy(rsinfo->expectedDesc);
    else {
        elog(ERROR, "Functions returning 'record' type are not supported yet");
        *isNull = TRUE;
        return;
    }

    for (j = 0; j < res->cols; j++) {
        parseTypeString(res->types[j], &typeOid, &typeMod);
        typetup = SearchSysCache(TYPEOID, typeOid, 0, 0, 0);

        if (!HeapTupleIsValid(typetup)) {
            MemoryContextSwitchTo(oldContext);
            MemoryContextDelete(messageContext);
            elog(FATAL, "[plcontainer] Invalid heaptuple at result return");
            // This won`t run
            *isNull=TRUE;
        }

        type = (Form_pg_type)GETSTRUCT(typetup);

        strcpy(tupdesc->attrs[j]->attname.data, res->names[j]);
        tupdesc->attrs[j]->atttypid = typeOid;
        ReleaseSysCache(typetup);
    }

    attinmeta = TupleDescGetAttInMetadata(tupdesc);

    * OK, go to work *
    rsinfo->returnMode = SFRM_Materialize;
    MemoryContextSwitchTo(oldContext);

     *
     * SFRM_Materialize mode expects us to return a NULL Datum. The actual
     * tuples are in our tuplestore and passed back through
     * rsinfo->setResult. rsinfo->setDesc is set to the tuple description
     * that we actually used to build our tuples with, so the caller can
     * verify we did what it was expecting.
     *
    rsinfo->setDesc = tupdesc;

    for (i=0; i<res->rows;i++){

        values = palloc(sizeof(char *)* res->cols);
        for (j=0; j< res->cols;j++){
            values[j] = res->data[i][j].value;
        }

        * construct the tuple *
        tuple = BuildTupleFromCStrings(attinmeta, values);
        pfree(values);

        * switch to appropriate context while storing the tuple *
        oldContext = MemoryContextSwitchTo(messageContext);

        * now store it *
        tuplestore_puttuple(tupstore, tuple);

        MemoryContextSwitchTo(oldContext);
    }
    rsinfo->setResult = tupstore;
    MemoryContextSwitchTo(oldContext);
    MemoryContextDelete(messageContext);

    *isNull = TRUE;
}
*/
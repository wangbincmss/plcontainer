/*
Copyright 1994 The PL-J Project. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE PL-J PROJECT ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE PL-J PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the authors and should not be
interpreted as representing official policies, either expressed or implied, of the PL-J Project.
*/


/**
 * file name:        plj-callmkr.c
 * description:        PL/pgJ call message creator routine. This file
 *                    was renamed from plpgj_call_maker.c
 *                    It is replaceable with pljava-way of declaring java
 *                    method calls. (read the readme!)
 * author:        Laszlo Hornyak
 */

/* standard headers */
#include <errno.h>
#include <string.h>

/* postgres headers */
#include "postgres.h"

#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "utils/datum.h"
#include "utils/lsyscache.h"

/* message and function definitions */
#include "common/comm_utils.h"
#include "common/messages/messages.h"
#include "message_fns.h"

static void function_cache_up(int index);
static plcProcInfo *function_cache_get(Oid funcOid);
static void function_cache_put(plcProcInfo *func);
static void free_subtypes(plcTypeInfo *types, int ntypes);
static void free_proc_info(plcProcInfo *proc);
static void fill_type_info(Oid typeOid, plcTypeInfo *type);
static bool plc_procedure_valid(plcProcInfo *proc, HeapTuple procTup);
static rawdata *plc_backend_array_next(plcIterator *self);
static plcIterator *init_array_iter(Datum d, plcTypeInfo *argType);
static char *fill_callreq_value(Datum funcArg, plcTypeInfo *argType);
static void fill_callreq_arguments(FunctionCallInfo fcinfo, plcProcInfo *pinfo, callreq req);

static plcProcInfo *plcFunctionCache[PLC_FUNCTION_CACHE_SIZE];
static int plcFunctionCacheInitialized = 0;

/* Move up the cache item */
static void function_cache_up(int index) {
    plcProcInfo *tmp;
    int i;
    if (index > 0) {
        tmp = plcFunctionCache[index];
        for (i = index; i > 0; i--) {
            plcFunctionCache[i-1] = plcFunctionCache[i];
        }
        plcFunctionCache[0] = tmp;
    }
}

static plcProcInfo *function_cache_get(Oid funcOid) {
    int i;
    plcProcInfo *resFunc = NULL;
    /* Initialize all the elements with nulls initially */
    if (!plcFunctionCacheInitialized) {
        for (i = 0; i < PLC_FUNCTION_CACHE_SIZE; i++) {
            plcFunctionCache[i] = NULL;
        }
        plcFunctionCacheInitialized = 1;
    }
    for (i = 0; i < PLC_FUNCTION_CACHE_SIZE; i++) {
        if (plcFunctionCache[i] != NULL &&
                plcFunctionCache[i]->funcOid == funcOid) {
            function_cache_up(i);
            resFunc = plcFunctionCache[i];
            break;
        }
    }
    return resFunc;
}

static void function_cache_put(plcProcInfo *func) {
    int i;
    plcProcInfo *oldFunc;
    oldFunc = function_cache_get(func->funcOid);
    /* If the function is not cached already */
    if (oldFunc == NULL) {
        /* If the last element exists we need to free its memory */
        if (plcFunctionCache[PLC_FUNCTION_CACHE_SIZE-1] != NULL) {
            free_proc_info(plcFunctionCache[PLC_FUNCTION_CACHE_SIZE-1]);
        }
        /* Move our LRU cache right */
        for (i = PLC_FUNCTION_CACHE_SIZE-1; i > 0; i--) {
            plcFunctionCache[i] = plcFunctionCache[i-1];
        }
        plcFunctionCache[0] = func;
    } else {
        free_proc_info(oldFunc);
        plcFunctionCache[0] = func;
    }
}

static void free_subtypes(plcTypeInfo *types, int ntypes) {
    int i = 0;
    for (i = 0; i < ntypes; i++) {
        if (types->nSubTypes > 0)
            free_subtypes(types->subTypes, types->nSubTypes);
    }
    pfree(types);
}

static void free_proc_info(plcProcInfo *proc) {
    int i;
    for (i = 0; i < proc->nargs; i++) {
        pfree(proc->argnames[i]);
        if (proc->argtypes[i].nSubTypes > 0)
            free_subtypes(proc->argtypes[i].subTypes,
                          proc->argtypes[i].nSubTypes);
    }
    if (proc->nargs > 0) {
        pfree(proc->argnames);
        pfree(proc->argtypes);
    }
    pfree(proc);
}

static void fill_type_info(Oid typeOid, plcTypeInfo *type) {
    HeapTuple    typeTup;
    Form_pg_type typeStruct;
    char		 dummy_delim;

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
    type->nSubTypes = 0;
    type->subTypes = NULL;

    switch(typeOid){
        case BOOLOID:
            type->type = PLC_DATA_INT1;
            break;
        case INT2OID:
            type->type = PLC_DATA_INT2;
            break;
        case INT4OID:
            type->type = PLC_DATA_INT4;
            break;
        case INT8OID:
            type->type = PLC_DATA_INT8;
            break;
        case FLOAT4OID:
            type->type = PLC_DATA_FLOAT4;
            break;
        case FLOAT8OID:
            type->type = PLC_DATA_FLOAT8;
            break;
        case TEXTOID:
        case VARCHAROID:
        case CHAROID:
            type->type = PLC_DATA_TEXT;
            break;
        default:
            if (typeStruct->typelem != 0) {
                type->type = PLC_DATA_ARRAY;
                type->nSubTypes = 1;
                type->subTypes = (plcTypeInfo*)plc_top_alloc(sizeof(plcTypeInfo));
                fill_type_info(typeStruct->typelem, &type->subTypes[0]);
            } else {
                elog(ERROR, "Data type with OID %d is not supported", typeOid);
            }
            break;
    }
}

/*
 * Decide whether a cached PLyProcedure struct is still valid
 */
static bool plc_procedure_valid(plcProcInfo *proc, HeapTuple procTup) {
    bool valid = false;

    if (proc != NULL) {
        /* If the pg_proc tuple has changed, it's not valid */
        if (proc->fn_xmin == HeapTupleHeaderGetXmin(procTup->t_data) &&
            ItemPointerEquals(&proc->fn_tid, &procTup->t_self)) {

            valid = true;

            /* TODO: Implementing UDT we would need this part of code */
            /*
            int  i;
            // If there are composite input arguments, they might have changed
            for (i = 0; i < proc->nargs; i++) {
                Oid       relid;
                HeapTuple relTup;

                // Short-circuit on first changed argument
                if (!valid)
                    break;

                // Only check input arguments that are composite
                if (proc->args[i].is_rowtype != 1)
                    continue;

                Assert(OidIsValid(proc->args[i].typ_relid));
                Assert(TransactionIdIsValid(proc->args[i].typrel_xmin));
                Assert(ItemPointerIsValid(&proc->args[i].typrel_tid));

                // Get the pg_class tuple for the argument type
                relid = proc->args[i].typ_relid;
                relTup = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));
                if (!HeapTupleIsValid(relTup))
                    elog(ERROR, "cache lookup failed for relation %u", relid);

                // If it has changed, the function is not valid
                if (!(proc->args[i].typrel_xmin == HeapTupleHeaderGetXmin(relTup->t_data) &&
                        ItemPointerEquals(&proc->args[i].typrel_tid, &relTup->t_self)))
                    valid = false;

                ReleaseSysCache(relTup);
            }
            */
        }
    }
    return valid;
}


plcProcInfo * get_proc_info(FunctionCallInfo fcinfo) {
    int           i, len;
    Datum *       argnames, argnamesArray;
    Datum         srcdatum, namedatum;
    bool          isnull;
    Oid           procoid;
    Form_pg_proc  procTup;
    HeapTuple     procHeapTup, textHeapTup;
    Form_pg_type  typeTup;
    plcProcInfo  *pinfo = NULL;

    procoid = fcinfo->flinfo->fn_oid;
    procHeapTup = SearchSysCache(PROCOID, procoid, 0, 0, 0);
    if (!HeapTupleIsValid(procHeapTup)) {
        elog(ERROR, "cannot find proc with oid %u", procoid);
    }

    pinfo   = function_cache_get(procoid);
    /*
     * All the catalog operations are done only if the cached function
     * information has changed in the catalog
     */
    if (!plc_procedure_valid(pinfo, procHeapTup)) {

        /*
         * Here we are using plc_top_alloc as the function structure should be
         * available across the function handler call
         *
         * Note: we free the procedure from within function_put_cache below
         */
        pinfo = plc_top_alloc(sizeof(plcProcInfo));
        if (pinfo == NULL) {
            elog(FATAL, "Cannot allocate memory for plcProcInfo structure");
        }
        /* Remember transactional information to allow caching */
        pinfo->funcOid = procoid;
        pinfo->fn_xmin = HeapTupleHeaderGetXmin(procHeapTup->t_data);
        pinfo->fn_tid  = procHeapTup->t_self;

        procTup = (Form_pg_proc)GETSTRUCT(procHeapTup);
        fill_type_info(procTup->prorettype, &pinfo->rettype);

        pinfo->nargs = procTup->pronargs;
        if (pinfo->nargs > 0) {
            pinfo->argtypes = plc_top_alloc(pinfo->nargs * sizeof(plcTypeInfo));
            for (i = 0; i < pinfo->nargs; i++) {
                fill_type_info(procTup->proargtypes.values[i], &pinfo->argtypes[i]);
            }

            argnamesArray = SysCacheGetAttr(PROCOID, procHeapTup,
                                            Anum_pg_proc_proargnames, &isnull);
            if (isnull) {
                ReleaseSysCache(procHeapTup);
                elog(ERROR, "null arguments!!!");
            }

            textHeapTup = SearchSysCache(TYPEOID, ObjectIdGetDatum(TEXTOID), 0, 0, 0);
            if (!HeapTupleIsValid(textHeapTup)) {
                elog(FATAL, "cannot find text type in cache");
            }
            typeTup = (Form_pg_type)GETSTRUCT(textHeapTup);
            deconstruct_array(DatumGetArrayTypeP(argnamesArray), TEXTOID,
                              typeTup->typlen, typeTup->typbyval, typeTup->typalign,
                              &argnames, NULL, &len);
            if (len != pinfo->nargs) {
                elog(FATAL, "something bad happened, nargs != len");
            }

            pinfo->argnames = plc_top_alloc(pinfo->nargs * sizeof(char*));
            for (i = 0; i < pinfo->nargs; i++) {
                pinfo->argnames[i] =
                    plc_top_strdup(DatumGetCString(
                            DirectFunctionCall1(textout, argnames[i])
                        ));
            }
            ReleaseSysCache(textHeapTup);
        } else {
            pinfo->argtypes = NULL;
            pinfo->argnames = NULL;
        }

        /* Get the text and name of the function */
        srcdatum = SysCacheGetAttr(PROCOID, procHeapTup, Anum_pg_proc_prosrc, &isnull);
        if (isnull)
            elog(ERROR, "null prosrc");
        pinfo->src = plc_top_strdup(DatumGetCString(DirectFunctionCall1(textout, srcdatum)));
        namedatum = SysCacheGetAttr(PROCOID, procHeapTup, Anum_pg_proc_proname, &isnull);
        if (isnull)
            elog(ERROR, "null proname");
        pinfo->name = plc_top_strdup(DatumGetCString(DirectFunctionCall1(nameout, namedatum)));

        /* Cache the function for later use */
        function_cache_put(pinfo);
    }
    ReleaseSysCache(procHeapTup);
    return pinfo;
}

static rawdata *plc_backend_array_next(plcIterator *self) {
    plcArrayMeta *meta;
    ArrayType    *array;
    plcTypeInfo  *typ;
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
    lbounds = (int*)self->position + 2;
    pos     = (int*)self->position + 2 + meta->ndims;

    el = array_ref(array, meta->ndims, pos, typ->typlen,
                   typ->subTypes[0].typlen, typ->subTypes[0].typbyval,
                   typ->subTypes[0].typalign, &isnull);
    if (isnull) {
        res->isnull = 1;
        res->value  = NULL;
    } else {
        res->isnull = 0;
        res->value = fill_callreq_value(el, &typ->subTypes[0]);
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

static plcIterator *init_array_iter(Datum d, plcTypeInfo *argType) {
    ArrayType    *array = DatumGetArrayTypeP(d);
    plcIterator  *iter;
    plcArrayMeta *meta;
    int           i;

    iter = (plcIterator*)palloc(sizeof(plcIterator));
    meta = (plcArrayMeta*)palloc(sizeof(plcArrayMeta));
    iter->meta = meta;

    meta->type = argType->subTypes[0].type;
    meta->ndims = ARR_NDIM(array);
    meta->dims = (int*)palloc(meta->ndims * sizeof(int));
    iter->position = (char*)palloc(sizeof(int) * meta->ndims * 2 + 2);
    ((plcTypeInfo**)iter->position)[0] = argType;
    meta->size = meta->ndims > 0 ? 1 : 0;
    for (i = 0; i < meta->ndims; i++) {
        meta->dims[i] = ARR_DIMS(array)[i];
        meta->size *= ARR_DIMS(array)[i];
        ((int*)iter->position)[i + 2] = ARR_LBOUND(array)[i];
        ((int*)iter->position)[i + meta->ndims + 2] = ARR_LBOUND(array)[i];
    }
    iter->data = (char*)array;
    iter->next = plc_backend_array_next;
    iter->cleanup =  NULL;

    return iter;
}

static char *fill_callreq_value(Datum funcArg, plcTypeInfo *argType) {
    char *out = NULL;
    switch(argType->type) {
        case PLC_DATA_INT1:
            out = (char*)pmalloc(1);
            *((char*)out) = DatumGetBool(funcArg);
            break;
        case PLC_DATA_INT2:
            out = (char*)pmalloc(2);
            *((int16*)out) = DatumGetInt16(funcArg);
            break;
        case PLC_DATA_INT4:
            out = (char*)pmalloc(4);
            *((int32*)out) = DatumGetInt32(funcArg);
            break;
        case PLC_DATA_INT8:
            out = (char*)pmalloc(8);
            *((int64*)out) = DatumGetInt64(funcArg);
            break;
        case PLC_DATA_FLOAT4:
            out = (char*)pmalloc(4);
            *((float4*)out) = DatumGetFloat4(funcArg);
            break;
        case PLC_DATA_FLOAT8:
            out = (char*)pmalloc(8);
            *((float8*)out) = DatumGetFloat8(funcArg);
            break;
        case PLC_DATA_TEXT:
            out = DatumGetCString(OidFunctionCall1(argType->output, funcArg));
            break;
        case PLC_DATA_ARRAY:
            out = (char*)init_array_iter(funcArg, argType);
            break;
        case PLC_DATA_RECORD:
        case PLC_DATA_UDT:
        default:
            lprintf(ERROR, "Type %d is not yet supported by PLcontainer", (int)argType->type);
    }
    return out;
}

static void copy_type_info(plcType *type, plcTypeInfo *ptype) {
    type->type = ptype->type;
    type->nSubTypes = ptype->nSubTypes;
    if (type->nSubTypes > 0) {
        int i = 0;
        type->subTypes = (plcType*)pmalloc(type->nSubTypes * sizeof(plcType));
        for (i = 0; i < type->nSubTypes; i++)
            copy_type_info(&type->subTypes[i], &ptype->subTypes[i]);
    } else {
        type->subTypes = NULL;
    }
}

static void
fill_callreq_arguments(FunctionCallInfo fcinfo, plcProcInfo *pinfo, callreq req) {
    int   i;

    req->nargs = pinfo->nargs;
    req->args  = pmalloc(sizeof(*req->args) * pinfo->nargs);

    for (i = 0; i < pinfo->nargs; i++) {
        req->args[i].name = pinfo->argnames[i];
        copy_type_info(&req->args[i].type, &pinfo->argtypes[i]);

        if (fcinfo->argnull[i]) {
            req->args[i].data.isnull = 1;
            req->args[i].data.value = NULL;
        } else {
            req->args[i].data.isnull = 0;
            req->args[i].data.value = fill_callreq_value(fcinfo->arg[i], &pinfo->argtypes[i]);
        }
    }
}

callreq
plcontainer_create_call(FunctionCallInfo fcinfo, plcProcInfo *pinfo) {
    callreq   req;

    req          = pmalloc(sizeof(*req));
    req->msgtype = MT_CALLREQ;
    req->proc.name = pinfo->name;
    req->proc.src  = pinfo->src;
    copy_type_info(&req->retType, &pinfo->rettype);

    fill_callreq_arguments(fcinfo, pinfo, req);

    return req;
}

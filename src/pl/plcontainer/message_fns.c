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

/* message and function definitions */
#include "common/comm_logging.h"
#include "common/messages/messages.h"
#include "message_fns.h"

static void function_cache_up(int index);
static plcProcInfo *function_cache_get(Oid funcOid);
static void function_cache_put(plcProcInfo *func);
static void free_proc_info(plcProcInfo *proc);
static void fill_type_info(Oid typoid, plcTypeInfo *type);
static bool plc_procedure_valid(plcProcInfo *proc, HeapTuple procTup);
static void fill_callreq_arguments(FunctionCallInfo fcinfo, plcProcInfo *pinfo,
                                   callreq req);

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
    /* If the function is not cached already */
    if (!function_cache_get(func->funcOid)) {
        /* If the last element exists we need to free its memory */
        if (plcFunctionCache[PLC_FUNCTION_CACHE_SIZE-1] != NULL) {
            free_proc_info(plcFunctionCache[PLC_FUNCTION_CACHE_SIZE-1]);
        }
        /* Move our LRU cache right */
        for (i = PLC_FUNCTION_CACHE_SIZE-1; i > 0; i--) {
            plcFunctionCache[i] = plcFunctionCache[i-1];
        }
        plcFunctionCache[0] = func;
    }
}

static void free_proc_info(plcProcInfo *proc) {
    int i;
    for (i = 0; i < proc->nargs; i++) {
        free(proc->argnames[i]);
        free(proc->argtypes[i].name);
    }
    if (proc->nargs > 0) {
        free(proc->argnames);
        free(proc->argtypes);
    }
    free(proc->rettype.name);
    free(proc);
}

static void fill_type_info(Oid typoid, plcTypeInfo *type) {
    HeapTuple    typeTup;
    Form_pg_type typeStruct;

    typeTup = SearchSysCache(TYPEOID, typoid, 0, 0, 0);
    if (!HeapTupleIsValid(typeTup))
        elog(ERROR, "cache lookup failed for type %u", typoid);

    typeStruct = (Form_pg_type)GETSTRUCT(typeTup);
    ReleaseSysCache(typeTup);

    /* Disallow non basic types */
    if (typeStruct->typtype != TYPTYPE_BASE) {
        elog(ERROR, "plpython functions don't support non-basic types: %s",
             format_type_be(typoid));
    }

    type->name   = strdup(NameStr(typeStruct->typname));
    type->output = typeStruct->typoutput;
    type->input  = typeStruct->typinput;
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
    procTup = (Form_pg_proc)GETSTRUCT(procHeapTup);

    pinfo   = function_cache_get(procoid);
    /*
     * All the catalog operations are done only if the cached function
     * information has changed in the catalog
     */
    if (!plc_procedure_valid(pinfo, procHeapTup)) {
        /*
         * Here we are using malloc as the funciton structure should be
         * available across the function handler call
         */
        pinfo = malloc(sizeof(plcProcInfo));
        if (pinfo == NULL) {
            elog(FATAL, "Cannot allocate memory for plcProcInfo structure");
        }
        /* Remember transactional informatin to allow caching */
        pinfo->funcOid = procoid;
        pinfo->fn_xmin = HeapTupleHeaderGetXmin(procHeapTup->t_data);
        pinfo->fn_tid  = procHeapTup->t_self;

        fill_type_info(procTup->prorettype, &pinfo->rettype);

        pinfo->nargs = procTup->pronargs;
        if (pinfo->nargs > 0) {
            pinfo->argtypes = malloc(pinfo->nargs * sizeof(plcTypeInfo));
            for (i = 0; i < pinfo->nargs; i++) {
                fill_type_info(procTup->proargtypes.values[i], pinfo->argtypes + i);
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

            pinfo->argnames = malloc(pinfo->nargs * sizeof(char*));
            for (i = 0; i < pinfo->nargs; i++) {
                pinfo->argnames[i] =
                    strdup(DatumGetCString(
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
        pinfo->src = strdup(DatumGetCString(DirectFunctionCall1(textout, srcdatum)));
        namedatum = SysCacheGetAttr(PROCOID, procHeapTup, Anum_pg_proc_proname, &isnull);
        if (isnull)
            elog(ERROR, "null proname");
        pinfo->name = strdup(DatumGetCString(DirectFunctionCall1(nameout, namedatum)));

        /* Cache the function for later use */
        function_cache_put(pinfo);
    }
    ReleaseSysCache(procHeapTup);
    return pinfo;
}

static void
fill_callreq_arguments(FunctionCallInfo fcinfo, plcProcInfo *pinfo, callreq req) {
    int   i;
    Datum val;

    req->nargs = pinfo->nargs;
    req->args  = pmalloc(sizeof(*req->args) * pinfo->nargs);

    for (i = 0; i < pinfo->nargs; i++) {
        req->args[i].name = pinfo->argnames[i];
        req->args[i].type = pinfo->argtypes[i].name;

        if (fcinfo->argnull[i]) {
            //TODO: this can't be freed so it will probably fail
            /*
             * Use \N to mark NULL
             */
            req->args[i].value = pstrdup("\\N");
        }else{
            val                = fcinfo->arg[i];
            req->args[i].value =
                DatumGetCString(OidFunctionCall1(pinfo->argtypes[i].output, val));
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

    fill_callreq_arguments(fcinfo, pinfo, req);

    return req;
}

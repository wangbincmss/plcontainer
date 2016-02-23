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

static void
fill_type_info(Oid typoid, type_info *type) {
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

    type->name   = pstrdup(NameStr(typeStruct->typname));
    type->output = typeStruct->typoutput;
    type->input  = typeStruct->typinput;
}

void
fill_proc_info(FunctionCallInfo fcinfo, proc_info *pinfo) {
    int          i, len;
    Datum *      argnames, argnamesArray;
    bool         isnull;
    Oid          procoid;
    Form_pg_proc procTup;
    HeapTuple    procHeapTup, textHeapTup;
    Form_pg_type typeTup;

    procoid     = fcinfo->flinfo->fn_oid;
    procHeapTup = SearchSysCache(PROCOID, procoid, 0, 0, 0);
    if (!HeapTupleIsValid(procHeapTup)) {
        elog(ERROR, "cannot find proc with oid %u", procoid);
    }

    procTup = (Form_pg_proc)GETSTRUCT(procHeapTup);

    fill_type_info(procTup->prorettype, &pinfo->rettype);

    pinfo->nargs = procTup->pronargs;
    if (pinfo->nargs == 0) {
        ReleaseSysCache(procHeapTup);
        return;
    }

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

    for (i = 0; i < pinfo->nargs; i++) {
        pinfo->argnames[i] =
            DatumGetCString(DirectFunctionCall1(textout, argnames[i]));
    }

    ReleaseSysCache(procHeapTup);
    ReleaseSysCache(textHeapTup);
}

static void
fill_callreq_arguments(FunctionCallInfo fcinfo, proc_info *pinfo, callreq req) {
    int   i;
    Datum val;

    req->nargs = pinfo->nargs;
    req->args  = pmalloc(sizeof(*req->args) * pinfo->nargs);

    for (i = 0; i < pinfo->nargs; i++) {
        req->args[i].name = pinfo->argnames[i];
        req->args[i].type = pinfo->argtypes[i].name;

        if (fcinfo->argnull[i]){
            //TODO: this can't be freed so it will probably fail
            req->args[i].value = "\\N";
        }else{
            val               = fcinfo->arg[i];
            req->args[i].value =
                DatumGetCString(OidFunctionCall1(pinfo->argtypes[i].output, val));
        }
    }
}

callreq
plcontainer_create_call(FunctionCallInfo fcinfo, proc_info *pinfo) {
    Datum     srcdatum, namedatum;
    bool      isnull;
    HeapTuple procTup;
    Oid       fn_oid;
    callreq   req;

    req          = pmalloc(sizeof(*req));
    req->msgtype = MT_CALLREQ;

    fn_oid  = fcinfo->flinfo->fn_oid;
    procTup = SearchSysCache(PROCOID, ObjectIdGetDatum(fn_oid), 0, 0, 0);
    if (!HeapTupleIsValid(procTup))
        elog(ERROR, "cache lookup failed for function %u", fn_oid);

    /* get the text and name of the function. */
    srcdatum = SysCacheGetAttr(PROCOID, procTup, Anum_pg_proc_prosrc, &isnull);
    if (isnull)
        elog(ERROR, "null prosrc");
    req->proc.src = DatumGetCString(DirectFunctionCall1(textout, srcdatum));
    namedatum =
        SysCacheGetAttr(PROCOID, procTup, Anum_pg_proc_proname, &isnull);
    if (isnull)
        elog(ERROR, "null pronamem");
    req->proc.name = DatumGetCString(DirectFunctionCall1(nameout, namedatum));

    ReleaseSysCache(procTup);

    fill_callreq_arguments(fcinfo, pinfo, req);

    return req;
}

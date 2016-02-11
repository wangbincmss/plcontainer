
/**
 * file name:		plj-callmkr.c
 * description:		PL/pgJ call message creator routine. This file
 *			was renamed from plpgj_call_maker.c
 *			It is replaceable with pljava-way of declaring java
 *			method calls. (read the readme!)
 * author:		Laszlo Hornyak
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
        val               = fcinfo->arg[i];
        req->args[i].name = pinfo->argnames[i];
        req->args[i].type = pinfo->argtypes[i].name;
        req->args[i].value =
            DatumGetCString(OidFunctionCall1(pinfo->argtypes[i].output, val));
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

#ifndef PLPGJ_MESSAGE_FNS
#define PLPGJ_MESSAGE_FNS

#include "common/messages/messages.h"
#include "fmgr.h"

typedef struct {
    char *name;
    /* used to convert a given value from/to "...." */
    RegProcedure output, input;
} type_info;

typedef struct {
    int       nargs;
    type_info rettype;
    char *    argnames[FUNC_MAX_ARGS];
    type_info argtypes[FUNC_MAX_ARGS];
} proc_info;

void fill_proc_info(FunctionCallInfo fcinfo, proc_info *pinfo);

callreq plcontainer_create_call(FunctionCallInfo fcinfo, proc_info *pinfo);

#endif

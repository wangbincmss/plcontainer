#ifndef PLC_MESSAGE_CALLREQ_H
#define PLC_MESSAGE_CALLREQ_H

#include "message_base.h"

typedef struct {
    char *src;  // source code of the procedure
    char *name; // name of procedure
} plcProcSrc;

typedef struct call_req {
    base_message_content;
    plcProcSrc   proc;
    plcType      retType;
    int          nargs;
    plcArgument *args;
} call_req, *callreq;

/*
  Frees a callreq and all subfields of the struct, this function
  assumes ownership of all pointers in the struct and substructs
*/
void free_callreq(callreq req);

#endif /* PLC_MESSAGE_CALLREQ_H */
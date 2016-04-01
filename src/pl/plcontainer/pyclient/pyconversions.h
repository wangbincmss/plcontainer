#ifndef PLC_PYCONVERSIONS_H
#define PLC_PYCONVERSIONS_H

#include <Python.h>
#include "common/messages/messages.h"

typedef PyObject *(*plcPyInputFunc)(char*);
typedef int (*plcPyOutputFunc)(PyObject*, char**, char);

typedef struct plcPyTypeConverter {
    plcPyInputFunc  inputfunc;
    plcPyOutputFunc outputfunc;
} plcPyTypeConverter;

typedef struct plcPyCallReq {
    callreq             call;
    plcPyTypeConverter *inconv;
    plcPyTypeConverter *outconv;
} plcPyCallReq;

typedef struct plcPyResult {
    plcontainer_result  res;
    plcPyTypeConverter *inconv;
} plcPyResult;

plcPyCallReq *plc_init_call_conversions(callreq call);
plcPyResult  *plc_init_result_conversions(plcontainer_result res);
void plc_free_call_conversions(plcPyCallReq *req);
void plc_free_result_conversions(plcPyResult *res);

#endif /* PLC_PYCONVERSIONS_H */
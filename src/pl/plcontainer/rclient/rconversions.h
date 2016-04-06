#ifndef PLC_RCONVERSIONS_H
#define PLC_RCONVERSIONS_H

#include <R.h>
#include <Rversion.h>
#include <Rembedded.h>
#include <Rinternals.h>
#include <R_ext/Parse.h>
#include <Rdefines.h>

#if (R_VERSION >= 132352) /* R_VERSION >= 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)               R_ParseVector(a_, b_, (ParseStatus *) c_, R_NilValue)
#else /* R_VERSION < 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)               R_ParseVector(a_, b_, (ParseStatus *) c_)
#endif /* R_VERSION >= 2.5.0 */

/* R's definition conflicts with the ones defined by postgres */
#undef WARNING
#undef ERROR

#include "common/messages/messages.h"
#include "common/comm_utils.h"

typedef struct plcRType plcRType;
typedef SEXP (*plcRInputFunc)(char*);
typedef int (*plcROutputFunc)(SEXP , char**, plcRType*);

typedef struct plcRTypeConv {
    plcRInputFunc  inputfunc;
    plcROutputFunc outputfunc;
} plcRTypeConv;

typedef struct plcRResult {
    plcontainer_result  res;
    plcRTypeConv      *inconv;
} plcRResult;

typedef struct plcRType {
    plcDatatype    type;
    char          *name;
    int            nSubTypes;
    plcRType      *subTypes;
    plcRTypeConv  conv;
} plcRType;

typedef struct plcRFunction {
    plcProcSrc  proc;
    callreq     call;
    SEXP   		RProc;
    int         nargs;
    plcRType  *args;
    plcRType   res;
} plcRFunction;

SEXP get_r_vector(plcDatatype type_id, int numels);
int get_entry_length(plcDatatype type);


#endif //PLC_RCONVERSIONS_H

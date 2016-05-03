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
#ifdef WARNING
#undef WARNING

#define WARNING		19			/* Warnings.  NOTICE is for expected messages
								 * like implicit sequence creation by SERIAL.
								 * WARNING is for unexpected messages. */
#endif
#ifdef ERROR
#undef ERROR
#define ERROR		20			/* user error - abort transaction; return to
								 * known state */
#endif

#include "common/messages/messages.h"
#include "common/comm_utils.h"


typedef struct plcRType plcRType;
typedef SEXP (*plcRInputFunc)(char*);
typedef int (*plcROutputFunc)(SEXP , char**, plcRType*);

#define PLC_MAX_ARRAY_DIMS 2

typedef struct plcRArrPointer {
    size_t    pos;
    SEXP obj;
} plcRArrPointer;

typedef struct plcRArrMeta {
    int              ndims;
    size_t          *dims;
    plcRType       *type;
    plcROutputFunc  outputfunc;
} plcRArrMeta;


typedef struct plcRTypeConv {
    plcRInputFunc  inputfunc;
    plcROutputFunc outputfunc;
} plcRTypeConv;

typedef struct plcRResult {
    plcontainer_result  res;
    plcRTypeConv      *inconv;
} plcRResult;

struct plcRType {
    plcDatatype    type;
    char          *name;
    int            nSubTypes;
    plcRType      *subTypes;
    plcRTypeConv  conv;
};

typedef struct plcRFunction {
    plcProcSrc  proc;
    callreq     call;
    SEXP   		RProc;
    int         nargs;
    plcRType  *args;
    plcRType   res;
} plcRFunction;

int get_entry_length(plcDatatype type);
plcRFunction *plc_R_init_function(callreq call) ;
void plc_r_copy_type(plcType *type, plcRType *pytype);
void plc_r_free_function(plcRFunction *func);

#endif //PLC_RCONVERSIONS_H

#ifndef PLC_RCONVERSIONS_H
#define PLC_RCONVERSIONS_H

#include <R.h>
#include <Rembedded.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include "common/messages/messages.h"
#include "common/comm_utils.h"

#define PLC_MAX_ARRAY_DIMS 2

typedef struct plcRType plcRType;
typedef SEXP (*plcRInputFunc)(char*);
typedef int (*plcROutputFunc)(SEXP , char**, plcRType*);

typedef struct plcRArrPointer {
    size_t pos;
    SEXP   obj;
} plcRArrPointer;

typedef struct plcRArrMeta {
    int             ndims;
    size_t         *dims;
    plcRType       *type;
    plcROutputFunc  outputfunc;
} plcRArrMeta;

typedef struct plcRTypeConv {
    plcRInputFunc  inputfunc;
    plcROutputFunc outputfunc;
} plcRTypeConv;

typedef struct plcRResult {
    plcontainer_result  res;
    plcRTypeConv       *inconv;
} plcRResult;

struct plcRType {
    plcDatatype    type;
    char          *name;
    int            nSubTypes;
    plcRType      *subTypes;
    plcRTypeConv   conv;
};

typedef struct plcRFunction {
    plcProcSrc  proc;
    callreq     call;
    SEXP        RProc;
    int         nargs;
    int         retset;
    unsigned int  objectid;
    plcRType  *args;
    plcRType   res;
} plcRFunction;

int get_entry_length(plcDatatype type);
plcRFunction *plc_R_init_function(callreq call) ;
void plc_r_copy_type(plcType *type, plcRType *pytype);
void plc_r_free_function(plcRFunction *func);
SEXP get_r_vector(plcDatatype type_id, int numels);
rawdata *plc_r_vector_element_rawdata(SEXP vector, int idx, plcDatatype type);
int plc_r_matrix_as_setof(SEXP input, int start, int dim1, char **output, plcRType *type);

#endif /* PLC_RCONVERSIONS_H */

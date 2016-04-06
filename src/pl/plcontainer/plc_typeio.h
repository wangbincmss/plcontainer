#ifndef PLC_TYPEIO_H
#define PLC_TYPEIO_H

#include "postgres.h"

#include "common/messages/messages.h"
#include "plcontainer.h"

typedef struct plcTypeInfo plcTypeInfo;
typedef char *(*plcDatumOutput)(Datum, plcTypeInfo*);

struct plcTypeInfo {
    plcDatatype     type;
    Oid             typeOid;
    RegProcedure    output, input; /* used to convert a given value from/to "...." */
    plcDatumOutput  outfunc;
    Oid             typioparam;
    bool            typbyval;
    int16           typlen;
    char            typalign;
    int             nSubTypes;
    plcTypeInfo    *subTypes;
};

Datum get_array_datum(plcArray *arr, plcTypeInfo *ret_type);
//void get_tuple_store( MemoryContext oldContext, MemoryContext messageContext,
//        ReturnSetInfo *rsinfo,plcontainer_result res, int *isNull );
plcIterator *init_array_iter(Datum d, plcTypeInfo *argType);

void fill_type_info(Oid typeOid, plcTypeInfo *type);
void copy_type_info(plcType *type, plcTypeInfo *ptype);
void free_type_info(plcTypeInfo *types, int ntypes);
char *fill_type_value(Datum funcArg, plcTypeInfo *argType);

#endif /* PLC_TYPEIO_H */
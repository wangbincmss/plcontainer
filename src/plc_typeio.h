#ifndef PLC_TYPEIO_H
#define PLC_TYPEIO_H

#include "postgres.h"

#include "common/messages/messages.h"
#include "plcontainer.h"

typedef struct plcTypeInfo plcTypeInfo;
typedef char *(*plcDatumOutput)(Datum, plcTypeInfo*);
typedef Datum (*plcDatumInput)(char*, plcTypeInfo*);

struct plcTypeInfo {
    plcDatatype     type;
    Oid             typeOid;
    RegProcedure    output, input; /* used to convert a given value from/to "...." */
    plcDatumOutput  outfunc;
    plcDatumInput   infunc;
    Oid             typioparam;
    Oid             typelem;
    bool            typbyval;
    int16           typlen;
    char            typalign;
    int32           typmod;
    int             nSubTypes;
    plcTypeInfo    *subTypes;
};

void fill_type_info(Oid typeOid, plcTypeInfo *type, int issubtype);
void copy_type_info(plcType *type, plcTypeInfo *ptype);
void free_type_info(plcTypeInfo *types, int ntypes);
char *fill_type_value(Datum funcArg, plcTypeInfo *argType);

#endif /* PLC_TYPEIO_H */
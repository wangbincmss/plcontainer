#ifndef PLC_TYPEIO_H
#define PLC_TYPEIO_H

#include "postgres.h"
#include "funcapi.h"

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

    /* UDT-specific information */

    /* used to check if the composite type has been modified */
    bool            is_rowtype;
    bool            attisdropped;
    Oid             typ_relid;
    TransactionId   typrel_xmin;
    ItemPointerData typrel_tid;

    /* used in UDT as additional information for tuple input and output */
    TupleDesc       tupleDesc;
};

void fill_type_info(Oid typeOid, plcTypeInfo *pgtype, bool issubtype);
void copy_type_info(plcType *type, plcTypeInfo *ptype);
void free_type_info(plcTypeInfo *types, int ntypes);
char *fill_type_value(Datum funcArg, plcTypeInfo *argType);

#endif /* PLC_TYPEIO_H */
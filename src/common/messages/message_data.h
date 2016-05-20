#ifndef PLC_MESSAGE_DATA_H
#define PLC_MESSAGE_DATA_H

#include "message_base.h"

typedef struct plcIterator plcIterator;

typedef struct plcArrayMeta {
    plcDatatype  type;  // deprecated - should be moved to payload if required
    int          ndims;
    int         *dims;
    int          size;  // deprecated - should be moved to payload if required
} plcArrayMeta;

typedef struct plcArray {
    plcArrayMeta *meta;
    char         *data;
    char         *nulls;
} plcArray;

struct plcIterator {
    plcArrayMeta *meta;
    char         *data;
    char         *position;
    char         *payload;
    /*
     * used to return next element from client-side array structure to avoid
     * creating a copy of full array before sending it
     */
    rawdata *(*next)(plcIterator *self);
    /*
     * called after data is sent to free data
     */
    void (*cleanup)(plcIterator *self);
};

typedef struct plcUDT {
    int       nargs;
    plcType  *types;
    bool     *nulls;
    char    **data;
} plcUDT;

plcArray *plc_alloc_array(int ndims);
void plc_free_array(plcArray *arr);

#endif /* PLC_MESSAGE_DATA_H */

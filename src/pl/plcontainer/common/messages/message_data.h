#ifndef PLC_MESSAGE_DATA_H
#define PLC_MESSAGE_DATA_H

#include "message_base.h"

typedef struct plcIterator plcIterator;

struct plcIterator {
    char *data;
    char *meta;
    char *position;
    rawdata *(*next)(plcIterator *self);
};

typedef struct plcArrayMeta {
    plcDatatype  type;
    int          ndims;
    int          size;
    int         *dims;
} plcArrayMeta;

typedef struct plcArray {
    plcArrayMeta *meta;
    char         *data;
    char         *nulls;
} plcArray;

plcArray *plc_alloc_array(int ndims);
void plc_free_array(plcArray *arr);

#endif /* PLC_MESSAGE_DATA_H */
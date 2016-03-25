#ifndef PLC_MESSAGE_DATA_H
#define PLC_MESSAGE_DATA_H

#include "message_base.h"

typedef struct str_plcontainer_iterator *plcontainer_iterator, str_plcontainer_iterator;

struct str_plcontainer_iterator {
    char *data;
    char *meta;
    char *position;
    rawdata *(*next)(plcontainer_iterator self);
};

typedef struct str_plcontainer_array_meta {
    plcDatatype  type;
    int          ndims;
    int          size;
    int         *dims;
} str_plcontainer_array_meta, *plcontainer_array_meta;

typedef struct str_plcontainer_array {
    plcontainer_array_meta meta;
    char *data;
    char *nulls;
} str_plcontainer_array, *plcontainer_array;

plcontainer_array plc_alloc_array(int ndims);
void plc_free_array(plcontainer_array arr);

#endif /* PLC_MESSAGE_DATA_H */
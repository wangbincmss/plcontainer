#ifndef PLPGJ_MESSAGE_RESULT_H
#define PLPGJ_MESSAGE_RESULT_H
#include "message_base.h"

typedef struct str_plcontainer_result {
    base_message_content;
    int    rows;
    int    cols;
    char **types;
    char **names;
    raw   *data;
} str_plcontainer_result, *plcontainer_result;

typedef struct str_plcontainer_iterator *plcontainer_iterator, str_plcontainer_iterator;

struct str_plcontainer_iterator {
    char *data;
    char *meta;
    char *position;
    raw (*next)(plcontainer_iterator self);
};

typedef struct str_plcontainer_array_meta {
    int  ndims;
    int  size;
    int *dims;
} str_plcontainer_array_meta, *plcontainer_array_meta;

typedef struct str_plcontainer_array {
    plcontainer_array_meta meta;
    raw data;
} str_plcontainer_array, *plcontainer_array;

void free_result(plcontainer_result res);
plcontainer_array plc_alloc_array(int ndims);
void plc_free_array(plcontainer_array arr);

#endif

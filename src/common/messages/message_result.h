#ifndef PLC_MESSAGE_RESULT_H
#define PLC_MESSAGE_RESULT_H

#include "message_base.h"

typedef struct str_plcontainer_result {
    base_message_content;
    int           rows;
    int           cols;
    plcType      *types;
    char        **names;
    rawdata     **data;
} str_plcontainer_result, *plcontainer_result;

void free_result(plcontainer_result res);

#endif /* PLC_MESSAGE_RESULT_H */
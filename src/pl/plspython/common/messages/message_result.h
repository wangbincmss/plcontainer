#ifndef PLPGJ_MESSAGE_RESULT_H
#define PLPGJ_MESSAGE_RESULT_H
#include "message_base.h"

typedef struct {
    base_message_content;
    int    rows;
    int    cols;
    char **types;
    char **names;
    raw *  data;
} * plspython_result;

void free_result(plspython_result res);

#endif

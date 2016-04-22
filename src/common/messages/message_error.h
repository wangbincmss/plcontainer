#ifndef PLC_MESSAGE_ERROR_H
#define PLC_MESSAGE_ERROR_H

#include "message_base.h"

typedef struct str_error_message {
    base_message_content;
    char *message;
    char *stacktrace;
} str_error_message, *error_message;

#endif /* PLC_MESSAGE_ERROR_H */

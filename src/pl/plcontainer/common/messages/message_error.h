#ifndef PLPGJ_ERROR_MESSAGE
#define PLPGJ_ERROR_MESSAGE

#include "message_base.h"

typedef struct str_error_message {
    base_message_content;
    char *message;
    char *stacktrace;
} str_error_message, *error_message;

#endif

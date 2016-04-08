#ifndef PLC_MESSAGE_LOG_H
#define PLC_MESSAGE_LOG_H

#include "message_base.h"

typedef struct str_log_message {
    base_message_content;
    int   level;
    char *message;
} str_log_message, *log_message;

#endif /* PLC_MESSAGE_LOG_H */
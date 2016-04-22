#ifndef PLC_MESSAGE_PING_H
#define PLC_MESSAGE_PING_H

#include "message_base.h"

typedef struct str_ping_message {
    base_message_content;
} str_ping_message, *ping_message;

#endif /* PLC_MESSAGE_PING_H */
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
    /* Callback called from message sending function to return the error message
     * generated during the period engine could not send it */
    void        *(*exception_callback)(void);
} str_plcontainer_result, *plcontainer_result;

void free_result(plcontainer_result res, bool isSender);

#endif /* PLC_MESSAGE_RESULT_H */
#include <R.h>
#include <Rinternals.h>

#include "common/messages/messages.h"
#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "rcall.h"
#include "rlogging.h"

static SEXP plr_output(volatile int,  char *args);

SEXP plr_debug(char *args) {
    return plr_output(DEBUG2, args);
}

SEXP plr_log( char *args) {
    return plr_output(LOG,  args);
}

SEXP plr_info( char *args) {
    return plr_output(INFO,  args);
}

SEXP plr_notice( char *args) {
    return plr_output(NOTICE,  args);
}

SEXP plr_warning( char *args) {
    return plr_output(WARNING,  args);
}

SEXP plr_error( char *args) {
    return plr_output(ERROR,  args);
}

SEXP plr_fatal( char *args) {
    return plr_output(FATAL,  args);
}

static SEXP plr_output(volatile int level, char *msg_arg) {
    plcConn     *conn = plcconn_global;
    log_message msg;

    if (plc_is_execution_terminated == 0) {
        char *str_msg = strdup( msg_arg );


        if (level >= ERROR)
            plc_is_execution_terminated = 1;

        msg = pmalloc(sizeof(struct str_log_message));
        msg->msgtype = MT_LOG;
        msg->level = level;
        msg->message = str_msg;

        plcontainer_channel_send(conn, (message)msg);

        free(msg);
        free(str_msg);
    }

    /*
     * return a legal object so the interpreter will continue on its merry way
     */
    return R_NilValue;
}

#include <R.h>
#include <Rversion.h>
#include <Rembedded.h>
#include <Rinternals.h>
#include <R_ext/Parse.h>
#include <Rdefines.h>

#if (R_VERSION >= 132352) /* R_VERSION >= 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)               R_ParseVector(a_, b_, (ParseStatus *) c_, R_NilValue)
#else /* R_VERSION < 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)               R_ParseVector(a_, b_, (ParseStatus *) c_)
#endif /* R_VERSION >= 2.5.0 */

/* R's definition conflicts with the ones defined by postgres */
#ifdef WARNING
#undef WARNING

#define WARNING		19			/* Warnings.  NOTICE is for expected messages
								 * like implicit sequence creation by SERIAL.
								 * WARNING is for unexpected messages. */
#endif
#ifdef ERROR
#undef ERROR
#define ERROR		20			/* user error - abort transaction; return to
								 * known state */
#endif

#include "common/messages/messages.h"
#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "rcall.h"
#include "rlogging.h"

static SEXP plr_output(volatile int,  SEXP);

SEXP plr_debug(SEXP args)
{
    return plr_output(DEBUG2, args);
}

SEXP plr_log( SEXP args)
{
    return plr_output(LOG,  args);
}

SEXP plr_info( SEXP args)
{
    return plr_output(INFO,  args);
}

SEXP plr_notice( SEXP args)
{
    return plr_output(NOTICE,  args);
}

SEXP plr_warning( SEXP args)
{
    return plr_output(WARNING,  args);
}

SEXP plr_error( SEXP args)
{
    return plr_output(ERROR,  args);
}

SEXP plr_fatal( SEXP args)
{
    return plr_output(FATAL,  args);
}


static SEXP plr_output(volatile int level, SEXP args)
{
    plcConn     *conn = plcconn_global;
    log_message msg;

    if (plc_is_execution_terminated == 0) {
    	char *str_msg = strdup( CHAR( asChar(args)));


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

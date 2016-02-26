#include <assert.h>

#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>

#include <R.h>
#include <Rembedded.h>
#include <Rinternals.h>
#include <R_ext/Parse.h>

/* R's definition conflicts with the ones defined by postgres */
#undef WARNING
#undef ERROR

#include "common/comm_channel.h"
#include "common/comm_logging.h"
#include "common/comm_connectivity.h"
#include "common/comm_server.h"
#include "rcall.h"

/*
  based on examples from:
  1. https://github.com/parkerabercrombie/call-r-from-c/blob/master/r_test.c
  2. https://github.com/wch/r-source/tree/trunk/tests/Embedding
  3. http://pabercrombie.com/wordpress/2014/05/how-to-call-an-r-function-from-c/

  Other resources:
  - https://cran.r-project.org/doc/manuals/r-release/R-exts.html
  - http://adv-r.had.co.nz/C-interface.html
 */

static char *create_r_func(const char *, const char *);
static void send_error(plcConn* conn, char *msg);
static SEXP run_r_code(const char *code, int *errorOccured, plcConn* conn);
static char * create_r_func(const char *name, const char *src);

void r_init() {
    char   *argv[] = {"client", "--slave", "--vanilla"};
    int  signals[] = {SIGTERM, SIGSEGV, SIGABRT};
    size_t iter;

    Rf_initEmbeddedR(sizeof(argv) / sizeof(*argv), argv);
    /*
      A temporary hack to force the R environment to exit when termination
      signals are raised. Otherwise, R will prompt the user for what to do and
      will hang in the container
    */
    for (iter = 0; iter < sizeof(signals) / sizeof(*signals); iter++) {
        sighandler_t sig = signal(signals[iter], exit);
        if (sig == SIG_ERR) {
            fprintf(stderr, "Cannot reset signal handler for %d to default\n", signals[iter]);
            exit(1);
        }
    }
}

void handle_call(callreq req, plcConn* conn) {
    SEXP             r, toString, strres;
    int              errorOccured;
    char *           func;
    const char *     txt;
    plcontainer_result res;

    /* wrap the input in a function and evaluate the result */
    func = create_r_func(req->proc.name, req->proc.src);

    PROTECT(r = run_r_code(func, &errorOccured, conn));
    if (errorOccured) {
        /* run_r_code will send an error back */
        UNPROTECT(1);
        return;
    }
    /* get the symbol from the symbol table */
    PROTECT(toString = lang2(install("toString"), r));
    PROTECT(strres = R_tryEval(toString, R_GlobalEnv, &errorOccured));
    if (errorOccured) {
        UNPROTECT(3);
        send_error(conn, "cannot convert value to string");
        return;
    }

    txt = CHAR(asChar(strres));

    /* allocate a result */
    res          = calloc(1, sizeof(*res));
    res->msgtype = MT_RESULT;
    res->types   = malloc(sizeof(*res->types));
    res->names   = malloc(sizeof(*res->names));
    res->data    = malloc(sizeof(*res->data));
    res->data[0] = malloc(sizeof(*res->data[0]));

    res->rows = res->cols = 1;
    res->types[0]         = pstrdup("text");
    res->names[0]         = pstrdup("result");
    res->data[0]->isnull  = false;
    res->data[0]->value   = (char *)txt;

    /* send the result back */
    plcontainer_channel_send(conn, (message)res);

    /* free the result object and the python value */
    free_result(res);

    UNPROTECT(3);

    return;
}

static void send_error(plcConn* conn, char *msg) {
    /* an exception was thrown */
    error_message err;
    err             = malloc(sizeof(*err));
    err->msgtype    = MT_EXCEPTION;
    err->message    = msg;
    err->stacktrace = "";

    /* send the result back */
    plcontainer_channel_send(conn, (message)err);

    /* free the objects */
    free(err);
}

static SEXP run_r_code(const char *code, int *errorOccured, plcConn* conn) {
    /* int hadError; */
    ParseStatus status;
    char *      error;
    SEXP        e, r, tmp;
    int         i;

    PROTECT(tmp = mkString(code));
    /*
      limit the number of expressions to be parsed to 2:
        - the definition of the function, i.e. f <- function() {...}
        - the call to the function f()

      kind of useful to prevent injection, but pointless since we are
      running in a container. I think -1 is equivalent to no limit.
    */
    PROTECT(e = R_ParseVector(tmp, -1, &status, R_NilValue));
    if (status != PARSE_OK) {
        *errorOccured = 1;
        error         = "cannot parse code";
        goto error;
    }

    /* Loop is needed here as EXPSEXP will be of length > 1 */
    for (i = 0; i < length(e); i++) {
        r = R_tryEval(VECTOR_ELT(e, i), R_GlobalEnv, errorOccured);
        if (*errorOccured) {
            error = "cannot evaluate code";
            goto error;
        }
    }

    UNPROTECT(2);

    return r;

error:
    UNPROTECT(2);

    send_error(conn, error);

    return NULL;
}

static char * create_r_func(const char *name, const char *src) {
    int    plen;
    char * mrc;
    size_t mlen;

    /*
     * room for function source and function call
     */
    mlen = strlen(src) + strlen(name) + 40;

    mrc  = malloc(mlen);
    plen = snprintf(mrc, mlen, "%s <- function() {\n%s\n}\n%s()\n", name, src,
                    name);
    assert(plen >= 0 && ((size_t)plen) < mlen);
    return mrc;
}

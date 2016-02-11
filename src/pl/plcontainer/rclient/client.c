#include <assert.h>

#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <R.h>
#include <Rembedded.h>
#include <Rinternals.h>

#include <R_ext/Parse.h>

/* R's definition conflicts with the ones defined by postgres */
#undef WARNING
#undef ERROR

#include "common/comm_channel.h"
#include "common/comm_logging.h"
#include "common/libpq-mini.h"
#include <signal.h>

/*
  based on examples from:
  1. https://github.com/parkerabercrombie/call-r-from-c/blob/master/r_test.c
  2. https://github.com/wch/r-source/tree/trunk/tests/Embedding
  3. http://pabercrombie.com/wordpress/2014/05/how-to-call-an-r-function-from-c/

  Other resources:
  - https://cran.r-project.org/doc/manuals/r-release/R-exts.html
  - http://adv-r.had.co.nz/C-interface.html
 */

static void init();
static void receive_loop();
static void handle_call(callreq req);

static char *progname;

int
main(int argc, char **argv) {
    int                port, fd, sock;
    struct sockaddr_in addr, raddr;
    socklen_t          raddr_len;
    PGconn_min *       conn;

    if (argc > 0) {
        progname = argv[0];
    }

    port = 8080;
    fd   = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    addr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port   = htons(port),
        .sin_addr = {.s_addr = INADDR_ANY},
    };
    if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    if (listen(fd, 10) == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    while (true) {
        raddr_len = sizeof(raddr);
        sock      = accept(fd, (struct sockaddr *)&raddr, &raddr_len);
        if (sock == -1) {
            lprintf(ERROR, "failed to accept connection: %s", strerror(errno));
        }

        conn          = pq_min_connect_fd(sock);
        //conn->Pfdebug = stdout;
        plcontainer_channel_initialize(conn);

        receive_loop();

#ifndef _DEBUG_CLIENT
        break;
#endif

    }

    lprintf(NOTICE, "Client has finished execution");
    return 0;
}

void
receive_loop() {
    message msg;

    init();

    while (true) {
        msg = plcontainer_channel_receive();

        if (msg == NULL) {
            break;
        }

        switch (msg->msgtype) {
        case MT_CALLREQ:
            handle_call((callreq)msg);
            break;
        default:
            lprintf(ERROR, "received unknown message: %c", msg->msgtype);
        }
    }
}

/* VVVVVVVVVVVVVVVV R specific code below this line VVVVVVVVVVVVVVVV */

static char *create_r_func(const char *, const char *);

static void
send_error(char *msg) {
    /* an exception was thrown */
    error_message err;
    err             = malloc(sizeof(*err));
    err->msgtype    = MT_EXCEPTION;
    err->message    = msg;
    err->stacktrace = "";

    /* send the result back */
    plcontainer_channel_send((message)err);

    /* free the objects */
    free(err);
}

static SEXP
run_r_code(const char *code, int *errorOccured) {
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

    send_error(error);

    return NULL;
}

static void
handle_call(callreq req) {
    SEXP             r, toString, strres;
    int              errorOccured;
    char *           func;
    const char *     txt;
    plcontainer_result res;

    /* wrap the input in a function and evaluate the result */
    func = create_r_func(req->proc.name, req->proc.src);

    PROTECT(r = run_r_code(func, &errorOccured));
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
        send_error("cannot convert value to string");
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
    plcontainer_channel_send((message)res);

    /* free the result object and the python value */
    free_result(res);

    UNPROTECT(3);

    return;
}

static char *
create_r_func(const char *name, const char *src) {
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

typedef void (*sighandler_t)(int);

void
init_R(int argc, char **argv) {
    sighandler_t sig;

    Rf_initEmbeddedR(argc, argv);
    /*
      A temporary hack to force the R environment to exit when a
      SIGABRT is raised. Otherwise, R will prompt the user for what to
      do and will hang in the container. Although `signal` is not
      recommended for portabile code, we have a very controlled
      environment in the container, so it doesn't really matter.
    */
    sig = signal(SIGABRT, exit);
    if (sig == SIG_ERR) {
        fprintf(stderr, "cannot reset signal handler to default");
        exit(1);
    }
}

void
init() {
    /* Run `R --help` for help on what these arguments mean */
    char *argv[] = {"client", "--slave", "--vanilla"};
    init_R(sizeof(argv) / sizeof(*argv), argv);
}

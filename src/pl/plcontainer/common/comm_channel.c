#include "comm_channel.h"
#include "comm_logging.h"
#include "libpq-mini-misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

PGconn_min *min_conn = NULL;

static void send_char(char);
static void send_integer_4(int);
static void send_bytes(const char *, size_t);
static void send_string(const char *);

static void send_call(callreq call);
static void send_result(plcontainer_result res);
static void send_exception(error_message err);
static void send_sql(sql_msg);
static void send_sql_statement(sql_msg_statement msg);

/* static void send_trigger(trigger_callreq call); */
/* static void send_trigger_tuple(pparam *tuple, int colcount); */

static char  receive_char(void);
static int   receive_integer_4(void);
static char *receive_string(void);
static void  receive_bytes(char **, size_t *);

static void *  receive_exception(void);
static void *  receive_result(void);
static message receive_log(void);
/* static message receive_tupres(void); */

static sql_msg              receive_sql(void);
static callreq              receive_call(void);
static sql_msg_cursor_fetch receive_sql_fetch(void);
static sql_msg_statement    receive_sql_statement(void);
static sql_msg_prepare      receive_sql_prepare(void);
static sql_pexecute         receive_sql_pexec(void);
static sql_msg_cursor_close receive_sql_cursorclose(void);
static sql_msg_unprepare    receive_sql_unprepare(void);
static sql_msg_cursor_open  receive_sql_opencursor_sql();

void
plcontainer_channel_initialize(PGconn_min *conn) {
    min_conn = conn;
}

bool
plcontainer_channel_initialized() {
    return min_conn != NULL;
}

void
plcontainer_channel_send(message msg) {
    switch (msg->msgtype) {
    case MT_CALLREQ:
        send_call((callreq)msg);
        break;
    case MT_RESULT:
        send_result((plcontainer_result)msg);
        break;
    case MT_EXCEPTION:
        send_exception((error_message)msg);
        break;
    case MT_SQL:
        send_sql((sql_msg)msg);
        break;
    default:
        lprintf(ERROR, "UNHANDLED MESSAGE: %c", msg->msgtype);
        return;
    }
    pqmPutMsgEnd(min_conn);
    pqmFlush(min_conn);
}

message
plcontainer_channel_receive() {
    message msgret = NULL;
    char    type   = receive_char();
    //elog(DEBUG1, "plcontainer_channel_receive type = %c", type);

    switch (type) {
    case MT_RESULT:
        msgret = (message)receive_result();
        break;
    case MT_EXCEPTION:
        msgret = (message)receive_exception();
        break;
    case MT_LOG:
        msgret = (message)receive_log();
        break;
    case MT_SQL:
        msgret = (message)receive_sql();
        break;
    case MT_CALLREQ:
        msgret = (message)receive_call();
        break;
    case MT_EOF:
        return NULL;
    default:
        lprintf(ERROR, "message type unknown %c", type);
        return NULL;
    }

    msgret->msgtype = type;
    pqmMessageRecvd(min_conn);
    return msgret;
}

static char
receive_char() {
    char c;
    int  ret = pqmGetc(&c, min_conn);
    if (ret == EOF) {
        //lprintf(ERROR, "Unexpected EOF from socket");
        c = MT_EOF;
    }
    return c;
}

static int
receive_integer_4() {
    int i;

    int ret = pqmGetInt(&i, 4, min_conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }

    return i;
}

static void
receive_bytes(char **s, size_t *len) {
    int cnt = receive_integer_4();
    int ret;

    *len = cnt;
    if (cnt == 0) {
        *s = "";
        return;
    }
    *s  = pmalloc(sizeof(char) * (cnt + 1));
    ret = pqmGetnchar(*s, cnt, min_conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static char *
receive_string() {
    char * tmp_chr;
    size_t cnt;
    receive_bytes(&tmp_chr, &cnt);
    if (cnt > 0) {
        /* receive_bytes allocated cnt+1 for us */
        tmp_chr[cnt] = 0;
    }
    return tmp_chr;
}

static void
message_start() {
    int ret = -1;
    ret = pqmPutMsgStart(0, min_conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static void
send_char(char c) {
    int ret = pqmPutc(c, min_conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static void
send_integer_4(int i) {
    int ret = pqmPutInt(i, 4, min_conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static void
send_bytes(const char *s, size_t cnt) {
    int ret;
    send_integer_4(cnt);
    ret = pqmPutMsgBytes(s, cnt, min_conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static void
send_string(const char *s) {
    int cnt = strlen(s);
    send_bytes(s, cnt);
}

static void
send_call(callreq call) {
    int i;

    message_start();
    send_char(MT_CALLREQ);
    send_string(call->proc.name);
    send_string(call->proc.src);
    send_integer_4(call->nargs);

    for (i = 0; i < call->nargs; i++) {
        send_string(call->args[i].name);
        send_string(call->args[i].type);
        send_string(call->args[i].value);
    }
}

static void
send_result(plcontainer_result res) {
    int i, j;
    message_start();
    send_char(MT_RESULT);
    send_integer_4(res->rows);
    send_integer_4(res->cols);

    /* send types */
    for (j = 0; j < res->cols; j++) {
        send_string(res->types[j]);
        send_string(res->names[j]);
    }

    /* send rows */
    for (i = 0; i < res->rows; i++) {
        for (j = 0; j < res->cols; j++) {
            if (res->data[i][j].isnull) {
                send_char('N');
            } else {
                send_char('D');
                send_string(res->data[i][j].value);
            }
        }
    }
}

static void
send_exception(error_message err) {
    message_start();
    send_char(MT_EXCEPTION);
    send_string(err->message);
    send_string(err->stacktrace);
}
static void *
receive_exception() {
    // PQExpBuffer name,
    //			mesg;
    error_message ret;

    // name = createPQExpBuffer();
    // mesg = createPQExpBuffer();

    ret = pmalloc(sizeof(str_error_message));

    ret->message    = receive_string();
    ret->stacktrace = NULL;

    ret->msgtype = MT_EXCEPTION;

    if (min_conn->Pfdebug)
        fflush(min_conn->Pfdebug);
    return ret;
}

static void *
receive_result() {
    plcontainer_result ret;
    int              i, j;  // iterators
    ret          = pmalloc(sizeof(*ret));
    ret->msgtype = MT_RESULT;

    ret->rows = receive_integer_4();
    ret->cols = receive_integer_4();

    if (ret->rows > 0) {
        ret->data = pmalloc((ret->rows) * sizeof(raw));
    } else {
        ret->data  = NULL;
        ret->types = NULL;
    }

    /* types per column or per row ? */
    ret->types = pmalloc(ret->rows * sizeof(*ret->types));
    ret->names = pmalloc(ret->rows * sizeof(*ret->names));

    for (i = 0; i < ret->cols; i++) {
        ret->types[i] = receive_string();
        ret->names[i] = receive_string();
    }

    for (i = 0; i < ret->rows; i++) {
        if (ret->cols > 0) {
            ret->data[i] = pmalloc((ret->cols) * sizeof(*ret->data[i]));
        } else {
            ret->data[i] = NULL;
        }

        for (j = 0; j < ret->cols; j++) {
            char isn;

            isn = receive_char();
            if (isn == 'N') {
                ret->data[i][j].value  = NULL;
                ret->data[i][j].isnull = 1;
            } else {
                ret->data[i][j].isnull = 0;
                ret->data[i][j].value  = receive_string();
            }
        }
    }

    if (min_conn->Pfdebug)
        fflush(min_conn->Pfdebug);
    return ret;
}

static message
receive_log() {
    log_message ret;

    ret          = pmalloc(sizeof(str_log_message));
    ret->msgtype = MT_LOG;

    ret->level    = receive_integer_4();
    ret->category = receive_string();
    ret->message  = receive_string();

    if (min_conn->Pfdebug)
        fflush(min_conn->Pfdebug);

    return (message)ret;
}

static sql_msg_statement
receive_sql_statement() {
    sql_msg_statement ret;

    ret          = (sql_msg_statement)pmalloc(sizeof(struct str_sql_statement));
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_STATEMENT;
    ret->statement = receive_string();
    return ret;
}

static sql_msg_prepare
receive_sql_prepare() {
    sql_msg_prepare ret;
    int             i;

    ret            = (sql_msg_prepare)pmalloc(sizeof(struct str_sql_prepare));
    ret->msgtype   = MT_SQL;
    ret->sqltype   = SQL_TYPE_PREPARE;
    ret->statement = receive_string();
    ret->ntypes    = receive_integer_4();
    ret->types =
        ret->ntypes == 0 ? NULL : pmalloc(ret->ntypes * sizeof(char *));

    //	ret -> ntypes = 0;
    //	ret -> types = NULL;

    for (i = 0; i < ret->ntypes; i++) {
        lprintf(DEBUG1, "%d", i);
        ret->types[i] = receive_string();
    }

    return ret;
}

static sql_pexecute
receive_sql_pexec() {
    sql_pexecute      ret;
    int               i;
    struct fnc_param *param;

    //	pljlprintf(DEBUG1,"receive_sql_pexec");
    ret          = pmalloc(sizeof(struct str_sql_pexecute));
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_PEXECUTE;
    ret->planid  = receive_integer_4();
    ret->action  = receive_integer_4();
    ret->nparams = receive_integer_4();

    if (ret->nparams == 0) {
        ret->params = NULL;
    } else {
        ret->params = pmalloc(ret->nparams * sizeof(struct fnc_param));
    }

    for (i = 0; i < ret->nparams; i++) {
        char isnull;
        isnull = 0;
        lprintf(DEBUG1, ">>>");
        isnull = receive_char();
        param  = &ret->params[i];
        if (isnull == 'N') {
            param->data.isnull = 1;
            param->data.value  = NULL;
        } else {
            if (isnull != 'D')
                lprintf(WARNING, "Should be N or D: %d", (unsigned char)isnull);
            param->data.isnull = 0;
            param->type        = receive_string();
            param->data.value  = receive_string();
        }
    }
    lprintf(DEBUG1, "<<<");
    return ret;
}

static sql_msg_cursor_close
receive_sql_cursorclose() {
    sql_msg_cursor_close ret;

    ret             = pmalloc(sizeof(struct str_sql_msg_cursor_close));
    ret->msgtype    = MT_SQL;
    ret->sqltype    = SQL_TYPE_CURSOR_CLOSE;
    ret->cursorname = receive_string();

    return ret;
}

static sql_msg_unprepare
receive_sql_unprepare() {
    sql_msg_unprepare ret;

    ret          = pmalloc(sizeof(struct str_sql_unprepare));
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_UNPREPARE;
    ret->planid  = receive_integer_4();

    return ret;
}

static sql_msg_cursor_open
receive_sql_opencursor_sql() {
    sql_msg_cursor_open ret;

    ret             = pmalloc(sizeof(struct str_sql_msg_cursor_open));
    ret->msgtype    = MT_SQL;
    ret->sqltype    = SQL_TYPE_CURSOR_OPEN;
    ret->cursorname = receive_string();
    ret->query      = receive_string();

    return ret;
}

static sql_msg_cursor_fetch
receive_sql_fetch() {
    sql_msg_cursor_fetch ret;

    ret             = pmalloc(sizeof(struct str_sql_msg_cursor_fetch));
    ret->msgtype    = MT_SQL;
    ret->sqltype    = SQL_TYPE_FETCH;
    ret->cursorname = receive_string();
    ret->count      = receive_integer_4();
    ret->direction  = receive_integer_4();

    return ret;
}

static void
send_sql_statement(sql_msg_statement msg) {
    message_start();
    send_char(msg->msgtype);
    send_integer_4(msg->sqltype);
    send_string(msg->statement);
}

static void
send_sql(sql_msg msg) {
    switch (msg->sqltype) {
    case SQL_TYPE_STATEMENT:
        send_sql_statement((sql_msg_statement)msg);
        break;
    default:
        lprintf(ERROR, "unhandled message type");
    }
}

static callreq
receive_call() {
    int     i;
    callreq req;

    req            = pmalloc(sizeof(*req));
    req->proc.name = receive_string();
    req->proc.src  = receive_string();
    req->nargs     = receive_integer_4();
    req->args      = pmalloc(sizeof(*req->args) * req->nargs);
    for (i = 0; i < req->nargs; i++) {
        req->args[i].name  = receive_string();
        req->args[i].type  = receive_string();
        req->args[i].value = receive_string();
    }
    return req;
}

static sql_msg
receive_sql() {
    int typ;

    typ = receive_integer_4();
    switch (typ) {
    case SQL_TYPE_STATEMENT:
        return (sql_msg)receive_sql_statement();
    case SQL_TYPE_PREPARE:
        return (sql_msg)receive_sql_prepare();
    case SQL_TYPE_PEXECUTE:
        return (sql_msg)receive_sql_pexec();
    case SQL_TYPE_FETCH:
        return (sql_msg)receive_sql_fetch();
    case SQL_TYPE_CURSOR_CLOSE:
        return (sql_msg)receive_sql_cursorclose();
    case SQL_TYPE_UNPREPARE:
        return (sql_msg)receive_sql_unprepare();
    case SQL_TYPE_CURSOR_OPEN:
        return (sql_msg)receive_sql_opencursor_sql();
    default:
        // pljlogging_error = 1;
        lprintf(ERROR, "UNHANDLED SQL TYPE: %d", typ);
    }
    return NULL;  // which never happens, but syntax failure otherwise.
}

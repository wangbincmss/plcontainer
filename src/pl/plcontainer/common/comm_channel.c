/*
Copyright 1994 The PL-J Project. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE PL-J PROJECT ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE PL-J PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the authors and should not be
interpreted as representing official policies, either expressed or implied, of the PL-J Project.
*/

/**
 * file name:			comm_channel.c
 * description:			Channel API.
 * author:			Laszlo Hornyak Kocka
 */

#include "comm_channel.h"
#include "comm_logging.h"
#include "libpq-mini-misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void send_char(char, PGconn_min*);
static void send_integer_4(int, PGconn_min*);
static void send_bytes(const char *, size_t, PGconn_min*);
static void send_string(const char *, PGconn_min*);

static void send_call(callreq call, PGconn_min*);
static void send_result(plcontainer_result res, PGconn_min*);
static void send_exception(error_message err, PGconn_min*);
static void send_sql(sql_msg, PGconn_min*);
static void send_sql_statement(sql_msg_statement msg, PGconn_min*);

/* static void send_trigger(trigger_callreq call); */
/* static void send_trigger_tuple(pparam *tuple, int colcount); */

static char  receive_char(PGconn_min*);
static int   receive_integer_4(PGconn_min*);
static char *receive_string(PGconn_min*);
static void  receive_bytes(char **, size_t *, PGconn_min*);

static void *  receive_exception(PGconn_min*);
static void *  receive_result(PGconn_min*);
static message receive_log(PGconn_min*);
/* static message receive_tupres(void); */

static sql_msg              receive_sql(PGconn_min*);
static callreq              receive_call(PGconn_min*);
static sql_msg_cursor_fetch receive_sql_fetch(PGconn_min*);
static sql_msg_statement    receive_sql_statement(PGconn_min*);
static sql_msg_prepare      receive_sql_prepare(PGconn_min*);
static sql_pexecute         receive_sql_pexec(PGconn_min*);
static sql_msg_cursor_close receive_sql_cursorclose(PGconn_min*);
static sql_msg_unprepare    receive_sql_unprepare(PGconn_min*);
static sql_msg_cursor_open  receive_sql_opencursor_sql(PGconn_min*);

void
plcontainer_channel_send(message msg, PGconn_min *conn) {
    switch (msg->msgtype) {
        case MT_CALLREQ:
            send_call((callreq)msg, conn);
            break;
        case MT_RESULT:
            send_result((plcontainer_result)msg, conn);
            break;
        case MT_EXCEPTION:
            send_exception((error_message)msg, conn);
            break;
        case MT_SQL:
            send_sql((sql_msg)msg, conn);
            break;
        default:
            lprintf(ERROR, "UNHANDLED MESSAGE: %c", msg->msgtype);
            return;
    }
    pqmPutMsgEnd(conn);
    pqmFlush(conn);
}

message plcontainer_channel_receive(PGconn_min *conn) {
    message msgret = NULL;
    char    type   = receive_char(conn);

    switch (type) {
        case MT_RESULT:
            msgret = (message)receive_result(conn);
            break;
        case MT_EXCEPTION:
            msgret = (message)receive_exception(conn);
            break;
        case MT_LOG:
            msgret = (message)receive_log(conn);
            break;
        case MT_SQL:
            msgret = (message)receive_sql(conn);
            break;
        case MT_CALLREQ:
            msgret = (message)receive_call(conn);
            break;
        case MT_EOF:
            return NULL;
        default:
            lprintf(ERROR, "message type unknown %c", type);
            return NULL;
    }

    msgret->msgtype = type;
    pqmMessageRecvd(conn);
    return msgret;
}

static char receive_char(PGconn_min *conn) {
    char c;
    int  ret = pqmGetc(&c, conn);
    if (ret == EOF) {
        c = MT_EOF;
    }
    return c;
}

static int
receive_integer_4(PGconn_min *conn) {
    int i;

    int ret = pqmGetInt(&i, 4, conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }

    return i;
}

static void receive_bytes(char **s, size_t *len, PGconn_min *conn) {
    int cnt = receive_integer_4(conn);
    int ret;

    *len = cnt;
    if (cnt == 0) {
        *s = "";
        return;
    }
    *s  = pmalloc(sizeof(char) * (cnt + 1));
    ret = pqmGetnchar(*s, cnt, conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static char * receive_string(PGconn_min *conn) {
    char * tmp_chr;
    size_t cnt;
    receive_bytes(&tmp_chr, &cnt, conn);
    if (cnt > 0) {
        /* receive_bytes allocated cnt+1 for us */
        tmp_chr[cnt] = 0;
    }
    return tmp_chr;
}

static void message_start(PGconn_min *conn) {
    int ret = -1;
    ret = pqmPutMsgStart(0, conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static void send_char(char c, PGconn_min *conn) {
    int ret = pqmPutc(c, conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static void send_integer_4(int i, PGconn_min *conn) {
    int ret = pqmPutInt(i, 4, conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static void send_bytes(const char *s, size_t cnt, PGconn_min *conn) {
    int ret;
    send_integer_4(cnt, conn);
    ret = pqmPutMsgBytes(s, cnt, conn);
    if (ret == EOF) {
        lprintf(ERROR, "Unexpected EOF from socket");
    }
}

static void send_string(const char *s, PGconn_min *conn) {
    int cnt = strlen(s);
    send_bytes(s, cnt, conn);
}

static void send_call(callreq call, PGconn_min *conn) {
    int i;

    message_start(conn);
    send_char(MT_CALLREQ, conn);
    send_string(call->proc.name, conn);
    send_string(call->proc.src, conn);
    send_integer_4(call->nargs, conn);

    for (i = 0; i < call->nargs; i++) {
        send_string(call->args[i].name, conn);
        send_string(call->args[i].type, conn);
        send_string(call->args[i].value, conn);
    }
}

static void send_result(plcontainer_result res, PGconn_min *conn) {
    int i, j;
    message_start(conn);
    send_char(MT_RESULT, conn);
    send_integer_4(res->rows, conn);
    send_integer_4(res->cols, conn);

    /* send types */
    for (j = 0; j < res->cols; j++) {
        send_string(res->types[j], conn);
        send_string(res->names[j], conn);
    }

    /* send rows */
    for (i = 0; i < res->rows; i++) {
        for (j = 0; j < res->cols; j++) {
            if (res->data[i][j].isnull) {
                send_char('N', conn);
            } else {
                send_char('D', conn);
                send_string(res->data[i][j].value, conn);
            }
        }
    }
}

static void send_exception(error_message err, PGconn_min *conn) {
    message_start(conn);
    send_char(MT_EXCEPTION, conn);
    send_string(err->message, conn);
    send_string(err->stacktrace, conn);
}

static void * receive_exception(PGconn_min *conn) {
    error_message ret;

    ret = pmalloc(sizeof(str_error_message));

    ret->message    = receive_string(conn);
    ret->stacktrace = NULL;

    ret->msgtype = MT_EXCEPTION;

    if (conn->Pfdebug)
        fflush(conn->Pfdebug);
    return ret;
}

static void * receive_result(PGconn_min *conn) {
    plcontainer_result ret;
    int              i, j;  // iterators
    ret          = pmalloc(sizeof(*ret));
    ret->msgtype = MT_RESULT;

    ret->rows = receive_integer_4(conn);
    ret->cols = receive_integer_4(conn);

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
        ret->types[i] = receive_string(conn);
        ret->names[i] = receive_string(conn);
    }

    for (i = 0; i < ret->rows; i++) {
        if (ret->cols > 0) {
            ret->data[i] = pmalloc((ret->cols) * sizeof(*ret->data[i]));
        } else {
            ret->data[i] = NULL;
        }

        for (j = 0; j < ret->cols; j++) {
            char isn;

            isn = receive_char(conn);
            if (isn == 'N') {
                ret->data[i][j].value  = NULL;
                ret->data[i][j].isnull = 1;
            } else {
                ret->data[i][j].isnull = 0;
                ret->data[i][j].value  = receive_string(conn);
            }
        }
    }

    if (conn->Pfdebug)
        fflush(conn->Pfdebug);
    return ret;
}

static message receive_log(PGconn_min *conn) {
    log_message ret;

    ret          = pmalloc(sizeof(str_log_message));
    ret->msgtype = MT_LOG;

    ret->level    = receive_integer_4(conn);
    ret->category = receive_string(conn);
    ret->message  = receive_string(conn);

    if (conn->Pfdebug)
        fflush(conn->Pfdebug);

    return (message)ret;
}

static sql_msg_statement receive_sql_statement(PGconn_min *conn) {
    sql_msg_statement ret;

    ret          = (sql_msg_statement)pmalloc(sizeof(struct str_sql_statement));
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_STATEMENT;
    ret->statement = receive_string(conn);
    return ret;
}

static sql_msg_prepare receive_sql_prepare(PGconn_min *conn) {
    sql_msg_prepare ret;
    int             i;

    ret            = (sql_msg_prepare)pmalloc(sizeof(struct str_sql_prepare));
    ret->msgtype   = MT_SQL;
    ret->sqltype   = SQL_TYPE_PREPARE;
    ret->statement = receive_string(conn);
    ret->ntypes    = receive_integer_4(conn);
    ret->types =
        ret->ntypes == 0 ? NULL : pmalloc(ret->ntypes * sizeof(char *));

    //	ret -> ntypes = 0;
    //	ret -> types = NULL;

    for (i = 0; i < ret->ntypes; i++) {
        lprintf(DEBUG1, "%d", i);
        ret->types[i] = receive_string(conn);
    }

    return ret;
}

static sql_pexecute receive_sql_pexec(PGconn_min *conn) {
    sql_pexecute      ret;
    int               i;
    struct fnc_param *param;

    //	pljlprintf(DEBUG1,"receive_sql_pexec");
    ret          = pmalloc(sizeof(struct str_sql_pexecute));
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_PEXECUTE;
    ret->planid  = receive_integer_4(conn);
    ret->action  = receive_integer_4(conn);
    ret->nparams = receive_integer_4(conn);

    if (ret->nparams == 0) {
        ret->params = NULL;
    } else {
        ret->params = pmalloc(ret->nparams * sizeof(struct fnc_param));
    }

    for (i = 0; i < ret->nparams; i++) {
        char isnull;
        isnull = 0;
        lprintf(DEBUG1, ">>>");
        isnull = receive_char(conn);
        param  = &ret->params[i];
        if (isnull == 'N') {
            param->data.isnull = 1;
            param->data.value  = NULL;
        } else {
            if (isnull != 'D')
                lprintf(WARNING, "Should be N or D: %d", (unsigned char)isnull);
            param->data.isnull = 0;
            param->type        = receive_string(conn);
            param->data.value  = receive_string(conn);
        }
    }
    lprintf(DEBUG1, "<<<");
    return ret;
}

static sql_msg_cursor_close receive_sql_cursorclose(PGconn_min *conn) {
    sql_msg_cursor_close ret;

    ret             = pmalloc(sizeof(struct str_sql_msg_cursor_close));
    ret->msgtype    = MT_SQL;
    ret->sqltype    = SQL_TYPE_CURSOR_CLOSE;
    ret->cursorname = receive_string(conn);

    return ret;
}

static sql_msg_unprepare receive_sql_unprepare(PGconn_min *conn) {
    sql_msg_unprepare ret;

    ret          = pmalloc(sizeof(struct str_sql_unprepare));
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_UNPREPARE;
    ret->planid  = receive_integer_4(conn);

    return ret;
}

static sql_msg_cursor_open receive_sql_opencursor_sql(PGconn_min *conn) {
    sql_msg_cursor_open ret;

    ret             = pmalloc(sizeof(struct str_sql_msg_cursor_open));
    ret->msgtype    = MT_SQL;
    ret->sqltype    = SQL_TYPE_CURSOR_OPEN;
    ret->cursorname = receive_string(conn);
    ret->query      = receive_string(conn);

    return ret;
}

static sql_msg_cursor_fetch receive_sql_fetch(PGconn_min *conn) {
    sql_msg_cursor_fetch ret;

    ret             = pmalloc(sizeof(struct str_sql_msg_cursor_fetch));
    ret->msgtype    = MT_SQL;
    ret->sqltype    = SQL_TYPE_FETCH;
    ret->cursorname = receive_string(conn);
    ret->count      = receive_integer_4(conn);
    ret->direction  = receive_integer_4(conn);

    return ret;
}

static void send_sql_statement(sql_msg_statement msg, PGconn_min *conn) {
    message_start(conn);
    send_char(msg->msgtype, conn);
    send_integer_4(msg->sqltype, conn);
    send_string(msg->statement, conn);
}

static void send_sql(sql_msg msg, PGconn_min *conn) {
    switch (msg->sqltype) {
    case SQL_TYPE_STATEMENT:
        send_sql_statement((sql_msg_statement)msg, conn);
        break;
    default:
        lprintf(ERROR, "unhandled message type");
    }
}

static callreq receive_call(PGconn_min *conn) {
    int     i;
    callreq req;

    req            = pmalloc(sizeof(*req));
    req->proc.name = receive_string(conn);
    req->proc.src  = receive_string(conn);
    req->nargs     = receive_integer_4(conn);
    req->args      = pmalloc(sizeof(*req->args) * req->nargs);
    for (i = 0; i < req->nargs; i++) {
        req->args[i].name  = receive_string(conn);
        req->args[i].type  = receive_string(conn);
        req->args[i].value = receive_string(conn);
    }
    return req;
}

static sql_msg receive_sql(PGconn_min *conn) {
    int typ;

    typ = receive_integer_4(conn);
    switch (typ) {
    case SQL_TYPE_STATEMENT:
        return (sql_msg)receive_sql_statement(conn);
    case SQL_TYPE_PREPARE:
        return (sql_msg)receive_sql_prepare(conn);
    case SQL_TYPE_PEXECUTE:
        return (sql_msg)receive_sql_pexec(conn);
    case SQL_TYPE_FETCH:
        return (sql_msg)receive_sql_fetch(conn);
    case SQL_TYPE_CURSOR_CLOSE:
        return (sql_msg)receive_sql_cursorclose(conn);
    case SQL_TYPE_UNPREPARE:
        return (sql_msg)receive_sql_unprepare(conn);
    case SQL_TYPE_CURSOR_OPEN:
        return (sql_msg)receive_sql_opencursor_sql(conn);
    default:
        // pljlogging_error = 1;
        lprintf(ERROR, "UNHANDLED SQL TYPE: %d", typ);
    }
    return NULL;  // which never happens, but syntax failure otherwise.
}

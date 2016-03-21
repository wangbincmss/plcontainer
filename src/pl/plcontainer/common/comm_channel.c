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
 * file name:            comm_channel.c
 * description:            Channel API.
 * author:            Laszlo Hornyak Kocka
 */

#include "comm_channel.h"
#include "comm_logging.h"
#include "comm_connectivity.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int message_start(plcConn *conn, char msgType);
static int message_end(plcConn *conn);
static int send_char(plcConn *conn, char c);
static int send_integer_4(plcConn *conn, int i);
static int send_bytes(plcConn *conn, char *s, size_t cnt);
static int send_string(plcConn *conn, char *s);
static int receive_char(plcConn *conn, char *c);
static int receive_message_type(plcConn *conn, char *c);
static int receive_uinteger_2(plcConn *conn, unsigned short *i);
static int receive_integer_4(plcConn *conn, int *i);
static int receive_uinteger_4(plcConn *conn, unsigned int *i);
static int receive_bytes(plcConn *conn, char **s, size_t *len);
static int receive_string(plcConn *conn, char **s);
static int receive_bytes_prealloc(plcConn *conn, char *s, size_t len);
static int send_call(plcConn *conn, callreq call);
static int send_result(plcConn *conn, plcontainer_result res);
static int send_exception(plcConn *conn, error_message err);
static int send_sql(plcConn *conn, sql_msg msg);
static int receive_exception(plcConn *conn, message *mExc);
static int receive_result(plcConn *conn, message *mRes);
static int receive_log(plcConn *conn, message *mLog);
static int receive_sql_statement(plcConn *conn, message *mStmt);
static int receive_sql_prepare(plcConn *conn, message *mPrep);
static int receive_sql_pexec(plcConn *conn, message *mPExec);
static int receive_sql_cursorclose(plcConn *conn, message *mCurc);
static int receive_sql_unprepare(plcConn *conn, message *mUnp);
static int receive_sql_opencursor_sql(plcConn *conn, message *mCuro);
static int receive_sql_fetch(plcConn *conn, message *mCurf);
static int receive_call(plcConn *conn, message *mCall);
static int receive_sql(plcConn *conn, message *mSql);

/* Public API Functions */

int plcontainer_channel_send(plcConn *conn, message msg) {
    int res;
    switch (msg->msgtype) {
        case MT_CALLREQ:
            res = send_call(conn, (callreq)msg);
            break;
        case MT_RESULT:
            res = send_result(conn, (plcontainer_result)msg);
            break;
        case MT_EXCEPTION:
            res = send_exception(conn, (error_message)msg);
            break;
        case MT_SQL:
            res = send_sql(conn, (sql_msg)msg);
            break;
        default:
            lprintf(ERROR, "UNHANDLED MESSAGE: '%c'", msg->msgtype);
            res = -1;
            break;
    }
    return res;
}

int plcontainer_channel_receive(plcConn *conn, message *plcMsg) {
    int  res;
    char cType;
    res = receive_message_type(conn, &cType);
    if (res == -2) {
        res = -3;
    }
    if (res == 0) {
        switch (cType) {
            case MT_RESULT:
                res = receive_result(conn, plcMsg);
                break;
            case MT_EXCEPTION:
                res = receive_exception(conn, plcMsg);
                break;
            case MT_LOG:
                res = receive_log(conn, plcMsg);
                break;
            case MT_SQL:
                res = receive_sql(conn, plcMsg);
                break;
            case MT_CALLREQ:
                res = receive_call(conn, plcMsg);
                break;
            default:
                lprintf(ERROR, "message type unknown %c", cType);
                plcMsg = NULL;
                res = -1;
                break;
        }
    }
    return res;
}

/* Send-Receive for Primitive Datatypes */

static int message_start(plcConn *conn, char msgType) {
    return plcBufferAppend(conn, &msgType, 1);
}

static int message_end(plcConn *conn) {
    return plcBufferFlush(conn);
}

static int send_char(plcConn *conn, char c) {
    return plcBufferAppend(conn, &c, 1);
}

static int send_integer_4(plcConn *conn, int i) {
    return plcBufferAppend(conn, (char*)&i, 4);
}

static int send_bytes(plcConn *conn, char *s, size_t cnt) {
    int res = send_integer_4(conn, cnt);
    if (res == 0) {
        res = plcBufferAppend(conn, s, cnt);
    }
    return res;
}

static int send_string(plcConn *conn, char *s) {
    int cnt = strlen(s);
    return send_bytes(conn, s, cnt);
}

static int receive_char(plcConn *conn, char *c) {
    return plcBufferRead(conn, c, 1);
}

static int receive_message_type(plcConn *conn, char *c) {
    int res = plcBufferReceive(conn, 1);
    if (res == -2) {
        lprintf (WARNING, "Cannot receive data from the socket anymore");
    } else {
        res = plcBufferRead(conn, c, 1);
    }
    return res;
}

static int receive_uinteger_2(plcConn *conn, unsigned short *i) {
    return plcBufferRead(conn, (char*)i, 2);
}

static int receive_integer_4(plcConn *conn, int *i) {
    return plcBufferRead(conn, (char*)i, 4);
}

static int receive_uinteger_4(plcConn *conn, unsigned int *i) {
    return plcBufferRead(conn, (char*)i, 4);
}

static int receive_bytes(plcConn *conn, char **s, size_t *len) {
    int cnt;
    int res = 0;
    if (receive_integer_4(conn, &cnt) < 0) {
        return -1;
    }

    *len = cnt;
    *s   = pmalloc(cnt + 1);
    if (*len > 0) {
        res  = plcBufferRead(conn, *s, cnt);
    }
    (*s)[cnt] = 0;

    return res;
}

static int receive_string(plcConn *conn, char **s) {
    size_t cnt;
    return receive_bytes(conn, s, &cnt);
}

static int receive_bytes_prealloc(plcConn *conn, char *s, size_t len) {
    return plcBufferRead(conn, s, len);
}

/* Send Functions for the Main Engine */

static int send_call(plcConn *conn, callreq call) {
    int res = 0;
    int i;

    res += message_start(conn, MT_CALLREQ);
    res += send_string(conn, call->proc.name);
    res += send_string(conn, call->proc.src);
    res += send_integer_4(conn, call->nargs);

    for (i = 0; i < call->nargs; i++) {
        res += send_string(conn, call->args[i].name);
        res += send_string(conn, call->args[i].type);
        res += send_string(conn, call->args[i].value);
    }
    res += message_end(conn);
    return (res < 0) ? -1 : 0;
}

static int send_result(plcConn *conn, plcontainer_result ret) {
    int res = 0;
    int i, j;
    res += message_start(conn, MT_RESULT);
    res += send_integer_4(conn, ret->rows);
    res += send_integer_4(conn, ret->cols);

    /* send types */
    for (j = 0; j < ret->cols; j++) {
        res += send_string(conn, ret->types[j]);
        res += send_string(conn, ret->names[j]);
    }

    /* send rows */
    for (i = 0; i < ret->rows; i++) {
        for (j = 0; j < ret->cols; j++) {
            if (ret->data[i][j].isnull) {
                res += send_char(conn, 'N');
            } else {
                res += send_char(conn, 'D');
                res += send_string(conn, ret->data[i][j].value);
            }
        }
    }
    res += message_end(conn);
    return (res < 0) ? -1 : 0;
}

static int send_exception(plcConn *conn, error_message err) {
    int res = 0;
    res += message_start(conn, MT_EXCEPTION);
    res += send_string(conn, err->message);
    res += send_string(conn, err->stacktrace);
    res += message_end(conn);
    return (res < 0) ? -1 : 0;
}

static int send_sql(plcConn *conn, sql_msg msg) {
    int res = 0;
    if (msg->sqltype == SQL_TYPE_STATEMENT) {
        res += message_start(conn, MT_SQL);
        res += send_integer_4(conn, ((sql_msg_statement)msg)->sqltype);
        res += send_string(conn, ((sql_msg_statement)msg)->statement);
        res += message_end(conn);
    } else {
        lprintf(ERROR, "Unhandled SQL Message type '%c'", msg->sqltype);
        res = -1;
    }
    return (res < 0) ? -1 : 0;
}

/* Receive Functions for the Main Engine */

static int receive_exception(plcConn *conn, message *mExc) {
    int res = 0;
    error_message ret;

    *mExc = pmalloc(sizeof(str_error_message));
    ret = (error_message) *mExc;
    ret->msgtype = MT_EXCEPTION;
    res += receive_string(conn, &ret->message);
    res += receive_string(conn, &ret->stacktrace);

    return res;
}

static int receive_result(plcConn *conn, message *mRes) {
    int i, j;
    int res = 0;
    plcontainer_result ret;

    *mRes = (message)pmalloc(sizeof(str_plcontainer_result));
    ret = (plcontainer_result) *mRes;
    ret->msgtype = MT_RESULT;
    res += receive_integer_4(conn, &ret->rows);
    res += receive_integer_4(conn, &ret->cols);

    if (res == 0) {
        if (ret->rows > 0) {
            ret->data = pmalloc((ret->rows) * sizeof(raw));
        } else {
            ret->data  = NULL;
            ret->types = NULL;
        }

        /* types per column  */
        ret->types = pmalloc(ret->cols * sizeof(*ret->types));
        ret->names = pmalloc(ret->cols * sizeof(*ret->names));

        for (i = 0; i < ret->cols; i++) {
             res += receive_string(conn, &ret->types[i]);
             res += receive_string(conn, &ret->names[i]);
        }

        for (i = 0; i < ret->rows; i++) {
            if (ret->cols > 0) {
                ret->data[i] = pmalloc((ret->cols) * sizeof(*ret->data[i]));
            } else {
                ret->data[i] = NULL;
            }

            for (j = 0; j < ret->cols; j++) {
                char isn;
                res += receive_char(conn, &isn);
                if (isn == 'N') {
                    ret->data[i][j].isnull = 1;
                    ret->data[i][j].value  = NULL;
                } else {
                    ret->data[i][j].isnull = 0;
                    res += receive_string(conn, &ret->data[i][j].value);
                }
            }
            if (res < 0) {
                break;
            }
        }
    }

    return (res < 0) ? -1 : 0;
}

static int receive_log(plcConn *conn, message *mLog) {
    int res = 0;
    log_message ret;

    *mLog = pmalloc(sizeof(str_log_message));
    ret          = (log_message) *mLog;
    ret->msgtype = MT_LOG;

    res += receive_integer_4(conn, &ret->level);
    res += receive_string(conn, &ret->category);
    res += receive_string(conn, &ret->message);

    return (res < 0) ? -1 : 0;
}

static int receive_sql_statement(plcConn *conn, message *mStmt) {
    int res = 0;
    sql_msg_statement ret;

    *mStmt = (message)pmalloc(sizeof(struct str_sql_statement));
    ret          = (sql_msg_statement) *mStmt;
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_STATEMENT;
    res = receive_string(conn, &ret->statement);
    return res;
}

static int receive_sql_prepare(plcConn *conn, message *mPrep) {
    int             i;
    int             res = 0;
    sql_msg_prepare ret;

    *mPrep       = (message)pmalloc(sizeof(struct str_sql_prepare));
    ret          = (sql_msg_prepare) *mPrep;
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_PREPARE;
    res          += receive_string(conn, &ret->statement);
    res          += receive_integer_4(conn, &ret->ntypes);
    ret->types =
        ret->ntypes == 0 ? NULL : pmalloc(ret->ntypes * sizeof(char *));

    for (i = 0; i < ret->ntypes; i++) {
        res += receive_string(conn, &ret->types[i]);
    }

    return (res < 0) ? -1 : 0;
}

static int receive_sql_pexec(plcConn *conn, message *mPExec) {
    sql_pexecute      ret;
    int               i;
    int               res = 0;
    struct fnc_param *param;

    *mPExec      = (message) pmalloc(sizeof(struct str_sql_pexecute));
    ret          = (sql_pexecute) *mPExec;
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_PEXECUTE;
    res += receive_integer_4(conn, &ret->planid);
    res += receive_bytes_prealloc(conn, (char*)&ret->action, sizeof(sql_action));
    res += receive_integer_4(conn, &ret->nparams);

    if (res == 0) {
        if (ret->nparams == 0) {
            ret->params = NULL;
        } else {
            ret->params = pmalloc(ret->nparams * sizeof(struct fnc_param));
        }

        for (i = 0; i < ret->nparams; i++) {
            char isnull;
            isnull = 0;
            res += receive_char(conn, &isnull);
            param  = &ret->params[i];
            if (isnull == 'N') {
                param->data.isnull = 1;
                param->data.value  = NULL;
            } else {
                param->data.isnull = 0;
                res += receive_string(conn, &param->type);
                res += receive_string(conn, &param->data.value);
            }
            if (res < 0) {
                break;
            }
        }
    }
    return (res < 0) ? -1 : 0;
}

static int receive_sql_cursorclose(plcConn *conn, message *mCurc) {
    int res = 0;
    sql_msg_cursor_close ret;

    *mCurc       = (message)pmalloc(sizeof(struct str_sql_msg_cursor_close));
    ret          = (sql_msg_cursor_close)*mCurc;
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_CURSOR_CLOSE;
    res = receive_string(conn, &ret->cursorname);

    return res;
}

static int receive_sql_unprepare(plcConn *conn, message *mUnp) {
    int res = 0;
    sql_msg_unprepare ret;

    *mUnp        = (message)pmalloc(sizeof(struct str_sql_unprepare));
    ret          = (sql_msg_unprepare)*mUnp;
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_UNPREPARE;
    res = receive_integer_4(conn, &ret->planid);

    return res;
}

static int receive_sql_opencursor_sql(plcConn *conn, message *mCuro) {
    int res = 0;
    sql_msg_cursor_open ret;

    *mCuro       = (message)pmalloc(sizeof(struct str_sql_msg_cursor_open));
    ret          = (sql_msg_cursor_open)*mCuro;
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_CURSOR_OPEN;
    res += receive_string(conn, &ret->cursorname);
    res += receive_string(conn, &ret->query);

    return (res < 0) ? -1 : 0;
}

static int receive_sql_fetch(plcConn *conn, message *mCurf) {
    int res = 0;
    sql_msg_cursor_fetch ret;

    *mCurf       = (message)pmalloc(sizeof(struct str_sql_msg_cursor_fetch));
    ret          = (sql_msg_cursor_fetch) *mCurf;
    ret->msgtype = MT_SQL;
    ret->sqltype = SQL_TYPE_FETCH;
    res += receive_string(conn, &ret->cursorname);
    res += receive_uinteger_4(conn, &ret->count);
    res += receive_uinteger_2(conn, &ret->direction);

    return (res < 0) ? -1 : 0;
}

static int receive_call(plcConn *conn, message *mCall) {
    int res = 0;
    int i;
    callreq req;

    *mCall         = (message)pmalloc(sizeof(struct call_req));
    req            = (callreq) *mCall;
    req->msgtype   = MT_CALLREQ;
    res += receive_string(conn, &req->proc.name);
    res += receive_string(conn, &req->proc.src);
    res += receive_integer_4(conn, &req->nargs);
    if (res == 0) {
        req->args = pmalloc(sizeof(*req->args) * req->nargs);
        for (i = 0; i < req->nargs; i++) {
            res += receive_string(conn, &req->args[i].name);
            res += receive_string(conn, &req->args[i].type);
            res += receive_string(conn, &req->args[i].value);
            if (res < 0) {
                break;
            }
        }
    }
    return (res < 0) ? -1 : 0;
}

static int receive_sql(plcConn *conn, message *mSql) {
    int res = 0;
    int sqlType;
    res = receive_integer_4(conn, &sqlType);
    if (res == 0) {
        switch (sqlType) {
            case SQL_TYPE_STATEMENT:
                res = receive_sql_statement(conn, mSql);
                break;
            case SQL_TYPE_PREPARE:
                res = receive_sql_prepare(conn, mSql);
                break;
            case SQL_TYPE_PEXECUTE:
                res = receive_sql_pexec(conn, mSql);
                break;
            case SQL_TYPE_FETCH:
                res = receive_sql_fetch(conn, mSql);
                break;
            case SQL_TYPE_CURSOR_CLOSE:
                res = receive_sql_cursorclose(conn, mSql);
                break;
            case SQL_TYPE_UNPREPARE:
                res = receive_sql_unprepare(conn, mSql);
                break;
            case SQL_TYPE_CURSOR_OPEN:
                res = receive_sql_opencursor_sql(conn, mSql);
                break;
            default:
                res = -1;
                lprintf(ERROR, "UNHANDLED SQL TYPE: %d", sqlType);
                break;
        }
    }
    return res;
}

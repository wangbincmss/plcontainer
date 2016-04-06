#ifndef PLC_MESSAGE_SQL_H
#define PLC_MESSAGE_SQL_H

#include "message_base.h"

typedef enum {
    SQL_TYPE_INVALID = 0,
    SQL_TYPE_STATEMENT,
    SQL_TYPE_CURSOR_CLOSE,
    SQL_TYPE_FETCH,
    SQL_TYPE_CURSOR_OPEN,
    SQL_TYPE_PREPARE,
    SQL_TYPE_PEXECUTE,
    SQL_TYPE_UNPREPARE,
    SQL_TYPE_MAX
} st;

#define sql_message_content st sqltype

typedef struct str_sql_msg {
    base_message_content;
    sql_message_content;
} * sql_msg;

typedef struct str_sql_statement {
    base_message_content;
    sql_message_content;
    char *statement;
} * sql_msg_statement;

typedef struct str_sql_prepare {
    base_message_content;
    sql_message_content;
    char *statement;
    int ntypes;
    char **types;
} * sql_msg_prepare;

typedef struct str_sql_unprepare {
    base_message_content;
    sql_message_content;
    int planid;
} * sql_msg_unprepare;

typedef enum {
    SQL_PEXEC_ACTION_EXECUTE = 0,
    SQL_PEXEC_ACTION_UPDATE,
    SQL_PEXEC_ACTION_OPENCURSOR
} sql_action;

typedef struct str_sql_pexecute {
    base_message_content;
    sql_message_content;
    int planid;
    int nparams;
    plcArgument *params;
    sql_action action;
} * sql_pexecute;

/**
 * For opening and closing cursors.
 */
typedef struct str_sql_msg_cursor_close {
    base_message_content;
    sql_message_content;
    char *cursorname;
} * sql_msg_cursor_close;

typedef struct str_sql_msg_cursor_open {
    base_message_content;
    sql_message_content;
    char *cursorname;
    char *query;
} * sql_msg_cursor_open;

/**
 * For fetching from cursors.
 */
typedef struct str_sql_msg_cursor_fetch {
    base_message_content;
    sql_message_content;
    char *cursorname;
    int count;
    /** direction: 0 if forward, 1 backward */
    short direction;

} * sql_msg_cursor_fetch;

typedef struct str_sql_msg_blob_create {
    base_message_content;
    sql_message_content;
} * sql_msg_blob_create;

typedef struct str_sql_msg_blob_delete {
    base_message_content;
    sql_message_content;
    long blobid;
} * sql_msg_blob_delete;

typedef struct str_sql_msg_blob_open {
    base_message_content;
    sql_message_content;
    long blobid;
} * sql_msg_blob_open;

typedef struct str_sql_msg_blob_close {
    base_message_content;
    sql_message_content;
    long blobid;
} sql_msg_blob_close;

typedef struct str_sql_msg_blob_read {
    base_message_content;
    sql_message_content;
    long blobid;
    int max;
    unsigned char strict;
} * sql_msg_blob_read;

typedef struct str_sql_msg_blob_write {
    base_message_content;
    sql_message_content;
    long     blobid;
    rawdata *data;
} sql_msg_blob_write;

typedef struct str_sql_msg_blob_seek {
    base_message_content;
    sql_message_content;
    long blobid;
    long position;
    unsigned char relative;
} * sql_msg_blob_seek;

#endif /* PLC_MESSAGE_SQL_H */

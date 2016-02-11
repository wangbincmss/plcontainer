#ifndef LIBPQ_MIN_MISC_H
#define LIBPQ_MIN_MISC_H

#include "libpq-mini.h"

/* Send functions */

int pqmPutMsgStart(char, PGconn_min *conn);

int pqmPutInt(int, size_t, PGconn_min *conn);

int pqmPutInt(int, size_t, PGconn_min *conn);

int pqmPutMsgBytes(const void *buf, size_t len, PGconn_min *conn);

int pqmPutMsgEnd(PGconn_min *conn);

int pqmPutc(char c, PGconn_min *conn);

int pqmPuts(const char *s, PGconn_min *conn);

int pqmPutnchar(const char *s, size_t len, PGconn_min *conn);

int pqmFlush(PGconn_min *conn);

/* Receive functions */

int pqmGetSome(PGconn_min *conn);

int pqmReadData(PGconn_min *conn);

void pqmMessageRecvd(PGconn_min *conn);

int pqmGetc(char *result, PGconn_min *conn);

int pqmGetnchar(char *s, size_t len, PGconn_min *conn);

int pqmGetInt(int *result, size_t bytes, PGconn_min *conn);

#endif

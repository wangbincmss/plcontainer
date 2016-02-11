#ifndef LIBPQ_SECURE_H
#define LIBPQ_SECURE_H
#include "libpq-mini.h"

ssize_t pqmsecure_read(PGconn_min *conn, void *ptr, size_t len);

ssize_t pqmsecure_write(PGconn_min *conn, const void *ptr, size_t len);

#endif

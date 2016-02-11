#ifndef __COMM_LOGGING_H__
#define __COMM_LOGGING_H__

/*
  COMM_STANDALONE should be defined for standalone interpreters
  running inside containers, since they don't have access to postgres
  symbols. If it was defined, lprintf will print the logs to stdout or
  in case of an error to stderr. pmalloc, pfree & pstrdup will use the
  std library.
*/
#ifdef COMM_STANDALONE

#include <utils/elog.h>

#define lprintf(lvl, fmt, ...)            \
    do {                                  \
        FILE *out = stdout;               \
        if (lvl >= ERROR) {               \
            out = stderr;                 \
        }                                 \
        fprintf(out, #lvl ": ");          \
        fprintf(out, fmt, ##__VA_ARGS__); \
        fprintf(out, "\n");               \
        if (lvl >= ERROR) {               \
            abort();                      \
        }                                 \
    } while (0)
#define pmalloc malloc
#define pfree free
#define pstrdup strdup

#else /* COMM_STANDALONE */

#include "postgres.h"

#define lprintf elog
#define pmalloc palloc
/* pfree & pstrdup are already defined by postgres */

#endif /* COMM_STANDALONE */

#endif /* __COMM_LOGGING_H__ */

#ifndef __RCALL_H__
#define __RCALL_H__

#include "common/libpq-mini.h"

#define UNUSED __attribute__ (( unused ))

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

// Signal processing function signature
typedef void (*sighandler_t)(int);

// Initialization of R module
void r_init(void);

// Processing of the Greenplum function call
void handle_call(callreq req);

#endif /* __RCALL_H__ */

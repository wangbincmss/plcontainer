#ifndef __PYCALL_H__
#define __PYCALL_H__

#include "common/libpq-mini.h"

#define UNUSED __attribute__ (( unused ))

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

// Initialization of Python module
void python_init(void);

// Processing of the Greenplum function call
void handle_call(callreq req, PGconn_min* conn);

#endif /* __PYCALL_H__ */

#ifndef PLC_PYCALL_H
#define PLC_PYCALL_H

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#include "common/comm_connectivity.h"
#include "pyconversions.h"

#define UNUSED __attribute__ (( unused ))

// Initialization of Python module
void python_init(void);

// Processing of the Greenplum function call
void handle_call(callreq req, plcConn* conn);

#endif /* PLC_PYCALL_H */

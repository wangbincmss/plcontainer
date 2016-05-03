#ifndef PLC_RCALL_H
#define PLC_RCALL_H

#include "common/comm_connectivity.h"

#define UNUSED __attribute__ (( unused ))

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

// Global connection object
plcConn* plcconn_global;

// Global execution termination flag
int plc_is_execution_terminated;

// Processing of the Greenplum function call
void handle_call(callreq req, plcConn* conn);

#endif /* PLC_RCALL_H */

#ifndef PLC_PYCALL_H
#define PLC_PYCALL_H

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#include "common/comm_connectivity.h"
#include "pyconversions.h"

#define UNUSED __attribute__ (( unused ))

// Global connection object
extern plcConn* plcconn_global;

// Global execution termination flag
int plc_is_execution_terminated;
int plc_sending_data;

// Initialization of Python module
int python_init(void);

// Processing of the Greenplum function call
void handle_call(plcMsgCallreq *req, plcConn* conn);

#endif /* PLC_PYCALL_H */

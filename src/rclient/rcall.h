#ifndef PLC_RCALL_H
#define PLC_RCALL_H

#include "common/messages/messages.h"
#include "common/comm_connectivity.h"

#define UNUSED __attribute__ (( unused ))

// Global connection object
plcConn* plcconn_global;

// Global execution termination flag
int plc_is_execution_terminated;

// Processing of the Greenplum function call
void handle_call(callreq req, plcConn* conn);

// Initialization of R module
void r_init( );

#endif /* PLC_RCALL_H */

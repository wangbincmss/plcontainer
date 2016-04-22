#ifndef PLC_PYERROR_H
#define PLC_PYERROR_H

#include "common/comm_connectivity.h"

// Raising exception to the backend
void raise_execution_error (plcConn *conn, const char *format, ...);

#endif /* PLC_PYERROR_H */

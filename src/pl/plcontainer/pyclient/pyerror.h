#ifndef __PYERROR_H__
#define __PYERROR_H__

#include "common/comm_connectivity.h"

// Raising exception to the backend
void raise_execution_error (plcConn *conn, const char *format, ...);

#endif /* __PYERROR_H__ */

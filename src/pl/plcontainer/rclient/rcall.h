#ifndef __RCALL_H__
#define __RCALL_H__

#include "common/comm_connectivity.h"


#define UNUSED __attribute__ (( unused ))

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif



// Processing of the Greenplum function call
void handle_call(callreq req, plcConn* conn);


#endif /* __RCALL_H__ */

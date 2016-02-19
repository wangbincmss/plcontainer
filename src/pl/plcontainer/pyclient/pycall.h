#ifndef __PYCALL_H__
#define __PYCALL_H__

#include "common/libpq-mini.h"

// Initialization of Python module
void python_init();

// Processing of the Greenplum function call
void handle_call(callreq req);

#endif /* __PYCALL_H__ */

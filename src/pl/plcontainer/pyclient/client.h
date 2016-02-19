#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "common/libpq-mini.h"

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#define UNUSED __attribute__ (( unused ))

// Enabling debug would create infinite loop of client receiving connections
//#define _DEBUG_CLIENT

#define CLIENT_PORT 8080

#endif /* __CLIENT_H__ */

#ifndef __COMM_SERVER_H__
#define __COMM_SERVER_H__

#include "comm_connectivity.h"

// Enabling debug would create infinite loop of client receiving connections
//#define _DEBUG_SERVER

#define SERVER_PORT 8080

// Timeout in seconds for server to wait for client connection
#define TIMEOUT_SEC 20

int  start_listener(void);
void connection_wait(int sock);
plcConn* connection_init(int sock);
void receive_loop( void (*handle_call)(callreq, plcConn*), plcConn* conn);

#endif /* __COMM_SERVER_H__ */

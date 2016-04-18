#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <assert.h>

#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "common/comm_connectivity.h"
#include "common/comm_server.h"
#include "pycall.h"

int main(int argc UNUSED, char **argv UNUSED) {
    int      sock;
    plcConn* conn;

    assert(sizeof(char) == 1);
    assert(sizeof(short) == 2);
    assert(sizeof(int) == 4);
    assert(sizeof(long long) == 8);
    assert(sizeof(float) == 4);
    assert(sizeof(double) == 8);

    // Bind the socket and start listening the port
    sock = start_listener();

    // Initialize Python
    if (python_init() < 0) {
        lprintf(ERROR, "Cannot initialize Python");
        return -1;
    }

    #ifdef _DEBUG_CLIENT
        // In debug mode we have a cycle of connections with infinite wait time
        while (true) {
            conn = connection_init(sock);
            receive_loop(handle_call, conn);
        }
    #else
        // In release mode we wait for incoming connection for limited time
        // and the client works for a single connection only
        connection_wait(sock);
        conn = connection_init(sock);
        receive_loop(handle_call, conn);
    #endif

    lprintf(NOTICE, "Client has finished execution");
    return 0;
}

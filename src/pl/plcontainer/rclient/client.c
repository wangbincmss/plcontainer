#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "common/comm_channel.h"
#include "common/comm_logging.h"
#include "common/libpq-mini.h"
#include "common/comm_server.h"
#include "rcall.h"

int main(int argc UNUSED, char **argv UNUSED) {
    int         sock;
    PGconn_min* conn;

    // Bind the socket and start listening the port
    sock = start_listener();

    // Initialize R
    r_init();

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

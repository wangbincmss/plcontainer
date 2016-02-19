#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "common/comm_channel.h"
#include "common/comm_logging.h"
#include "common/libpq-mini.h"
#include "pycall.h"
#include "client.h"

static void receive_loop();
static int  start_listener();
static void connection_wait(int sock);
static void connection_init(int sock);

int main(int argc UNUSED, char **argv UNUSED) {
    int sock;

    // Bind the socket and start listening the port
    sock = start_listener();

    #ifdef _DEBUG_CLIENT
        // In debug mode we have a cycle of connections with infinite wait time
        while (true) {
            connection_init(sock);
            receive_loop();
        }
    #else
        // In release mode we wait for incoming connection for limited time
        // and the client works for a single connection only
        connection_wait(sock);
        connection_init(sock);
        receive_loop();
    #endif

    lprintf(NOTICE, "Client has finished execution");
    return 0;
}

/*
 * Functoin binds the socket and starts listening on it
 */
int start_listener() {
    struct sockaddr_in addr;
    int                sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    addr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port   = htons(CLIENT_PORT),
        .sin_addr = {.s_addr = INADDR_ANY},
    };
    if (bind(sock, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        lprintf(ERROR, "Cannot bind the port: %s", strerror(errno));
    }

    if (listen(sock, 10) == -1) {
        lprintf(ERROR, "Cannot listen the socket: %s", strerror(errno));
    }

    return sock;
}

/*
 * Fuction waits for the socket to accept connection for finite amount of time
 * and errors out when the timeout is reached and no client connected
 */
void connection_wait(int sock) {
    struct timeval     timeout;
    int                rv;
    fd_set             fdset;

    FD_ZERO(&fdset);    /* clear the set */
    FD_SET(sock, &fdset); /* add our file descriptor to the set */
    timeout.tv_sec  = 20;
    timeout.tv_usec = 0;

    rv = select(sock + 1, &fdset, NULL, NULL, &timeout);
    if (rv == -1) {
        lprintf(ERROR, "Failed to select() socket: %s", strerror(errno));
    }
    if (rv == 0) {
        lprintf(ERROR, "Socket timeout - no client connected within 20 seconds");
    }
}

/*
 * Function accepts the connection and initializes libpq structure for it
 */
void connection_init(int sock) {
    socklen_t          raddr_len;
    struct sockaddr_in raddr;
    PGconn_min *       pqconn;
    int                connection;

    raddr_len  = sizeof(raddr);
    connection = accept(sock, (struct sockaddr *)&raddr, &raddr_len);
    if (connection == -1) {
        lprintf(ERROR, "failed to accept connection: %s", strerror(errno));
    }

    pqconn = pq_min_connect_fd(connection);
    //pqconn->Pfdebug = stdout;
    plcontainer_channel_initialize(pqconn);
}

/*
 * The loop of receiving commands from the Greenplum process and processing them
 */
void receive_loop() {
    message msg;

    python_init();

    while (true) {
        msg = plcontainer_channel_receive();

        if (msg == NULL) {
            break;
        }

        switch (msg->msgtype) {
        case MT_CALLREQ:
            handle_call((callreq)msg);
            free_callreq((callreq)msg);
            break;
        default:
            lprintf(ERROR, "received unknown message: %c", msg->msgtype);
        }
    }
}

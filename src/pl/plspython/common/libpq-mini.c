#include <c.h>

#include "comm_logging.h"
#include "libpq-mini.h"

/* #include "utils/elog.h" */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

PGconn_min *
pq_min_connect_fd(int sock) {
    PGconn_min *conn;

    conn = (PGconn_min *)malloc(sizeof(PGconn_min));
    memset(conn, 0, sizeof(PGconn_min));

    /*
     * in buff
     */
    conn->inBuffer  = malloc(8192);
    conn->inBufSize = 8192;

    conn->outBuffer  = malloc(8192);
    conn->outBufSize = 8192;

    conn->Pfdebug = 0;

    conn->sock = sock;

    return conn;
}

PGconn_min *
pq_min_connect(int port) {
    struct hostent *   server;
    struct sockaddr_in raddr; /** Remote address */

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        goto error;
    }
    server = gethostbyname("localhost");
    if (server == NULL) {
        perror("gethostbyname");
        goto error;
    }

    raddr.sin_family = AF_INET;
    memcpy(&((raddr).sin_addr.s_addr), (char *)server->h_addr_list[0],
           server->h_length);

    raddr.sin_port = htons(port);
    //	log(DEBUG1, "connect");
    if (connect(sock, (const struct sockaddr *)&raddr,
                sizeof(struct sockaddr_in)) < 0) {
        perror("connect");
        goto error;
    }
    return pq_min_connect_fd(sock);

error:
    // pljlogging_error = 1;
    lprintf(ERROR, "Connection error: %s", strerror(errno));
    return NULL;
}

void
pq_min_finish(PGconn_min *conn) {
    /*
     * TODO: this case should be handled more correctly
     */
    if (conn == NULL) {
        return;
        //		pljlog(WARNING, "Connection is null, was it initialized
        // at all?");
    }
    close(conn->sock);
}

void
pq_min_set_trace(PGconn_min *conn, FILE *tf) {
    conn->Pfdebug = tf;
}

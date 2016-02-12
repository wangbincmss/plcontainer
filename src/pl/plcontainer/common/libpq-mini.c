/*
Copyright 1994 The PL-J Project. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE PL-J PROJECT ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE PL-J PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the authors and should not be
interpreted as representing official policies, either expressed or implied, of the PL-J Project.
*/

/**
 * file:			libpq-mini.c
 * description:		very minimal functionality from the original
 * 				    libpq implementation. Structures and
 * 				    functionalities are extremely simplified.
 * author:			PostgreSQL developement group.
 * author:			Laszlo Hornyak
 */

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

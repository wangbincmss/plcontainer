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
 * file:			libpq-min.h
 * description:			very minimal functionality from the original
 * 				libpq implementation. Structures and
 * 				functionalities are extremely simplified.
 * author:			PostgreSQL developement group.
 * author:			Laszlo Hornyak
 */

#ifndef LIBPQ_MIN_H
#define LIBPQ_MIN_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdio.h>

// no support for non blocking functions
#define pqmIsnonblocking(conn) 0

struct pg_conn_min {
    // Connection data
    int sock;

    char *inBuffer;
    int inBufSize;
    int inCursor;
    int inStart;
    int inEnd;

    char *outBuffer;
    int outMsgStart;
    int outMsgEnd;
    int outBufSize;
    int outCount;

    FILE *Pfdebug;
};

typedef struct pg_conn_min PGconn_min;

/* Create a connection using the given socket */
PGconn_min *pq_min_connect_fd(int sock);

/** from PQconnectStart */
PGconn_min *pq_min_connect(int port);

/** from PQfinish */
void pq_min_finish(PGconn_min *);

void pq_min_set_trace(PGconn_min *, FILE *);

typedef enum {
    /*
     * Although it is okay to add to this list, values which become unused
     * should never be removed, nor should constants be redefined - that
     * would break compatibility with existing code.
     */
    CONNECTION_OK,
    CONNECTION_BAD,
    /* Non-blocking mode only below here */

    /*
     * The existence of these should never be relied upon - they should
     * only be used for user feedback or similar purposes.
     */
    CONNECTION_STARTED,           /* Waiting for connection to be made.  */
    CONNECTION_MADE,              /* Connection OK; waiting to send.         */
    CONNECTION_AWAITING_RESPONSE, /* Waiting for a response from the
                                                                   * postmaster.
                                   */
    CONNECTION_AUTH_OK,           /* Received authentication; waiting for
                                                   * backend startup. */
    CONNECTION_SETENV,            /* Negotiating environment. */
    CONNECTION_SSL_STARTUP,       /* Negotiating SSL. */
    CONNECTION_NEEDED             /* Internal state: connect() needed */
} ConnStatusType;

typedef enum {
    PGRES_POLLING_FAILED = 0,
    PGRES_POLLING_READING, /* These two indicate that one may        */
    PGRES_POLLING_WRITING, /* use select before polling again.   */
    PGRES_POLLING_OK,
    PGRES_POLLING_ACTIVE /* unused; keep for awhile for backwards
                                                  * compatibility */
} PostgresPollingStatusType;

#endif

#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include "comm_utils.h"
#include "comm_connectivity.h"

static ssize_t plcSocketRecv(plcConn *conn, void *ptr, size_t len);
static ssize_t plcSocketSend(plcConn *conn, const void *ptr, size_t len);
static int plcBufferMaybeFlush (plcConn *conn, int isForse);
static int plcBufferMaybeReset (plcConn *conn, int bufType);
static int plcBufferMaybeGrow (plcConn *conn, int bufType, size_t bufAppend);

/*
 *  Read data from the socket
 */
static ssize_t plcSocketRecv(plcConn *conn, void *ptr, size_t len) {
     ssize_t n = 0;
     n = recv(conn->sock, ptr, len, 0);
     if (conn->debug) {
         lprintf(INFO, ">> Sent %d bytes", (int)n);
     }
     return n;
 }

/*
 *  Write data to the socket
 */
static ssize_t plcSocketSend(plcConn *conn, const void *ptr, size_t len) {
     ssize_t n = 0;
     n = send(conn->sock, ptr, len, 0);
     if (conn->debug) {
         lprintf(INFO, "<< Received %d bytes", (int)n);
     }
     return n;
 }

/*
 *  Initialize plcConn data structure and input/output buffers
 */
plcConn * plcConnInit(int sock) {
    plcConn *conn;

    // Initializing main structures
    conn = (plcConn*)plc_top_alloc(sizeof(plcConn));
    conn->buffer[PLC_INPUT_BUFFER]  = (plcBuffer*)plc_top_alloc(sizeof(plcBuffer));
    conn->buffer[PLC_OUTPUT_BUFFER] = (plcBuffer*)plc_top_alloc(sizeof(plcBuffer));

    // Initializing buffers
    conn->buffer[PLC_INPUT_BUFFER]->data = (char*)plc_top_alloc(PLC_BUFFER_SIZE);
    conn->buffer[PLC_INPUT_BUFFER]->bufSize = PLC_BUFFER_SIZE;
    conn->buffer[PLC_INPUT_BUFFER]->pStart = 0;
    conn->buffer[PLC_INPUT_BUFFER]->pEnd = 0;
    conn->buffer[PLC_OUTPUT_BUFFER]->data = (char*)plc_top_alloc(PLC_BUFFER_SIZE);
    conn->buffer[PLC_OUTPUT_BUFFER]->bufSize = PLC_BUFFER_SIZE;
    conn->buffer[PLC_OUTPUT_BUFFER]->pStart = 0;
    conn->buffer[PLC_OUTPUT_BUFFER]->pEnd = 0;

    // Initializing control parameters
    conn->debug = 0;
    conn->sock = sock;

    return conn;
}

/*
 *  Connect to the specified host of the localhost and initialize the plcConn
 *  data structure
 */
plcConn *plcConnect(int port) {
    struct hostent     *server;
    struct sockaddr_in  raddr; /** Remote address */
    plcConn            *result = NULL;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        lprintf(ERROR, "PLContainer: Cannot create socket");
        return result;
    }

    server = gethostbyname("localhost");
    if (server == NULL) {
        lprintf(ERROR, "PLContainer: Failed to call gethostbyname('localhost')");
        return result;
    }

    raddr.sin_family = AF_INET;
    memcpy(&((raddr).sin_addr.s_addr), (char *)server->h_addr_list[0],
           server->h_length);

    raddr.sin_port = htons(port);
    if (connect(sock, (const struct sockaddr *)&raddr,
            sizeof(struct sockaddr_in)) < 0) {
        char ipAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(raddr.sin_addr), ipAddr, INET_ADDRSTRLEN);
        lprintf(DEBUG1, "PLContainer: Failed to connect to %s", ipAddr);
        return result;
    }

    result = plcConnInit(sock);

    return result;
}

/*
 *  Close the plcConn connection and deallocate the buffers
 */
void plcDisconnect(plcConn *conn) {
    if (conn != NULL) {
        close(conn->sock);
        pfree(conn->buffer[PLC_INPUT_BUFFER]->data);
        pfree(conn->buffer[PLC_OUTPUT_BUFFER]->data);
        pfree(conn->buffer[PLC_INPUT_BUFFER]);
        pfree(conn->buffer[PLC_OUTPUT_BUFFER]);
        pfree(conn);
    }
    return;
}

/*
 * Function flushes the output buffer if it has reached a certain margin in
 * size or if the isForse parameter has passed to it
 *
 * Returns 0 on success, -1 on failure
 */
static int plcBufferMaybeFlush (plcConn *conn, int isForse) {
    char *newBuffer;
    int sent = 0;
    plcBuffer *buf = conn->buffer[PLC_OUTPUT_BUFFER];

    /*
     * Flush the buffer if it has less than PLC_BUFFER_MIN_FREE of free space
     * available or data size in the buffer is greater than initial buffer size
     */
    if (buf->bufSize - buf->pEnd < PLC_BUFFER_MIN_FREE ||
        buf->pEnd > PLC_BUFFER_SIZE || isForse) {
        while (buf->pStart < buf->pEnd) {
            sent = plcSocketSend(conn, buf->data, buf->pEnd - buf->pStart);
            if (sent <= 0) {
                lprintf(LOG, "plcBufferMaybeFlush: Socket write failed, send "
                             "return code is %d, error message is '%s'",
                             sent, strerror(errno));
                return -1;
            }
            buf->pStart += sent;
        }
        /*
         * If the amount of empty space left after flush is bigger than the
         * buffer size we reallocate the buffer of a smaller size
         */
        if (buf->bufSize - buf->pEnd > PLC_BUFFER_SIZE) {
            int newSize = (buf->pEnd / PLC_BUFFER_SIZE + 1 ) * PLC_BUFFER_SIZE;
            newBuffer = (char*)plc_top_alloc(newSize);
            if (newBuffer == NULL) {
                lprintf(ERROR, "plcBufferMaybeFlush: Cannot allocate %d bytes "
                               "for output buffer", newSize);
                return -1;
            }
            pfree(buf->data);
            buf->data = newBuffer;
            buf->bufSize = newSize;
        }
        // Reset the buffer pointers to the beginning
        buf->pStart = 0;
        buf->pEnd = 0;
    }
    return 0;
}

/*
 * Function resets the buffer from the current position to the beginning of
 * the buffer array if it has reached the middle of the buffer or the buffer
 * is empty
 *
 * Returns 0 on success, -1 on failure
 */
static int plcBufferMaybeReset (plcConn *conn, int bufType) {
    plcBuffer *buf = conn->buffer[bufType];
    if (buf->pStart == buf->pEnd) {
        buf->pStart = 0;
        buf->pEnd = 0;
    }
    /*
     * If our start point in a buffer has passed half of its size, we need
     * to move the data closer to the start of the buffer
     */
    if (buf->pStart > buf->bufSize / 2) {
        memcpy(buf->data, buf->data + buf->pStart, buf->pEnd - buf->pStart);
        buf->pEnd = buf->pEnd - buf->pStart;
        buf->pStart = 0;
    }
    return 0;
}

/*
 * Function checks whether we need to increase the size of the buffer or not.
 * Buffer grows if we want to insert more bytes than the amount of free space in
 * this buffer (given we leave PLC_BUFFER_MIN_FREE free bytes)
 *
 * Returns 0 on success, -1 on failure
 */
static int plcBufferMaybeGrow (plcConn *conn, int bufType, size_t bufAppend) {
    plcBuffer *buf = conn->buffer[bufType];
    char *newBuffer;
    int newSize;
    // Consider resetting the buffer before reallocating it
    plcBufferMaybeReset(conn, bufType);
    // If not enough space in buffer to handle this size of data
    if (buf->bufSize - buf->pEnd - PLC_BUFFER_MIN_FREE < (int)bufAppend) {
        newSize = ((buf->pEnd + bufAppend) / PLC_BUFFER_SIZE + 1) * PLC_BUFFER_SIZE;
        newBuffer = (char*)plc_top_alloc(newSize);
        if (newBuffer == NULL) {
            lprintf(ERROR, "plcBufferMaybeGrow: Cannot allocate %d bytes for buffer",
                           newSize);
            return -1;
        }
        memcpy(newBuffer,
               buf->data + buf->pStart,
               (size_t)(buf->pEnd - buf->pStart));
        pfree(buf->data);
        buf->data = newBuffer;
        buf->pEnd = buf->pEnd - buf->pStart;
        buf->pStart = 0;
        buf->bufSize = newSize;
    }
    return 0;
}

/*
 * Append some data to the buffer. This function does not guarantee that the
 * data would be immediately sent, you have to forcefully flush buffer to
 * achieve this
 *
 * Returns 0 on success, -1 if failed
 */
int plcBufferAppend (plcConn *conn, char *srcBuffer, size_t nBytes) {
    plcBuffer *buf = conn->buffer[PLC_OUTPUT_BUFFER];
    // Return error if failed to flush the buffer when required
    if (plcBufferMaybeFlush(conn, 0) < 0) {
        return -1;
    }
    if (plcBufferMaybeGrow(conn, PLC_OUTPUT_BUFFER, nBytes) < 0) {
        return -1;
    }
    memcpy(buf->data + buf->pEnd, srcBuffer, nBytes);
    buf->pEnd = buf->pEnd + nBytes;
    return 0;
}

/*
 * Read some data from the buffer. If buffer does not have enough data in it,
 * it will ask the socket to receive more data and put it into the buffer
 *
 * Returns 0 on success, -1 or -2 if failed
 */
int plcBufferRead (plcConn *conn, char *resBuffer, size_t nBytes) {
    plcBuffer *buf = conn->buffer[PLC_INPUT_BUFFER];
    int res = 0;
    res = plcBufferReceive (conn, nBytes);
    if (res == 0) {
        memcpy(resBuffer, buf->data + buf->pStart, nBytes);
        buf->pStart = buf->pStart + nBytes;
    } else {
        lprintf(LOG, "plcBufferRead: Socket read failed, "
                     "received return code is %d, error message is '%s'",
                     res, strerror(errno));
    }
    return res;
}

/*
 * Function checks whether we have nBytes bytes in the buffer. If not, it reads
 * the data from the socket. If the buffer is too small, it would be grown
 *
 * Returns 0 on success, -1 if failed and -2 if socket read has returned no data
 */
int plcBufferReceive (plcConn *conn, size_t nBytes) {
    plcBuffer *buf = conn->buffer[PLC_INPUT_BUFFER];
    int recBytes;
    int oldEnd;
    if (buf->pEnd - buf->pStart < (int)nBytes) {
        // Growing by at least half of the initial buffer size
        if (plcBufferMaybeGrow(conn, PLC_INPUT_BUFFER, PLC_BUFFER_SIZE/2) < 0) {
            return -1;
        }
        oldEnd = buf->pEnd;
        while (buf->pEnd - oldEnd < (int)nBytes) {
            recBytes = plcSocketRecv(conn,
                                     buf->data,
                                     PLC_BUFFER_SIZE);
                                     //buf->bufSize - buf->pEnd);
            if (recBytes == 0) {
                return -2;
            }
            if (recBytes < 0) {
                return -1;
            }
            buf->pEnd = buf->pEnd + recBytes;
        }
    }
    return 0;
}

/*
 * Function forcefully flushes the buffer
 *
 * Returns 0 on success, -1 if failed
 */
int plcBufferFlush (plcConn *conn) {
    return plcBufferMaybeFlush(conn, 1);
}

/*
 * Set connection debug output
 */
void plcConnectionSetDebug(plcConn *conn) {
    conn->debug = 1;
}

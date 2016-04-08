#ifndef PLC_COMM_CONNECTIVITY_H
#define PLC_COMM_CONNECTIVITY_H

#include <stddef.h>

#define PLC_BUFFER_SIZE 8192
#define PLC_BUFFER_MIN_FREE 1000
#define PLC_INPUT_BUFFER 0
#define PLC_OUTPUT_BUFFER 1

struct plc_buffer {
    char *data;
    int pStart;
    int pEnd;
    int bufSize;
};

typedef struct plc_buffer plcBuffer;

struct plc_conn {
    int sock;
    plcBuffer* buffer[2];
    int debug;
};

typedef struct plc_conn plcConn;

plcConn * plcConnect(int port);
plcConn * plcConnInit(int sock);
void plcDisconnect(plcConn *conn);
int plcBufferAppend (plcConn *conn, char *prt, size_t len);
int plcBufferRead (plcConn *conn, char *resBuffer, size_t len);
int plcBufferReceive (plcConn *conn, size_t nBytes);
int plcBufferFlush (plcConn *conn);
void plcConnectionSetDebug(plcConn *conn);

#endif /* PLC_COMM_CONNECTIVITY_H */

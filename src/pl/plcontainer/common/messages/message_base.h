#ifndef PLC_MESSAGE_BASE_H
#define PLC_MESSAGE_BASE_H

#define base_message_content unsigned short msgtype;

typedef struct str_message { base_message_content } str_message, *message;

typedef struct {
    int   isnull;
    char *value;
} rawdata;

typedef enum {
    PLC_DATA_INT1 = 0, // 1-byte integer
    PLC_DATA_INT2,     // 2-byte integer
    PLC_DATA_INT4,     // 4-byte integer
    PLC_DATA_INT8,     // 8-byte integer
    PLC_DATA_FLOAT4,   // 4-byte float
    PLC_DATA_FLOAT8,   // 8-byte float
    PLC_DATA_TEXT,     // Text - transferred as a set of bytes of predefined length
    PLC_DATA_ARRAY,    // Array - array type specification should follow
    PLC_DATA_RECORD,   // Anonymous record, supported only as return type
    PLC_DATA_UDT,      // User-defined type, specification to follow
    PLC_DATA_INVALID   // Invalid data type
} plcDatatype;

typedef struct {
    plcDatatype  type;
    char        *name;
    rawdata      data;
} plcArgument;

#endif /* PLC_MESSAGE_BASE_H */
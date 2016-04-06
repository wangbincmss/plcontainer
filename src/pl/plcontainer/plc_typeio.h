#ifndef PLC_TYPEIO_H
#define PLC_TYPEIO_H

#include "postgres.h"

#include "common/messages/messages.h"
#include "plcontainer.h"
#include "message_fns.h"

Datum get_array_datum(plcArray *arr, plcTypeInfo *ret_type);
//void get_tuple_store( MemoryContext oldContext, MemoryContext messageContext,
//        ReturnSetInfo *rsinfo,plcontainer_result res, int *isNull );
plcIterator *init_array_iter(Datum d, plcTypeInfo *argType);
char *fill_type_value(Datum funcArg, plcTypeInfo *argType);

#endif /* PLC_TYPEIO_H */
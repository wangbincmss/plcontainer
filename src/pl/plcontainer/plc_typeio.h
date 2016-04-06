#ifndef PLC_TYPEIO_H
#define PLC_TYPEIO_H

#include "postgres.h"

#include "plcontainer.h"
#include "message_fns.h"

Datum get_array_datum(plcArray *arr, plcTypeInfo *ret_type);
//void get_tuple_store( MemoryContext oldContext, MemoryContext messageContext,
//        ReturnSetInfo *rsinfo,plcontainer_result res, int *isNull );

#endif /* PLC_TYPEIO_H */

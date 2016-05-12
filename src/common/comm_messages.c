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
 * file:            commm_messages.c
 * author:            PostgreSQL developement group.
 * author:            Laszlo Hornyak
 */

#include <stdlib.h>

#include "comm_utils.h"
#include "messages/messages.h"

/* Recursive function to free up the type structure */
static void free_subtypes(plcType *typArr) {
    if (typArr->nSubTypes > 0) {
        int i = 0;
        for (i = 0; i < typArr->nSubTypes; i++)
            free_subtypes(&typArr->subTypes[i]);
        pfree(typArr->subTypes);
    }
}

void free_callreq(callreq req, int shared) {
    int i;

    if (!shared) {
        /* free the procedure */
        pfree(req->proc.name);
        pfree(req->proc.src);
    }

    /* free the arguments */
    for (i = 0; i < req->nargs; i++) {
        if (!shared && req->args[i].name != NULL) {
            pfree(req->args[i].name);
        }
        if (!req->args[i].data.isnull) {
            if (req->args[i].type.type == PLC_DATA_ARRAY ){
                plc_free_array((plcArray *)req->args[i].data.value);
            }else{
                pfree(req->args[i].data.value);
            }
        }
        free_subtypes(&req->args[i].type);
    }
    pfree(req->args);

    free_subtypes(&req->retType);

    /* free the top-level request */
    pfree(req);
}

void free_result(plcontainer_result res) {
    int i,j;

    /* free the types and names arrays */
    for (i = 0; i < res->cols; i++) {
        pfree(res->names[i]);
        free_subtypes(&res->types[i]);
    }
    pfree(res->types);
    pfree(res->names);

    /* free the data array */
    if (res->data != NULL) {
        for (i = 0; i < res->rows; i++) {
            for (j = 0; j < res->cols; j++) {
                if (res->data[i][j].value) {
                    /* free the data if it is not null */
                    pfree(res->data[i][j].value);
                }
            }
            /* free the row */
            pfree(res->data[i]);
        }
        pfree(res->data);
    }

    pfree(res);
}

plcArray *plc_alloc_array(int ndims) {
    plcArray *arr;
    arr = (plcArray*)pmalloc(sizeof(plcArray));
    arr->meta = (plcArrayMeta*)pmalloc(sizeof(plcArrayMeta));
    arr->meta->ndims = ndims;
    arr->meta->dims  = NULL;
    if (ndims > 0)
        arr->meta->dims = (int*)pmalloc(ndims * sizeof(int));
    arr->meta->size = 0;
    return arr;
}

void plc_free_array(plcArray *arr) {
    int i;
    if (arr->meta->type == PLC_DATA_TEXT) {
        for (i = 0; i < arr->meta->size; i++)
            if ( ((char**)arr->data)[i] != NULL )
                pfree(((char**)arr->data)[i]);
    }
    pfree(arr->data);
    pfree(arr->nulls);
    pfree(arr->meta->dims);
    pfree(arr->meta);
    pfree(arr);
}

int plc_get_type_length(plcDatatype dt) {
    int res = 0;
    switch (dt) {
        case PLC_DATA_INT1:
            res = 1;
            break;
        case PLC_DATA_INT2:
            res = 2;
            break;
        case PLC_DATA_INT4:
            res = 4;
            break;
        case PLC_DATA_INT8:
            res = 8;
            break;
        case PLC_DATA_FLOAT4:
            res = 4;
            break;
        case PLC_DATA_FLOAT8:
            res = 8;
            break;
        case PLC_DATA_TEXT:
            /* 8 = the size of pointer */
            res = 8;
            break;
        case PLC_DATA_ARRAY:
        case PLC_DATA_RECORD:
        case PLC_DATA_UDT:
        default:
            lprintf(ERROR, "Type %d cannot be passed plc_get_type_length function",
                    (int)dt);
            break;
    }
    return res;
}

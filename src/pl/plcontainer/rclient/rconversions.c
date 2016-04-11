#include "rconversions.h"


#define UNUSED __attribute__ (( unused ))


static SEXP plc_r_object_from_int1(char *input);
static SEXP plc_r_object_from_int2(char *input);
static SEXP plc_r_object_from_int4(char *input);
static SEXP plc_r_object_from_int8(char *input);
static SEXP plc_r_object_from_float4(char *input);
static SEXP plc_r_object_from_float8(char *input);
static SEXP plc_r_object_from_text(char *input);
static SEXP plc_r_object_from_text_ptr(char *input);
static SEXP plc_r_object_from_array_dim(plcArray *arr, plcRInputFunc infunc,
                    int *idx, int *ipos, char **pos, int vallen, int dim);
static SEXP plc_r_object_from_array (char *input);

static int plc_r_object_as_int1(SEXP *input, char **output, plcRType *type);
static int plc_r_object_as_int2(SEXP *input, char **output, plcRType *type);
static int plc_r_object_as_int4(SEXP *input, char **output, plcRType *type);
static int plc_r_object_as_int8(SEXP *input, char **output, plcRType *type);
static int plc_r_object_as_float4(SEXP *input, char **output, plcRType *type);
static int plc_r_object_as_float8(SEXP *input, char **output, plcRType *type);
static int plc_r_object_as_text(SEXP *input, char **output, plcRType *type);
static int plc_r_object_as_array(SEXP *input, char **output, plcRType *type);

static void plc_r_object_iter_free (plcIterator *iter);
static rawdata *plc_r_object_as_array_next (plcIterator *iter);

static plcRInputFunc plc_get_input_function(plcDatatype dt);
static plcROutputFunc plc_get_output_function(plcDatatype dt);

SEXP get_r_vector(plcDatatype type_id, int numels);

static  SEXP plc_r_object_from_int1(char *input) {
	SEXP arg;
    PROTECT( arg = ScalarLogical( (int) *input ) );
    return arg;
}

static SEXP plc_r_object_from_int2(char *input) {
	SEXP arg;
	PROTECT(arg = ScalarInteger( *((short*)input)) );
    return arg;
}

static SEXP plc_r_object_from_int4(char *input) {
	SEXP arg;
	PROTECT(arg = ScalarInteger( *((int*)input)) );
    return arg;
}

static SEXP plc_r_object_from_int8(char *input) {
	SEXP arg;
	PROTECT(arg = ScalarReal( (double)*((int64*)input)) );
    return arg;
}

static SEXP plc_r_object_from_float4(char *input) {
	SEXP arg;
	PROTECT(arg = ScalarReal( (double) *((float*)input)) );
    return arg;
}

static SEXP plc_r_object_from_float8(char *input) {
	SEXP arg;
	PROTECT(arg = ScalarReal( *((double*)input)) );
    return arg;
}

static SEXP plc_r_object_from_text(char *input) {
	SEXP arg;
	PROTECT( arg = mkString( input ) );
    return arg;
}

static SEXP plc_r_object_from_text_ptr(char *input) {
	SEXP arg;
	PROTECT( arg = mkString( *((char**)input)) );
    return arg;
}

static SEXP plc_r_object_from_array_dim(plcArray *arr,
										 plcRInputFunc infunc,
										 int *idx,
										 int *ipos,
										 char **pos,
										 int vallen,
										 int dim) {
    SEXP res = NULL;
    if (dim == arr->meta->ndims) {
        if (arr->nulls[*ipos] != 0) {
            res = R_NilValue;
        } else {
        	/*
        	 * call the input function for the element in the array
        	 */
            res = infunc(*pos);
            *ipos += 1;
            /*
             * this is the length of the item
             */
            *pos = *pos + vallen;
        }
    } else {
        PROTECT( res = get_r_vector(arr->meta->type,  arr->meta->dims[dim]) );
        for (idx[dim] = 0; idx[dim] < arr->meta->dims[dim]; idx[dim]++) {
            SEXP obj;
            obj = plc_r_object_from_array_dim(arr, infunc, idx, ipos, pos, vallen, dim+1);
            if (obj == NULL) {
            	UNPROTECT(1);
                PROTECT(res=R_NilValue);
                return res;
            }

            switch(arr->meta->type){
            /* 2 and 4 byte integer pgsql datatype => use R INTEGER */
                 case PLC_DATA_INT2:
                     INTEGER_DATA(res)[idx[dim]] = asInteger(obj) ;
                     break;
                 case PLC_DATA_INT4:
                     INTEGER_DATA(res)[idx[dim]] = asInteger(obj) ;
                     break;

                     /*
                      * Other numeric types => use R REAL
                      * Note pgsql int8 is mapped to R REAL
                      * because R INTEGER is only 4 byte
                      */
                 case PLC_DATA_INT8:
                     NUMERIC_DATA(res)[idx[dim]] = (int64)asReal(obj);
                     break;

                 case PLC_DATA_FLOAT4:
                     NUMERIC_DATA(res)[idx[dim]] = (float4 )asReal(obj);
                     break;

                 case PLC_DATA_FLOAT8:

                     NUMERIC_DATA(res)[idx[dim]] = (float8 )asReal(obj);
                     break;
                 case PLC_DATA_INT1:
                     LOGICAL_DATA(res)[idx[dim]] = asLogical(obj);
                     break;
                 case PLC_DATA_RECORD:
                 case PLC_DATA_UDT:
                 case PLC_DATA_INVALID:
                 case PLC_DATA_ARRAY:
                 	lprintf(ERROR, "unhandled type %d", arr->meta->type);
                 	break;
                 case PLC_DATA_TEXT:
                 default:
                     /* Everything else is defaulted to string */
                     SET_STRING_ELT(res, idx[dim], obj);
            }
            UNPROTECT(1);
        }
    }
    return res;
}

static SEXP plc_r_object_from_array (char *input) {
    plcArray *arr = (plcArray*)input;
    SEXP res = R_NilValue;

    if (arr->meta->ndims == 0) {
    	PROTECT( res = get_r_vector(arr->meta->type,  1) );
    } else {
        int  *idx;
        int  ipos;
        char *pos;
        int   vallen = 0;
        plcRInputFunc infunc;

        idx = malloc(sizeof(int) * arr->meta->ndims);
        memset(idx, 0, sizeof(int) * arr->meta->ndims);
        ipos = 0;
        pos = arr->data;
        vallen = plc_get_type_length(arr->meta->type);
        infunc = plc_get_input_function(arr->meta->type);
        if (arr->meta->type == PLC_DATA_TEXT)
            infunc = plc_r_object_from_text_ptr;

        PROTECT(res = plc_r_object_from_array_dim(arr, infunc, idx, &ipos, &pos, vallen, 0));
    }
    UNPROTECT(1);
    return res;
}

static int plc_r_object_as_int1(SEXP *input, char **output, plcRType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(1);
    *output = out;
    switch(TYPEOF(*input)){
    	case LGLSXP:
    		*((int8*)out) = (int8)asLogical(*input);
    		break;
    	case INTSXP:
    		*((int8*)out) = (int8)asInteger(*input)==0?0:1;
    		break;
    	case REALSXP:
    		*((int8*)out) = (int8)asReal(*input) == 0?0:1;
    		break;
    	default:
    		res = -1;
    }
    return res;
}

static int plc_r_object_as_int2(SEXP *input, char **output, plcRType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(2);
    *output = out;

    switch(TYPEOF(*input)){
       	case LGLSXP:
       		*((short*)out) = (short)asLogical(*input);
       		break;
       	case INTSXP:
       	    *((short*)out) = (short)asInteger(*input);
       		break;
       	case REALSXP:
       	    *((short*)out) = (short)asReal(*input);
       		break;
       	default:
       		res = -1;
       }

    return res;
}

static int plc_r_object_as_int4(SEXP *input, char **output, plcRType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(4);
    *output = out;

    switch(TYPEOF(*input)){
       	case LGLSXP:
       		*((int*)out) = (int)asLogical(*input);
       		break;
       	case INTSXP:
            *((int*)out) = (int)asInteger(*input);
       		break;
       	case REALSXP:
       	    *((int*)out) = (int)asReal(*input);
       		break;
       	default:
       		res = -1;
    }

    return res;
}

static int plc_r_object_as_int8(SEXP *input, char **output, plcRType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(8);
    *output = out;

    switch(TYPEOF(*input)){
		case LGLSXP:
			*((int64*)out) = (int64)asLogical(*input);
			break;
		case INTSXP:
			*((int64*)out) = (int64)asInteger(*input);
			break;
		case REALSXP:
			*((int64*)out) = (int64)asReal(*input);
			break;
		default:
			res = -1;
	}
    return res;
}

static int plc_r_object_as_float4(SEXP *input, char **output, plcRType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(4);
    *output = out;

    switch(TYPEOF(*input)){
		case LGLSXP:
			*((float4*)out) = (float4)asLogical(*input);
			break;
		case INTSXP:
			*((float4*)out) = (float4)asInteger(*input);
			break;
		case REALSXP:
			*((float4*)out) = (float4)asReal(*input);
			break;
		default:
			res = -1;
	}

    return res;
}

static int plc_r_object_as_float8(SEXP *input, char **output, plcRType *type UNUSED) {
    int res = 0;
    char *out = (char*)malloc(8);
    *output = out;

    switch(TYPEOF(*input)){
		case LGLSXP:
			*((float8*)out) = (float8)asLogical(*input);
			break;
		case INTSXP:
			*((float8*)out) = (float8)asInteger(*input);
			break;
		case REALSXP:
			*((float8*)out) = (float8)asReal(*input);
			break;
		default:
			res = -1;
	}
    return res;
}

static int plc_r_object_as_text(SEXP *input, char **output, plcRType *type UNUSED) {
    int res = 0;

    *output = strdup(CHAR(asChar(*input)));
    return res;
}


static void plc_r_object_iter_free (plcIterator *iter) {
    plcArrayMeta *meta;
    plcRArrMeta *Rmeta;
    meta = (plcArrayMeta*)iter->meta;
    Rmeta = (plcRArrMeta*)iter->payload;
    pfree(meta->dims);
    pfree(Rmeta->dims);
    pfree(iter->meta);
    pfree(iter->payload);
    pfree(iter->position);
    return;
}

static rawdata *plc_r_object_as_array_next (plcIterator *iter) {
    plcRArrMeta    *meta;
    plcRArrPointer *ptrs;
    rawdata         *res;
    SEXP        	mtx;
    int              ptr;

    meta = (plcRArrMeta*)iter->payload;
    ptrs = (plcRArrPointer*)iter->position;
    res  = (rawdata*)pmalloc(sizeof(rawdata));

    int idx;

    ptr = meta->ndims - 1;
    idx  =  ptrs[ptr].pos;
    mtx = ptrs[ptr].obj[0 ];
    if (mtx == R_NilValue || ( asInteger(mtx) == NA_INTEGER || asLogical(mtx) == NA_LOGICAL || mtx == NA_STRING) ) {
        res->isnull = 1;
        res->value = NULL;
    } else {
        res->isnull = 0;
        switch (meta->type->type)
		{
			case PLC_DATA_INT2:
			case PLC_DATA_INT4:
				/* 2 and 4 byte integer pgsql datatype => use R INTEGER */
				res->value = pmalloc(4);
				*((int *)res->value) = INTEGER_DATA(mtx)[idx];
				break;

				/*
				 * Other numeric types => use R REAL
				 * Note pgsql int8 is mapped to R REAL
				 * because R INTEGER is only 4 byte
				 */

			case PLC_DATA_INT8:
				res->value = pmalloc(8);
				*((int64 *)res->value) = (int64)(NUMERIC_DATA(mtx)[idx]);
				break;

			case PLC_DATA_FLOAT4:
				res->value = pmalloc(4);
				*((float4 *)res->value) = (float4)(NUMERIC_DATA(mtx)[idx]);

				break;
			case PLC_DATA_FLOAT8:
				res->value = pmalloc(8);
				*((float8 *)res->value) = (float8)(NUMERIC_DATA(mtx)[idx]);

				break;
			case PLC_DATA_INT1:
				res->value = pmalloc(1);
				*((int *)res->value) = LOGICAL_DATA(mtx)[idx];
				break;
			case PLC_DATA_RECORD:
			case PLC_DATA_UDT:
			case PLC_DATA_INVALID:
			case PLC_DATA_ARRAY:
				lprintf(ERROR, "un-handled type %d", meta->type->type);
				break;
			case PLC_DATA_TEXT:

				if (STRING_ELT(mtx, idx) != NA_STRING){
					res->isnull = FALSE;
					res->value  = pstrdup((char *) CHAR( STRING_ELT(mtx, idx) ));
				} else {
					res->isnull = TRUE;
					res->value  = NULL;
				}

			default:
				/* Everything else is defaulted to string */
				;//SET_STRING_ELT(*obj, elnum, COPY_TO_USER_STRING(value));
		}

    }

    while (ptr >= 0) {
        ptrs[ptr].pos += 1;
        /* If we finished up iterating over this dimension */
        if (ptrs[ptr].pos == meta->dims[ptr]) {
        	//TODO unprotect
            ptrs[ptr].obj = NULL;
            ptrs[ptr].pos = 0;
            ptr -= 1;
        }
        /* If we found the "next" dimension to iterate over */
        else if (ptrs[ptr].pos < meta->dims[ptr]) {
            ptr += 1;
            while (ptr < meta->ndims) {
            	//TODO this won't work
//                ptrs[ptr].obj = ptrs[ptr-1].obj[ ptrs[ptr-1].pos ];
                //TODO protect
                ptrs[ptr].pos = 0;
                ptr += 1;
            }
            break;
        }
    }

    return res;
}

static int plc_r_object_as_array(SEXP *input, char **output, plcRType *type) {
    plcRArrMeta    *meta;
    plcArrayMeta    *arrmeta;
    plcIterator     *iter;
    size_t           dims[PLC_MAX_ARRAY_DIMS];
   // SEXP        	 *stack[PLC_MAX_ARRAY_DIMS];
    int              ndims = 0;
    int              res = 0;
    int              i = 0;
    plcRArrPointer *ptrs;

    /* We allow only vector to be returned as arrays */
    if (*input != R_NilValue && (isVector(*input) || isMatrix(*input)) ) {
        /* TODO this is just for vectors */

    	ndims = 1;
    	dims[0] = length(*input);

        /* Allocate the iterator */
        iter = (plcIterator*)pmalloc(sizeof(plcIterator));

        /* Initialize metas */
        arrmeta = (plcArrayMeta*)pmalloc(sizeof(plcArrayMeta));
        arrmeta->ndims = ndims;
        arrmeta->dims = (int*)pmalloc(ndims * sizeof(int));
        arrmeta->size = (ndims == 0) ? 0 : 1;
        arrmeta->type = type->subTypes[0].type;

        meta = (plcRArrMeta*)pmalloc(sizeof(plcRArrMeta));
        meta->ndims = ndims;
        meta->dims  = (size_t*)pmalloc(ndims * sizeof(size_t));
        meta->outputfunc = plc_get_output_function(type->subTypes[0].type);
        meta->type = &type->subTypes[0];

        for (i = 0; i < ndims; i++) {
            meta->dims[i] = dims[i];
            arrmeta->dims[i] = (int)dims[i];
            arrmeta->size *= (int)dims[i];
        }

        iter->meta = arrmeta;
        iter->payload = (char*)meta;

        /* Initializing initial position */
        ptrs = (plcRArrPointer*)pmalloc(ndims * sizeof(plcRArrPointer));
        for (i = 0; i < ndims; i++) {
            ptrs[i].pos = 0;
            /* TODO this only works for one dimensional arrays */
            ptrs[i].obj = input;
        }
        iter->position = (char*)ptrs;

        /* not sure why this is necessary */
        /* Initializing "data" */
        iter->data = (char*)input;

        /* Initializing "next" and "cleanup" functions */
        iter->next = plc_r_object_as_array_next;
        iter->cleanup = plc_r_object_iter_free;

        *output = (char*)iter;
    } else {
        *output = NULL;
        res = -1;
    }

    return res;
}



static plcRInputFunc plc_get_input_function(plcDatatype dt) {
    plcRInputFunc res = NULL;
    switch (dt) {
        case PLC_DATA_INT1:
            res = plc_r_object_from_int1;
            break;
        case PLC_DATA_INT2:
            res = plc_r_object_from_int2;
            break;
        case PLC_DATA_INT4:
            res = plc_r_object_from_int4;
            break;
        case PLC_DATA_INT8:
            res = plc_r_object_from_int8;
            break;
        case PLC_DATA_FLOAT4:
            res = plc_r_object_from_float4;
            break;
        case PLC_DATA_FLOAT8:
            res = plc_r_object_from_float8;
            break;
        case PLC_DATA_TEXT:
            res = plc_r_object_from_text;
            break;
        case PLC_DATA_ARRAY:
            res = plc_r_object_from_array;
            break;
        case PLC_DATA_RECORD:
        case PLC_DATA_UDT:
        default:
            lprintf(ERROR, "Type %d cannot be passed plc_get_input_function function",
                    (int)dt);
            break;
    }
    return res;
}

static plcROutputFunc plc_get_output_function(plcDatatype dt) {
    plcROutputFunc res = NULL;
    switch (dt) {
        case PLC_DATA_INT1:
            res = plc_r_object_as_int1;
            break;
        case PLC_DATA_INT2:
            res = plc_r_object_as_int2;
            break;
        case PLC_DATA_INT4:
            res = plc_r_object_as_int4;
            break;
        case PLC_DATA_INT8:
            res = plc_r_object_as_int8;
            break;
        case PLC_DATA_FLOAT4:
            res = plc_r_object_as_float4;
            break;
        case PLC_DATA_FLOAT8:
            res = plc_r_object_as_float8;
            break;
        case PLC_DATA_TEXT:
            res = plc_r_object_as_text;
            break;
        case PLC_DATA_ARRAY:
            res = plc_r_object_as_array;
            break;
        case PLC_DATA_RECORD:
        case PLC_DATA_UDT:
        default:
            lprintf(ERROR, "Type %d cannot be passed plc_get_output_function function",
                    (int)dt);
            break;
    }
    return res;
}

static void plc_parse_type(plcRType *Rtype, plcType *type) {
    int i = 0;
    //Rtype->name = strdup(type->name); TODO: implement type name to support UDTs
    Rtype->name = strdup("results");
    Rtype->type = type->type;
    Rtype->nSubTypes = type->nSubTypes;
    Rtype->conv.inputfunc  = plc_get_input_function(Rtype->type);
    Rtype->conv.outputfunc = plc_get_output_function(Rtype->type);
    if (Rtype->nSubTypes > 0) {
        Rtype->subTypes = (plcRType*)malloc(Rtype->nSubTypes * sizeof(plcRType));
        for (i = 0; i < type->nSubTypes; i++)
            plc_parse_type(&Rtype->subTypes[i], &type->subTypes[i]);
    } else {
        Rtype->subTypes = NULL;
    }
}


plcRFunction *plc_R_init_function(callreq call) {
    plcRFunction *res;
    int i;

    res = (plcRFunction*)malloc(sizeof(plcRFunction));
    res->call = call;
    res->proc.src  = strdup(call->proc.src);
    res->proc.name = strdup(call->proc.name);
    res->nargs = call->nargs;
    res->args = (plcRType*)malloc(res->nargs * sizeof(plcRType));

    for (i = 0; i < res->nargs; i++)
        plc_parse_type(&res->args[i], &call->args[i].type);

    plc_parse_type(&res->res, &call->retType);

    return res;
}

plcRResult *plc_init_result_conversions(plcontainer_result res) {
    plcRResult *Rres = NULL;
    int i;

    Rres = (plcRResult*)malloc(sizeof(plcRResult));
    Rres->res = res;
    Rres->inconv = (plcRTypeConv*)malloc(res->cols * sizeof(plcRTypeConv));

    for (i = 0; i < res->cols; i++)
        Rres->inconv[i].inputfunc = plc_get_input_function(res->types[i].type);

    return Rres;
}



int get_entry_length(plcDatatype type)
{
	 switch (type) {
		case PLC_DATA_INT1:   return 1;
		case PLC_DATA_INT2:   return 2;
		case PLC_DATA_INT4:   return 4;
		case PLC_DATA_INT8:   return 8;
		case PLC_DATA_FLOAT4: return 4;
		case PLC_DATA_FLOAT8: return 8;
		case PLC_DATA_TEXT:   return 0;
		case PLC_DATA_ARRAY:
			lprintf(ERROR, "Array cannot be part of the array. "
					"Multi-dimensional arrays should be passed in a single entry");
			break;
		case PLC_DATA_RECORD:
			lprintf(ERROR, "Record data type not implemented yet");
			break;
		case PLC_DATA_UDT:
			lprintf(ERROR, "User-defined data types are not implemented yet");
			break;
		default:
			lprintf(ERROR, "Received unsupported argument type: %d", type);
			break;

	}
	return -1;
}

static void plc_r_free_type(plcRType *type) {
    int i = 0;
    for (i = 0; i < type->nSubTypes; i++)
        plc_r_free_type(&type->subTypes[i]);
    if (type->nSubTypes > 0)
        free(type->subTypes);
    return;
}

void plc_r_free_function(plcRFunction *func) {
    int i = 0;
    for (i = 0; i < func->nargs; i++)
        plc_r_free_type(&func->args[i]);
    plc_r_free_type(&func->res);
    free(func->args);
    free(func);
}

void plc_free_result_conversions(plcRResult *res) {
    free(res->inconv);
    free(res);
}
void plc_r_copy_type(plcType *type, plcRType *pytype) {
    type->type = pytype->type;
    type->nSubTypes = pytype->nSubTypes;
    if (type->nSubTypes > 0) {
        int i = 0;
        type->subTypes = (plcType*)pmalloc(type->nSubTypes * sizeof(plcType));
        for (i = 0; i < type->nSubTypes; i++)
            plc_r_copy_type(&type->subTypes[i], &pytype->subTypes[i]);
    } else {
        type->subTypes = NULL;
    }
}
/*
 * create an R vector of a given type and size based on pg output function oid
 */
SEXP get_r_vector(plcDatatype type_id, int numels)
{
    SEXP result;

    switch (type_id){
    case PLC_DATA_INT2:
    case PLC_DATA_INT4:
        PROTECT( result = NEW_INTEGER(numels) );
        break;
    case PLC_DATA_INT8:
    case PLC_DATA_FLOAT4:
    case PLC_DATA_FLOAT8:
        PROTECT( result = NEW_NUMERIC(numels) );
        break;
    case PLC_DATA_INT1:
        PROTECT( result = NEW_LOGICAL(numels) );
        break;
    case PLC_DATA_TEXT:
        default:
        PROTECT( result = NEW_CHARACTER(numels) );
        break;
    }
    UNPROTECT(1);
    return result;
}

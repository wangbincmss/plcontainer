#include <assert.h>

#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>

#include <R.h>
#include <Rversion.h>
#include <Rembedded.h>
#include <Rinternals.h>
#include <R_ext/Parse.h>
#include <Rdefines.h>


/* R's definition conflicts with the ones defined by postgres */
#undef WARNING
#undef ERROR

#include "common/comm_channel.h"
#include "common/comm_logging.h"
#include "common/comm_connectivity.h"
#include "common/comm_server.h"
#include "rcall.h"
#include "rsupport.h"

/*
 * set by hook throw_r_error
 */
extern char * last_R_error_msg;
SEXP convert_args(callreq req);


/*
  based on examples from:
  1. https://github.com/parkerabercrombie/call-r-from-c/blob/master/r_test.c
  2. https://github.com/wch/r-source/tree/trunk/tests/Embedding
  3. http://pabercrombie.com/wordpress/2014/05/how-to-call-an-r-function-from-c/

  Other resources:
  - https://cran.r-project.org/doc/manuals/r-release/R-exts.html
  - http://adv-r.had.co.nz/C-interface.html
 */

static void send_error(plcConn* conn, char *msg);
static char * create_r_func(const char *name, const char *src);

static char *create_r_func(callreq req);
static SEXP parse_r_code(const char *code, PGconn_min* conn);


void handle_call(callreq req, plcConn* conn) {
    SEXP             r,
					 strres,
					 call,
					 rargs,
					 obj,
					 args;

    int              i,
	                 errorOccurred;

    char *           func;
    const char *     txt;
    plcontainer_result res;

    /* wrap the input in a function and evaluate the result */
    func = create_r_func(req);

    PROTECT(r = parse_r_code(func, conn));

    if (errorOccurred) {
    	//TODO send real error message
        /* run_r_code will send an error back */
        UNPROTECT(1); //r
        return;
    }

    if(req->nargs > 0)
	{
        rargs = convert_args(req);
		PROTECT(obj = args = allocList(req->nargs));

		for (i = 0; i < req->nargs; i++)
		{
			SETCAR(obj, VECTOR_ELT(rargs, i));
			obj = CDR(obj);
		}
		UNPROTECT(1);
		PROTECT(call = lcons(r, args));
	}
	else
	{
		PROTECT(call = allocVector(LANGSXP,1));
		SETCAR(call, r);
	}

	strres = R_tryEval(call, R_GlobalEnv, &errorOccurred);
	UNPROTECT(1); //call


    if (errorOccurred) {
    	UNPROTECT(1); //r
    	//TODO send real error message
        send_error("cannot convert value to string", conn);
        return;
    }

    txt = CHAR(asChar(strres));

    /* allocate a result */
    res          = calloc(1, sizeof(*res));
    res->msgtype = MT_RESULT;
    res->types   = malloc(sizeof(*res->types));
    res->names   = malloc(sizeof(*res->names));
    res->data    = malloc(sizeof(*res->data));
    res->data[0] = malloc(sizeof(*res->data[0]));

    res->rows = res->cols = 1;
    res->types[0]         = pstrdup("text");
    res->names[0]         = pstrdup("result");
    res->data[0]->isnull  = false;
    res->data[0]->value   = (char *)txt;

    /* send the result back */
    plcontainer_channel_send(conn, (message)res);

    /* free the result object and the python value */
    free_result(res);

    UNPROTECT(1);

    return;
}

static void send_error(plcConn* conn, char *msg) {
    /* an exception was thrown */
    error_message err;
    err             = malloc(sizeof(*err));
    err->msgtype    = MT_EXCEPTION;
    err->message    = msg;
    err->stacktrace = "";

    /* send the result back */
    plcontainer_channel_send(conn, (message)err);

    /* free the objects */
    free(err);
}

static SEXP parse_r_code(const char *code,  plcConn* conn) {
    /* int hadError; */
    ParseStatus status;
    char *      error;
    SEXP        tmp,
			    rbody,
				fun;

    PROTECT(rbody = mkString(code));
    /*
      limit the number of expressions to be parsed to 2:
        - the definition of the function, i.e. f <- function() {...}
        - the call to the function f()

      kind of useful to prevent injection, but pointless since we are
      running in a container. I think -1 is equivalent to no limit.
    */
    PROTECT(tmp = R_ParseVector(rbody, -1, &status, R_NilValue));

    if (tmp != R_NilValue){
    	PROTECT(fun = VECTOR_ELT(tmp, 0));
    }else{
    	PROTECT(fun = R_NilValue);
    }

    if (status != PARSE_OK) {
        UNPROTECT(3);
        error         = last_R_error_msg;
        goto error;
    }

    UNPROTECT(3);
    return fun;

error:

    send_error(conn, error);
    return NULL;
}

static char * create_r_func(callreq req) {
    int    plen;
    char * mrc;
    size_t mlen = 0;

    int i;

    // calculate space required for args
    for (i=0;i<req->nargs;i++){
    	// +4 for , and space
    	mlen += strlen(req->args[i].name) + 4;
    }
    /*
     * room for function source and function call
     */
    mlen += strlen(req->proc.src) + strlen(req->proc.name) + 40;

    mrc  = malloc(mlen);
    plen = snprintf(mrc,mlen,"%s <- function(",req->proc.name);


    for (i=0;i<req->nargs;i++){

        strcat( mrc,req->args[i].name);

    	/* add a comma if not the last arg */
    	if ( i < (req->nargs-1) ){
    		strcat(mrc,", ") ;
    		plen += 2;
    	}

    	/* keep track of where we are copying */
    	plen+=strlen(req->args[i].name);
    }

    /* finish the function definition from where we left off */
    plen = snprintf(mrc+plen, mlen, ") {\n%s\n}\n%s()\n", req->proc.src,
                    req->proc.name);
    assert(plen >= 0 && ((size_t)plen) < mlen);
    return mrc;
}

#define INTEGER_ARG 1
#define NUMBER_ARG  2
#define BOOL_ARG 3
#define STRING_ARG  4

static SEXP get_r_array(int type, int size)
{
	SEXP result;

	switch (type){
	case INTEGER_ARG:
		PROTECT( result = NEW_INTEGER(size) );
		break;
	case NUMBER_ARG:
		PROTECT( result = NEW_NUMERIC(size) );
		break;
	case BOOL_ARG:
		PROTECT( result = NEW_LOGICAL(size) );
		break;
	case STRING_ARG:
		default:
		PROTECT( result = NEW_CHARACTER(size) );
		break;
	}
	UNPROTECT(1);
	return result;
}

SEXP convert_args(callreq req)
{
	SEXP	rargs, element;

	int    i;

    /* create the argument list */
	PROTECT(rargs = allocVector(VECSXP, req->nargs));

	for (i = 0; i < req->nargs; i++) {

        /*
        *  Use \N as null
        */
        if ( strcmp( req->args[i].value, "\\N" ) == 0 ) {
        	SET_VECTOR_ELT( rargs, i, R_NilValue );
        } else {
            if ( strcmp(req->args[i].type, "bool") == 0 ) {
            	PROTECT(element = get_r_array(BOOL_ARG,1));
                LOGICAL_DATA(element)[0] = (req->args[i].value[0]=='t'?1:0);
            	SET_VECTOR_ELT( rargs, i, element );
            	UNPROTECT(1);

            } else if ( (strcmp(req->args[i].type, "varchar") == 0)
            		 || (strcmp(req->args[i].type, "text") == 0) ) {

            	PROTECT(element = get_r_array(STRING_ARG,1));
            	SET_STRING_ELT(element, 0, COPY_TO_USER_STRING((char *)(req->args[i].value)));
            	SET_VECTOR_ELT( rargs, i, element );

            } else if ( (strcmp(req->args[i].type, "int2") == 0)
            		|| (strcmp(req->args[i].type, "int4") == 0) ) {

            	PROTECT(element = get_r_array(INTEGER_ARG,1));
            	INTEGER_DATA(element)[0] = atof((char *)req->args[i].value);
            	SET_VECTOR_ELT( rargs, i, element );

            } else if ( (strcmp(req->args[i].type, "int8") == 0)
                    || (strcmp(req->args[i].type, "float4") == 0)
					|| (strcmp(req->args[i].type, "float8") == 0) )
            {
            	PROTECT(element = get_r_array(NUMBER_ARG,1));
            	REAL(element)[0] = atof((char *)req->args[i].value);
            	SET_VECTOR_ELT( rargs, i, element );

            } else {
                lprintf(ERROR, "unknown type %s", req->args[i].type);
            }
        }
    }
	UNPROTECT(1);
	return rargs;
}


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

#if (R_VERSION >= 132352) /* R_VERSION >= 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)               R_ParseVector(a_, b_, (ParseStatus *) c_, R_NilValue)
#else /* R_VERSION < 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)               R_ParseVector(a_, b_, (ParseStatus *) c_)
#endif /* R_VERSION >= 2.5.0 */


/* R's definition conflicts with the ones defined by postgres */
#undef WARNING
#undef ERROR

#include "common/comm_channel.h"
#include "common/comm_logging.h"
#include "common/comm_connectivity.h"
#include "common/comm_server.h"
#include "rcall.h"


SEXP convert_args(callreq req);

#define OPTIONS_NULL_CMD	"options(error = expression(NULL))"

/* install the error handler to call our throw_r_error */
#define THROWRERROR_CMD \
			"pg.throwrerror <-function(msg) " \
			"{" \
			"  msglen <- nchar(msg);" \
			"  if (substr(msg, msglen, msglen + 1) == \"\\n\")" \
			"    msg <- substr(msg, 1, msglen - 1);" \
			"  .C(\"throw_r_error\", as.character(msg));" \
			"}"
#define OPTIONS_THROWRERROR_CMD \
			"options(error = expression(pg.throwrerror(geterrmessage())))"

/* install the notice handler to call our throw_r_notice */
#define THROWNOTICE_CMD \
			"pg.thrownotice <-function(msg) " \
			"{.C(\"throw_pg_notice\", as.character(msg))}"
#define THROWERROR_CMD \
			"pg.throwerror <-function(msg) " \
			"{stop(msg, call. = FALSE)}"
#define OPTIONS_THROWWARN_CMD \
			"options(warning.expression = expression(pg.thrownotice(last.warning)))"

#define QUOTE_LITERAL_CMD \
			"pg.quoteliteral <-function(sql) " \
			"{.Call(\"plr_quote_literal\", sql)}"
#define QUOTE_IDENT_CMD \
			"pg.quoteident <-function(sql) " \
			"{.Call(\"plr_quote_ident\", sql)}"
#define SPI_EXEC_CMD \
			"pg.spi.exec <-function(sql) {.Call(\"plr_SPI_exec\", sql)}"

int R_SignalHandlers = 1;  /* Exposed in R_interface.h */

static void load_r_cmd(const char *cmd);
static char * get_load_self_ref_cmd(const char *libstr);

// Initialization of R module
void r_init( );


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
static char * create_r_func(callreq req);

static char *create_r_func(callreq req);
static SEXP parse_r_code(const char *code, plcConn* conn, int *errorOccurred);

/*
 * set by hook throw_r_error
 */
char *last_R_error_msg,
     *last_R_notice;

extern SEXP plr_SPI_execp(const char * sql);



plcConn* plcconn;


void r_init( ) {
    char   *argv[] = {"client", "--slave", "--vanilla"};

    /*
     * Stop R using its own signal handlers Otherwise, R will prompt the user for what to do and
         will hang in the container
    */
    R_SignalHandlers = 0;

    if( !Rf_initEmbeddedR(sizeof(argv) / sizeof(*argv), argv) ){
    	//TODO: return an error
    	;
    }



    /*
	 * temporarily turn off R error reporting -- it will be turned back on
	 * once the custom R error handler is installed from the plr library
	 */
	load_r_cmd(OPTIONS_NULL_CMD);

	/* next load the plr library into R */
	load_r_cmd(get_load_self_ref_cmd("librcall.so"));

    load_r_cmd(THROWRERROR_CMD);
    load_r_cmd(OPTIONS_THROWRERROR_CMD);
    load_r_cmd(THROWNOTICE_CMD);
    load_r_cmd(THROWERROR_CMD);
    load_r_cmd(OPTIONS_THROWWARN_CMD);
    load_r_cmd(QUOTE_LITERAL_CMD);
    load_r_cmd(QUOTE_IDENT_CMD);
    load_r_cmd(SPI_EXEC_CMD);
}

static  char *get_load_self_ref_cmd(const char *libstr)
{
	char   *buf =  (char *) malloc(strlen(libstr) + 12 + 1);;

	sprintf(buf, "dyn.load(\"%s\")", libstr);
	return buf;
}

static void
load_r_cmd(const char *cmd)
{
	SEXP		cmdSexp,
				cmdexpr;
	int			i,
				status=0;


	PROTECT(cmdSexp = NEW_CHARACTER(1));
	SET_STRING_ELT(cmdSexp, 0, COPY_TO_USER_STRING(cmd));
	PROTECT(cmdexpr = R_PARSEVECTOR(cmdSexp, -1, &status));
	if (status != PARSE_OK) {
		UNPROTECT(2);
		goto error;
	}

	/* Loop is needed here as EXPSEXP may be of length > 1 */
	for(i = 0; i < length(cmdexpr); i++)
	{
		R_tryEval(VECTOR_ELT(cmdexpr, i), R_GlobalEnv, &status);
		if(status != 0)
		{
			goto error;
		}
	}

	UNPROTECT(2);
	return;

error:
	// TODO send error back to client
	printf("Error loading %s \n ",cmd);
	return;

}

void handle_call(callreq req, plcConn* conn) {
    SEXP             r,
					 strres,
					 call,
					 rargs,
					 obj,
					 args;

    int              i,
	                 errorOccurred;

    char 			*func,
					*errmsg;

    const char *     txt;
    plcontainer_result res;

    /*
     * Keep our connection for future calls from R back to us.
    */
    plcconn = conn;

    /* wrap the input in a function and evaluate the result */
    func = create_r_func(req);

    PROTECT(r = parse_r_code(func, conn, &errorOccurred));

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
    	if (last_R_error_msg){
    		errmsg = strdup(last_R_error_msg);
    	}else{
    		errmsg = strdup("Error executing\n");
    		errmsg = realloc(errmsg, strlen(errmsg)+strlen(req->proc.src));
    		errmsg = strcat(errmsg, req->proc.src);
    	}
    	send_error(conn, last_R_error_msg);
    	free(errmsg);
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

static SEXP parse_r_code(const char *code,  plcConn* conn, int *errorOccurred) {
    /* int hadError; */
    ParseStatus status;
    char *      errmsg;
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
        if (last_R_error_msg != NULL){
        	errmsg  = strdup(last_R_error_msg);
        }else{
        	errmsg =  strdup("Parse Error\n");
        	errmsg =  realloc(errmsg, strlen(errmsg)+strlen(code));
        	errmsg =  strcat(errmsg, code);
        }
        goto error;
    }

    UNPROTECT(3);
    *errorOccurred=0;
    return fun;

error:
	/*
	 * set the global error flag
	 */
	*errorOccurred=1;
    send_error(conn, errmsg);
    free(errmsg);
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

#ifdef XXX
/*
 * plr_quote_literal() - quote literal strings that are to
 *			  be used in SPI_exec query strings
 */
SEXP
plr_quote_literal(SEXP rval)
{
	const char *value;
	text	   *value_text;
	text	   *result_text;
	SEXP		result;

	/* extract the C string */
	PROTECT(rval =  AS_CHARACTER(rval));
	value = CHAR(STRING_ELT(rval, 0));

	/* convert using the pgsql quote_literal function */
	value_text = PG_STR_GET_TEXT(value);
	result_text = value_text; //DatumGetTextP(DirectFunctionCall1(quote_literal, PointerGetDatum(value_text)));

	/* copy result back into an R object */
	PROTECT(result = NEW_CHARACTER(1));
	SET_STRING_ELT(result, 0, COPY_TO_USER_STRING(PG_TEXT_GET_STR(result_text)));
	UNPROTECT(2);

	return result;
}

/*
 * plr_quote_literal() - quote identifiers that are to
 *			  be used in SPI_exec query strings
 */
SEXP
plr_quote_ident(SEXP rval)
{
	const char *value;
	text	   *value_text;
	text	   *result_text;
	SEXP		result;

	/* extract the C string */
	PROTECT(rval =  AS_CHARACTER(rval));
	value = CHAR(STRING_ELT(rval, 0));

	/* convert using the pgsql quote_literal function */
	value_text = PG_STR_GET_TEXT(value);
	result_text = value_text; //DatumGetTextP(DirectFunctionCall1(quote_ident, PointerGetDatum(value_text)));

	/* copy result back into an R object */
	PROTECT(result = NEW_CHARACTER(1));
	SET_STRING_ELT(result, 0, COPY_TO_USER_STRING(PG_TEXT_GET_STR(result_text)));
	UNPROTECT(2);

	return result;
}
#endif //XXX

/*
 * create an R vector of a given type and size based on pg output function oid
 */
static SEXP
get_r_vector(char type, int numels)
{
	SEXP	result;

	switch (type)
	{
		case 'i':
			/* 2 and 4 byte integer pgsql datatype => use R INTEGER */
			PROTECT(result = NEW_INTEGER(numels));
			break;

		case 'f':
			/*
			 * Other numeric types => use R REAL
			 * Note pgsql int8 is mapped to R REAL
			 * because R INTEGER is only 4 byte
			 */
			PROTECT(result = NEW_NUMERIC(numels));
			break;
		case 't':
			PROTECT(result = NEW_LOGICAL(numels));
			break;
		default:
			/* Everything else is defaulted to string */
			PROTECT(result = NEW_CHARACTER(numels));
	}
	UNPROTECT(1);

	return result;
}

/*
 * given a single non-array pg value, convert to its R value representation
 */
static void
pg_get_one_r(char *value,  char type, SEXP *obj, int elnum)
{
	switch (type)
	{
		case 'i':
			/* 2 and 4 byte integer pgsql datatype => use R INTEGER */
			if (value)
				INTEGER_DATA(*obj)[elnum] = atoi(value);
			else
				INTEGER_DATA(*obj)[elnum] = NA_INTEGER;
			break;
		case 'f':
			/*
			 * Other numeric types => use R REAL
			 * Note pgsql int8 is mapped to R REAL
			 * because R INTEGER is only 4 byte
			 */
			if (value)
				NUMERIC_DATA(*obj)[elnum] = atof(value);
			else
				NUMERIC_DATA(*obj)[elnum] = NA_REAL;
			break;
		case 't':
			if (value)
				LOGICAL_DATA(*obj)[elnum] = ((*value == 't') ? 1 : 0);
			else
				LOGICAL_DATA(*obj)[elnum] = NA_LOGICAL;
			break;
		default:
			/* Everything else is defaulted to string */
			if (value)
				SET_STRING_ELT(*obj, elnum, COPY_TO_USER_STRING(value));
			else
				SET_STRING_ELT(*obj, elnum, NA_STRING);
	}
}

/*
 * plr_SPI_exec - The builtin SPI_exec command for the R interpreter
 */
SEXP
plr_SPI_exec( SEXP rsql )
{
	const char  		   *sql;
	SEXP			r_result = NULL,
					names,
					fldvec;

	int             res = 0,
					i,j;

	sql_msg_statement  msg;
	plcontainer_result result;
	message            resp;

	PROTECT(rsql =  AS_CHARACTER(rsql));
	sql = CHAR(STRING_ELT(rsql, 0));
	UNPROTECT(1);

	if (sql == NULL){
		error("%s", "cannot execute empty query");
		return NULL;
	}


	msg            = pmalloc(sizeof(*msg));
	msg->msgtype   = MT_SQL;
	msg->sqltype   = SQL_TYPE_STATEMENT;
	/*
	 * satisfy compiler
	 */
	msg->statement = (char *)sql;

	plcontainer_channel_send(plcconn, (message)msg);

	/* we don't need it anymore */
	pfree(msg);

	receive:
	res = plcontainer_channel_receive(plcconn, &resp);
	if (res < 0) {
		lprintf (ERROR, "Error receiving data from the backend, %d", res);
		return NULL;
	}

	switch (resp->msgtype) {
	   case MT_CALLREQ:
		  handle_call((callreq)resp, plcconn);
		  free_callreq((callreq)resp);
		  goto receive;
	   case MT_RESULT:
		   break;
	   default:
		   lprintf(WARNING, "didn't receive result back %c", resp->msgtype);
		   return NULL;
	}

	result = (plcontainer_result)resp;
	if (result->rows == 0){
		return R_NilValue;
	}

	PROTECT(r_result = NEW_LIST(result->cols));
	PROTECT(names = NEW_CHARACTER(result->cols));


	for (j=0; j<result->cols;j++){
		/*
		 * set the names of the columns
		 */
		SET_STRING_ELT(names, j, Rf_mkChar(result->names[j]));

		for ( i=0; i<result->rows; i++ ){
			PROTECT(fldvec = get_r_vector(result->types[i][j], result->rows));
			pg_get_one_r(result->data[i][j].value, result->types[i][j], &fldvec, i);
			UNPROTECT(1);

		}
		SET_VECTOR_ELT(r_result, j, fldvec);
	}

	free_result(result);
	UNPROTECT(2);
	return r_result;
}


void
throw_pg_notice(const char **msg)
{
	if (msg && *msg)
		last_R_notice = strdup(*msg);
}

void
throw_r_error(const char **msg)
{
	if (msg && *msg)
		last_R_error_msg = strdup(*msg);
	else
		last_R_error_msg = strdup("caught error calling R function");
}


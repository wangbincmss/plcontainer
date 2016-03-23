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

static SEXP coerce_to_char(SEXP rval);
SEXP convert_args(callreq req);

#define OPTIONS_NULL_CMD    "options(error = expression(NULL))"

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

#define SPI_DBGETQUERY_CMD \
            "dbGetQuery <-function(sql) {\n" \
            "data <- pg.spi.exec(sql)\n" \
            "return(data)\n" \
            "}"

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

static plcontainer_iterator matrix_iterator(SEXP mtx);
static void matrix_iterator_free(plcontainer_iterator iter);

/*
 * set by hook throw_r_error
 */
char *last_R_error_msg,
     *last_R_notice;

extern SEXP plr_SPI_execp(const char * sql);



plcConn* plcconn;


void r_init( ) {
    char   *argv[] = {"client", "--slave", "--vanilla"};
    char   *    buf;

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
    load_r_cmd(buf=get_load_self_ref_cmd("librcall.so"));
    pfree(buf);

    load_r_cmd(THROWRERROR_CMD);
    load_r_cmd(OPTIONS_THROWRERROR_CMD);
    load_r_cmd(THROWNOTICE_CMD);
    load_r_cmd(THROWERROR_CMD);
    load_r_cmd(OPTIONS_THROWWARN_CMD);
    load_r_cmd(QUOTE_LITERAL_CMD);
    load_r_cmd(QUOTE_IDENT_CMD);
    load_r_cmd(SPI_EXEC_CMD);
    load_r_cmd(SPI_DBGETQUERY_CMD);
}

static  char *get_load_self_ref_cmd(const char *libstr)
{
    char   *buf =  (char *) pmalloc(strlen(libstr) + 12 + 1);;

    sprintf(buf, "dyn.load(\"%s\")", libstr);
    return buf;
}

static void
load_r_cmd(const char *cmd)
{
    SEXP        cmdSexp,
                cmdexpr;
    int            i,
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
static char *get_base_type(SEXP rval)
{
    switch (TYPEOF(rval)) {
        case INTSXP:
            return "_int8";
        case REALSXP:
            return "_float8";
        case STRSXP:
            return "_text";
        default:
            return NULL;
    }
}

void handle_call(callreq req, plcConn* conn) {
    SEXP             r,
                     dfcol,
                     strres,
                     call,
                     rargs,
                     obj,
                     args;

    int              i,
                     errorOccurred;

    char            *func,
                    *errmsg;

    const char *     value;
    plcontainer_result res;

    /*
     * Keep our connection for future calls from R back to us.
    */
    plcconn = conn;

    /* wrap the input in a function and evaluate the result */
    func = create_r_func(req);

    PROTECT(r = parse_r_code(func, conn, &errorOccurred));

    pfree(func);
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
        send_error(conn, errmsg);
        free(errmsg);
        return;
    }

    if (isFrame(strres) ){

        SEXP names;
        PROTECT(names = getAttrib(strres,R_NamesSymbol));
        int cols;
        int rows, col, row;


        if( isFrame(strres) ){
            cols = length(strres);
        }else{
            cols = 1;
        }


        /* allocate a result */
        res          = pmalloc(sizeof(*res));
        res->msgtype = MT_RESULT;
        res->types   = pmalloc(sizeof(*res->types)*cols);
        res->names   = pmalloc(sizeof(*res->names)*cols);

        for ( col=0; col < cols; col++){

            res->names[col] = pstrdup(CHAR(STRING_ELT(names,col)));
            res->types[col] = pstrdup("text");

            if (TYPEOF(strres) == VECSXP){
                PROTECT(dfcol = VECTOR_ELT(strres, col));
            }else if (TYPEOF(strres) == LISTSXP){
                PROTECT(dfcol = CAR(strres));
                strres = CDR(strres);
            }else{
                /* internal error */
                // TODO send error
                //elog(ERROR, "plr: bad internal representation of data.frame");
            }

            if (ATTRIB(dfcol) == R_NilValue ||
                TYPEOF(CAR(ATTRIB(dfcol))) != STRSXP){
                    PROTECT(obj = coerce_to_char(dfcol));
            }else{
                PROTECT(obj = coerce_to_char(CAR(ATTRIB(dfcol))));
            }

            /*
             * get the first column just to get the number of rows
             */

            if (col == 0){
                if (isFrame(strres) || isMatrix(strres) ){
                    rows = length(obj);
                }else{
                    rows = 1;
                }


                res->data    = pmalloc(sizeof(*res->data) * rows);
                /*
                 * allocate memory when we do the first column
                 */
                for (row=0; row< rows;row++){
                    res->data[row] = pmalloc(cols * sizeof(res->data[0][0]));
                }

                res->rows = rows;
                res->cols = cols;
            }

            for (row=0;row < rows; row++){

                value = strdup(CHAR(STRING_ELT(obj, row)));
                //int idx = ((row * cols) + col);

                if (STRING_ELT(obj, i) == NA_STRING || value == NULL)
                {
                    res->data[row][col].isnull  = true;
                    res->data[row][col].value   = NULL;
                }
                else
                {
                    res->data[row][col].isnull  = false;
                    res->data[row][col].value   = (char *)value;
                }
            }
            UNPROTECT(2);

        }
        UNPROTECT(1);
    } else {
        /* this is not a data frame */
        res          = pmalloc(sizeof(*res));
        res->msgtype = MT_RESULT;
        res->types   = pmalloc(sizeof(*res->types));
        res->names   = pmalloc(sizeof(*res->names));
        res->data    = pmalloc(sizeof(*res->data));
        res->data[0] = pmalloc(sizeof(*res->data[0]));
        res->rows = res->cols = 1;
        res->names[0]         = pstrdup("result");
        res->data[0]->isnull  = false;
        if (isMatrix(strres)) {
            char* basetype = get_base_type(strres);
            if (basetype == NULL) {
                char *errmsg = pmalloc(100);
                sprintf(errmsg,
                        "Matrices of the type '%d' are not supported yet",
                        TYPEOF(strres));
                send_error(conn, errmsg);
                pfree(errmsg);
            }
            res->types[0]         = pstrdup(basetype);
            res->data[0]->value   = (char*)matrix_iterator(strres);
        } else {
            res->types[0]         = pstrdup("text");
            res->data[0]->value   = pstrdup(CHAR(asChar(strres)));
        }
    }

    /* send the result back */
    plcontainer_channel_send(conn, (message)res);

    /* free the result object and the R values */
    free_result(res);

    UNPROTECT(1);

    return;
}

raw matrix_iterator_next (plcontainer_iterator iter) {
    plcontainer_array_meta meta;
    int *position;
    SEXP mtx;
    raw res;

    meta = (plcontainer_array_meta)iter->meta;
    position = (int*)iter->position;
    mtx = (SEXP)iter->data;
    res = pmalloc(sizeof(str_raw));

    //lprintf(WARNING, "Position: %d, %d", position[0], position[1]);

    if (STRING_ELT(mtx, position[1]*meta->dims[0] + position[0]) != NA_STRING){
        res->isnull = FALSE;
        res->value  = pstrdup((char *) CHAR(
                        STRING_ELT(mtx, position[1]*meta->dims[0] + position[0]))
                      );
    } else {
        res->isnull = FALSE;
        res->value  = NULL;
    }

    position[1] += 1;
    if (position[1] == meta->dims[1]) {
        position[1] = 0;
        position[0] += 1;
    }
    if (position[0] == meta->dims[0])
        matrix_iterator_free(iter);

    return res;
}

static plcontainer_iterator matrix_iterator(SEXP mtx) {
    plcontainer_array_meta meta;
    int *position;
    SEXP obj;
    plcontainer_iterator iter;

    /* Allocate the iterator */
    iter = (plcontainer_iterator)pmalloc(sizeof(str_plcontainer_iterator));

    /* Initialize meta */
    meta = (plcontainer_array_meta)pmalloc(sizeof(str_plcontainer_array_meta));
    meta->ndims = 2;
    meta->dims  = (int*)pmalloc(2 * sizeof(int));
    meta->dims[0] = nrows(mtx);
    meta->dims[1] = ncols(mtx);
    meta->size = meta->dims[0] * meta->dims[1];
    iter->meta = (char*)meta;

    /* Initializing initial position */
    position = (int*)pmalloc(2 * sizeof(int));
    position[0] = 0;
    position[1] = 0;
    iter->position = (char*)position;

    /* Initializing "data" */
    PROTECT(obj =  coerce_to_char(mtx));
    iter->data = (char*)obj;

    /* Initializing "next" function */
    iter->next = matrix_iterator_next;

    return iter;
}

static void matrix_iterator_free(plcontainer_iterator iter) {
    pfree(((plcontainer_array_meta)iter->meta)->dims);
    pfree(iter->meta);
    pfree(iter->position);
    UNPROTECT(1);
}

static void send_error(plcConn* conn, char *msg) {
    /* an exception was thrown */
    error_message err;
    err             = pmalloc(sizeof(*err));
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

    mrc  = pmalloc(mlen);
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
#define BOOL_ARG    3
#define STRING_ARG  4

/*
 * create an R vector of a given type and size based on pg output function oid
 */
static SEXP
get_r_vector(int type_id, int numels)
{
    SEXP result;

    switch (type_id){
    case INTEGER_ARG:
        PROTECT( result = NEW_INTEGER(numels) );
        break;
    case NUMBER_ARG:
        PROTECT( result = NEW_NUMERIC(numels) );
        break;
    case BOOL_ARG:
        PROTECT( result = NEW_LOGICAL(numels) );
        break;
    case STRING_ARG:
        default:
        PROTECT( result = NEW_CHARACTER(numels) );
        break;
    }
    UNPROTECT(1);
    return result;
}

static int find_type(const char *type)
{
    if ( strcmp(type, "bool") == 0 ) {
        return BOOL_ARG;
    } else if ( (strcmp(type, "varchar") == 0)
             || (strcmp(type, "text") == 0) ) {
        return STRING_ARG;
    } else if ( (strcmp(type, "int2") == 0)
            || (strcmp(type, "int4") == 0) ) {
        return INTEGER_ARG;
    } else if ( (strcmp(type, "int8") == 0)
            || (strcmp(type, "float4") == 0)
            || (strcmp(type, "float8") == 0) ){
        return NUMBER_ARG;
    }else{
        return STRING_ARG;
    }
}

SEXP convert_args(callreq req)
{
    SEXP    rargs, element;

    int    i,
           arg_type;

    /* create the argument list */
    PROTECT(rargs = allocVector(VECSXP, req->nargs));

    for (i = 0; i < req->nargs; i++) {

        /*
        *  Use \N as null
        */
        if ( strcmp( req->args[i].value, "\\N" ) == 0 ) {
            SET_VECTOR_ELT( rargs, i, R_NilValue );
        } else {
            arg_type =  find_type(req->args[i].type);
            switch(arg_type){

            case BOOL_ARG:
                PROTECT(element = get_r_vector(BOOL_ARG,1));
                LOGICAL_DATA(element)[0] = (req->args[i].value[0]=='t'?1:0);
                SET_VECTOR_ELT( rargs, i, element );
                UNPROTECT(1);
                break;

            case STRING_ARG:

                PROTECT(element = get_r_vector(STRING_ARG,1));
                SET_STRING_ELT(element, 0, COPY_TO_USER_STRING((char *)(req->args[i].value)));
                SET_VECTOR_ELT( rargs, i, element );
                break;

            case INTEGER_ARG:

                PROTECT(element = get_r_vector(INTEGER_ARG,1));
                INTEGER_DATA(element)[0] = atof((char *)req->args[i].value);
                SET_VECTOR_ELT( rargs, i, element );
                break;

            case NUMBER_ARG:
                PROTECT(element = get_r_vector(NUMBER_ARG,1));
                REAL(element)[0] = atof((char *)req->args[i].value);
                SET_VECTOR_ELT( rargs, i, element );
                break;

            default:
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
 *              be used in SPI_exec query strings
 */
SEXP
plr_quote_literal(SEXP rval)
{
    const char *value;
    text       *value_text;
    text       *result_text;
    SEXP        result;

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
 *              be used in SPI_exec query strings
 */
SEXP
plr_quote_ident(SEXP rval)
{
    const char *value;
    text       *value_text;
    text       *result_text;
    SEXP        result;

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
 * given a single non-array pg value, convert to its R value representation
 */
static void
pg_get_one_r(char *value,  int column_type, SEXP *obj, int elnum)
{
    switch (column_type)
    {
        case INTEGER_ARG:
            /* 2 and 4 byte integer pgsql datatype => use R INTEGER */
            if (value)
                INTEGER_DATA(*obj)[elnum] = atoi(value);
            else
                INTEGER_DATA(*obj)[elnum] = NA_INTEGER;
            break;
        case NUMBER_ARG:
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
        case BOOL_ARG:
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
    const char             *sql;
    SEXP            r_result = NULL,
                    names,
                    row_names,
                    fldvec;

    int             res = 0,
                    i,j,
                    column_type;
    char             buf[256];

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
    /*
     * r_result is a list of columns
     */
    PROTECT(r_result = NEW_LIST(result->cols));
    /*
     * names for each column
     */
    PROTECT(names = NEW_CHARACTER(result->cols));

    /*
     * we store everything in columns because vectors can only have one type
     * normally we get tuples back in rows with each column possibly a different type,
     * instead we store each column in a single vector
     */

    for (j=0; j<result->cols;j++){
        /*
         * set the names of the column
         */
        SET_STRING_ELT(names, j, Rf_mkChar(result->names[j]));
        column_type = find_type(result->types[0]);

        //create a vector of the type that is rows long
        PROTECT(fldvec = get_r_vector(column_type, result->rows));

        for ( i=0; i<result->rows; i++ ){
            /*
             * store the value
             */
            pg_get_one_r(result->data[i][j].value, column_type, &fldvec, i);
        }

        UNPROTECT(1);
        SET_VECTOR_ELT(r_result, j, fldvec);
    }

    /* attach the column names */
    setAttrib(r_result, R_NamesSymbol, names);

    /* attach row names - basically just the row number, zero based */
    PROTECT(row_names = allocVector(STRSXP, result->rows));

    for ( i=0; i < result->rows; i++ ){
        sprintf(buf, "%d", i+1);
        SET_STRING_ELT(row_names, i, COPY_TO_USER_STRING(buf));
    }

    setAttrib(r_result, R_RowNamesSymbol, row_names);

    /* finally, tell R we are a data.frame */
    setAttrib(r_result, R_ClassSymbol, mkString("data.frame"));

    /*
     * result has
     *
     * an attribute names which is a vector of names
     * a vector of vectors num columns long by num rows
     */
    free_result(result);
    UNPROTECT(3);
    return r_result;
}

static SEXP
coerce_to_char(SEXP rval)
{
    SEXP    obj = NULL;

    switch (TYPEOF(rval))
    {
        case LISTSXP:
        case NILSXP:
        case SYMSXP:
        case VECSXP:
        case EXPRSXP:
        case LGLSXP:
        case INTSXP:
        case REALSXP:
        case CPLXSXP:
        case STRSXP:
        case RAWSXP:
            PROTECT(obj = AS_CHARACTER(rval));
            break;
        default:
            ;//TODO error
    }
    UNPROTECT(1);

    return obj;
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


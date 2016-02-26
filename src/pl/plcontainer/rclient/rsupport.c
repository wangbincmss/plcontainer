#include "rsupport.h"


int R_SignalHandlers = 1;  /* Exposed in R_interface.h */
char *last_R_error_msg,
     *last_R_notice;

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

static void load_r_cmd(const char *cmd);
static char * get_load_self_ref_cmd(const char *libstr);


void r_init() {
    char   *argv[] = {"client", "--slave", "--vanilla"};

    if( !Rf_initEmbeddedR(sizeof(argv) / sizeof(*argv), argv) ){
    	//TODO: return an error
    	;
    }
    /*
    * Stop R using its own signal handlers Otherwise, R will prompt the user for what to do and
      will hang in the container
    */
    R_SignalHandlers = 0;

    /*
    	 * temporarily turn off R error reporting -- it will be turned back on
    	 * once the custom R error handler is installed from the plr library
    	 */
	load_r_cmd(OPTIONS_NULL_CMD);

	/* next load the plr library into R */
	load_r_cmd(get_load_self_ref_cmd("librsupport.so"));

    load_r_cmd(THROWRERROR_CMD);
    load_r_cmd(OPTIONS_THROWRERROR_CMD);
    load_r_cmd(THROWNOTICE_CMD);
    load_r_cmd(THROWERROR_CMD);
    load_r_cmd(OPTIONS_THROWWARN_CMD);
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

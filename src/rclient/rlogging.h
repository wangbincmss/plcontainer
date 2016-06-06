#ifndef PLC_RLOGGING_H
#define PLC_RLOGGING_H

#include <R.h>

SEXP plr_debug( char *args );
SEXP plr_log( char *args );
SEXP plr_info( char *args );
SEXP plr_notice( char *args );
SEXP plr_warning( char *args );
SEXP plr_error( char *args );
SEXP plr_fatal( char *args );

#endif /* PLC_RLOGGING_H */

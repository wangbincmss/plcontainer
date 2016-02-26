#ifndef __RSUPPORT_H__
#define __RSUPPORT_H__

#include <R.h>
#include <Rversion.h>
#include <Rembedded.h>
#include <Rinternals.h>
#include <R_ext/Parse.h>
#include <Rdefines.h>


#if (R_VERSION >= 132352) /* R_VERSION >= 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)		R_ParseVector(a_, b_, (ParseStatus *) c_, R_NilValue)
#else /* R_VERSION < 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)		R_ParseVector(a_, b_, (ParseStatus *) c_)
#endif /* R_VERSION >= 2.5.0 */


// Signal processing function signature
typedef void (*sighandler_t)(int);

// Initialization of R module
void r_init(void);

extern void throw_pg_notice(const char **msg);
extern void throw_r_error(const char **msg);


#endif /* __RSUPPORT_H__ */

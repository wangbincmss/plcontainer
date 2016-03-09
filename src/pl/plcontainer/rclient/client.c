#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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

#if (R_VERSION >= 132352) /* R_VERSION >= 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)               R_ParseVector(a_, b_, (ParseStatus *) c_, R_NilValue)
#else /* R_VERSION < 2.5.0 */
#define R_PARSEVECTOR(a_, b_, c_)               R_ParseVector(a_, b_, (ParseStatus *) c_)
#endif /* R_VERSION >= 2.5.0 */




void r_init();

int main(int argc UNUSED, char **argv UNUSED) {
    int      sock;
    plcConn* conn;

    // Bind the socket and start listening the port
    sock = start_listener();

    // Initialize R
    r_init();


#ifdef _DEBUG_CLIENT
        // In debug mode we have a cycle of connections with infinite wait time

    while (true) {
        conn = connection_init(sock);
		receive_loop(handle_call, conn);
    }
#else


    // In release mode we wait for incoming connection for limited time
	// and the client works for a single connection only
	connection_wait(sock);
	conn = connection_init(sock);

	receive_loop(handle_call, conn);
#endif

    lprintf(NOTICE, "Client has finished execution");
    return 0;
}




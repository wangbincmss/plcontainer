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
 * file:			libpq-mini-secure.c
 * description:		very minimal functionality from the original
 * 				    libpq implementation. Structures and
 * 				    functionalities are extremely simplified.
 * author:			PostgreSQL developement group.
 * author:			Laszlo Hornyak
 */

/*-------------------------------------------------------------------------
 *
 * fe-secure.c
 *	  functions related to setting up a secure connection to the backend.
 *	  Secure connections are expected to provide confidentiality,
 *	  message integrity and endpoint authentication.
 *
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /cvsroot/pgsql-server/src/interfaces/libpq/fe-secure.c,v 1.32
 *2003/09/29 16:38:04 petere
 *Exp $
 *
 * NOTES
 *	  The client *requires* a valid server certificate.  Since
 *	  SSH tunnels provide anonymous confidentiality, the presumption
 *	  is that sites that want endpoint authentication will use the
 *	  direct SSL support, while sites that are comfortable with
 *	  anonymous connections will use SSH tunnels.
 *
 *	  This code verifies the server certificate, to detect simple
 *	  "man-in-the-middle" and "impersonation" attacks.	The
 *	  server certificate, or better yet the CA certificate used
 *	  to sign the server certificate, should be present in the
 *	  "$HOME/.postgresql/root.crt" file.  If this file isn't
 *	  readable, or the server certificate can't be validated,
 *	  pqsecure_open_client() will return an error code.
 *
 *	  Additionally, the server certificate's "common name" must
 *	  resolve to the other end of the socket.  This makes it
 *	  substantially harder to pull off a "man-in-the-middle" or
 *	  "impersonation" attack even if the server's private key
 *	  has been stolen.	This check limits acceptable network
 *	  layers to Unix sockets (weird, but legal), TCPv4 and TCPv6.
 *
 *	  Unfortunately neither the current front- or back-end handle
 *	  failure gracefully, resulting in the backend hiccupping.
 *	  This points out problems in each (the frontend shouldn't even
 *	  try to do SSL if pqsecure_initialize() fails, and the backend
 *	  shouldn't crash/recover if an SSH negotiation fails.  The
 *	  backend definitely needs to be fixed, to prevent a "denial
 *	  of service" attack, but I don't know enough about how the
 *	  backend works (especially that pre-SSL negotiation) to identify
 *	  a fix.
 *
 *	  ...
 *
 *	  Unlike the server's static private key, the client's
 *	  static private key ($HOME/.postgresql/postgresql.key)
 *	  should normally be stored encrypted.	However we still
 *	  support EPH since it's useful for other reasons.
 *
 *	  ...
 *
 *	  Client certificates are supported, if the server requests
 *	  or requires them.  Client certificates can be used for
 *	  authentication, to prevent sessions from being hijacked,
 *	  or to allow "road warriors" to access the database while
 *	  keeping it closed to everyone else.
 *
 *	  The user's certificate and private key are located in
 *		$HOME/.postgresql/postgresql.crt
 *	  and
 *		$HOME/.postgresql/postgresql.key
 *	  respectively.
 *
 *	  ...
 *
 *	  We don't provide informational callbacks here (like
 *	  info_cb() in be-secure.c), since there's mechanism to
 *	  display that information to the client.
 *
 * OS DEPENDENCIES
 *	  The code currently assumes a POSIX password entry.  How should
 *	  Windows and Mac users be handled?
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "libpq-mini.h"
#include "libpq-mini-secure.h"

#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

/* #include "libpq-fe.h" */

/* #include "libpq-int.h" */

/* #include "fe-auth.h" */

/* #include "pqsignal.h" */

#include "executor/spi.h"

#ifdef WIN32
#include "win32.h"
#else
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#include <arpa/inet.h>
#endif

#ifndef HAVE_STRDUP
#include "strdup.h"
#endif

#ifndef WIN32
#include <pwd.h>
#endif
#include <sys/stat.h>

#include <signal.h>

/* ------------------------------------------------------------ */

/*			 Procedures common to all secure sessions
 */

/* ------------------------------------------------------------ */

/*
 *	Read data from a secure connection.
 */
ssize_t
pqmsecure_read(PGconn_min *conn, void *ptr, size_t len) {
    ssize_t n = 0;

    n = recv(conn->sock, ptr, len, 0);
    return n;
}

/*
 *	Write data to a secure connection.
 */
ssize_t
pqmsecure_write(PGconn_min *conn, const void *ptr, size_t len) {
    ssize_t n;

    n = send(conn->sock, ptr, len, 0);

    return n;
}

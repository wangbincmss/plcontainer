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


#ifndef LIBPQ_MIN_MISC_H
#define LIBPQ_MIN_MISC_H

#include "libpq-mini.h"

/* Send functions */

int pqmPutMsgStart(char, PGconn_min *conn);

int pqmPutInt(int, size_t, PGconn_min *conn);

int pqmPutInt(int, size_t, PGconn_min *conn);

int pqmPutMsgBytes(const void *buf, size_t len, PGconn_min *conn);

int pqmPutMsgEnd(PGconn_min *conn);

int pqmPutc(char c, PGconn_min *conn);

int pqmPuts(const char *s, PGconn_min *conn);

int pqmPutnchar(const char *s, size_t len, PGconn_min *conn);

int pqmFlush(PGconn_min *conn);

/* Receive functions */

int pqmGetSome(PGconn_min *conn);

int pqmReadData(PGconn_min *conn);

void pqmMessageRecvd(PGconn_min *conn);

int pqmGetc(char *result, PGconn_min *conn);

int pqmGetnchar(char *s, size_t len, PGconn_min *conn);

int pqmGetInt(int *result, size_t bytes, PGconn_min *conn);

#endif

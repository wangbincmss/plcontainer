#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gtest/gtest.h"

extern "C" {
#include "common/comm_connectivity.h"
#include "common/comm_channel.h"
#include "common/messages/messages.h"

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#include <R.h>
#include <Rembedded.h>
#include <Rinternals.h>
#include "../rclient/rcall.h"
extern void r_init();
extern char * last_R_error_msg;
SEXP convert_args(callreq req);

}


TEST(RTypes, CallRequest) {

    int fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    ASSERT_NE(ret, -1);
    plcConn *send, *recv;
    
    send = plcConnInit(fds[0]);
    // send->Pfdebug = stdout;
    recv = plcConnInit(fds[1]);
    //         // recv->Pfdebug = stdout;
    
r_init();
    pid_t pid = fork();
    if (pid == 0) {
        // this is the child
        callreq req = (callreq)malloc(sizeof(call_req));
        req->msgtype = MT_CALLREQ;
        req->proc.name = (char *)"foobar";
        req->proc.src = (char *)"function definition";

        req->nargs    = 9;
        req->args = (argument *)malloc(req->nargs*sizeof(argument));

        argument bool1, bool2, varchar, text, int2, int4, int8, float4, float8;

        bool1.name="arg1";
        bool1.type="bool";
        bool1.value="t";

        bool2.name="arg2";
        bool2.type="bool";
        bool2.value="f";

        varchar.name="arg3";
        varchar.type="varchar";
        varchar.value="first";

        text.name="arg4";
        text.type="text";
        text.value="second";

        int2.name="arg5";
        int2.type="int2";
        int2.value="10";

        int4.name="arg6";
        int4.type="int4";
        int4.value="100";

        int8.name="arg7";
        int8.type="int8";
        int8.value="1000";

        float4.name="arg8";
        float4.type="float4";
        float4.value="16.8";

        float8.name="arg9";
        float8.type="float8";
        float8.value="100.2";

        memcpy( (void *)&req->args[0], (const void *)&bool1, sizeof(argument) );
        memcpy( (void *)&req->args[1], (const void *)&bool2, sizeof(argument) );

        memcpy( (void *)&req->args[2], (const void *)&varchar, sizeof(argument) );
        memcpy( (void *)&req->args[3], (const void *)&text, sizeof(argument) );

        memcpy( (void *)&req->args[4], (const void *)&int2, sizeof(argument) );
        memcpy( (void *)&req->args[5], (const void *)&int4, sizeof(argument) );

        memcpy( (void *)&req->args[6], (const void *)&int8, sizeof(argument) );
        memcpy( (void *)&req->args[7], (const void *)&float4, sizeof(argument) );

        memcpy( (void *)&req->args[8], (const void *)&float8, sizeof(argument) );

        plcontainer_channel_send(send, (message)req );

        free(req->args);
        free(req);

        exit(0);
    }

    message msg;
    plcontainer_channel_receive(recv,&msg);
    ASSERT_EQ(MT_CALLREQ, (char)msg->msgtype);
    callreq req = (callreq)msg;
    ASSERT_STREQ(req->proc.name, "foobar");
    ASSERT_STREQ(req->proc.src, "function definition");


    SEXP args,element;
    int i;
    PROTECT( args = convert_args((callreq )req) );


    i = length(args);
    printf("array is %d elements \n", i);

    ASSERT_EQ(LOGICAL(VECTOR_ELT(args,0))[0],1);
    ASSERT_EQ(LOGICAL(VECTOR_ELT(args,1))[0],0);


    ASSERT_EQ(INTEGER(VECTOR_ELT(args,4))[0],10);
    ASSERT_EQ(INTEGER(VECTOR_ELT(args,5))[0],100);
    ASSERT_EQ(REAL(VECTOR_ELT(args,6))[0],1000);

    ASSERT_EQ(REAL(VECTOR_ELT(args,7))[0],16.8);
    ASSERT_EQ(REAL(VECTOR_ELT(args,8))[0],100.2);

    // wait for child to exit
    int status;
    ret = waitpid(pid, &status, 0);
    ASSERT_NE(ret, -1);
    ASSERT_EQ(status, 0);
}

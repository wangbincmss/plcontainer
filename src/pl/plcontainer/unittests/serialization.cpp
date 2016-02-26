#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gtest/gtest.h"

extern "C" {
#include "common/comm_connectivity.h"
#include "common/comm_channel.h"
#include "common/messages/messages.h"
}
#define TEST_SIZE 1024

TEST(MessageSerialization, CallRequest) {
    int fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    ASSERT_NE(ret, -1);
    plcConn *send, *recv;
    send = plcConnInit(fds[0]);
    // send->Pfdebug = stdout;
    recv = plcConnInit(fds[1]);
    // recv->Pfdebug = stdout;
    pid_t pid = fork();
    if (pid == 0) {

    	char *value1 = (char *)malloc(TEST_SIZE);
		int i;
		for (i=0; i< TEST_SIZE-1;i++){
			value1[i]= 'A' + random() % 26;
		}
		value1[TEST_SIZE-1]='\0';

        // this is the child
		callreq req = (callreq)malloc(sizeof( call_req));
		req->msgtype = MT_CALLREQ;
		req->proc.name="foobar";
		req->proc.src = "function definition";
		req->nargs = 3;
		req->args = (argument *)malloc(3*sizeof(argument));

		req->args[0].name = "arg1";
		req->args[0].type="text";
		req->args[0].value=value1;

		req->args[1].name="arg2";
		req->args[1].type="text";
		req->args[1].value="hello";

		req->args[2].name="arg3";
		req->args[2].type="float";
		req->args[2].value="12.5";
        plcontainer_channel_send(send, (message)req);
        exit(0);
    }

    // this is the child
    message msg;
    plcontainer_channel_receive(recv, &msg);
    ASSERT_EQ(MT_CALLREQ, (char)msg->msgtype);
    callreq req = (callreq)msg;
    ASSERT_STREQ(req->proc.name, "foobar");
	ASSERT_STREQ(req->proc.src, "function definition");

	ASSERT_STREQ(req->args[0].name, "arg1");
	ASSERT_STREQ(req->args[0].type, "text");
	ASSERT_EQ(strlen(req->args[0].value), TEST_SIZE-1);

	ASSERT_STREQ(req->args[1].name, "arg2");
	ASSERT_STREQ(req->args[1].type, "text");
	ASSERT_STREQ(req->args[1].value, "hello");

	ASSERT_STREQ(req->args[2].name, "arg3");
	ASSERT_STREQ(req->args[2].type, "float");
	ASSERT_STREQ(req->args[2].value, "12.5");

    // wait for child to exit
    int status;
    ret = waitpid(pid, &status, 0);
    ASSERT_NE(ret, -1);
    ASSERT_EQ(status, 0);
}

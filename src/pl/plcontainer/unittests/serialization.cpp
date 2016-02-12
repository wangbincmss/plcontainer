#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gtest/gtest.h"

extern "C" {
#include "common/libpq-mini.h"
#include "common/comm_channel.h"
#include "common/messages/messages.h"
}

TEST(MessageSerialization, CallRequest) {
    int fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    ASSERT_NE(ret, -1);
    PGconn_min *send, *recv;
    send = pq_min_connect_fd(fds[0]);
    // send->Pfdebug = stdout;
    recv = pq_min_connect_fd(fds[1]);
    // recv->Pfdebug = stdout;
    pid_t pid = fork();
    if (pid == 0) {
        plpgj_channel_initialize(send);
        // this is the child
        callreq req = (callreq)malloc(sizeof(callreq));
        req->msgtype = MT_CALLREQ;
        req->proc.name = "foobar";
        req->proc.src = "function definition";
        plpgj_channel_send((message)req);
        exit(0);
    }

    // this is the parent
    plpgj_channel_initialize(recv);
    // this is the child
    message msg = plpgj_channel_receive();
    ASSERT_EQ(MT_CALLREQ, (char)msg->msgtype);
    callreq req = (callreq)msg;
    ASSERT_STREQ(req->proc.name, "foobar");
    ASSERT_STREQ(req->proc.src, "function definition");
    // wait for child to exit
    int status;
    ret = waitpid(pid, &status, 0);
    ASSERT_NE(ret, -1);
    ASSERT_EQ(status, 0);
}

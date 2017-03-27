/*------------------------------------------------------------------------------
/*
/*
/* Copyright (c) 2016, Pivotal.
/*
/*------------------------------------------------------------------------------
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

#include <Python.h>
}


TEST(PythonTypes, CallRequest) {
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

        plcontainer_channel_send(send, (message)req);

        free(req->args);
        free(req);

        exit(0);
    }

    message msg;
    plcontainer_channel_receive(recv, &msg);
    ASSERT_EQ(MT_CALLREQ, (char)msg->msgtype);
    callreq req = (callreq)msg;
    ASSERT_STREQ(req->proc.name, "foobar");
    ASSERT_STREQ(req->proc.src, "function definition");

    /* create the argument list */
    PyObject *args = PyTuple_New(req->nargs);
    PyObject *arg;

    int i;
    for (i = 0; i < req->nargs; i++) {

       if ( strcmp(req->args[i].type, "bool") == 0 ) {

           if ( strcmp(req->args[i].value,"t") == 0 )  {
               arg = Py_True;
           }else{
               arg = Py_False;
           }
       } else if (strcmp(req->args[i].type, "varchar") == 0) {
           arg = PyString_FromString((char *)req->args[i].value);
           char *str = PyString_AsString(arg);
           ASSERT_STREQ(str,"first");
       } else if (strcmp(req->args[i].type, "text") == 0) {
           arg = PyString_FromString((char *)req->args[i].value);
           char *str = PyString_AsString(arg);
           ASSERT_STREQ(str,"second");
       } else if (strcmp(req->args[i].type, "int2") == 0) {
           arg = PyLong_FromString((char *)req->args[i].value, NULL, 0);
           int overflow;
           long val = PyLong_AsLongAndOverflow(arg,&overflow);
           ASSERT_EQ(val,10);
       } else if (strcmp(req->args[i].type, "int4") == 0) {
           arg = PyLong_FromString((char *)req->args[i].value, NULL, 0);
           int overflow;
           long val = PyLong_AsLongAndOverflow(arg,&overflow);
           ASSERT_EQ(val,100);
       } else if (strcmp(req->args[i].type, "int8") == 0) {
           arg = PyLong_FromString((char *)req->args[i].value, NULL, 0);
           int overflow;
           long val = PyLong_AsLongAndOverflow(arg,&overflow);
           ASSERT_EQ(val,1000);
       } else if (strcmp(req->args[i].type, "float4") == 0) {
           arg = PyFloat_FromString(PyString_FromString((char *)req->args[i].value), NULL);
           double val = PyFloat_AsDouble(arg);
           ASSERT_EQ(val,16.8);
       } else if (strcmp(req->args[i].type, "float8") == 0) {
           arg = PyFloat_FromString(PyString_FromString((char *)req->args[i].value), NULL);
           double val = PyFloat_AsDouble(arg);
           ASSERT_EQ(val,100.2);
       }
    }


    // wait for child to exit
    int status;
    ret = waitpid(pid, &status, 0);
    ASSERT_NE(ret, -1);
    ASSERT_EQ(status, 0);
}

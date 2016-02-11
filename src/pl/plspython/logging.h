#ifndef PLSPYTHONLOGGING_H
#define PLSPYTHONLOGGING_H

#define PLSPYTHON_DEBUG

#ifdef PLSPYTHON_DEBUG
    #define plelog(x) elog(DEBUG1, x)
#else
    #define plelog(x)
#endif

#endif // MSGHANDLER_H

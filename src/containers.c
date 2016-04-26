#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "postgres.h"
#include "utils/ps_status.h"

#include "common/comm_utils.h"
#include "common/comm_channel.h"
#include "common/messages/messages.h"
#include "plc_configuration.h"
#include "containers.h"

typedef struct {
    char    *name;
    plcConn *conn;
} container_t;

#define CONTAINER_NUMBER 10
static int containers_init = 0;
static container_t *containers;

static void insert_container(char *image, plcConn *conn);
static void init_containers();
static inline bool is_whitespace (const char c);

#ifndef CONTAINER_DEBUG

static char *get_memory_option(plcContainer *cont);
static char *get_sharing_options(plcContainer *cont);
static char *shell(const char *cmd);
static void cleanup(char *dockerid);

static char *shell(const char *cmd) {
    FILE* fCmd;
    int ret;
    char* data;

    fCmd = popen(cmd, "r");
    if (fCmd == NULL) {
        lprintf(FATAL, "Cannot execute command '%s', error is: %s",
                       cmd, strerror(errno));
    }

    data = pmalloc(1024);
    if (data == NULL) {
        lprintf(ERROR, "Cannot allocate command buffer '%s': %s",
                       cmd, strerror(errno));
    }

    if (fgets(data, 1024, fCmd) == NULL) {
        lprintf(ERROR, "Cannot read output of the command '%s': %s",
                       cmd, strerror(errno));
    }

    ret = pclose(fCmd);
    if (ret < 0) {
        lprintf(FATAL, "Cannot close the command file descriptor '%s': %s",
                       cmd, strerror(errno));
    }

    return data;
}

static char *get_memory_option(plcContainer *cont) {
    char *res = pmalloc(30);
    if (cont->memoryMb > 0) {
        sprintf(res, "-m %dm", cont->memoryMb);
    } else {
        res[0] = '\0';
    }
    return res;
}

static char *get_sharing_options(plcContainer *cont) {
    char *res = NULL;

    if (cont->nSharedDirs > 0) {
        char **volumes = NULL;
        int totallen = 0;
        char *pos;
        int i;

        volumes = pmalloc(cont->nSharedDirs * sizeof(char*));
        for (i = 0; i < cont->nSharedDirs; i++) {
            volumes[i] = pmalloc(10 + strlen(cont->sharedDirs[i].host) +
                                 strlen(cont->sharedDirs[i].container));
            if (cont->sharedDirs[i].mode == PLC_ACCESS_READONLY) {
                sprintf(volumes[i], "-v %s:%s:ro", cont->sharedDirs[i].host,
                        cont->sharedDirs[i].container);
            } else if (cont->sharedDirs[i].mode == PLC_ACCESS_READWRITE) {
                sprintf(volumes[i], "-v %s:%s:rw", cont->sharedDirs[i].host,
                        cont->sharedDirs[i].container);
            } else {
                elog(ERROR, "Cannot determine directory sharing mode");
            }
            totallen += strlen(volumes[i]);
        }

        res = pmalloc(totallen + 2*cont->nSharedDirs);
        pos = res;
        for (i = 0; i < cont->nSharedDirs; i++) {
            memcpy(pos, volumes[i], strlen(volumes[i]));
            pos += strlen(volumes[i]);
            *pos = ' ';
            pos += 1;
            pfree(volumes[i]);
        }
        *pos = '\0';
        pfree(volumes);
    } else {
        res = pmalloc(1);
        res[0] = '\0';
    }
    return res;
}

static void cleanup(char *dockerid) {
    pid_t pid;

    /* We fork the process to syncronously wait for container to exit */
    pid = fork();
    if (pid == 0) {
        char psname[200];
        char cmd[1000];

        /* Setting application name to let the system know it is us */
        sprintf(psname, "plcontainer cleaner %s", dockerid);
        set_ps_display(psname, false);

        /* Wait for container to exit */
        sprintf(cmd, "docker wait %s", dockerid);
        system(cmd);

        /* Remove the container leftover filesystem data */
        sprintf(cmd, "docker rm %s", dockerid);
        system(cmd);

        _exit(0);
    }
}

#endif /* not CONTAINER_DEBUG */

static void insert_container(char *image, plcConn *conn) {
    size_t i;
    for (i = 0; i < CONTAINER_NUMBER; i++) {
        if (containers[i].name == NULL) {
            containers[i].name = plc_top_strdup(image);
            containers[i].conn = conn;
            return;
        }
    }
}

static void init_containers() {
    containers = (container_t*)plc_top_alloc(CONTAINER_NUMBER * sizeof(container_t));
    memset(containers, 0, CONTAINER_NUMBER * sizeof(container_t));
    containers_init = 1;
}

plcConn *find_container(const char *image) {
    size_t i;
    if (containers_init == 0)
        init_containers();
    for (i = 0; i < CONTAINER_NUMBER; i++) {
        if (containers[i].name != NULL &&
            strcmp(containers[i].name, image) == 0) {
            return containers[i].conn;
        }
    }

    return NULL;
}

plcConn *start_container(plcContainer *cont) {
    int port;
    unsigned int sleepus = 25000;
    unsigned int sleepms = 0;
    ping_message mping = NULL;

#ifdef CONTAINER_DEBUG

    static plcConn *conn;
    if (conn != NULL) {
        return conn;
    }
    port = 8080;

#else

    plcConn *conn;

    char  *cmd;
    char *dockerid, *ports, *exposed;
    int   cnt;
    int   len;

    char *mem   = get_memory_option(cont);
    char *share = get_sharing_options(cont);

    len = 100 + strlen(mem) + strlen(share) + strlen(cont->name);
    cmd = pmalloc(len);
    cnt = snprintf(cmd, len, "docker run -d -P %s %s %s", mem, share, cont->name);
    if (cnt < 0 || cnt >= len) {
        lprintf(FATAL, "docker image name is too long");
    }
    pfree(mem);
    pfree(share);
    /*
      output:
      $ sudo docker run -d -P plcontainer
      bd1a714ac07cf31b15b26697a65e2405d993696a6dd8ad08a06210d3bc47c942
     */

    elog(DEBUG1, "Calling following command: '%s'", cmd);
    dockerid = shell(cmd);
    cnt      = snprintf(cmd, len, "docker port %s", dockerid);
    if (cnt < 0 || cnt >= len) {
        lprintf(FATAL, "docker image name is too long");
    }
    elog(DEBUG1, "Calling following command: '%s'", cmd);
    ports = shell(cmd);
    /*
       output:
       $ sudo docker port dockerid
       8080/tcp -> 0.0.0.0:32777
    */
    exposed = strstr(ports, ":");
    if (exposed == NULL) {
        lprintf(FATAL, "cannot find port in %s", ports);
    }
    port = atoi(exposed + 1);

    /* Create a process to clean up the container after it finishes */
    cleanup(dockerid);

    pfree(cmd);
    pfree(ports);
    pfree(dockerid);

#endif // CONTAINER_DEBUG

    /* Making a series of connection attempts unless connection timeout of
     * CONTAINER_CONNECT_TIMEOUT_MS is reached. Exponential backoff for
     * reconnecting first attempts: 25ms, 50ms, 100ms, 200ms, 200ms, etc.
     */
    mping = palloc(sizeof(str_ping_message));
    mping->msgtype = MT_PING;
    while (sleepms < CONTAINER_CONNECT_TIMEOUT_MS) {
        int          res = 0;
        message      mresp = NULL;

        conn = plcConnect(port);
        if (conn != NULL) {
            res = plcontainer_channel_send(conn, (message)mping);
            if (res == 0) {
                res = plcontainer_channel_receive(conn, &mresp);
                if (mresp != NULL)
                    pfree(mresp);
                if (res == 0)
                    break;
            }
            plcDisconnect(conn);
        }

        usleep(sleepus);
        elog(DEBUG1, "Waiting for %u ms for before reconnecting", sleepus/1000);
        sleepms += sleepus / 1000;
        sleepus = sleepus >= 200000 ? 200000 : sleepus * 2;
    }

    if (sleepms >= CONTAINER_CONNECT_TIMEOUT_MS) {
        elog(ERROR, "Cannot connect to the container, %d ms timeout reached",
                    CONTAINER_CONNECT_TIMEOUT_MS);
        conn = NULL;
    } else {
        insert_container(cont->name, conn);
    }

    return conn;
}

static inline bool is_whitespace (const char c) {
    return (c == ' ' || c == '\n' || c == '\t' || c == '\r');
}

char *parse_container_meta(const char *source) {
    int first, last, len;
    char *name = NULL;
    int nameptr = 0;

    first = 0;
    len = strlen(source);

    /* Ignore whitespaces in the beginning of the function declaration */
    while (first < len && is_whitespace(source[first]))
        first++;

    /* Read everything up to the first newline or end of string */
    last = first;
    while (last < len && source[last] != '\n' && source[last] != '\r')
        last++;

    /* If the string is too small or not starting with hash - no declaration */
    if (last - first < 12 || source[first] != '#') {
        lprintf(ERROR, "No container declaration found");
        return name;
    }

    /* Ignore whitespaces after the hash sign */
    first++;
    while (first < len && is_whitespace(source[first]))
        first++;

    /* Line should be "# container :", fail if not so */
    if (strncmp(&source[first], "container", 9) != 0) {
        lprintf(ERROR, "Container declaration should start with '#container:'");
        return name;
    }

    /* Follow the line up to colon sign */
    while (first < last && source[first] != ':')
        first++;
    first++;

    /* If no colon found - bad declaration */
    if (first >= last) {
        lprintf(ERROR, "No colon found in container declaration");
        return name;
    }

    /*
     * Allocate container name variable and copy container name
     * ignoring whitespaces, i.e. container name cannot contain whitespaces
     */
    name = (char*)pmalloc(last-first);
    while (first < last && source[first] != ':') {
        if (!is_whitespace(source[first])) {
            name[nameptr] = source[first];
            nameptr++;
        }
        first++;
    }

    /* Name cannot be empty */
    if (nameptr == 0) {
        lprintf(ERROR, "Container name cannot be empty");
        pfree(name);
        return NULL;
    }
    name[nameptr] = '\0';

    return name;
}

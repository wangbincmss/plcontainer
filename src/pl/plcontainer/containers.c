#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/comm_utils.h"
#include "containers.h"

typedef struct {
    char    *name;
    plcConn *conn;
} container_t;

#define CONTAINER_NUMBER 10
static int containers_init = 0;
static container_t *containers;

static inline bool is_whitespace (const char c);

#ifndef CONTAINER_DEBUG

static char *
shell(const char *cmd) {
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

#endif

static void
insert_container(const char *image, plcConn *conn) {
    size_t i;
    for (i = 0; i < CONTAINER_NUMBER; i++) {
        if (containers[i].name == NULL) {
            int len = strlen(image);
            containers[i].name = plc_top_alloc(len+1);
            memcpy(containers[i].name, image, len+1);
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

plcConn *
find_container(const char *image) {
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

plcConn *
start_container(const char *image, int shared) {
    int port;

#ifdef CONTAINER_DEBUG

    static plcConn *conn;
    if (conn != NULL) {
        return conn;
    }
    port = 8080;

#else

    plcConn *conn;

    char  cmd[300];
    char *dockerid, *ports, *exposed;
    int   cnt;

    cnt = snprintf(cmd, sizeof(cmd), "docker run -d -P %s", image);
    if (cnt < 0 || cnt >= (int)sizeof(cmd)) {
        lprintf(FATAL, "docker image name is too long");
    }
    /*
      output:
      $ sudo docker run -d -P plcontainer
      bd1a714ac07cf31b15b26697a65e2405d993696a6dd8ad08a06210d3bc47c942
     */

    dockerid = shell(cmd);
    cnt      = snprintf(cmd, sizeof(cmd), "docker port %s", dockerid);
    if (cnt < 0 || cnt >= (int)sizeof(cmd)) {
        lprintf(FATAL, "docker image name is too long");
    }
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
    pfree(ports);
    pfree(dockerid);

#endif // CONTAINER_DEBUG

    conn = plcConnect(port);
    insert_container(image, conn);
    return conn;
}

static inline bool is_whitespace (const char c) {
    return (c == ' ' || c == '\n' || c == '\t' || c == '\r');
}

char *
parse_container_meta(const char *source, int *shared) {
    int first, last, len;
    char *name = NULL;
    int nameptr = 0;
    *shared = 0;

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

    /* Searching for container sharing declaration */
    if (first < last && source[first] == ':') {
        /* Scroll to first non-whitespace */
        first++;
        while (first < last && is_whitespace(source[first]))
            first++;
        if (first == last) {
            lprintf(ERROR, "Container sharing mode declaration is empty");
            pfree(name);
            return NULL;
        }
        /* The only supported mode is "shared" */
        if (last - first < 6 || strncmp(&source[first],"shared", 6) != 0) {
            lprintf(ERROR, "Container sharing mode declaration can be only 'shared'");
            pfree(name);
            return NULL;
        }
        *shared = 1;
    }
    return name;
}

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/comm_logging.h"
#include "containers.h"

typedef struct {
    const char *name;
    plcConn    *conn;
} container_t;

static container_t containers[10];

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

    data = palloc(1024);
    if (data == NULL) {
        lprintf(ERROR, "Cannot allocate command buffer: %s", strerror(errno));
    }

    if (fgets(data, 1024, fCmd) == NULL) {
        lprintf(ERROR, "Cannot read output of the command: %s", strerror(errno));
    }

    ret = pclose(fCmd);
    if (ret < 0) {
        lprintf(FATAL, "Cannot close the command file descriptor: %s",
                       strerror(errno));
    }

    return data;
}

#endif

static void
insert_container(const char *image, plcConn *conn) {
    size_t i;
    for (i = 0; i < sizeof(containers) / sizeof(*containers); i++) {
        if (containers[i].name == NULL) {
            containers[i].name = strdup(image);
            containers[i].conn = conn;
            return;
        }
    }
}

plcConn *
find_container(const char *image) {
    size_t i;
    for (i = 0; i < sizeof(containers) / sizeof(*containers); i++) {
        if (containers[i].name != NULL &&
            strcmp(containers[i].name, image) == 0) {
            return containers[i].conn;
        }
    }

    return NULL;
}

plcConn *
start_container(const char *image) {
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

char *
parse_container_name(const char *source) {
    char *newline, *dup, *image, *name;
    int   len, first, last;

    len = strlen(source+1);
    dup = palloc(len);

    if (dup == NULL) {
        lprintf(FATAL, "cannot allocate memory");
    }

    dup = strncpy(dup, source, len+1);

    if (strlen(dup) == 0) {
        lprintf(ERROR, "empty string received");
    }

    /*
       if '#' isn't the first or second character after newline then error out,
       e.g.:

       CREATE FUNCTION stupid() RETURNS text AS $$    <--- newline here
       # container: plc_python
       return "zarkon"
       $$ LANGUAGE plcontainer;

       # without the initial newline
       CREATE FUNCTION stupid() RETURNS text AS $$ # container: plc_python
       return "zarkon"
       $$ LANGUAGE plcontainer;
    */
    if (!(dup[0] == '#' || (dup[0] == '\n' && dup[1] == '#'))) {
        lprintf(ERROR, "no container declaration");
    }

    newline = strchr(dup + 1, '\n');
    if (newline == NULL) {
        lprintf(ERROR, "no new line found!");
    }

    /* search for the colon in the first line only */
    newline[0] = '\0';
    image      = strrchr(dup, ':');
    if (image == NULL) {
        lprintf(ERROR, "no colon found!");
    }

    /* skip the colon */
    image++;

    /* skip any whitespace, e.g. trim() */
    len   = strlen(image);
    first = 0, last = len - 1;
    while (first < len && isspace(image[first]))
        first++;
    while (last > first && isspace(image[last]))
        last--;
    image[last + 1] = '\0';
    image += first;

    len = strlen(image);

    /* error if the image name is empty */
    if (strlen(image) == 0) {
        lprintf(ERROR, "empty image name");
    }

    name = palloc(len+1);
    strncpy(name, image, len+1);
    pfree(dup);

    return name;
}

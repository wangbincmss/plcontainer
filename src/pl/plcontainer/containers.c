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
    PGconn_min *conn;
} container_t;

static container_t containers[10];
//#define CONTAINER_DEBUG 1
#ifndef CONTAINER_DEBUG

static char *
shell(const char *cmd) {
    int   ret, cmdexit, outfd, errfd, fds[2];
    char *data = malloc(1024);

    ret = pipe(fds);
    if (ret == -1) {
        lprintf(FATAL, "cannot call pipe: %s", strerror(errno));
    }

    if ((outfd = dup(1)) == -1) {
        lprintf(FATAL, "cannot dup stdout: %s", strerror(errno));
    }

    if ((errfd = dup(1)) == -1) {
        lprintf(FATAL, "cannot dup stderr: %s", strerror(errno));
    }

    if (dup2(fds[1], 1) == -1) {
        lprintf(FATAL, "cannot dup to stdout: %s", strerror(errno));
    }

    if (dup2(fds[1], 2) == -1) {
        lprintf(FATAL, "cannot dup to stderr: %s", strerror(errno));
    }

    cmdexit = system(cmd);

    if (close(fds[1]) == -1) {
        lprintf(ERROR, "cannot close fd: %s", strerror(errno));
    }

    ret = read(fds[0], data, 1024);
    if (ret == -1) {
        lprintf(ERROR, "cannot read from fd: %s", strerror(errno));
    }

    /*
       string should end with newline. TODO: this is little dangerous
       in case the output change. To be honest we should be using an
       api instead of the command line interface. This is just a POC
    */
    data[ret - 1] = '\0';

    if (dup2(outfd, 1) == -1) {
        lprintf(FATAL, "cannot dup: %s", strerror(errno));
    }

    if (dup2(outfd, 1) == -1) {
        lprintf(FATAL, "cannot dup: %s", strerror(errno));
    }

    if (cmdexit != 0) {
        lprintf(ERROR, "cannot start container: '%s', command '%s'", data, cmd);
    }
    return data;
}

#endif

static void
insert_container(const char *image, PGconn_min *conn) {
    size_t i;
    for (i = 0; i < sizeof(containers) / sizeof(*containers); i++) {
        if (containers[i].name == NULL) {
            containers[i].name = strdup(image);
            containers[i].conn = conn;
            return;
        }
    }
}

PGconn_min *
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

PGconn_min *
start_container(const char *image) {
    int port;
#ifdef CONTAINER_DEBUG
    static PGconn_min *conn;
    if (conn != NULL) {
        return conn;
    }
    port = 8080;
#else
    PGconn_min *conn;

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
    free(ports);
    free(dockerid);
#endif

    conn = pq_min_connect(port);
    insert_container(image, conn);
    return conn;
}

char *
parse_container_name(const char *source) {
    char *newline, *dup, *image;
    int   len, first, last;

    dup = strdup(source);
    if (dup == NULL) {
        lprintf(FATAL, "cannot allocate memory");
    }

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

    image = strdup(image);
    free(dup);

    /* error if the image name is empty */
    if (strlen(image) == 0) {
        lprintf(ERROR, "empty image name");
    }

    return image;
}

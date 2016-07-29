#ifndef PLC_CONTAINERS_H
#define PLC_CONTAINERS_H

#include "common/comm_connectivity.h"
#include "plc_configuration.h"

//#define CONTAINER_DEBUG
#define CONTAINER_CONNECT_TIMEOUT_MS 5000

/* given source code of the function, extract the container name */
char *parse_container_meta(const char *source);

/* return the port of a started container, -1 if the container isn't started */
plcConn *find_container(const char *image);

/* start a new docker container using the given image  */
plcConn *start_container(plcContainer *cont);

/* Function terminates all the container connections */
void stop_containers(void);

#endif /* PLC_CONTAINERS_H */

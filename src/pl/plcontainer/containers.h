#ifndef __CONTAINERS_H__
#define __CONTAINERS_H__

//#define CONTAINER_DEBUG

#include "common/comm_connectivity.h"

/*
   given source code of the function, extract the container name.

   source code should follow this convention:

   ```
   # container: plc_python
   return "python string"
   ```
*/
char *parse_container_meta(const char *source, int *shared);

/* return the port of a started container, -1 if the container isn't started */
plcConn *find_container(const char *image);

/* start a new docker container using the given image  */
plcConn *start_container(const char *image, int shared);

#endif /* __CONTAINERS_H__ */

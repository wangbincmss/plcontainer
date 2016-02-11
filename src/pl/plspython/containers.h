#ifndef __CONTAINERS_H__
#define __CONTAINERS_H__

#define CONTAINER_DEBUG

#include "common/libpq-mini.h"

/*
   given source code of the function, extract the container name.

   source code should follow this convention:

   ```
   # container: plspython
   return "python string"
   ```
*/
char *parse_container_name(const char *source);

/* return the port of a started container, -1 if the container isn't started */
PGconn_min *find_container(const char *image);

/* start a new docker container using the given image  */
PGconn_min *start_container(const char *image);

#endif /* __CONTAINERS_H__ */

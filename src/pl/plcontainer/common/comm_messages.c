#include <stdlib.h>

#include "messages/message_callreq.h"
#include "messages/message_result.h"

void
free_callreq(callreq req) {
    int i;

    /* free the procedure */
    free(req->proc.name);
    free(req->proc.src);

    /* free the arguments */
    for (i = 0; i < req->nargs; i++) {
        free(req->args[i].name);
        free(req->args[i].value);
        free(req->args[i].type);
    }
    free(req->args);

    /* free the top-level request */
    free(req);
}

void
free_result(plcontainer_result res) {
    int i;

    /* free the types array */
    for (i = 0; i < res->cols; i++) {
        free(res->types[i]);
        free(res->names[i]);
    }
    free(res->types);
    free(res->names);

    /* free the data array */
    for (i = 0; i < res->rows; i++) {
        /* free the row */
        free(res->data[i]);
    }
    free(res->data);

    free(res);
}

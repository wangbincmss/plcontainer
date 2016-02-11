
/**
 * file name:			plcontainer_channel.h
 * description:			Channel API.
 * author:			Laszlo Hornyak Kocka
 */

#ifndef PLCONTAINER_CHANNEL_H
#define PLCONTAINER_CHANNEL_H

#include "c.h"
#include "libpq-mini.h"
#include "messages/messages.h"

bool    plcontainer_channel_initialized(void);
void    plcontainer_channel_initialize(PGconn_min *);
void    plcontainer_channel_send(message);
message plcontainer_channel_receive(void);

#endif // PLCONTAINER_CHANNEL_H

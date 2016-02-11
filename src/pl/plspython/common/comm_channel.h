
/**
 * file name:			plspython_channel.h
 * description:			Channel API.
 * author:			Laszlo Hornyak Kocka
 */

#ifndef PLSPYTHON_CHANNEL_H
#define PLSPYTHON_CHANNEL_H

#include "c.h"
#include "libpq-mini.h"
#include "messages/messages.h"

bool    plspython_channel_initialized(void);
void    plspython_channel_initialize(PGconn_min *);
void    plspython_channel_send(message);
message plspython_channel_receive(void);

#endif

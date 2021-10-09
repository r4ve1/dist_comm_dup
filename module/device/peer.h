#ifndef DEVICE_PEER_H
#define DEVICE_PEER_H

#include "../network/peer.h"

#define EIFNOTFOUND 50000

int init_peer_major_device(void);
int register_peer_device(struct peer *peer);

#endif

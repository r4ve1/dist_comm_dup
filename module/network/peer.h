#ifndef NETWORK_PEER_H
#define NETWORK_PEER_H

#include <linux/ip.h>
#include <linux/in.h>
#include "../packet.h"

#define MAX_PEERS 256

#define EINVALIDPEERID 10000

typedef struct peer
{
  int id;
  char *name;
  unsigned int addr;
  struct interface **interfaces;
  int interface_count;
  struct device *dev;
} peer_t;

extern struct peer *peers;
extern int peer_count;

int get_peer(struct sockaddr_in *from_addr);
int add_peer(struct sockaddr_in *from_addr, char *packet, uint32_t packet_length);
int update_peer_interfaces(int id, char *packet, uint32_t packet_length);

int execute_remote_interface(int id, struct interface *interface);

#endif

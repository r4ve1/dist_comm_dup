#ifndef LIB_NETLINK_H
#define LIB_NETLINK_H
#include <dist_comm/interface.h>
#include <linux/netlink.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/socket.h>
/*
 * Generate a port_id for current thread
 */
uint32_t get_portid();

/*
 * Initialize a netlink client
 */
int init_netlink_client(uint32_t portid);
/*
 * send msg through netlink
 */
int netlink_sendto_kern(int sock_fd, uint32_t portid, uint8_t type, char *packet, uint32_t packet_length);
/*
 * recv msg from netlink
 */
int netlink_recvfrom_kern(int sock_fd, char *packet, uint32_t packet_length);

typedef struct netlink_header {
  uint8_t type;
} netlink_header_t;
#endif

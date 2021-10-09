#ifndef UTIL_SOCKET_H
#define UTIL_SOCKET_H

#include <linux/in.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/sock.h>
int sock_sendto(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
int sock_recvfrom(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);


#endif

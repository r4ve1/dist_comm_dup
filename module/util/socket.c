#include "socket.h"

#include <linux/netdevice.h>

int sock_sendto(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len) {
  struct msghdr msg;
  struct iovec iov;
  mm_segment_t oldfs;
  int size = 0;

  if (sock->sk == NULL)
    return 0;

  iov.iov_base = buf;
  iov.iov_len = len;

  msg.msg_flags = 0;
  msg.msg_name = addr;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  size = sock_sendmsg(sock, &msg, len);
  set_fs(oldfs);

  return size;
}

int sock_recvfrom(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len) {
  struct msghdr msg;
  struct iovec iov;
  mm_segment_t oldfs;
  int size = 0;

  if (sock->sk == NULL)
    return 0;

  iov.iov_base = buf;
  iov.iov_len = len;

  msg.msg_flags = 0;
  msg.msg_name = addr;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  size = sock_recvmsg(sock, &msg, len, msg.msg_flags);
  set_fs(oldfs);

  return size;
}

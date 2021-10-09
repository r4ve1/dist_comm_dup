#include <dist_comm/netlink.h>

uint32_t get_portid() {
  return (getpid() << 16) | (pthread_self() >> 16);
}

int init_netlink_client(uint32_t portid) {
  int sock_fd, ret;
  struct sockaddr_nl src_addr;
  // init a netlink client socket
  sock_fd = socket(PF_NETLINK, SOCK_RAW, INTERFACE_NETLINK);
  if (sock_fd < 0) {
    return -1;
  }
  // set src addr & dest addr
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = portid;
  src_addr.nl_groups = 0;
  ret = bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
  if (ret < 0) {
    return -1;
  }
  return sock_fd;
}
int netlink_sendto_kern(int sock_fd, uint32_t portid, uint8_t type, char *packet, uint32_t packet_length) {
  struct iovec iov;
  struct msghdr msg;
  struct sockaddr_nl dest_addr;
  struct nlmsghdr *nlh;
  netlink_header_t header;
  header.type = type;
  printf("Entering %s\n", __FUNCTION__);
  int ret, packet_len = NLMSG_SPACE(packet_length + sizeof(netlink_header_t));

  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0;
  dest_addr.nl_groups = 0;

  nlh = (struct nlmsghdr *)malloc(packet_len);
  memset(nlh, 0, packet_len);
  nlh->nlmsg_len = packet_len;
  nlh->nlmsg_pid = portid;
  nlh->nlmsg_flags = 0;
  // header||data
  memcpy(NLMSG_DATA(nlh), &header, sizeof(netlink_header_t));
  memcpy(NLMSG_DATA(nlh) + sizeof(netlink_header_t), packet, packet_length);
  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  printf("Sending message to kernel\n");
  ret = sendmsg(sock_fd, &msg, 0);
  if (ret < 0) {
    printf("sendmsg() error: %s\n", strerror(errno));
  }
  free(nlh);
  return ret;
}

int netlink_recvfrom_kern(int sock_fd, char *packet, uint32_t packet_length) {
  int ret;
  uint32_t payload_length;
  struct nlmsghdr buf[8192 / sizeof(struct nlmsghdr)];
  struct iovec iov = {buf, sizeof(buf)};
  struct sockaddr_nl sa;
  struct msghdr msg;
  struct nlmsghdr *nh;
  memset(&msg, 0, sizeof(struct msghdr));
  msg.msg_name = &sa;
  msg.msg_namelen = sizeof(sa);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  ret = recvmsg(sock_fd, &msg, 0);
  if (ret <= 0) {
    printf("error recvmsg(): %s\n", strerror(errno));
    return -1;
  }
  payload_length = buf->nlmsg_len - sizeof(struct nlmsghdr);
  if (payload_length > packet_length) {
    printf("buffer is too small");
    return -1;
  }
  memcpy(packet, NLMSG_DATA(buf), payload_length);
  return payload_length;
}

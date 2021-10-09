#include "peer.h"

#include <linux/slab.h>

#include "../device/peer.h"
#include "../util/socket.h"
#include "service.h"

struct peer *peers;
int peer_count = 0;

static void init_peers(void) {
  peers = (struct peer *)kzalloc(MAX_PEERS * sizeof(struct peer), GFP_KERNEL);
  peer_count = 0;
}

int get_peer(struct sockaddr_in *from_addr) {
  int i;
  if (peers == NULL)
    init_peers();
  for (i = 0; i < peer_count; ++i) {
    if (peers[i].addr == from_addr->sin_addr.s_addr)
      return i;
  }
  return -1;
}

int add_peer(struct sockaddr_in *from_addr, char *packet, uint32_t packet_length) {
  int err;
  if (peers == NULL)
    init_peers();
  if (peer_count >= MAX_PEERS)
    return -EINVALIDPEERID;
  if ((err = unserialize_interfaces_packet(packet, packet_length, &peers[peer_count].interfaces, &peers[peer_count].interface_count)) < 0)
    return err;
  peers[peer_count].id = peer_count;
  peers[peer_count].name = NULL;
  peers[peer_count].addr = from_addr->sin_addr.s_addr;
  peers[peer_count].interfaces = NULL;
  register_peer_device(&peers[peer_count]);
  return peer_count++;
}

int update_peer_interfaces(int id, char *packet, uint32_t packet_length) {
  int err, count;
  struct interface **interfaces;
  if (peers == NULL)
    return -1;
  if (id >= peer_count)
    return -EINVALIDPEERID;
  if ((err = unserialize_interfaces_packet(packet, packet_length, &interfaces, &count)) < 0)
    return err;
  free_interfaces(peers[id].interfaces, peers[id].interface_count);
  peers[id].interfaces = interfaces;
  peers[id].interface_count = count;
  return 0;
}
int init_socket(int peerid, struct socket **sock_p, struct sockaddr_in **addr_p) {
  int err = 0;
  struct socket *sock;
  struct sockaddr_in *addr;
  if (peerid < 0 || peerid >= peer_count) {
    printk(KERN_ERR "Invalid peer id %d\n", peerid);
    return -EINVALIDPEERID;
  }
  if ((err = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock)) < 0) {
    printk(KERN_ERR "Failed to create execution socket, error = %d\n", err);
    return err;
  }
  *sock_p = sock;
  addr = (struct sockaddr_in *)kzalloc(sizeof(struct sockaddr_in), GFP_KERNEL);
  addr->sin_addr.s_addr = peers[peerid].addr;
  addr->sin_family = AF_INET;
  addr->sin_port = htons(SERVICE_SERVER_PORT);
  /* Connect execution server */
  if ((err = sock->ops->connect(sock, (struct sockaddr *)addr, sizeof(struct sockaddr), 0)) < 0) {
    printk(KERN_ERR "Failed to connect remote service server, error = %d\n", err);
  }
  *addr_p = addr;
  return err;
}

interface_t *fetch_remote_interface_info(int peerid, interface_t *interface) {
  struct socket *sock;
  struct sockaddr_in *addr;
  arg_value_t *argv = NULL;
  uint32_t packet_length = 0;
  char *packet = NULL, *buffer = NULL;
  int err = 0;
  if ((err = init_socket(peerid, &sock, &addr)) < 0) {
    printk(KERN_ERR "Failed to create execution socket, error = %d\n", err);
    return NULL;
  }
  argv = (arg_value_t *)kzalloc(sizeof(arg_value_t), GFP_KERNEL);
  argv[0].length = sizeof(uint32_t);
  argv[0].addr = (char *)&interface->id;
  serialize_rpc_packet(0, 1, argv, &packet, &packet_length);
  if ((err = sock_sendto(sock, addr, packet, packet_length)) < 0) {
    printk(KERN_ERR "Failed to send execution packet, error = %d\n", err);
    goto end_execute_remote_interface;
  }
  // TODO[LXR]: recv from socket ant modify interface's field
  buffer = kzalloc(BUFFER_INC_SIZE, GFP_KERNEL);
  packet_length = sock_recvfrom(sock, addr, buffer, BUFFER_INC_SIZE);
  if (packet_length < 0) {
    printk(KERN_WARNING "Can't recv interface result\n");
  }
  unserialize_interface_info(buffer, packet_length, &interface->info);
// assert
end_execute_remote_interface:
  if (packet != NULL)
    kfree(packet);
  if (buffer != NULL)
    kfree(buffer);
  return NULL;
}

int execute_remote_interface(int peerid, struct interface *interface) {
  char *packet, *buffer;
  int err = 0, size;
  uint32_t packet_length = 0;
  struct socket *sock;
  struct sockaddr_in *addr;
  if ((err = init_socket(peerid, &sock, &addr)) < 0) {
    printk(KERN_ERR "Failed to create execution socket, error = %d\n", err);
    return err;
  }
  err = serialize_rpc_packet(interface->id, interface->argc, interface->argv, &packet, &packet_length);
  if ((err = sock_sendto(sock, addr, (unsigned char *)&packet_length, sizeof(packet_length))) < 0) {
    printk(KERN_ERR "Failed to send execution packet length, error = %d\n", err);
    goto end_execute_remote_interface;
  }
  if ((err = sock_sendto(sock, addr, packet, packet_length)) < 0) {
    printk(KERN_ERR "Failed to send execution packet, error = %d\n", err);
    goto end_execute_remote_interface;
  }
  // TODO[LXR]: recv from socket ant modify interface's field
  sock_recvfrom(sock, NULL, (unsigned char *)&size, 4);
  if (size <= 0) {
    printk(KERN_ERR "Invalid return packet size\n");
    err = -EINVALIDSIZE;
    goto end_execute_remote_interface;
  }
  buffer = (char *)kzalloc(size * sizeof(char), GFP_KERNEL);
  if (!buffer) {
    printk(KERN_ERR "Failed to alloc buffer, probably due to invalid packet size\n");
    err = -EINVALIDSIZE;
    goto end_execute_remote_interface;
  }
  packet_length = sock_recvfrom(sock, NULL, buffer, size);
  if (packet_length < 0) {
    printk(KERN_WARNING "Can't recv interface return packet\n");
  }
  unserialize_rpc_packet(buffer, packet_length, &interface->retv, &interface->retc);
end_execute_remote_interface:
  if (packet != NULL)
    kfree(packet);
  if (buffer != NULL)
    kfree(buffer);
  sock_release(sock);
  return err;
}

#include "broadcast.h"

#include <linux/delay.h>
#include <linux/inetdevice.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <net/sock.h>

#include "../interface.h"
#include "../packet.h"
#include "../util/socket.h"
#include "discovery.h"

struct task_struct *broadcast_loop_task;

int start_broadcast(void) {
  broadcast_loop_task = kthread_create(&broadcast_loop, NULL, "broadcast_loop");
  wake_up_process(broadcast_loop_task);
  return 0;
}

int do_broadcast(u32 dst_brd) {
  struct socket *broadcast_sock;
  struct sockaddr_in *broadcast_addr;
  int err, broadcast = 1;
  uint32_t packet_length;
  char *packet;
  /* Create socket */
  if ((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &broadcast_sock)) < 0) {
    printk(KERN_ERR "Failed to create broadcast UDP client socket, error = %d\n", err);
    return err;
  }
  /* Set broadcast */
  if ((err = sock_setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof broadcast)) < 0) {
    printk(KERN_ERR "Failed to set UDP client socket to broadcast, error = %d\n", err);
    return err;
  }
  /* Set dest addr to broadcast address and port to discovery server port */
  broadcast_addr = (struct sockaddr_in *)kzalloc(sizeof(struct sockaddr_in), GFP_KERNEL);
  broadcast_addr->sin_addr.s_addr = dst_brd;
  broadcast_addr->sin_family = AF_INET;
  broadcast_addr->sin_port = htons(DISCOVERY_SERVER_PORT);
  /* Connect discovery server */
  if ((err = broadcast_sock->ops->connect(broadcast_sock, (struct sockaddr *)broadcast_addr, sizeof(struct sockaddr), 0)) < 0) {
    sock_release(broadcast_sock);
    kfree((void *)broadcast_addr);
    printk(KERN_ERR "Failed to connect discovery server, error = %d\n", err);
    return err;
  }
  /* Send interfaces list */
  if (INTERFACE_LIST == NULL)
    init_interfaces();
  serialize_interfaces_packet(INTERFACE_LIST, INTERFACE_COUNT, &packet, &packet_length);
  sock_sendto(broadcast_sock, broadcast_addr, packet, packet_length);
  sock_release(broadcast_sock);
  kfree((void *)packet);
  kfree((void *)broadcast_addr);
  return 0;
}

int broadcast_loop(void *data) {
  struct net_device *dev;
  struct in_ifaddr *ifa;
  u32 broadcast[256];
  int i, count = 0;
  while (true) {
    msleep(BROADCAST_INTERVAL);
    count = 0;
    read_lock(&dev_base_lock);
    dev = first_net_device(&init_net);
    while (dev) {
      if (memcmp(dev->name, "lo", 2) != 0) {
        for (ifa = dev->ip_ptr->ifa_list; ifa; ifa = ifa->ifa_next) {
          broadcast[count] = ifa->ifa_broadcast;
          if (ifa->ifa_broadcast == 0)
            broadcast[count] = (ifa->ifa_address & ifa->ifa_mask) | (~ifa->ifa_mask);
          // printk(KERN_DEBUG "address: %pI4, mask: %pI4, broadcast: %pI4\n", &ifa->ifa_address, &ifa->ifa_mask, &broadcast[count]);
          ++count;
        }
      }
      dev = next_net_device(dev);
    }
    read_unlock(&dev_base_lock);
    for (i = 0; i < count; ++i)
      do_broadcast(broadcast[i]);
  }
  return 0;
}

void stop_broadcast(void) {
  if (broadcast_loop_task != NULL) {
    kthread_stop(broadcast_loop_task);
    broadcast_loop_task = NULL;
  }
}

#include "discovery.h"

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>

#include "../util/socket.h"
#include "peer.h"

struct socket *discovery_server_sock;
struct sockaddr_in *discovery_server_addr;
struct task_struct *discovery_server_loop_task;

int start_discovery_server(void) {
  int err;
  if ((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &discovery_server_sock)) < 0) {
    printk(KERN_ERR "Failed to create discovery server UDP server socket, error = %d\n", err);
    return err;
  }
  discovery_server_addr = (struct sockaddr_in *)kzalloc(sizeof(struct sockaddr_in), GFP_KERNEL);
  discovery_server_addr->sin_family = AF_INET;
  discovery_server_addr->sin_addr.s_addr = INADDR_ANY;
  discovery_server_addr->sin_port = htons(DISCOVERY_SERVER_PORT);
  if ((err = discovery_server_sock->ops->bind(discovery_server_sock, (struct sockaddr *)discovery_server_addr, sizeof(struct sockaddr))) < 0) {
    printk(KERN_ERR "Failed to bind discovery server, error = %d\n", err);
    return err;
  }
  discovery_server_loop_task = kthread_create(&discovery_server_loop, NULL, "discovery_server_loop");
  wake_up_process(discovery_server_loop_task);
  return 0;
}

int discovery_server_loop(void *data) {
  char *buffer;
  struct sockaddr_in from_addr;
  int size = 0, id, err;
  memset(&from_addr, 0, sizeof(struct sockaddr_in));
  buffer = (char *)kzalloc(MAX_BUFFER_SIZE * sizeof(char), GFP_KERNEL);
  printk(KERN_INFO "Discovery server listening on port %d\n", DISCOVERY_SERVER_PORT);
  while (true) {
    size = sock_recvfrom(discovery_server_sock, &from_addr, buffer, MAX_BUFFER_SIZE);
    // printk(KERN_DEBUG "Received data from %pI4, size %d\n", &from_addr.sin_addr.s_addr, size);
    /* Add or update peer */
    if ((id = get_peer(&from_addr)) < 0) {
      id = add_peer(&from_addr, buffer, size);
      if (id < 0) {
        printk(KERN_ERR "Failed to add peer for %pI4, error: %d\n", &from_addr.sin_addr.s_addr, id);
        continue;
      }
      printk(KERN_INFO "Added peer id %d for %pI4\n", id, &from_addr.sin_addr.s_addr);
    } else {
      err = update_peer_interfaces(id, buffer, size);
      if (err < 0) {
        printk(KERN_ERR "Failed to update peer id %d's interfaces, error: %d\n", id, err);
        continue;
      }
      // printk(KERN_DEBUG "Updated peer id %d's interfaces\n", id);
    }
  }
  return 0;
}

void stop_discovery_server(void) {
  if (discovery_server_loop_task != NULL) {
    kthread_stop(discovery_server_loop_task);
    discovery_server_loop_task = NULL;
  }
}

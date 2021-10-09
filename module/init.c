
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "device/manager.h"
#include "device/peer.h"
#include "interface.h"
#include "network/broadcast.h"
#include "network/discovery.h"
#include "network/service.h"
#include "proc.h"
int init_module(void) {
  int err;
  printk("Entering: %s\n", __FUNCTION__);
  if ((err = init_peer_major_device()) < 0) {
    printk("Failed to init peer major device\n");
    return -1;
  }
  if ((err = start_discovery_server()) < 0) {
    printk("Failed to start discovery server\n");
    return -1;
  }
  start_broadcast();
  if ((err = start_service_server()) < 0) {
    printk("Failed to start service server\n");
    return -1;
  }
  if ((err = init_interface_socket()) < 0) {
    printk("Failed to init interface netlink socket\n");
    return -1;
  } else {
    printk(KERN_INFO "Netlink initialized\n");
  }
  start_broadcast();
  if ((err = register_manager_device()) < 0) {
    printk("Failed to register manager device\n");
    return -1;
  }
  if ((err = init_procfs()) < 0) {
    printk("Failed to init proc fs");
    return -1;
  }
  return 0;
}

MODULE_LICENSE("GPL");

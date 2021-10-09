#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "device/manager.h"
#include "interface.h"
#include "network/broadcast.h"
#include "network/discovery.h"
#include "network/service.h"
#include "proc.h"
void cleanup_module(void) {
  stop_discovery_server();
  stop_broadcast();
  stop_service_server();
  unregister_manager_device();
  netlink_kernel_release(KERN_NETLINK);
  fint_procfs();
}

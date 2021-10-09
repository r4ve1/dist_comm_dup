#include "manager.h"

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "../interface.h"
#include "../network/discovery.h"
#include "../network/peer.h"

#define CHRDEV_NAME "dc_manager"
#define CLASS_NAME "dc_manager"
#define DEV_NAME "dcm"

#define CMD_MAGIC_NUM 100
#define HELLO_WORLD_CMD _IOR(CMD_MAGIC_NUM, 0, char *)
#define REG_INTERFACE_CMD _IOW(CMD_MAGIC_NUM, 1, char *)
#define REMOVE_INTERFACE_CMD _IOW(CMD_MAGIC_NUM, 3, int)
#define TEST_EXECUTE_REMOTE_CMD _IOW(CMD_MAGIC_NUM, 4, char *)

int manager_open(struct inode *inode, struct file *file) {
#ifdef DEBUG
  printk(KERN_INFO "manager_open(%p)\n", file);
#endif
  try_module_get(THIS_MODULE);
  return 0;
}

int manager_release(struct inode *inode, struct file *file) {
#ifdef DEBUG
  printk(KERN_INFO "manager_release(%p,%p)\n", inode, file);
#endif
  module_put(THIS_MODULE);
  return 0;
}

long manager_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
  int i, packet_len;
  char *packet;
  interface_t *interface = NULL, *_u_interface = NULL;
  switch (ioctl_num) {
    case REG_INTERFACE_CMD:
      return register_interface((char *)ioctl_param);
    case REMOVE_INTERFACE_CMD:
      return remove_interface((int)ioctl_param);
    case TEST_EXECUTE_REMOTE_CMD: {
      _u_interface = (interface_t *)ioctl_param;
      interface = alloc_interface(_u_interface->argc, _u_interface->retc, FALSE, TRUE, FALSE);
      if (!interface) {
        return 0;
      }
      interface->id = _u_interface->id;
      for (i = 0; i < _u_interface->argc; i++) {
        interface->argv[i].length = _u_interface->argv[i].length;
        interface->argv[i].addr = memdup_user(_u_interface->argv[i].addr, interface->argv[i].length);
      }
      serialize_rpc_packet(interface->id, interface->argc, interface->argv, &packet, &packet_len);
      execute_interface(packet, packet_len, NULL);
      return 0;
    }
  }

  return 0;
}

static int manager_major;
static struct class *manager_class;
static struct device *manager_device;
static struct cdev manager_cdev;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = manager_ioctl,
    .open = manager_open,
    .release = manager_release,
};

int register_manager_device(void) {
  int err;
  if ((err = alloc_chrdev_region(&manager_major, 0, 1, CHRDEV_NAME)) < 0) {
    printk(KERN_ERR "Failed to register character device, error = %d\n", err);
    return err;
  }
  manager_class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(manager_class)) {
    unregister_chrdev(manager_major, CHRDEV_NAME);
    printk(KERN_ERR "Failed to create dc_manager class\n");
    return -1;
  }
  manager_device = device_create(manager_class, NULL, manager_major, NULL, DEV_NAME);
  if (IS_ERR(manager_device)) {
    class_destroy(manager_class);
    unregister_chrdev_region(manager_major, 1);
    printk(KERN_ERR "Failed to create /dev/dcm\n");
    return -1;
  }
  cdev_init(&manager_cdev, &fops);
  if ((err = cdev_add(&manager_cdev, manager_major, 1)) < 0) {
    device_destroy(manager_class, manager_major);
    class_destroy(manager_class);
    unregister_chrdev_region(manager_major, 1);
    printk(KERN_ERR "Failed to add cdev\n");
    return -1;
  }
  printk(KERN_INFO "Successfully created /dev/dcm\n");
  return 0;
}

int unregister_manager_device(void) {
  cdev_del(&manager_cdev);
  device_destroy(manager_class, manager_major);
  class_destroy(manager_class);
  unregister_chrdev_region(manager_major, 1);
  return 0;
}

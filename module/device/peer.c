#include "peer.h"

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define CHRDEV_NAME "dc_peer"
#define CLASS_NAME "dc_peer"
#define DEV_NAME "dcp%d"

#define CMD_MAGIC_NUM 101
#define EXEC_INTERFACE_CMD _IOW(CMD_MAGIC_NUM, 1, struct interface *)

char peer_read_buffer[0x1000];

ssize_t peer_read(struct file *file, char __user *user_buf, size_t user_len, loff_t *off) {
  int i, id = MINOR(file->f_path.dentry->d_inode->i_rdev), len = 0, count, res;
  peer_t *_peer = peers + id;
  len = sprintf(peer_read_buffer,
                "Peer ID: %d\n"
                "Peer Address: %pI4\n"
                "Peer Name: %s\n"
                "Peer Interface Count: %d\n"
                "Peer Interface List:\n",
                _peer->id, &_peer->addr, _peer->name, _peer->interface_count);
  for (i = 0; i < _peer->interface_count; ++i)
    len += sprintf(peer_read_buffer + len, "Interface %d, %d args %d rets\n", _peer->interfaces[i]->id, _peer->interfaces[i]->argc, _peer->interfaces[i]->retc);
  if (*off >= len)
    return 0;
  count = user_len < (len - *off) ? user_len : (len - *off);
  res = copy_to_user(user_buf, peer_read_buffer, count);
  *off += len;
  return count;
}

int peer_open(struct inode *inode, struct file *file) {
#ifdef DEBUG
  printk(KERN_INFO "peer_open(%p)\n", file);
#endif
  try_module_get(THIS_MODULE);
  return 0;
}

int peer_release(struct inode *inode, struct file *file) {
#ifdef DEBUG
  printk(KERN_INFO "peer_release(%p,%p)\n", inode, file);
#endif
  module_put(THIS_MODULE);
  return 0;
}

long peer_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
  int i, id, ret;
  interface_t *_t_interface = NULL, *_u_interface = NULL, *interface = NULL;
  switch (ioctl_num) {
    case EXEC_INTERFACE_CMD:
      id = MINOR(file->f_path.dentry->d_inode->i_rdev);
      _u_interface = (struct interface *)ioctl_param;
      _t_interface = find_interface(peers[id].interfaces, peers[id].interface_count, _u_interface->id);
      if (!_t_interface) {
        printk(KERN_ALERT "Interface %d not found for peer %d\n", _u_interface->id, id);
        return -EIFNOTFOUND;
      }
      // check if argc and retc matches
      if (_t_interface->argc != _u_interface->argc || _t_interface->retc != _u_interface->retc) {
        printk(KERN_ALERT "Interface argc or retc not match\n");
        return -1;
      }
      if ((_u_interface->argc && !_u_interface->argv) || (_u_interface->retc && !_u_interface->retv)) {
        printk(KERN_ALERT "Interface argv or retv not initialized\n");
        return -1;
      }
      interface = (interface_t*)kzalloc(sizeof(interface_t), GFP_KERNEL);
      memcpy(interface, _t_interface, sizeof(interface_t));
      interface->argv = (arg_value_t*)kzalloc(sizeof(arg_value_t) * _t_interface->argc, GFP_KERNEL);
      for (i = 0; i < interface->argc; ++i) {
        interface->argv[i].length = _u_interface->argv[i].length;
        interface->argv[i].addr = (char*)memdup_user(_u_interface->argv[i].addr, sizeof(char)*_u_interface->argv[i].length);
      }
      execute_remote_interface(id, interface);
      for (i = 0; i < interface->retc; ++i) {
        _u_interface->retv[i].length = interface->retv[i].length;
        ret = copy_to_user(_u_interface->retv[i].addr, interface->retv[i].addr, sizeof(char)*interface->retv[i].length);
      }
      kfree(interface->argv);
      kfree(interface);
      // TODO: introduce GC
      return 0;
  }
  return 0;
}

static struct file_operations fops = {
    .read = peer_read,
    .open = peer_open,
    .release = peer_release,
    .unlocked_ioctl = peer_ioctl,
};

static int peer_major;
static struct class *peer_class;
static struct cdev peer_cdev;

int init_peer_major_device(void) {
  int err;
  if ((err = alloc_chrdev_region(&peer_major, 0, 100, CHRDEV_NAME)) < 0) {
    printk(KERN_ERR "Failed to register character device, error = %d\n", err);
    return err;
  }
  peer_class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(peer_class)) {
    unregister_chrdev(peer_major, CHRDEV_NAME);
    printk(KERN_ERR "Failed to create dc_manager class\n");
    return -1;
  }
  cdev_init(&peer_cdev, &fops);
  if ((err = cdev_add(&peer_cdev, peer_major, 100)) < 0) {
    device_destroy(peer_class, peer_major);
    printk(KERN_ERR "Failed to add cdev\n");
    return -1;
  }
  return 0;
}

int register_peer_device(struct peer *peer) {
  dev_t devNo = MKDEV(MAJOR(peer_major), peer->id);
  peer->dev = device_create(peer_class, NULL, devNo, NULL, DEV_NAME, peer->id);
  if (IS_ERR(peer->dev)) {
    printk(KERN_ERR "Failed to create /dev/dcp%d\n", peer->id);
    return -1;
  }
  printk(KERN_INFO "Successfully created /dev/dcp%d\n", peer->id);
  return 0;
}

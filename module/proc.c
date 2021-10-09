#include "proc.h"

#include <fs/proc/internal.h>
#include <linux/slab.h>

#include "packet.h"
static struct proc_dir_entry* procfs_home = NULL;
static struct file_operations ops = {
    .read = proc_read,
    .owner = THIS_MODULE,
};
int init_procfs() {
  procfs_home = proc_mkdir("dist_comm", NULL);
  if (procfs_home == NULL) {
    return -1;
  }
  return 0;
}
int fint_procfs() {
  uint32_t i;
  for (i = 0; i < INTERFACE_COUNT; i++) {
    if (INTERFACE_LIST[i]->ctrl->proc_entry) {
      proc_remove(INTERFACE_LIST[i]->ctrl->proc_entry);
    }
  }
  proc_remove(procfs_home);
  return 0;
}
/*
 * If interface registered by the same pid already exist, copy it's proc_entry field
 * otherwise register a new proc_entry
 */
int add_interface_to_proc(interface_t* income_interface) {
  int i;
  struct proc_dir_entry* new_entry;
  char tmp[0x30];
  for (i = 0; i < INTERFACE_COUNT; i++) {
    if (INTERFACE_LIST[i]->ctrl->bind_pid == income_interface->ctrl->bind_pid) {
      income_interface->ctrl->proc_entry = INTERFACE_LIST[i]->ctrl->proc_entry;
      return 0;
    }
  }
  // it's the first interface registered by the process
  sprintf(tmp, "%d", income_interface->ctrl->bind_pid);
  new_entry = proc_create(tmp, 0644, procfs_home, &ops);
  if (new_entry == NULL) {
    printk(KERN_ERR "Can't create new entry\n");
    return -1;
  }
  income_interface->ctrl->proc_entry = new_entry;
  return 0;
}

ssize_t proc_read(struct file* file, char __user* _u_buf, size_t count, loff_t* ppos) {
  struct proc_dir_entry* entry = PDE(file->f_inode);
  uint32_t i, buf_len = 0, ret, max_len = BUFFER_INC_SIZE;
  char* dest = (char*)kzalloc(max_len, GFP_KERNEL);
  interface_t* interface;
  if (*ppos > 0) {
    return 0;
  }
  for (i = 0; i < INTERFACE_COUNT; i++) {
    if (INTERFACE_LIST[i]->ctrl->proc_entry == entry) {
      interface = INTERFACE_LIST[i];
      format_interface(&dest, &buf_len, &max_len, interface);
    }
  }
  if (count < buf_len) {
    printk(KERN_WARNING "User buffer is too small\n");
    return -1;
  }
  ret = copy_to_user(_u_buf, dest, buf_len);
  if (ret) {
    return -EFAULT;
  }
  kfree(dest);
  *ppos = buf_len;
  return buf_len;
}

int format_interface(char** dest, uint32_t* length_p, uint32_t* max_length_p, interface_t* interface) {
  int i = 0;
  char tmp[0x500];
  char ARGUEMNTS[] = "\targuments:\n";
  char RETURN[] = "\treturn_values:\n";
  attr_info_t* attr_info;
  sprintf(tmp, "interface:\n\tid: %d\n\tname: %s\n\tdescription: %s\n", interface->id, interface->info->name, interface->info->description);
  copy_to_buffer(dest, length_p, max_length_p, tmp, strlen(tmp));
  copy_to_buffer(dest, length_p, max_length_p, ARGUEMNTS, strlen(ARGUEMNTS));
  for (i = 0; i < interface->argc; i++) {
    attr_info = &interface->info->arg_info[i];
    sprintf(tmp, "\t\targ%d:\n\t\t\tflag: %d\n\t\t\tname: %s \n\t\t\tdescription: %s\n", i, interface->arg_flags[i], attr_info->name, attr_info->description);
    copy_to_buffer(dest, length_p, max_length_p, tmp, strlen(tmp));
  }
  copy_to_buffer(dest, length_p, max_length_p, RETURN, strlen(RETURN));

  for (i = 0; i < interface->retc; i++) {
    attr_info = &interface->info->ret_info[i];
    sprintf(tmp, "\t\tret%d:\n\t\t\tflag: %d\n\t\t\tname: %s \n\t\t\tdescription: %s\n", i, interface->ret_flags[i], attr_info->name, attr_info->description);
    copy_to_buffer(dest, length_p, max_length_p, tmp, strlen(tmp));
  }
  return 0;
}

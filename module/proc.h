#ifndef PROC_H
#define PROC_H
#include <linux/proc_fs.h>

#include "interface.h"
#define PROCFS_HOME "dist_comm"
int init_procfs(void);
int fint_procfs(void);
int add_interface_to_proc(interface_t* income_interface);
ssize_t proc_read(struct file* file, char __user* _u_buf, size_t count, loff_t* ppos);
int format_interface(char** dest, uint32_t* length_p, uint32_t* max_length_p, interface_t* interface);
#endif

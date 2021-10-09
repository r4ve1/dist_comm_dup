#ifndef BROADCAST_DISCOVERY_H
#define BROADCAST_DISCOVERY_H

#include <linux/kernel.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#define BROADCAST_INTERVAL 1000 // in milliseconds

extern struct task_struct *broadcast_loop_task;

int start_broadcast(void);
int do_broadcast(u32 dst_brd);
int broadcast_loop(void *);
void stop_broadcast(void);

#endif

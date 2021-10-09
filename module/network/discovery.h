#ifndef NETWORK_DISCOVERY_H
#define NETWORK_DISCOVERY_H

#include <linux/ip.h>
#include <linux/sched.h>

#define DISCOVERY_SERVER_PORT 21233
#define MAX_BUFFER_SIZE 65536

extern struct socket *discovery_server_sock;
extern struct sockaddr_in *discovery_server_addr;
extern struct task_struct *discovery_server_loop_task;

int start_discovery_server(void);
void stop_discovery_server(void);
int discovery_server_loop(void *);

#endif

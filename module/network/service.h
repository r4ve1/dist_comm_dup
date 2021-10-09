#ifndef NETWORK_SERVICE_H
#define NETWORK_SERVICE_H

#include <linux/ip.h>
#include <linux/sched.h>

#define SERVICE_SERVER_PORT 21234

#define EINVALIDSIZE 30001

extern struct socket *service_server_sock;
extern struct sockaddr_in *service_server_addr;
extern struct task_struct *service_server_loop_task;
extern char *buffer;

int start_service_server(void);
void stop_service_server(void);
int service_server_loop(void *);

#endif

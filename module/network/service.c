#include "service.h"

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>

#include "../packet.h"
#include "../util/socket.h"
#include "peer.h"

#define MAX_BUFFER_SIZE 65536

struct socket *service_server_sock;
struct sockaddr_in *service_server_addr;
struct task_struct *service_server_loop_task;

int start_service_server(void) {
  int err;
  if ((err = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &service_server_sock)) < 0) {
    printk(KERN_ERR "Failed to create service server TCP server socket, error = %d\n", err);
    return err;
  }
  service_server_addr = (struct sockaddr_in *)kzalloc(sizeof(struct sockaddr_in), GFP_KERNEL);
  service_server_addr->sin_family = AF_INET;
  service_server_addr->sin_addr.s_addr = INADDR_ANY;
  service_server_addr->sin_port = htons(SERVICE_SERVER_PORT);
  if ((err = service_server_sock->ops->bind(service_server_sock, (struct sockaddr *)service_server_addr, sizeof(struct sockaddr))) < 0) {
    printk(KERN_ERR "Failed to bind service server, error = %d\n", err);
    return err;
  }
  service_server_loop_task = kthread_create(&service_server_loop, NULL, "service_server_loop");
  wake_up_process(service_server_loop_task);
  return 0;
}

int handle_connection(void *_sock) {
  struct socket *sock = (struct socket *)_sock;
  int size = 0, err = 0, length;
  char *buffer = NULL;
  struct sockaddr_in addr;
  length = sizeof(struct sockaddr);
  if ((err = sock->ops->getname(sock, (struct sockaddr *)&addr, &length, 2)) < 0) {
    printk(KERN_ERR "Failed to get connection address, error = %d\n", err);
    return err;
  }
  printk(KERN_INFO "Received service connection from %pI4\n", &addr.sin_addr.s_addr);
  sock_recvfrom(sock, NULL, (unsigned char *)&size, 4);
  if (size <= 0) {
    printk(KERN_ERR "Invalid packet size\n");
    err = -EINVALIDSIZE;
    goto end_handle_connection;
  }
  buffer = (char *)kzalloc(size * sizeof(char), GFP_KERNEL);
  if (!buffer) {
    printk(KERN_ERR "Failed to alloc buffer, probably due to invalid packet size\n");
    err = -EINVALIDSIZE;
    goto end_handle_connection;
  }
  length = sock_recvfrom(sock, NULL, buffer, size);
  execute_interface(buffer, length, sock);
end_handle_connection:
  if (buffer != NULL)
    kfree(buffer);
  return err;
}

int service_server_loop(void *data) {
  struct socket *client_sock;
  struct task_struct *handle_connection_task;
  int err;
  if ((err = service_server_sock->ops->listen(service_server_sock, 10)) < 0) {
    printk(KERN_ERR "Failed to listen service server, error = %d\n", err);
    return err;
  }
  printk(KERN_INFO "Service server listening on port %d\n", SERVICE_SERVER_PORT);
  while (true) {
    if ((err = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &client_sock)) < 0) {
      printk(KERN_ERR "Failed to create service client accept socket, error = %d\n", err);
      return err;
    }
    if ((err = service_server_sock->ops->accept(service_server_sock, client_sock, 0)) < 0) {
      printk(KERN_ERR "Failed to accept connection, error = %d\n", err);
      return err;
    }
    handle_connection_task = kthread_create(&handle_connection, (void *)client_sock, "service_server_handle_connection");
    wake_up_process(handle_connection_task);
  }
  return 0;
}

void stop_service_server(void) {
  if (service_server_loop_task != NULL) {
    kthread_stop(service_server_loop_task);
    service_server_loop_task = NULL;
  }
}

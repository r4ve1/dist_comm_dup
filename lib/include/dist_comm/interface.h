#ifndef LIB_INTERFACE_H
#define LIB_INTERFACE_H
#include <dist_comm/netlink.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INTERFACE_NETLINK 30
#define MAX_PAYLOAD 512
#define IMMDIETE 0
#define PTR 1
#define INCOME_HANDSHAKE 0
#define INCOME_RESULT 1
#define TRUE 1
#define FALSE 0
typedef struct {
  char *name;
  char *description;
} attr_info_t;

typedef struct {
  uint32_t length;
  char *addr;
} attr_value_t;

typedef attr_info_t arg_info_t, ret_info_t;
typedef attr_value_t arg_value_t, ret_value_t;

typedef int (*callback_t)(arg_value_t *argv, ret_value_t *retv);

typedef uint8_t attr_flags_t;
typedef attr_flags_t arg_flags_t;
typedef attr_flags_t ret_flags_t;

typedef struct interface_info {
  char *name;
  char *description;
  arg_info_t *arg_info;
  ret_info_t *ret_info;
} info_t;

typedef struct interface_ctrl {
  uint8_t activated;
  uint32_t client_portid;
  callback_t callback;
  struct socket *socket;
  pid_t bind_pid;
  struct proc_dir_entry *proc_entry;
} ctrl_t;

typedef struct interface {
  uint32_t id, argc, retc;
  arg_flags_t *arg_flags;
  ret_flags_t *ret_flags;
  ctrl_t *ctrl;
  info_t *info;
  // argv point to a list of arg_value_t
  arg_value_t *argv;
  // retv point to a list of ret_value_t
  ret_value_t *retv;
} interface_t;
/*
 * Alloc a interface
 */
interface_t *alloc_interface(uint32_t argc, uint32_t retc, uint8_t alloc_init, uint8_t alloc_argv, uint8_t alloc_retv);
int free_interface(interface_t *interface);
/*
 * Call after ioctl(REGISTER_INTERFACE_CMD)
 */
int activate_and_listen(interface_t **managed_interfaces, uint32_t count);
interface_t *find_interface(interface_t **interface_list, int count, int id);
int handle_rpc(int sock_fd, uint32_t portid, interface_t **managed_interfaces, int count);
int register_interface(interface_t *);
#endif

#ifndef INTERFACE_H
#define INTERFACE_H

#include <linux/err.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <net/sock.h>

#include "util/socket.h"

#define IF_IMMEDATE 0
#define IF_STREAM 1
#define GRADIENT 10
#define INTERFACE_NETLINK 30
#define MAX_INTERFACES 256
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

typedef struct netlink_header {
  uint8_t type;
} netlink_header_t;
/*
 * Register a interface
 * Args:
 * buffer: a ptr to interface_t (user space)
 * return: -1 if failed, return :id if succeed
 */
uint32_t register_interface(const char __user *_interface_user);
/*
 * Remove a interface
 */
uint32_t remove_interface(uint32_t id);

/*
 * Init netlink socket in kernel space
 */
int init_interface_socket(void);
/*
 * Callback function for incoming netlink session
 */
void handle_netlink_income(struct sk_buff *skb);
// void init_interfaces(void);

/*
 * Execute the interface and send return data through socket
 */
int execute_interface(char *packet, uint32_t packet_length, struct socket *sock);
/*
 * Alloc interface, you can choose what field you'll alloc
 */
interface_t *alloc_interface(uint32_t argc, uint32_t retc, uint8_t alloc_init, uint8_t alloc_argv, uint8_t alloc_retv);
int free_interface(interface_t *interface);
int send_msg_to_user(struct sock *sk, u32 pid, char *data, uint32_t size);
u32 parse_user_pid(struct sk_buff *skb);
void init_interfaces(void);
void free_interfaces(interface_t **interfaces, int count);

/*
 * Handler of netlink income
 */
void handle_interface_result(char *body, uint32_t length, uint32_t portid);
void handle_activate_interface(char *body, uint32_t body_len, uint32_t portid);
/*
 * Generate interface info (names & descriptions)
 */
int fetch_interface_info(char *packet, uint32_t packet_length, struct socket *sock);
interface_t *find_interface(interface_t **interface_list, int count, int id);
extern struct sock *KERN_NETLINK;
extern interface_t **INTERFACE_LIST;
extern uint32_t INTERFACE_COUNT;
extern uint32_t ALLOCATED_ID;
extern uint32_t INTERFACE_CAPACITY;
#endif

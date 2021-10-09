#include "interface.h"

#include "packet.h"
#include "proc.h"
#define MAX_BUFFER_SIZE 1024

uint32_t ALLOCATED_ID = 1;
uint32_t INTERFACE_COUNT = 0;
uint32_t INTERFACE_CAPACITY = 0;
interface_t **INTERFACE_LIST = NULL;
struct sock *KERN_NETLINK = NULL;

uint32_t register_interface(const char __user *_interface_user) {
  interface_t *interface, *_u_interface = (interface_t *)_interface_user;
  uint32_t i;
  printk(KERN_DEBUG "About to register a interface\n");
  // alloc interface_t
  printk(KERN_INFO "Entering %s\n", __FUNCTION__);
  interface = alloc_interface(_u_interface->argc, _u_interface->retc, TRUE, FALSE, FALSE);
  if (IS_ERR_OR_NULL(interface)) {
    printk(KERN_ERR "Error during mem alloc\n");
    return -ENOMEM;
  }
  // log pid and add to procfs
  interface->ctrl->bind_pid = current->pid;
  add_interface_to_proc(interface);

  // copy info_t
  interface->info->name = strndup_user(_u_interface->info->name, strlen_user(_u_interface->info->name));
  interface->info->description = strndup_user(_u_interface->info->description, strlen_user(_u_interface->info->description));
  // copy arg_info
  for (i = 0; i < interface->argc; i++) {
    interface->info->arg_info[i].name = strndup_user(_u_interface->info->arg_info[i].name, strlen_user(_u_interface->info->arg_info[i].name));
    interface->info->arg_info[i].description = strndup_user(_u_interface->info->arg_info[i].description, strlen_user(_u_interface->info->arg_info[i].description));
  }
  // copy ret_info
  for (i = 0; i < interface->retc; i++) {
    interface->info->ret_info[i].name = strndup_user(_u_interface->info->ret_info[i].name, strlen_user(_u_interface->info->ret_info[i].name));
    interface->info->ret_info[i].description = strndup_user(_u_interface->info->ret_info[i].description, strlen_user(_u_interface->info->ret_info[i].description));
  }
  // alloc id
  interface->id = ALLOCATED_ID++;
  printk(KERN_INFO "Assigned id %d to new interface\n", interface->id);
  // save to interface_list_g
  if (INTERFACE_CAPACITY < INTERFACE_COUNT + 1) {
    INTERFACE_CAPACITY += GRADIENT;
    INTERFACE_LIST = (interface_t **)krealloc(INTERFACE_LIST, sizeof(interface_t *) * INTERFACE_CAPACITY, GFP_KERNEL);
    if (IS_ERR_OR_NULL(INTERFACE_LIST)) {
      printk(KERN_ERR "error during krealloc()\n");
    }
    printk(KERN_INFO "INTERFACE_LIST resized, current capacity is %d\n", INTERFACE_CAPACITY);
  }
  INTERFACE_LIST[INTERFACE_COUNT++] = interface;
  printk(KERN_INFO "Done\n");
  return interface->id;
}
uint32_t remove_interface(uint32_t id) {
  uint32_t i;
  interface_t *interface = NULL;
  // Don't use find_interface()
  for (i = 0; i < INTERFACE_COUNT; i++) {
    if (INTERFACE_LIST[i]->id == id) {
      interface = INTERFACE_LIST[i];
      break;
    }
  }
  if (!interface) {
    printk(KERN_WARNING "interface id %d not found, doing nothing\n", id);
    return -1;
  }
  printk(KERN_INFO "Got interface with id %d, going to free it\n", id);
  for (; i < INTERFACE_COUNT - 1; i++) {
    INTERFACE_LIST[i] = INTERFACE_LIST[i - 1];
  }
  INTERFACE_COUNT--;
  // Free interface's resources
  free_interface(interface);
  printk(KERN_INFO "interface %d freed\n", id);
  return id;
}

int init_interface_socket(void) {
  struct netlink_kernel_cfg callbacks = {
      .input = handle_netlink_income,
  };
  KERN_NETLINK = netlink_kernel_create(&init_net, INTERFACE_NETLINK, &callbacks);
  if (!KERN_NETLINK) {
    printk(KERN_ERR "Can't create netlink socket");
    return -1;
  } else {
    printk(KERN_INFO "Created netlink socket with unit==%d", INTERFACE_NETLINK);
  }
  return 0;
}

void handle_netlink_income(struct sk_buff *skb) {
  struct nlmsghdr *data = (struct nlmsghdr *)skb->data;
  uint32_t length = data->nlmsg_len, body_length = length - sizeof(netlink_header_t);
  uint32_t portid = data->nlmsg_pid;
  char *raw_data = (char *)nlmsg_data(data);
  char *body;
  netlink_header_t header;
  printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
  if (length < sizeof(netlink_header_t)) {
    printk(KERN_WARNING "Message format error");
  }
  memcpy(&header, raw_data, sizeof(netlink_header_t));
  body = raw_data + sizeof(netlink_header_t);
  switch (header.type) {
    case INCOME_HANDSHAKE:
      handle_activate_interface(body, body_length, portid);
      break;
    case INCOME_RESULT:
      handle_interface_result(body, body_length, portid);
      break;
    default:
      break;
  }
}
void handle_interface_result(char *packet, uint32_t packet_length, uint32_t portid) {
  //format: interfaceID||length||data
  uint32_t id, ret, retc;
  interface_t *interface = NULL;
  ret_value_t *retv;
  printk(KERN_INFO "Entering %s\n", __FUNCTION__);
  id = interface_id(packet);
  interface = find_interface(INTERFACE_LIST, INTERFACE_COUNT, id);
  if (!interface) {
    printk(KERN_WARNING "Can't find interface with id %d\n", id);
    return;
  }
  // TEST
  unserialize_rpc_packet(packet, packet_length, &retv, &retc);
  printk(KERN_INFO "Retn: %s\n", retv[0].addr);
  // ENDTEST
  printk(KERN_INFO "Get rpc result, about to send it through socket\n");
  if (!interface->ctrl->socket) {
    printk(KERN_WARNING "Can't find reply socket\n");
    return;
  }
  ret = sock_sendto(interface->ctrl->socket, NULL, (unsigned char *)&packet_length, sizeof(packet_length));
  if (ret < 0) {
    printk(KERN_WARNING "Can't send return packet length to peer\n");
    return;
  }
  ret = sock_sendto(interface->ctrl->socket, NULL, packet, packet_length);
  if (ret < 0) {
    printk(KERN_WARNING "Can't send return packet to peer\n");
    return;
  }
  // TODO: Just write buffer back to socket
}
void handle_activate_interface(char *packet, uint32_t packet_length, uint32_t portid) {
  uint32_t count, *ids, i, j;
  // get _count and _ids
  if (packet_length < sizeof(count)) {
    printk(KERN_WARNING "packet format not permitted\n");
    return;
  }
  memcpy(&count, packet, sizeof(count));
  if (packet_length < sizeof(uint32_t) * (count + 1)) {
    printk(KERN_WARNING "packet format not permitted\n");
    return;
  }
  // activate interfaces
  printk(KERN_INFO "Portid %d want to activate %d interfaces\n", portid, count);
  ids = ((uint32_t *)packet) + 1;
  printk(KERN_INFO "These interfaces will be activated:");
  for (i = 0; i < count; i++) {
    for (j = 0; j < INTERFACE_COUNT; j++) {
      if (INTERFACE_LIST[j]->id == ids[i]) {
        INTERFACE_LIST[j]->ctrl->activated = 1;
        INTERFACE_LIST[j]->ctrl->client_portid = portid;
        printk(" %d", ids[i]);
        break;
      }
    }
  }
  printk("\n");
  printk(KERN_INFO "Done\n");
}

int send_msg_to_user(struct sock *sk, uint32_t portid, char *data, uint32_t size) {
  struct sk_buff *skb_out;
  struct nlmsghdr *nlh;
  int ret;
  skb_out = nlmsg_new(size, GFP_KERNEL);
  if (!skb_out) {
    printk(KERN_ERR "Failed to allocate new skb of size: %d\n", size);
    return -1;
  }
  nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, size, 0);
  NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
  memcpy(nlmsg_data(nlh), data, size);
  ret = nlmsg_unicast(sk, skb_out, portid);
  if (ret < 0) {
    printk(KERN_ERR "Error while sending msg to user port with id==%d\n", portid);
    printk(KERN_ERR "nlmsg_unicast() error\n");
    return -1;
  }
  // kfree(packet);
  return 0;
}

u32 parse_user_pid(struct sk_buff *skb) {
  return ((struct nlmsghdr *)skb->data)->nlmsg_pid;
}

void init_interfaces(void) {
  INTERFACE_LIST = (interface_t **)kzalloc(sizeof(interface_t *) * INTERFACE_CAPACITY, GFP_KERNEL);
}

void free_interfaces(interface_t **interfaces, int count) {
  int i;
  if (!interfaces)
    return;
  if (count == 0) {
    kfree(interfaces);
    return;
  }
  for (i = 0; i < count; ++i) {
    interface_t *interface = interfaces[i];
    free_interface(interface);
  }
  kfree(interfaces);
}

int execute_interface(char *packet, uint32_t packet_length, struct socket *sock) {
  interface_t *interface = NULL;
  int ret, id;
  printk(KERN_INFO "Entering %s\n", __FUNCTION__);
  id = interface_id(packet);
  // TODO: name & descriptions
  if (id == 0) {
    return fetch_interface_info(packet, packet_length, sock);
  }

  // select interface by id
  interface = find_interface(INTERFACE_LIST, INTERFACE_COUNT, id);
  if (!interface) {
    printk(KERN_ERR "Can't find interface requested\n");
    return -1;
  }
  interface->ctrl->socket = sock;

  ret = send_msg_to_user(KERN_NETLINK, interface->ctrl->client_portid, packet, packet_length);
  if (ret < 0) {
    printk(KERN_ERR "Failed at send_msg_to_user()\n");
    return -1;
  }
  return 0;
}

int fetch_interface_info(char *packet, uint32_t packet_length, struct socket *sock) {
  arg_value_t *argv;
  uint32_t id, argc, reply_length;
  interface_t *dst_interface = NULL;
  char *reply_packet;
  int err;
  unserialize_rpc_packet(packet, packet_length, &argv, &argc);
  id = *(uint32_t *)argv[0].addr;
  dst_interface = find_interface(INTERFACE_LIST, INTERFACE_COUNT, id);
  if (!dst_interface) {
    printk(KERN_ALERT "Can't get interface with id %d\n", id);
    goto _CTOR;
  }
  err = serialize_interface_info(id, dst_interface->argc, dst_interface->retc, dst_interface->info, &reply_packet, &reply_length);
  if (err < 0) {
    printk(KERN_ALERT "Can't serialize into_t\n");
    goto _CTOR;
  }
  err = sock_sendto(sock, NULL, reply_packet, reply_length);
  if (err < 0) {
    printk(KERN_ALERT "Can't send back through socket\n");
    goto _CTOR;
  }
  return 0;
_CTOR:
  // TODO: Release the socket?
  return -1;
}

interface_t *find_interface(interface_t **interface_list, int count, int id) {
  interface_t *interface = NULL;
  int i;
  for (i = 0; i < count; i++) {
    if (interface_list[i]->id == id) {
      interface = interface_list[i];
      break;
    }
  }
  return interface;
}

interface_t *alloc_interface(uint32_t argc, uint32_t retc, uint8_t init_info, uint8_t init_argv, uint8_t init_retv) {
  interface_t *interface = (interface_t *)kzalloc(sizeof(interface_t), GFP_KERNEL);
  interface->argc = argc;
  interface->retc = retc;
  interface->id = -1;
  interface->ctrl = (ctrl_t *)kzalloc(sizeof(ctrl_t), GFP_KERNEL);
  interface->arg_flags = (arg_flags_t *)kzalloc(sizeof(arg_flags_t) * interface->argc, GFP_KERNEL);
  interface->ret_flags = (ret_flags_t *)kzalloc(sizeof(ret_flags_t) * interface->retc, GFP_KERNEL);
  if (init_info) {
    interface->info = (info_t *)kzalloc(sizeof(info_t), GFP_KERNEL);
    interface->info->arg_info = (arg_info_t *)kzalloc(sizeof(arg_info_t) * interface->argc, GFP_KERNEL);
    interface->info->ret_info = (ret_info_t *)kzalloc(sizeof(ret_info_t) * interface->retc, GFP_KERNEL);
  }
  if (init_argv) interface->argv = (arg_value_t *)kzalloc(sizeof(arg_value_t) * interface->argc, GFP_KERNEL);
  if (init_retv) interface->retv = (ret_value_t *)kzalloc(sizeof(ret_value_t) * interface->retc, GFP_KERNEL);
  return interface;
}

int free_interface(interface_t *interface) {
  // TODO: Do GC for attrv, info
  if (interface->arg_flags) kfree(interface->arg_flags);
  if (interface->ret_flags) kfree(interface->ret_flags);
  if (interface->ctrl) kfree(interface->ctrl);
  kfree(interface);
  return 0;
}

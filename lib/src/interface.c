#include <dist_comm/dev.h>
#include <dist_comm/interface.h>
#include <dist_comm/packet.h>
#include <linux/ioctl.h>
int activate_and_listen(interface_t **managed_interfaces, uint32_t count) {
  const uint32_t PORTID = get_portid();
  uint32_t i;
  int sock_fd, ret;
  char *packet, *buffer;
  char recv_buffer[0x500];
  // init client socket
  sock_fd = init_netlink_client(PORTID);
  if (sock_fd < 0) {
    printf("can't init socket\n");
    return -1;
  }
  // generate packet of handshake packet. format: count||[array of ids]
  packet = (char *)malloc(sizeof(uint32_t) * (count + 1));
  memcpy(packet, &count, sizeof(count));
  for (i = 0; i < count; i++) {
    memcpy(packet + sizeof(uint32_t) * (i + 1), &managed_interfaces[i]->id, sizeof(managed_interfaces[i]->id));
  }
  // send data
  ret = netlink_sendto_kern(sock_fd, PORTID, INCOME_HANDSHAKE, packet, sizeof(uint32_t) * (count + 1));
  if (ret < 0) {
    printf("can't send msg through socket\n");
    return -1;
  }
  handle_rpc(sock_fd, PORTID, managed_interfaces, count);
  return;
}

int handle_rpc(int sock_fd, uint32_t portid, interface_t **managed_interfaces, int count) {
  char *ret_buffer;
  char packet[MAX_PAYLOAD];
  int ret;
  uint32_t id, packet_length, argc, i;
  interface_t *interface;
  arg_value_t *argv;
  ret_value_t *retv;
  while (1) {
    id = -1, interface = NULL, argv = NULL, retv = NULL, ret_buffer = NULL;
    memset(packet, 0, MAX_PAYLOAD);
    packet_length = netlink_recvfrom_kern(sock_fd, packet, MAX_PAYLOAD);
    // format: id||argc||arg0.len||arg0.data||arg1.len||arg1.data...
    if (packet_length <= 0) {
      continue;
    }
    // parse interface id
    id = interface_id(packet);
    // select from managed interfaces
    interface = find_interface(managed_interfaces, count, id);
    if (!interface) {
      printf("%s: interface with id %d not found, ignore", __FUNCTION__, id);
      continue;
    }
    unserialize_rpc_packet(packet, packet_length, &id, &argv, &argc);
    if (interface->argc != argc) {
      printf("Argument count not match, expected %d, result %d\n", interface->argc, argc);
      goto _CTOR;
    }
    // alloc space for retv
    retv = (ret_value_t *)malloc(sizeof(ret_value_t) * interface->retc);
    for (i = 0; i < interface->retc; i++) {
      retv[i].length = -1;
      retv[i].addr = NULL;
    }
    ret = interface->ctrl->callback(argv, retv);
    if (ret < 0) {
      printf("%s: can't get function result\n", __FUNCTION__);
      goto _CTOR;
    }
    serialize_rpc_packet(id, interface->retc, retv, &ret_buffer, &packet_length);
    ret = netlink_sendto_kern(sock_fd, portid, INCOME_RESULT, ret_buffer, packet_length);
    if (ret < 0) {
      printf("%s: can't send ret back to kernel\n", __FUNCTION__);
    } else {
      printf("%s: interface call finished\n", __FUNCTION__);
    }
  _CTOR:
    // TODO: do GC
    if (retv) {
      for (i = 0; i < interface->retc; i++) {
        if (retv[i].addr) free(retv[i].addr);
      }
      free(retv);
    }
    if (argv) free(argv);
    if (ret_buffer) free(ret_buffer);
    // if (argv) {
    //   for (i = 0; i < argc, i++) {
    //     free(argv->impl->addr);
    //   }
    // }
    // free(interface->argv);
  }
}

int register_interface(interface_t *interface) {
  int file_desc = open(MANAGER_DEVICE_FILE_NAME, 0);
  int ret = ioctl(file_desc, MANAGER_REG_INTERFACE_CMD, interface);
  close(file_desc);
  return ret;
}

interface_t *alloc_interface(uint32_t argc, uint32_t retc, uint8_t init_info, uint8_t init_argv, uint8_t init_retv) {
  interface_t *interface = (interface_t *)malloc(sizeof(interface_t));
  int i;
  memset(interface, 0, sizeof(interface_t));
  interface->argc = argc;
  interface->retc = retc;
  interface->id = -1;
  interface->ctrl = (ctrl_t *)malloc(sizeof(ctrl_t));
  interface->arg_flags = (arg_flags_t *)malloc(sizeof(arg_flags_t) * argc);
  interface->ret_flags = (ret_flags_t *)malloc(sizeof(ret_flags_t) * retc);
  memset(interface->ctrl, 0, sizeof(ctrl_t));
  memset(interface->arg_flags, 0, sizeof(arg_flags_t) * interface->argc);
  memset(interface->ret_flags, 0, sizeof(ret_flags_t) * interface->retc);
  if (init_info) {
    interface->info = (info_t *)malloc(sizeof(info_t));
    memset(interface->info, 0, sizeof(info_t));
    interface->info->arg_info = (arg_info_t *)malloc(sizeof(arg_info_t) * interface->argc);
    interface->info->ret_info = (ret_info_t *)malloc(sizeof(ret_info_t) * interface->retc);
  }
  if (init_argv) interface->argv = (arg_value_t *)malloc(sizeof(arg_value_t) * interface->argc);
  if (init_retv) {
    interface->retv = (ret_value_t *)malloc(sizeof(ret_value_t) * interface->retc);
    for (i = 0; i < interface->retc; ++i)
      interface->retv[i].addr = (char *)malloc(sizeof(char) * MAX_PAYLOAD);
  }
  return interface;
}

int free_interface(interface_t *interface) {
  // TODO: Do GC for attrv, info
  if (interface->arg_flags) free(interface->arg_flags);
  if (interface->ret_flags) free(interface->ret_flags);
  free(interface);
  free(interface->ctrl);
  return 0;
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

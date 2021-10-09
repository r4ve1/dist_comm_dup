#include "packet.h"

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

inline void copy_to_buffer(char **buffer, uint32_t *buffer_length_p, uint32_t *max_size_p, void *src, uint32_t src_length) {
  if (*buffer_length_p + src_length > *max_size_p) {
    *max_size_p += BUFFER_INC_SIZE;
    *buffer = (char *)krealloc(*buffer, *max_size_p * sizeof(char), GFP_KERNEL);
  }
  memcpy(*buffer + *buffer_length_p, src, src_length);
  *buffer_length_p += src_length;
}
static inline char *dup_str(char *src) {
  uint32_t length = strlen(src);
  char *dest = (char *)kzalloc(length + 1, GFP_KERNEL);
  strcpy(dest, src);
  return dest;
}
int serialize_interfaces_packet(interface_t **interface_list, uint32_t count, char **packet_p, uint32_t *packet_length_p) {
  uint32_t i, length = 0, current_max = BUFFER_INC_SIZE;
  char *buf = (char *)kzalloc(current_max, GFP_KERNEL);
  interface_t *interface;
  // FORMAT: count||interfaces...
  copy_to_buffer(&buf, &length, &current_max, &count, sizeof(count));
  for (i = 0; i < count; ++i) {
    // FORMAT: id||argc||arg_flags...||retc||ret_flags...
    // id
    interface = interface_list[i];
    if (!interface) {
      printk(KERN_WARNING "Invalid interface list entry, skipped\n");
      continue;
    }
    copy_to_buffer(&buf, &length, &current_max, &interface->id, sizeof(interface->id));
    // argc & arg_flags
    if (!interface->arg_flags) {
      printk(KERN_WARNING "Invalid interface list entry, skipped\n");
      continue;
    }
    copy_to_buffer(&buf, &length, &current_max, &interface->argc, sizeof(interface->argc));
    copy_to_buffer(&buf, &length, &current_max, interface->arg_flags, sizeof(arg_flags_t) * interface->argc);
    // retc & ret_flags
    if (!interface->ret_flags) {
      printk(KERN_WARNING "Invalid interface list entry, skipped\n");
      continue;
    }
    copy_to_buffer(&buf, &length, &current_max, &interface->retc, sizeof(interface->retc));
    copy_to_buffer(&buf, &length, &current_max, interface->ret_flags, sizeof(ret_flags_t) * interface->retc);
  }
  *packet_p = (char *)krealloc(buf, length, GFP_KERNEL);
  *packet_length_p = length;
  return 0;
}

int unserialize_interfaces_packet(char *packet, uint32_t packet_length, interface_t ***interfaces, uint32_t *count_p) {
  uint32_t i, count;
  interface_t **interface_list;
  interface_t *interface;
  if (packet_length < sizeof(count)) {
    printk(KERN_ERR "Invalid interface list packet length\n");
    return -EINVALIDPACKETLENGTH;
  }
  memcpy(&count, packet, sizeof(count));
  if (count < 0 || count > MAX_INTERFACES)
    return -EINVALIDINTERFACECOUNT;
  packet += sizeof(count);
  interface_list = (interface_t **)kzalloc(count * sizeof(interface_t *), GFP_KERNEL);
  for (i = 0; i < count; i++) {
    // id
    interface = (interface_t *)kzalloc(sizeof(interface_t), GFP_KERNEL);
    memcpy(&interface->id, packet, sizeof(interface->id));
    packet += sizeof(interface->id);
    // argc & arg_flags
    memcpy(&interface->argc, packet, sizeof(interface->argc));
    packet += sizeof(interface->argc);
    interface->arg_flags = (arg_flags_t *)kzalloc(sizeof(arg_flags_t) * interface->argc, GFP_KERNEL);
    memcpy(interface->arg_flags, packet, sizeof(arg_flags_t) * interface->argc);
    packet += sizeof(arg_flags_t) * interface->argc;
    // retc & ret_flags
    memcpy(&interface->retc, packet, sizeof(interface->retc));
    packet += sizeof(interface->retc);
    interface->ret_flags = (ret_flags_t *)kzalloc(sizeof(ret_flags_t) * interface->retc, GFP_KERNEL);
    memcpy(interface->ret_flags, packet, sizeof(ret_flags_t) * interface->retc);
    packet += sizeof(ret_flags_t) * interface->retc;
    interface_list[i] = interface;
  }
  *interfaces = interface_list;
  *count_p = count;
  return 0;
}

int serialize_rpc_packet(uint32_t id, uint32_t argc, attr_value_t *values, char **packet_p, uint32_t *packet_length_p) {
  uint32_t i, current_max = BUFFER_INC_SIZE, length = 0;
  char *buf = (char *)kzalloc(current_max * sizeof(char), GFP_KERNEL);
  attr_value_t *attrv;
  // FORMAT: id||argc||arg0.len||arg0.value...
  copy_to_buffer(&buf, &length, &current_max, &id, sizeof(id));
  copy_to_buffer(&buf, &length, &current_max, &argc, sizeof(argc));
  for (i = 0; i < argc; ++i) {
    attrv = values + i;
    copy_to_buffer(&buf, &length, &current_max, &attrv->length, sizeof(attrv->length));
    if (attrv->length)
      copy_to_buffer(&buf, &length, &current_max, attrv->addr, attrv->length);
  }
  *packet_p = buf;
  *packet_length_p = length;
  return 0;
}

int unserialize_rpc_packet(char *packet, uint32_t packet_length, attr_value_t **attrv_p, uint32_t *attrc_p) {
  uint32_t i, attrc;
  attr_value_t *attrv;
  packet += sizeof(uint32_t);
  memcpy(&attrc, packet, sizeof(attrc));
  packet += sizeof(attrc);
  attrv = (attr_value_t *)kzalloc(attrc * sizeof(attr_value_t), GFP_KERNEL);
  for (i = 0; i < attrc; ++i) {
    memcpy(&attrv[i].length, packet, sizeof(attrv[i].length));
    packet += sizeof(attrv[i].length);
    if (attrv[i].length != 0) {
      attrv[i].addr = (char *)kzalloc(attrv[i].length * sizeof(char), GFP_KERNEL);
      memcpy(attrv[i].addr, packet, attrv[i].length);
      packet += attrv[i].length;
    }
  }
  *attrv_p = attrv;
  *attrc_p = attrc;
  return 0;
}
inline int interface_id(char *packet) {
  return *(int *)packet;
}

int serialize_interface_info(uint32_t id, uint32_t argc, uint32_t retc, info_t *info, char **packet_p, uint32_t *packet_length_p) {
  uint32_t i, current_max = BUFFER_INC_SIZE, length = 0;
  char *buf = (char *)kzalloc(current_max * sizeof(char), GFP_KERNEL);
  // FORMAT: id||name||desc||argc||arg0desc...||retc||ret0desc...
  copy_to_buffer(&buf, &length, &current_max, &id, sizeof(id));
  if (!info->name) return -1;
  copy_to_buffer(&buf, &length, &current_max, info->name, strlen(info->name) + 1);
  if (!info->description) return -1;
  copy_to_buffer(&buf, &length, &current_max, info->description, strlen(info->description) + 1);
  // argc & arg_info
  copy_to_buffer(&buf, &length, &current_max, &argc, sizeof(argc));
  for (i = 0; i < argc; i++) {
    if (!info->arg_info[i].name) return -1;
    copy_to_buffer(&buf, &length, &current_max, info->arg_info[i].name, strlen(info->arg_info[i].name) + 1);
    if (!info->arg_info[i].description) return -1;
    copy_to_buffer(&buf, &length, &current_max, info->arg_info[i].description, strlen(info->arg_info[i].description) + 1);
  }
  // retc & ret_info
  copy_to_buffer(&buf, &length, &current_max, &retc, sizeof(retc));
  for (i = 0; i < argc; i++) {
    if (!info->ret_info[i].name) return -1;
    copy_to_buffer(&buf, &length, &current_max, info->ret_info[i].name, strlen(info->ret_info[i].name) + 1);
    if (!info->ret_info[i].description) return -1;
    copy_to_buffer(&buf, &length, &current_max, info->ret_info[i].description, strlen(info->ret_info[i].description) + 1);
  }
  *packet_p = buf;
  *packet_length_p = length;
  return 0;
}

int unserialize_interface_info(char *packet, uint32_t packet_length, info_t **info_p) {
  // FORMAT: id||name||desc||argc||arg0desc...||retc||ret0desc...
  uint32_t i, argc, retc;
  info_t *info = (info_t *)kzalloc(sizeof(info_t), GFP_KERNEL);
  // id
  packet += sizeof(uint32_t);
  // name
  info->name = dup_str(packet);
  packet += strlen(info->name) + 1;
  // description
  info->description = dup_str(packet);
  packet += strlen(info->description) + 1;
  // argc & arg_info
  memcpy(&argc, packet, sizeof(argc));
  packet += sizeof(argc);
  info->arg_info = (arg_info_t *)kzalloc(sizeof(arg_info_t) * argc, GFP_KERNEL);
  for (i = 0; i < argc; i++) {
    info->arg_info[i].name = dup_str(packet);
    packet += strlen(info->arg_info[i].name) + 1;
    info->arg_info[i].description = dup_str(packet);
    packet += strlen(info->arg_info[i].description) + 1;
  }
  // retc & ret_info
  memcpy(&retc, packet, sizeof(retc));
  packet += sizeof(uint32_t);
  info->ret_info = (ret_info_t *)kzalloc(sizeof(ret_info_t) * retc, GFP_KERNEL);
  for (i = 0; i < retc; i++) {
    info->ret_info[i].name = dup_str(packet);
    packet += strlen(info->ret_info[i].name) + 1;
    info->ret_info[i].description = dup_str(packet);
    packet += strlen(info->ret_info[i].description) + 1;
  }
  *info_p = info;
  return 0;
}

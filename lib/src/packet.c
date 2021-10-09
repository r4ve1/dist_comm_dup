#include <dist_comm/packet.h>

static inline void copy_to_buffer(char **buffer, uint32_t *buffer_length_p, uint32_t *max_size_p, void *src, uint32_t src_length) {
  if (*buffer_length_p + src_length > *max_size_p) {
    *max_size_p += BUFFER_INC_SIZE;
    *buffer = (char *)realloc(*buffer, *max_size_p * sizeof(char));
  }
  memcpy(*buffer + *buffer_length_p, src, src_length);
  *buffer_length_p += src_length;
}

int serialize_rpc_packet(int id, int argc, attr_value_t *values, char **packet_p, uint32_t *packet_length_p) {
  uint32_t i, current_max = BUFFER_INC_SIZE, length = 0;
  char *buf = (char *)malloc(current_max * sizeof(char));
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

int unserialize_rpc_packet(char *packet, uint32_t packet_length, uint32_t *id_p, attr_value_t **attrv_p, uint32_t *attrc_p) {
  uint32_t i, attrc;
  attr_value_t *attrv;
  memcpy(id_p, packet, sizeof(*id_p));
  packet += sizeof(*id_p);
  memcpy(&attrc, packet, sizeof(attrc));
  packet += sizeof(attrc);
  attrv = (attr_value_t *)malloc(attrc * sizeof(attr_value_t));
  for (i = 0; i < attrc; ++i) {
    memcpy(&attrv[i].length, packet, sizeof(attrv[i].length));
    packet += sizeof(attrv[i].length);
    if (attrv[i].length != 0) {
      attrv[i].addr = (char *)malloc(attrv[i].length * sizeof(char));
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

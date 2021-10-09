#ifndef LIB_PACKET_H
#define LIB_PACKET_H
#include <stdio.h>
#include <stdlib.h>

#include "interface.h"
#define BUFFER_INC_SIZE 65536
int serialize_rpc_packet(int id, int argc, attr_value_t *values, char **packet_p, uint32_t *packet_length_p);
int unserialize_rpc_packet(char *packet, uint32_t packet_length, uint32_t *id_p, attr_value_t **attrv_p, uint32_t *attrc_p);
inline int interface_id(char *packet);
#endif

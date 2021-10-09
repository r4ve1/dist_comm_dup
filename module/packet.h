#ifndef NETWORK_INTERFACE_PACKET_H
#define NETWORK_INTERFACE_PACKET_H

#include "interface.h"

#define MAX_INTERFACE_LIST_PACKET_SIZE 65527
#define MAX_INTERFACES 256
#define BUFFER_INC_SIZE 65536

#define EINVALIDPACKETLENGTH 20001
#define EINVALIDINTERFACECOUNT 20002
/*
 * Serialize interfaces packet, return err(minus) or 0
 */
int serialize_interfaces_packet(interface_t **interface_list, uint32_t count, char **packet_p, uint32_t *packet_length_p);
/*
 * Unserialize interfaces packet, return err(minus) or 0
 */
int unserialize_interfaces_packet(char *packet, uint32_t packet_length, interface_t ***interfaces, uint32_t *count);
/*
 * Serialize rpc packet, return err(minus) or 0
 */
int serialize_rpc_packet(uint32_t id, uint32_t argc, attr_value_t *values, char **packet_p, uint32_t *packet_length_p);
/*
 * Unserialize rpc packet, return err(minus) or 0
 */
int unserialize_rpc_packet(char *packet, uint32_t length, attr_value_t **attrv_p, uint32_t *attrc_p);
/*
 * Serialize interface info packet, return err(minus) or 0
 */
int serialize_interface_info(uint32_t id, uint32_t argc, uint32_t retc, info_t *info, char **packet_p, uint32_t *packet_length_p);
/*
 * Unserialize interface info packet, return err(minus) or 0
 */
int unserialize_interface_info(char *packet, uint32_t packet_length, info_t **info_p);
inline int interface_id(char *packet);
inline void copy_to_buffer(char **buffer, uint32_t *buffer_length_p, uint32_t *max_size_p, void *src, uint32_t src_length);
#endif

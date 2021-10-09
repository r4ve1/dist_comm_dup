#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INTERFACE_LIST_PACKET_SIZE 65527
#define MAX_INTERFACES 256

typedef struct interface_field_info {
    char *name;
    char *description;
} arg_info_t;

typedef struct interface_attr_value {
    int length;
    void *addr;
} arg_value_t;

typedef struct interface_arg {
    char flags;
    struct interface_field_info *info;
    struct interface_attr_value *impl;
} interface_arg_t;

typedef struct interface_info {
    char *name;
    char *description;
} info_t;

typedef struct interface {
    int id, argc;
    struct interface_arg *argv;
    struct interface_info *info;
} interface_t;

char* generate_interfaces_packet(struct interface **interface_list, int count) {
    int i, j;
    int length = sizeof(count);
    char *buf = (char*)malloc(MAX_INTERFACE_LIST_PACKET_SIZE);
    memcpy(buf, &count, sizeof(count));
    for (i = 0; i < count; ++i) {
        struct interface *interface = *interface_list + i;
        if (!interface) {
            printf("Invalid interface list entry, skipped\n");
            continue;
        }
        memcpy(buf + length, &interface->id, sizeof(interface->id));
        length += sizeof(interface->id);
        memcpy(buf + length, &interface->argc, sizeof(interface->argc));
        length += sizeof(interface->argc);
        for (j = 0; j < interface->argc; ++j) {
            struct interface_arg *arg = interface->argv + j;
            if (!arg) {
                printf("Invalid interface argument entry, skipped\n");
                continue;
            }
            memcpy(buf + length, &arg->flags, sizeof(arg->flags));
            length += sizeof(arg->flags);
        }
    }
    buf = (char*)realloc(buf, length);
    return buf;
}



int main() {

}

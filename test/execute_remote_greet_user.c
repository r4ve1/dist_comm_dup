#include <dist_comm/dev.h>
#include <dist_comm/interface.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define HELLO_ARGC 1
#define HELLO_RETC 1
int execute_greet_user(char *dev, int id, char *username) {
  interface_t *interface = alloc_interface(1, 1, FALSE, TRUE, TRUE);
  interface->id = id;
  interface->argv[0].length = strlen(username) + 1;
  interface->argv[0].addr = (char *)malloc(sizeof(interface->argv[0].length));
  strcpy(interface->argv[0].addr, username);
  int file_desc = open(dev, 0);
  if (!file_desc) {
    printf("Can't open device %s\n", MANAGER_DEVICE_FILE_NAME);
    exit(1);
  }
  int ret = ioctl(file_desc, PEER_EXEC_INTERFACE_CMD, interface);
  printf("%s\n", interface->retv[0].addr);
  close(file_desc);
}
main(int argc, char *argv[]) {
  int ret, id;
  if (argc != 4) {
    printf("You must specify <dev> <interface_id> <username> to greet\n");
    exit(1);
  }
  id = atoi(argv[2]);
  ret = execute_greet_user(argv[1], id, argv[3]);
  if (ret < 0) {
    printf("Execute failed, abort\n");
  } else {
    printf("Execute succeed\n");
  }
}

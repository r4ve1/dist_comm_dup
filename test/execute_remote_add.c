#include <dist_comm/dev.h>
#include <dist_comm/interface.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define ADD_ARGC 2
#define ADD_RETC 1

int execute_add_user(char *dev, int id, char* num1, char* num2) {
  interface_t *interface = alloc_interface(ADD_ARGC, ADD_RETC, FALSE, TRUE, TRUE);
  interface->id = id;
  interface->argv[0].length = strlen(num1) + 1;
  interface->argv[0].addr = (char *)malloc(sizeof(interface->argv[0].length));
  strcpy(interface->argv[0].addr, num1);
  interface->argv[1].length = strlen(num2) + 1;
  interface->argv[1].addr = (char *)malloc(sizeof(interface->argv[1].length));
  strcpy(interface->argv[1].addr, num2);
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
  if (argc != 5) {
    printf("You must specify <dev> <id> <num1> <num2> to add\n");
    exit(1);
  }
  id = atoi(argv[2]);
  ret = execute_add_user(argv[1], id, argv[3], argv[4]);
  if (ret < 0) {
    printf("Execute failed, abort\n");
  } else {
    printf("Execute succeed\n");
  }
}

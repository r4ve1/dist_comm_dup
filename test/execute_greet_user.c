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
int execute_greet_user(int id, char *username) {
  interface_t *interface = alloc_interface(1, 1, FALSE, TRUE, FALSE);
  interface->id = id;
  interface->argv[0].length = strlen(username) + 1;
  interface->argv[0].addr = (char *)malloc(sizeof(interface->argv[0].length));
  strcpy(interface->argv[0].addr, username);
  int file_desc = open(MANAGER_DEVICE_FILE_NAME, 0);
  if (!file_desc) {
    printf("Can't open device %s\n", MANAGER_DEVICE_FILE_NAME);
    exit(1);
  }
  int ret = ioctl(file_desc, MANAGER_TEST_EXECUTE_REMOTE_CMD, interface);
  close(file_desc);
  exit(0);
}
main(int argc, char *argv[]) {
  int ret, id;
  if (argc != 3) {
    printf("You must specify a interface id and the user's name to greet\n");
    exit(1);
  }
  id = atoi(argv[1]);
  ret = execute_greet_user(id, argv[2]);
  if (ret < 0) {
    printf("Action failed, abort\n");
  } else {
    printf("Activate succeed\n");
  }
}

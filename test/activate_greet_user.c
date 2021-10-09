#include <dist_comm/dev.h>
#include <dist_comm/interface.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
interface_t *register_greet_interface(void);
int greet_user(arg_value_t *argv, ret_value_t *retv);

interface_t *register_greet_interface(void) {
  int argc = 1, retc = 1;
  interface_t *interface = alloc_interface(argc, retc, TRUE, FALSE, FALSE);
  int file_desc = open(MANAGER_DEVICE_FILE_NAME, 0);
  if (!file_desc) {
    printf("Can't open file device\n");
    exit(1);
  }
  interface->info->name = "GreetUser";
  interface->info->description = "Say hello to the user :)";
  interface->info->arg_info[0].name = "Username";
  interface->info->arg_info[0].description = "username of ther user to greet";
  interface->info->ret_info[0].name = "Return Value";
  interface->info->ret_info[0].description = "the formated greeting";
  interface->ctrl->callback = greet_user;
  int ret_val = ioctl(file_desc, MANAGER_REG_INTERFACE_CMD, interface);
  if (ret_val < 0) {
    printf("register_interface failed\n", ret_val);
    exit(1);
  } else {
    interface->id = ret_val;
    printf("register_interface success, interface id %d\n", ret_val);
  }
  close(file_desc);
  return interface;
}
int greet_user(arg_value_t *argv, ret_value_t *retv) {
  // this function
  char tmp[0x100];
  sprintf(tmp, "Hello %s", (char *)argv[0].addr);
  retv[0].length = strlen(tmp) + 1;
  retv[0].addr = (char *)malloc(retv[0].length);
  strcpy(retv[0].addr, tmp);
  return 0;
}
main() {
  int ret, i;
  interface_t **managed = (interface_t **)malloc(sizeof(interface_t *));

  managed[0] = register_greet_interface();
  ret = activate_and_listen(managed, 1);
  if (ret < 0) {
    printf("Action failed, abort\n");
  } else {
    printf("Activate succeed\n");
  }
}

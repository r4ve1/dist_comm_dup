#include <dist_comm/dev.h>
#include <dist_comm/interface.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int add(arg_value_t *argv, ret_value_t *retv) {
  // this function
  int num1 = atoi(argv[0].addr);
  int num2 = atoi(argv[1].addr);
  int result = num1 + num2;
  char tmp[0x100];
  sprintf(tmp, "%d", result);
  retv[0].length = strlen(tmp) + 1;
  retv[0].addr = (char *)malloc(retv[0].length);
  strcpy(retv[0].addr, tmp);
  return 0;
}

interface_t *register_add_interface(void) {
  int argc = 2, retc = 1;
  interface_t *interface = alloc_interface(argc, retc, TRUE, FALSE, FALSE);
  int file_desc = open(MANAGER_DEVICE_FILE_NAME, 0);
  if (!file_desc) {
    printf("Can't open file device\n");
    exit(1);
  }
  interface->info->name = "add_number";
  interface->info->description = "Add number";
  interface->info->arg_info[0].name = "num1";
  interface->info->arg_info[0].description = "Number 1";
  interface->info->arg_info[1].name = "num2";
  interface->info->arg_info[1].description = "Number 2";
  interface->info->ret_info[0].name = "return";
  interface->info->ret_info[0].description = "Result of num1+num2";
  interface->ctrl->callback = add;
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

main() {
  int ret, i;
  interface_t **managed = (interface_t **)malloc(sizeof(interface_t *));
  managed[0] = register_add_interface();
  ret = activate_and_listen(managed, 1);
  if (ret < 0) {
    printf("Action failed, abort\n");
  } else {
    printf("Activate succeed\n");
  }
}

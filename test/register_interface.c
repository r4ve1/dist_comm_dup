#include <dist_comm/dev.h>
#include <dist_comm/interface.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

interface_t* register_test_interface() {
  uint32_t argc = 2, retc = 1;
  interface_t* interface = alloc_interface(argc, retc, TRUE, FALSE, FALSE);
  interface->info->name = "Test";
  interface->info->description = "This interface is used for test";
  interface->info->arg_info[0].name = "Arg0";
  interface->info->arg_info[0].description = "Argument0";
  interface->info->arg_info[1].name = "Arg1";
  interface->info->arg_info[1].description = "Argument1";
  interface->info->ret_info[0].name = "Ret0";
  interface->info->ret_info[0].description = "Return0";
  int ret_val = register_interface(interface);
  if (ret_val < 0) {
    printf("register_interface failed\n", ret_val);
    return NULL;
  } else {
    interface->id = ret_val;
    printf("register_interface success, interface id %d\n", interface->id);
    return interface;
  }
}

main() {
  int ret_val, i;
  interface_t* interface = register_test_interface();
}

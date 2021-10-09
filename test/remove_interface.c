#include <dist_comm/dev.h>
#include <dist_comm/interface.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// interface_t* register_interface(int file_desc) {
//   int argc = 2;
//   interface_t* interface = alloc_interface(argc);
//   interface->info->name = "Test";
//   interface->info->description = "This interface is used for test";
//   interface->argv[0].info->name = "Arg0";
//   interface->argv[0].info->description = "Argument0";
//   interface->argv[1].info->name = "Arg1";
//   interface->argv[1].info->description = "Argument1";
//   int ret_val = ioctl(file_desc, REG_INTERFACE_CMD, &interface);

//   if (ret_val < 0) {
//     printf("register_interface failed\n", ret_val);
//     return NULL;
//   } else {
//     interface->id = ret_val;
//     printf("register_interface success, interface id %d\n", interface->id);
//     return interface;
//   }
// }
// int remove_interface(int file_desc, interface_t* interface) {
//   int ret;
//   printf("now tring to remove interface %d\n", interface->id);
//   ret = ioctl(file_desc, REMOVE_INTERFACE_CMD, interface->id);
//   if (ret < 0) {
//     printf("remove_interface() failed: %d\n", ret);
//   } else {
//     printf("remove_interface() success\n");
//   }
// }

main() {
  // int file_desc, ret, i;
  // interface_t* interface;
  // file_desc = open(MANAGER_DEVICE_FILE_NAME, 0);
  // if (file_desc < 0) {
  //   printf("Can't open device file: %s\n", MANAGER_DEVICE_FILE_NAME);
  //   exit(-1);
  // }
  // interface = register_interface(file_desc);
  // ret = remove_interface(file_desc, interface);
  // if (ret) {
  //   printf("Action failed, abort\n");
  // }
  // close(file_desc);
}

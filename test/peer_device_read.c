#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
char buffer[0x1000];
int main() {
  int fd = open("/dev/dcp0", 0);
  if (fd < 0) {
    printf("error!\n");
    return -1;
  }
  int n = read(fd, buffer, sizeof(buffer));
  printf("Length: %d\n", n);
  buffer[n] = 0;
  printf("Content: %s\n", buffer);
}

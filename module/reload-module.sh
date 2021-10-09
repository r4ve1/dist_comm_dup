#!/bin/bash
DEV_PATH=
rmmod dist_comm
rm ../dist_comm
clean
make
mknod ../dist/dist_comm c 100 0
insmod ../dist/dist_comm.ko
# dmesg
cd ..

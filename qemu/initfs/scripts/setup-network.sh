#!/bin/sh
ip addr add 172.16.222.2/24 brd 172.16.222.255 dev eth0
ip link set eth0 up

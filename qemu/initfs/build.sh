#!/bin/sh
chmod +x ./scripts/*
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../rootfs
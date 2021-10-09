DEV_NAME=dist_comm
obj-m += $(DEV_NAME).o
src-s = $(shell find $(PWD)  -type f -name '*.c' -exec realpath --relative-to $(PWD) {} \;)
$(DEV_NAME)-objs := $(patsubst %.c, %.o, $(src-s))
BUILD_DIR = ../dist
KERNEL_VERSION=3.10.0
# include debug info
KERNEL_PATH = /root/linux-3.10.0-1160.el7
MY_CFLAGS += -g -DDEBUG -O0 -I$(KERNEL_PATH)
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}
# modify this line before build
# KERNEL_PATH = /lib/modules/3.10.0-1160.el7.x86_64/build

all:
	make release
release:
	make -C $(KERNEL_PATH) M=$(PWD) modules
	make mv
debug:
	EXTRA_CFLAGS="$(MY_CFLAGS)"
	make -C $(KERNEL_PATH) M=$(PWD) modules
	make mv
mv:
	mv .tmp_versions $(BUILD_DIR)
	mv *.symvers $(BUILD_DIR)
	mv *.order $(BUILD_DIR)
	find . -name ".*.cmd" | xargs -I {} mv {} $(BUILD_DIR)
	mv *.ko $(BUILD_DIR)
	find . -name "*.o" | xargs -I {} mv {} $(BUILD_DIR)
	mv *.mod.c $(BUILD_DIR)
unload:
	rmmod $(DEV_NAME)
load:
	make all
	insmod $(BUILD_DIR)/$(DEV_NAME).ko
	mknod $(BUILD_DIR)/$(DEV_NAME) c 100 0
reload:
	make unload
	make load
clean:
	rm -rf $(BUILD_DIR)
	mkdir $(BUILD_DIR)

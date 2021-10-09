ROOT=$(PWD)
# LIB_HEADERS=$(wildcard lib/include/dist_comm/*.h)
TEST_SOURCES=$(wildcard test/*.c)
TEST_OBJ=$(TEST_SOURCES:.c=.o)
TEST_TARGETS=$(TEST_SOURCES:.c=.test)
LIB_HEADERS=$(wildcard lib/include/dist_comm/*.c)
LIB_SOURCES=$(wildcard lib/src/*.c)
LIB_OBJ=$(LIB_SOURCES:lib/src/%.c=dist/lib%.o)
LIB_A=dist/libdist_comm.a
INCLUDE=lib/include
.PHONY: module clean run run_second debug copy_test clean_test setup_network remove_network lib test_build_lib_obj
all: module build_test
module: clean clean_test
	cd module && $(MAKE) $(DEBUG)
clean:
	rm -rf dist
	mkdir dist
	rm -rf qemu/initfs/data
run: module copy_test
	cp -r dist qemu/initfs/data
	cd qemu/initfs && sed -i "s/172.16.222.3\/24/172.16.222.2\/24/g" scripts/setup-network.sh && ./build.sh
	cd $(ROOT) && qemu-system-x86_64 \
		-nographic \
		-kernel qemu/bzImage \
		-initrd qemu/rootfs \
		-append "root=/dev/ram nokaslr console=ttyS0 rdinit=/init" \
		-curses \
		-m 512m \
		-serial stdio \
		-netdev tap,id=devnet0,ifname=tap0,script=no,downscript=no \
		-device e1000,netdev=devnet0,mac=26:E1:20:D1:44:46 \
		$(EXTRA)
run_second:
	cd qemu/initfs && sed -i "s/172.16.222.2\/24/172.16.222.3\/24/g" scripts/setup-network.sh && ./build.sh
	cd $(ROOT) && qemu-system-x86_64 \
		-nographic \
		-kernel qemu/bzImage \
		-initrd qemu/rootfs \
		-append "root=/dev/ram nokaslr console=ttyS0 rdinit=/init" \
		-curses \
		-m 512m \
		-serial stdio \
		-netdev tap,id=devnet1,ifname=tap1,script=no,downscript=no \
		-device e1000,netdev=devnet1,mac=26:E1:20:D1:44:48 \
		$(EXTRA)
debug:
	EXTRA="-s -S" DEBUG='debug' $(MAKE) run
build_test: $(TEST_TARGETS)
test/%.test: test/%.o $(LIB_A)
	gcc --static -pthread -g $< -Ldist -ldist_comm -o $@
test/%.o: test/%.c
	gcc -g -I$(INCLUDE) -c $< -o $@
dist/libdist_comm.a: $(LIB_OBJ)
	ar rsc $@ $^
dist/lib%.o: lib/src/%.c
	gcc -g -I$(INCLUDE) -c $< -o $@
copy_test: build_test
	cp $(TEST_TARGETS) dist
clean_test:
	rm -rf $(TEST_TARGETS)
	rm -rf $(TEST_OBJ)
setup_network:
	tunctl -t tap0 -u root
	tunctl -t tap1 -u root
	brctl addbr br0
	brctl addif br0 tap0
	brctl addif br0 tap1
	ip link set tap0 up
	ip link set tap1 up
	ip link set br0 up
	ip addr add 172.16.222.1/24 dev br0
remove_network:
	ip link set br0 down
	brctl delbr br0
	tunctl -d tap0
	tunctl -d tap1

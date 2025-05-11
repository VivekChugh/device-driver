obj-m += concurrency.o

# KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
# PWD := $(shell pwd)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	insmod concurrency.ko

unload:
	rmmod concurrency.ko

# Example usage from user space (requires a separate C program):
# To create device node (run once after loading if it doesn't exist):
# sudo mknod /dev/concurrency c <MAJOR_NUMBER> 0
# (Replace <MAJOR_NUMBER> with the number printed by the module in dmesg)
# sudo chmod 666 /dev/concurrency

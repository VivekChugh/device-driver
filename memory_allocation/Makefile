obj-m += kernel_memory_example.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	insmod kernel_memory_example.ko

unload:
	rmmod kernel_memory_example.ko

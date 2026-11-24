obj-m += memkernel.o
memkernel-objs := entry.o memory.o process.o

KDIR ?= /root/android_kernel_a047f_eur
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- clean

.PHONY: all clean

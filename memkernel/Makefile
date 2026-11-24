obj-m += memkernel.o
memkernel-objs := entry.o memory.o process.o

KDIR ?= /root/android_kernel_a047f_eur
PWD  := $(shell pwd)

# Use LLVM=1 for GKI, or CROSS_COMPILE for legacy kernels
ifdef LLVM
TOOLCHAIN = LLVM=1
else
TOOLCHAIN = ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
endif

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm64 $(TOOLCHAIN) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm64 $(TOOLCHAIN) clean

.PHONY: all clean

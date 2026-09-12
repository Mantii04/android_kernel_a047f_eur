MODULE = ovo

obj-$(CONFIG_OVO) += $(MODULE).o
$(MODULE)-objs += core.o
$(MODULE)-objs += mmuhack.o
$(MODULE)-objs += kkit.o
$(MODULE)-objs += peekaboo.o
$(MODULE)-objs += memory.o
$(MODULE)-objs += server.o
$(MODULE)-objs += touch.o
$(MODULE)-objs += vma.o
$(MODULE)-objs += addr_pfn_map.o

ccflags-y += -DHIDE_SELF_MODULE=0
ccflags-y += -DBUILD_REMAP=1
ccflags-y += -DENABLE_REMAP2=0
ccflags-y += -DINJECT_SYSCALLS=0
ccflags-y += -DBIG_PAGE
ccflags-y += -O2
ccflags-y += -fvisibility=hidden
ccflags-y += -fomit-frame-pointer
ccflags-y += -fno-pic
ccflags-y += -fno-stack-protector
ccflags-y += -Wno-error

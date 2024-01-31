KDIR ?= /usr/src/linux-headers-$(shell uname -r)/
obj-m += dmp.o
#dmp-objs += dm_stats.o
# ccflags-y := -std=gnu11 -Wall

.PHONY: all

all: modules

%:
	$(MAKE) -C $(KDIR) M=$(PWD) $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

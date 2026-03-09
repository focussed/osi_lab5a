# Makefile for memory driver - Lab 5
# Update KERNEL_VERSION to match your running kernel

# Get the currently running kernel version
KERNEL_VERSION = $(shell uname -r)
KERNEL_DIR = /usr/src/linux-headers-$(KERNEL_VERSION)

# If the above directory doesn't exist, try this alternative
# KERNEL_DIR = /lib/modules/$(KERNEL_VERSION)/build

obj-m += memory.o

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.order *.symvers

install:
	sudo insmod memory.ko

remove:
	sudo rmmod memory

info:
	@echo "Kernel version: $(KERNEL_VERSION)"
	@echo "Kernel directory: $(KERNEL_DIR)"
	@echo "Check if kernel headers are installed:"
	@ls -ld $(KERNEL_DIR) 2>/dev/null || echo "Kernel headers not found!"

test:
	@echo "Testing driver..."
	-sudo chmod 666 /dev/memory
	-echo -n "98765" > /dev/memory
	@echo "Write completed"
	-echo -n "Reading data: "
	-sudo dd if=/dev/memory bs=5 count=1 2>/dev/null
	@echo ""

.PHONY: all clean install remove info test
obj-m := si4463.o
si4463-objs := si4463_spidev.o si4463_api.o

KDIR :=/home/wu/Desktop/edison/kernel/linux-edison-standard-build

PWD := $(shell pwd)

EXTRA_CFLAGS += -DNOMIPS -fno-pic

default:
	$(MAKE)	-C $(KDIR) SUBDIRS=$(PWD) modules 
clean:
	rm *.o *.ko 
	rm *.mod.c

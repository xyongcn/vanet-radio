obj-m := si4463_tdma.o
si4463_tdma-objs := normal_si4463.o si4463/si4463_api.o  slots.o ringbuffer.o

KDIR :=/home/wu/Desktop/galileo/802.11p-linux

PWD := $(shell pwd)

EXTRA_CFLAGS += 


default:
	$(MAKE)	-C $(KDIR) SUBDIRS=$(PWD) modules 
clean:
	rm *.o *.ko rf212/*.o rf212/*.cmd si4463/*.o si4463/*.cmd
	rm *.mod.c
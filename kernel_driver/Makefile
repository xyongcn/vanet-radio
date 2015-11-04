#obj-m := rf.o
#rf-objs := radio_spidev.o si4463/si4463_api.o ringbuffer.o rf212/rf212_api.o rf212/tal_pib.o rf212/tal.o

obj-m := si4463_802154.o
si4463_802154-objs := 802154_si4463.o si4463/si4463_api.o 


KDIR :=/home/wu/Desktop/edison/kernel/linux-edison-standard-build

PWD := $(shell pwd)

EXTRA_CFLAGS += 


default:
	$(MAKE)	-C $(KDIR) SUBDIRS=$(PWD) modules 
clean:
	rm *.o *.ko rf212/*.o rf212/*.cmd si4463/*.o si4463/*.cmd
	rm *.mod.c

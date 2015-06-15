#include "si4463_api.h"
#include "cmd.h"
#include "si4463.h"

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/gpio.h>
#include <linux/delay.h>

extern struct spi_device *spi_save;
extern struct spidev_data spidev_global;
/*-------------------------------------------------------------------------*/

void getCTS(void) {
	u8 reply = 0x00;
	u8 request = 0x44;

	int i = 10;

	while (reply != 0xFF) {
		request = 0x44;

		spidev_sync_transfer(&spidev_global, &request, &reply, 1);

		if (reply != 0xFF)
			printk(KERN_ALERT "getCTS: %x\n", reply);

		if (i-- < 0)
			break;
	}
}
/*
 void getCTS(void){
 u8 reply = 0x00;
 u8 request = 0x44;
 while (reply != 0xFF)
 {
 request = 0x44;
 spi_write_then_read(spi_save, &request, 1, &reply, 1);
 if (reply != 0xFF)
 {
 printk(KERN_ALERT "getCTS: %x\n", reply);
 }
 }

 }*/

u8 * SendCmdReceiveAnswer(int byteCountTx, int byteCountRx, u8 * in_buff,
		u8 * out_buff) {

	char answer, i, j, k;
//发送命令
	printk(KERN_ALERT "Send CMD! \n");
	gpio_set_value(CS_SELF, 0);
	for (i=0; i<byteCountTx; i++){
		spidev_global.buffer = &(in_buff[i]);
		printk(KERN_ALERT "%x ", *(spidev_global.buffer));
		spidev_sync_write(&spidev_global, 1);
	}
	gpio_set_value(CS_SELF, 1);
//	spidev_sync_transfer(&spidev_global, in_buff, out_buff, byteCountTx);

//	ndelay(100);
	gpio_set_value(CS_SELF, 0);

	u8 reply = 0x00;
	u8 request = 0x44;

	j = 10;

	while (reply != 0xFF) {
		request = 0x44;

		spidev_sync_transfer(&spidev_global, &request, &reply, 1);

		if (reply != 0xFF){
			printk(KERN_ALERT "getCTS: %x\n", reply);
			gpio_set_value(CS_SELF, 1);
		//	spidev_sync_transfer(&spidev_global, in_buff, out_buff, byteCountTx);
			ndelay(100);
			gpio_set_value(CS_SELF, 0);
		}
		if (j-- < 0)
			break;
	}


//	spidev_global.buffer = out_buff;
//	si4463_sync_read(&spidev_global, byteCountRx);
//	spidev_sync_transfer(&spidev_global, in_buff, out_buff, byteCountRx);

	printk(KERN_ALERT "RECV CMD! \n");
	for (k=0; k<byteCountRx; k++){
		spidev_global.buffer = &(out_buff[k]);
		spidev_sync_read(&spidev_global, 1);

		printk(KERN_ALERT "%x ", *(spidev_global.buffer));
	}

	gpio_set_value(CS_SELF, 1);
	//ppp(out_buff, byteCountRx);

	return out_buff;
}

void reset(void) {
/*	gpio_set_value(RADIO_SDN, 1);
	ndelay(1000);
	gpio_set_value(RADIO_SDN, 0);
	mdelay(6);*/
	u8 buff[10];
//POWER_UP
//const unsigned char init_command[] = {0x02, 0x01, 0x01, x3, x2, x1, x0};// no patch, boot main app. img, FREQ_VCXO, return 1 byte
	const unsigned char init_command[] = { RF_POWER_UP };


	SendCmdReceiveAnswer(7, 1, init_command, buff);
	//ppp(buff, 1);



	const unsigned char PART_INFO_command[] = { 0x01 }; // Part Info
	SendCmdReceiveAnswer(1, 9, PART_INFO_command, buff);
	//ppp(buff, 9);


	const unsigned char get_int_status_command[] = { 0x20, 0x00, 0x00, 0x00 }; //  Clear all pending interrupts and get the interrupt status back
	SendCmdReceiveAnswer(4, 9, get_int_status_command, buff);
	//ppp(buff, 9);

//const char gpio_pin_cfg_command[] = {0x13, 0x02, 0x02, 0x02, 0x02, 0x08, 0x11, 0x00}; //  Set all GPIOs LOW; Link NIRQ to CTS; Link SDO to MISO; Max drive strength
	const unsigned char gpio_pin_cfg_command[] = { 0x13, 0x14, 0x02, 0x21, 0x20,
			0x27, 0x0b, 0x00 };
	SendCmdReceiveAnswer(8, 8, gpio_pin_cfg_command, buff);
	//ppp(buff, 8);

}

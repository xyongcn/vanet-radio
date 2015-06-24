#include "si4463_api.h"
#include "cmd.h"
#include "si4463.h"

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>



extern struct spi_device *spi_save;
extern struct spidev_data spidev_global;

const unsigned char tx_ph_data[19] = {'a','b','c','d','e',0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55};
//const unsigned char tx_ph_data[14] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};

int payload_length = 19;
int freq_channel = 0;
/*-------------------------------------------------------------------------*/

void getCTS(void) {
	u8 reply = 0x00;
	u8 request = 0x44;
	int j = 10;
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
/*
u8 * SendCmdReceiveAnswer(int byteCountTx, int byteCountRx, u8 * tx_buff,
		u8 * rx_buff) {
	int k,err;
	printk(KERN_ALERT "Send CMD! \n");

	ppp(tx_buff, byteCountTx);
	err = spi_write(spi_save, (void*) tx_buff, byteCountTx);
	if(err != 0){
		printk(KERN_ALERT "SendCmdReceiveAnswer: spi_write return errno: %d ", err);
	}
	getCTS();
	err = spi_read(spi_save, (void*) rx_buff, byteCountRx);
	if(err != 0){
		printk(KERN_ALERT "SendCmdReceiveAnswer: spi_read return errno: %d ", err);
	}

	printk(KERN_ALERT "RECV CMD! \n");
	for (k=0; k<byteCountRx; k++){
		printk(KERN_ALERT "%x ", rx_buff[k]);
	}
	printk(KERN_ALERT "\n");

	return rx_buff;
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
	printk(KERN_ALERT "RECV CMD! \n");
	for (k=0; k<byteCountRx; k++){
		spidev_global.buffer = &(out_buff[k]);
		spidev_sync_read(&spidev_global, 1);

		printk(KERN_ALERT "%x ", *(spidev_global.buffer));
	}
	gpio_set_value(CS_SELF, 1);
	return out_buff;
}


u8 * spi_write_cmd(int byteCountTx, u8 * in_buff) {
	SendCmdReceiveAnswer(byteCountTx, 0, in_buff, NULL);
}

void reset(void) {
/*	gpio_set_value(RADIO_SDN, 1);
	ndelay(1000);
	gpio_set_value(RADIO_SDN, 0);
	mdelay(6);*/
	u8 buff[10];
//POWER_UP
/*
	printk(KERN_ALERT "Set CS to 1 \n");
	gpio_set_value(CS_SELF, 1);
	printk(KERN_ALERT "get CS %d \n",gpio_get_value(CS_SELF));
	mdelay(5000);
	printk(KERN_ALERT "Set CS to 0 \n");
	gpio_set_value(CS_SELF, 0);
	printk(KERN_ALERT "get CS %d \n",gpio_get_value(CS_SELF));
	mdelay(5000);
	printk(KERN_ALERT "Set CS to 1 \n");
	gpio_set_value(CS_SELF, 1);
	printk(KERN_ALERT "get CS %d \n",gpio_get_value(CS_SELF));
	mdelay(5000);
	printk(KERN_ALERT "Set CS to 0 \n");
	gpio_set_value(CS_SELF, 0);
	printk(KERN_ALERT "get CS %d \n",gpio_get_value(CS_SELF));
*/
//const unsigned char init_command[] = {0x02, 0x01, 0x01, x3, x2, x1, x0};// no patch, boot main app. img, FREQ_VCXO, return 1 byte
	u8 init_command[] = { RF_POWER_UP };


	SendCmdReceiveAnswer(7, 1, init_command, buff);
	//ppp(buff, 1);



	u8 PART_INFO_command[] = { 0x01 }; // Part Info
	SendCmdReceiveAnswer(1, 9, PART_INFO_command, buff);
	//ppp(buff, 9);


	u8 get_int_status_command[] = { 0x20, 0x00, 0x00, 0x00 }; //  Clear all pending interrupts and get the interrupt status back
	SendCmdReceiveAnswer(4, 9, get_int_status_command, buff);
	//ppp(buff, 9);

//const char gpio_pin_cfg_command[] = {0x13, 0x02, 0x02, 0x02, 0x02, 0x08, 0x11, 0x00}; //  Set all GPIOs LOW; Link NIRQ to CTS; Link SDO to MISO; Max drive strength
	u8 gpio_pin_cfg_command[] = { 0x13, 0x14, 0x02, 0x21, 0x20,
			0x27, 0x0b, 0x00 };
	SendCmdReceiveAnswer(8, 8, gpio_pin_cfg_command, buff);
	//ppp(buff, 8);


}

void set_frr_ctl(void) {
  u8 p[8];
  p[0] = 0x11;
  p[1] = 0x02;
  p[2] = 0x04;
  p[3] = 0x00;
  p[4] = 0x0a; //frr_a
  p[5] = 0x02; //frr_b
  p[6] = 0x09; //frr_c
  p[7] = 0x00; //frr_d
  spi_write_cmd(8, p);
}

void si4463_init(void)
{
	u8 app_command_buf[20], i;

	//spi_write(0x07, RF_GPIO_PIN_CFG_data);
	app_command_buf[0] = 0x13;			// 设置GPIO口
	app_command_buf[1]  = 0x14; 		// gpio 0,接收数据输出
	app_command_buf[2]  = 0x02;    		// gpio 1,输出低电平
	app_command_buf[3]  = 0x21;  		// gpio 2,接收天线开关
	app_command_buf[4]  = 0x20; 		// gpio 3,发射天线开关
	app_command_buf[5]  = 0x27;   		// nIRQ
	app_command_buf[6]  = 0x0b;  		// sdo
	spi_write_cmd(7, app_command_buf);

	// spi_write(0x05, RF_GLOBAL_XO_TUNE_1_data);
	//调整晶振,应该可以使用默认值

	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x00;
	app_command_buf[2]  = 0x01;
	app_command_buf[3]  = 0x00;
	app_command_buf[4]  = 127;  			// 频率调整
	spi_write_cmd(5, app_command_buf);

	// spi_write(0x05, RF_GLOBAL_CONFIG_1_data);
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x00;
	app_command_buf[2]  = 0x01;
	app_command_buf[3]  = 0x03;
	app_command_buf[4]  = 0x40;  		// 收发缓冲区64字节,PH,高灵敏度模式
	spi_write_cmd(5, app_command_buf);

	spi_write_cmd(0x08, RF_FRR_CTL_A_MODE_4_data);

	// spi_write(0x0D, RF_PREAMBLE_TX_LENGTH_9_data); 	// 设置前导码
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x10;
	app_command_buf[2]  = 0x09;
	app_command_buf[3]  = 0x00;
	app_command_buf[4]  = 0x08;							// 8 bytes 前导码
	app_command_buf[5]  = 0x14;							// 检测 20 bits
	app_command_buf[6]  = 0x00;
	app_command_buf[7]  = 0x0f;
	app_command_buf[8]  = 0x31;  						// 无曼切斯顿编码.1010.
	app_command_buf[9]  = 0x00;
	app_command_buf[10]  = 0x00;
	app_command_buf[11]  = 0x00;
	app_command_buf[12]  = 0x00;
	spi_write_cmd(13, app_command_buf);

	//  RF_SYNC_CONFIG_5_data,							// 设置同步字
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x11;
	app_command_buf[2]  = 0x05;
	app_command_buf[3]  = 0x00;
	app_command_buf[4]  = 0x01;   						// 2 字节，无曼切斯顿编码
	app_command_buf[5]  = 0x2d;   						// 字节 3
	app_command_buf[6]  = 0xd4;							// 字节 2
	app_command_buf[7]  = 0x00;							// 字节 1
	app_command_buf[8]  = 0x00;							// 字节 0
	spi_write_cmd(9, app_command_buf);

	//  packet crc
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x12;
	app_command_buf[2]  = 0x01;
	app_command_buf[3]  = 0x00;
	app_command_buf[4]  = 0x81;							// CRC = itu-c, 使能crc
	spi_write_cmd(5, app_command_buf);

	// packet   gernale configuration
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x12;
	app_command_buf[2]  = 0x01;
	app_command_buf[3]  = 0x06;
	app_command_buf[4]  = 0x02;							// CRC MSB， data MSB
	spi_write_cmd(5, app_command_buf);

	// spi_write(0x07, RF_PKT_LEN_3_data);
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x12;
	app_command_buf[2]  = 0x03;
	app_command_buf[3]  = 0x08;
	app_command_buf[4]  = 0x00;
	app_command_buf[5]  = 0x00;
	app_command_buf[6]  = 0x00;
	spi_write_cmd(7, app_command_buf);

	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x12;
	app_command_buf[2]  = 0x0c;
	app_command_buf[3]  = 0x0d;
	app_command_buf[4]  = 0x00;
	app_command_buf[5]  = payload_length;
	app_command_buf[6]  = 0x04;
	app_command_buf[7]  = 0xaa;
	app_command_buf[8]  = 0x00;//0x05;    //field 2 length
	app_command_buf[9]  = 0x00;//0x04;
	app_command_buf[10]  = 0x00;//0xaa;
	app_command_buf[11]  = 0x00;
	app_command_buf[12]  = 0x00;
	app_command_buf[13]  = 0x00;
	app_command_buf[14]  = 0x00;
	app_command_buf[15]  = 0x00;
	spi_write_cmd(16, app_command_buf);						// 设置 Field 1--4长度

	// spi_write(0x0C, RF_PKT_FIELD_4_LENGTH_12_8_8_data);
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x12;
	app_command_buf[2]  = 0x08;
	app_command_buf[3]  = 0x19;
	app_command_buf[4]  = 0x00;
	app_command_buf[5]  = 0x00;
	app_command_buf[6]  = 0x00;
	app_command_buf[7]  = 0x00;
	app_command_buf[8]  = 0x00;
	app_command_buf[9]  = 0x00;
	app_command_buf[10]  = 0x00;
	app_command_buf[11]  = 0x00;
	spi_write_cmd(12, app_command_buf);

	spi_write_cmd(0x10, RF_MODEM_MOD_TYPE_12_data);
	spi_write_cmd(0x05, RF_MODEM_FREQ_DEV_0_1_data);		 // 以上参数根据WDS配置得出

	spi_write_cmd(0x0C, RF_MODEM_TX_RAMP_DELAY_8_data);
	spi_write_cmd(0x0D, RF_MODEM_BCR_OSR_1_9_data);
	spi_write_cmd(0x0B, RF_MODEM_AFC_GEAR_7_data);
	spi_write_cmd(0x05, RF_MODEM_AGC_CONTROL_1_data);
	spi_write_cmd(0x0D, RF_MODEM_AGC_WINDOW_SIZE_9_data);
	spi_write_cmd(0x0F, RF_MODEM_OOK_CNT1_11_data);			 // 以上参数根据WDS配置得出

	// spi_write(0x05, RF_MODEM_RSSI_COMP_1_data); //Modem
	app_command_buf[0] = 0x11;
	app_command_buf[1] = 0x20;
	app_command_buf[2] = 0x01;
	app_command_buf[3] = 0x4e;
	app_command_buf[4]  = 0x40;
	spi_write_cmd(5, app_command_buf);

	spi_write_cmd(0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_data);
	spi_write_cmd(0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_data);
	spi_write_cmd(0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_data);       // 以上参数根据WDS配置得出

	// RF_PA
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x22;
	app_command_buf[2]  = 0x04;
	app_command_buf[3]  = 0x00;
	app_command_buf[4]  = 0x08;
	app_command_buf[5]  = 0x1f;							// 设置最大功率
	app_command_buf[6]  = 0x00;
	app_command_buf[7]  = 0x3d;
	spi_write_cmd(8, app_command_buf);

	spi_write_cmd(0x0B, RF_SYNTH_PFDCP_CPFF_7_data);

	// header match
	app_command_buf[0] = 0x11;
	app_command_buf[1]  = 0x30;
	app_command_buf[2]  = 0x0c;
	app_command_buf[3]  = 0x00;
	app_command_buf[4]  = 'a';
	app_command_buf[5]  = 0xff;
	//     if(ss_pin == 1)
	app_command_buf[6]  = 0x40;
	//      else
	//        app_command_buf[6]  = 0x40;
	app_command_buf[7]  = 'b';
	app_command_buf[8]  = 0xff;
	app_command_buf[9]  = 0x01;
	app_command_buf[10] = 'c';
	app_command_buf[11]  = 0xff;
	app_command_buf[12]  = 0x02;
	app_command_buf[13]  = 'd';
	app_command_buf[14]  = 0xff;
	app_command_buf[15]  = 0x03;
	spi_write_cmd(16, app_command_buf); 					// 配置头码

	spi_write_cmd(5, RF_MODEM_CLKGEN_BAND_1_data);
	spi_write_cmd(12, RF_FREQ_CONTROL_INTE_8_data); 	    // 设置频率

	set_frr_ctl();
}

void fifo_reset(void)			// 复位发射和接收 FIFO
{
	unsigned char p[2];

	p[0] = FIFO_INFO;
	p[1] = 0x03;
	//  SendCmdReceiveAnswer(2, 3, p);
	spi_write_cmd(2, p);
}

void clr_interrupt(void)		// 清中断标志
{
	unsigned char p[4];

	p[0] = GET_INT_STATUS;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	//SendCmdReceiveAnswer(4,9,p);
	spi_write_cmd(4, p);
	//spi_read(9,GET_INT_STATUS);
}

void enable_tx_interrupt(void)		// 使能发射中断
{
	unsigned char p[6];

	p[0] = 0x11;
	p[1] = 0x01;
	p[2] = 0x02;
	p[3] = 0x00;
	p[4] = 0x01;
	p[5] = 0x20;
	spi_write_cmd(6, p);
}

void enable_rx_interrupt(void)		// 使能接收中断
{
	unsigned char p[7];

	p[0] = 0x11;
	p[1] = 0x01;
	p[2] = 0x03;
	p[3] = 0x00;
	p[4] = 0x03;
	p[5] = 0x18;
	p[6] = 0x00;
	spi_write_cmd(0x07, p);
}

void tx_start(void)					// 开始发射
{
	unsigned char p[5];

	p[0] = START_TX ;
	p[1] = freq_channel ;  			// 通道0
	p[2] = 0x50;
	p[3] = 0;
	p[4] = 0;
	spi_write_cmd(5, p);
}

void tx_start_1B(void)					// 开始发射
{
	unsigned char p[1];

	p[0] = START_TX ;
	spi_write_cmd(1, p);
}

void rx_start(void)					// 开始接收
{
	unsigned char p[8];

	p[0] = START_RX ;
	p[1] = freq_channel ; 			// 通道0
	p[2] = 0x00;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0x08;
	p[6] = 0x08;
	p[7] = 0x08;
	spi_write_cmd(8, p);
}

void spi_write_fifo(unsigned char * data, int len) {
	int j;
	gpio_set_value(CS_SELF, 0);
	u8 cmd = WRITE_TX_FIFO;
	spidev_global.buffer = &cmd;
	spidev_sync_write(&spidev_global, 1);
	for (j = 0; j < len; j++) {
		cmd = data[j];
		spidev_sync_write(&spidev_global, 1);
	}
	gpio_set_value(CS_SELF, 1);
}

void spi_read_fifo(unsigned char * st, int len) {
	int j;
	gpio_set_value(CS_SELF, 0);
	u8 cmd = READ_RX_FIFO;
	u8 ret;
	spidev_global.buffer = &cmd;
	spidev_sync_write(&spidev_global, 1);
	cmd = 0x00;

	for (j = 0; j < len; j++) {
		spidev_sync_transfer(&spidev_global, &cmd, &(st[j]),  1);

	}
	gpio_set_value(CS_SELF, 1);

	//Serial.println();
	//  unsigned char p[] = {READ_RX_FIFO};
	//  SendCmdReceiveAnswer(1,payload_length,p);
}

void get_fifo_info(void)			// 复位发射和接收 FIFO
{
	u8 rx[10];
	unsigned char p[2];

	p[0] = FIFO_INFO;
	p[1] = 0x00;
	SendCmdReceiveAnswer(2, 3, p, rx);
	//	spi_write(2,p);
}

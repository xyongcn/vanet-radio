#include "si4463_api.h"
#include "cmd.h"
#include "si4463.h"

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/wait.h>


extern struct spi_device *spi_save;
extern struct spidev_data spidev_global;
extern wait_queue_head_t spi_wait_queue;

DEFINE_MUTEX(mutex_spi);

//extern struct task_struct *cmd_handler_tsk;

//const unsigned char tx_ph_data[19] = {'a','b','c','d','e',0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55};
//const unsigned char tx_ph_data[14] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};

int payload_length = 64;
int freq_channel = 0;

static u8 config_table[] = RADIO_CONFIGURATION_DATA_ARRAY;
/*-------------------------------------------------------------------------*/
void cs_low(void){
	gpio_set_value(CS_SELF, 0);
}
void cs_high(void){
	gpio_set_value(CS_SELF, 1);
}
int get_CCA_latch(void){
	return gpio_get_value(GPIO0);
}

bool get_CCA(void){
	return gpio_get_value(GPIO0)>0 ? 1 : 0;
}

static inline void getCTS(void) {
	u8 reply = 0x00;
	u8 request = 0x44;
	int j = 25;
	while (reply != 0xFF) {
		request = 0x44;

		spidev_sync_transfer(&spidev_global, &request, &reply, 1);

		if (reply != 0xFF){
			printk(KERN_ALERT "getCTS: %x\n", reply);
			cs_high();
		//	spidev_sync_transfer(&spidev_global, in_buff, out_buff, byteCountTx);
			ndelay(100);
			cs_low();
		}
		if (j-- < 0)
			break;
	}
}

static inline int getCTS_gpio(void) {
	int count = 2000;
	while (count-- && !gpio_get_value(GPIO1)){
		ndelay(10);
	}
	if (count <= 0){
		printk(KERN_ALERT "getCTS wrong!!!");
		return 0;
	}
	return 1;

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
	/* TEST */
//	if (byteCountTx == 1)
//		byteCountTx++;

	char answer, i, j, k;
//发送命令
	//printk(KERN_ALERT "Send CMD! \n");


	mutex_lock(&mutex_spi);

	cs_low();
	for (i=0; i<byteCountTx; i++){
		spidev_global.buffer = &(in_buff[i]);
		//printk(KERN_ALERT "%x ", *(spidev_global.buffer));
		spidev_sync_write(&spidev_global, 1);
	}

	cs_high();

	if(byteCountRx == 0) {
		mutex_unlock(&mutex_spi);
		return NULL;
	}

//	ndelay(100);
	ndelay(10);
	cs_low();

//	if(!getCTS_gpio())
		getCTS();

	for (k=0; k<byteCountRx; k++){
		spidev_global.buffer = &(out_buff[k]);
		spidev_sync_read(&spidev_global, 1);

//		printk(KERN_ALERT "%x ", *(spidev_global.buffer));
	}
	cs_high();
	mutex_unlock(&mutex_spi);
	return out_buff;
}
/*
int spi_write_cmd_async(int byteCountTx, u8 * tx_buff) {

	char answer, i, j, k;
	int status;
//发送命令
	//printk(KERN_ALERT "Send CMD! \n");
	cs_low();
	for (i=0; i<byteCountTx; i++){
		spidev_global.buffer = &(tx_buff[i]);
		//printk(KERN_ALERT "%x ", *(spidev_global.buffer));
		status = spidev_sync_write_nosleep(&spidev_global, 1);
	}
	cs_high();
	return status;
}
*/

u8 * spi_write_cmd(int byteCountTx, u8 * in_buff) {
	return SendCmdReceiveAnswer(byteCountTx, 0, in_buff, NULL);
}



void reset(void) {
	gpio_set_value(RADIO_SDN, 1);
	mdelay(10);
	gpio_set_value(RADIO_SDN, 0);
	mdelay(10);
	u8 buff[10];
//POWER_UP
/*
	printk(KERN_ALERT "Set CS to 1 \n");
	cs_high();
	printk(KERN_ALERT "get CS %d \n",gpio_get_value(CS_SELF));
	mdelay(5000);
	printk(KERN_ALERT "Set CS to 0 \n");
	cs_low();
	printk(KERN_ALERT "get CS %d \n",gpio_get_value(CS_SELF));
	mdelay(5000);
	printk(KERN_ALERT "Set CS to 1 \n");
	cs_high();
	printk(KERN_ALERT "get CS %d \n",gpio_get_value(CS_SELF));
	mdelay(5000);
	printk(KERN_ALERT "Set CS to 0 \n");
	cs_low();
	printk(KERN_ALERT "get CS %d \n",gpio_get_value(CS_SELF));
*/
//const unsigned char init_command[] = {0x02, 0x01, 0x01, x3, x2, x1, x0};// no patch, boot main app. img, FREQ_VCXO, return 1 byte
	u8 init_command[] = { RF_POWER_UP };
	SendCmdReceiveAnswer(7, 1, init_command, buff);
	mdelay(20);
	//ppp(buff, 1);
	SendCmdReceiveAnswer(7, 1, init_command, buff);
	mdelay(20);

	u8 PART_INFO_command[] = { 0x01 }; // Part Info
	SendCmdReceiveAnswer(1, 9, PART_INFO_command, buff);
	//ppp(buff, 9);

	u8 get_int_status_command[] = { 0x20, 0x00, 0x00, 0x00 }; //  Clear all pending interrupts and get the interrupt status back
	SendCmdReceiveAnswer(4, 9, get_int_status_command, buff);
	//ppp(buff, 9);

//const char gpio_pin_cfg_command[] = {0x13, 0x02, 0x02, 0x02, 0x02, 0x08, 0x11, 0x00}; //  Set all GPIOs LOW; Link NIRQ to CTS; Link SDO to MISO; Max drive strength
//	u8 gpio_pin_cfg_command[] = { 0x13, 0x14, 0x02, 0x21, 0x20,
//			0x27, 0x0b, 0x00 };
//	SendCmdReceiveAnswer(8, 8, gpio_pin_cfg_command, buff);
//	//ppp(buff, 8);
//

	setRFParameters();
//	Function_set_tran_property();
	fifo_reset();

}

void set_frr_ctl(void) {
  u8 p[8];
  p[0] = 0x11;
  p[1] = 0x02;
  p[2] = 0x04;
  p[3] = 0x00;
  p[4] = 0x04; //frr_a, INT_PH_PEND: Packet Handler status pending.
  p[5] = 0x02; //frr_b
  p[6] = 0x09; //frr_c
  p[7] = 0x00; //frr_d
  spi_write_cmd(8, p);
}
/*
void si4463_register_init(void)
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

//	spi_write_cmd(0x08, RF_FRR_CTL_A_MODE_4_data);

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
	app_command_buf[6]  = 0x00;
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

	set_frr_ctl(); //set frr register
}
*/
/***********************************************
* 函数名：SetRFParameters
* 功能：  射频参数设置
************************************************/
void setRFParameters(void)  //设置RF参数
{
    u8 i;
    int j = 0;

    while((i = config_table[j]) != 0)
    {
        j += 1;
        spi_write_cmd(i, config_table + j);
        j += i;
    }

    set_frr_ctl(); //set frr register
}


void Function_set_tran_property(){
	u8 abApi_Write[10];
	// Enable the proper interrupts
//	abApi_Write[0] = CMD_SET_PROPERTY;        // Use property command
//	abApi_Write[1] = PROP_INT_CTL_GROUP;      // Select property group
//	abApi_Write[2] = 4;               // Number of properties to be written
//	abApi_Write[3] = PROP_INT_CTL_ENABLE;     // Specify property
//	abApi_Write[4] = 0x01;              // INT_CTL: PH interrupt enable
//	abApi_Write[5] = 0x3B;              // INT_CTL_PH: PH PACKET_SENT, PACKET_RX, CRC2_ERR, TX_FIFO_ALMOST_EMPTY, RX_FIFO_ALMOST_FULL enabled
//	abApi_Write[6] = 0x00;              // INT_CTL_MODEM: -
//	abApi_Write[7] = 0x00;              // INT_CTL_CHIP_EN: -
//	bApi_SendCommand(8,abApi_Write);        // Send API command to the radio IC
//	vApi_WaitforCTS();                // Wait for CTS

	// General packet config (set bit order)
	abApi_Write[0] = CMD_SET_PROPERTY;        // Use property command
	abApi_Write[1] = PROP_PKT_GROUP;        // Select property group
	abApi_Write[2] = 1;               // Number of properties to be written
	abApi_Write[3] = PROP_PKT_CONFIG1;        // Specify property
	abApi_Write[4] = 0x80;              // Separate RX and TX FIELD properties are used, payload data goes MSB first
	spi_write_cmd(5,abApi_Write);        // Send command to the radio IC
//	vApi_WaitforCTS();                // Wait for CTS

	// Set variable packet length
	abApi_Write[0] = CMD_SET_PROPERTY;        // Use property command
	abApi_Write[1] = PROP_PKT_GROUP;        // Select property group
	abApi_Write[2] = 3;               // Number of properties to be written
	abApi_Write[3] = PROP_PKT_LEN;          // Specify property, PKT_LEN.ENDIAN=1, MSB byte first; PKT_LEN.SIZE=1,two bytes in length; PKT_LEN.IN_FIFO=0, length is not in RX FIFO.
	abApi_Write[4] = 0x22;              // PKT_LEN: length is put in the Rx FIFO, FIELD 2 is used for the payload (with variable length)
	abApi_Write[5] = 0x01;              // PKT_LEN_FIELD_SOURCE: FIELD 1 is used for the length information
	abApi_Write[6] = 0x00;              // PKT_LEN_ADJUST: no adjustment (FIELD 1 determines the actual payload length)
	spi_write_cmd(7,abApi_Write);        // Send command to the radio IC
//	vApi_WaitforCTS();                // Wait for CTS

	// Set packet fields for Tx (one field is used)
	abApi_Write[0] = CMD_SET_PROPERTY;        // Use property command
	abApi_Write[1] = PROP_PKT_GROUP;        // Select property group
	abApi_Write[2] = 4;               // Number of properties to be written
	abApi_Write[3] = PROP_PKT_FIELD_1_LENGTH_12_8;  // Specify first property
	abApi_Write[4] = 0x00;              // PKT_FIELD_1_LENGTH_12_8: defined later (under bSendPacket() )
	abApi_Write[5] = 0x00;              // PKT_FIELD_1_LENGTH_7_0: defined later (under bSendPacket() )
	abApi_Write[6] = 0x00;              // PKT_FIELD_1_CONFIG : No 4(G)FSK/Whitening/Manchester coding for this field
	abApi_Write[7] = 0xA2;              // PKT_FIELD_1_CRC_CONFIG: Start CRC calc. from this field, send CRC at the end of the field
	spi_write_cmd(8,abApi_Write);        // Send command to the radio IC
//	vApi_WaitforCTS();                // Wait for CTS

	// Set packet fields for Rx (two fields are used)
	// FIELD1 is fixed, 2byte length, used for PKT_LEN
	abApi_Write[0] = CMD_SET_PROPERTY;        // Use property command
	abApi_Write[1] = PROP_PKT_GROUP;        // Select property group
	abApi_Write[2] = 4;               // Number of properties to be written
	abApi_Write[3] = PROP_PKT_RX_FIELD_1_LENGTH_12_8; // Specify first property
	abApi_Write[4] = 0x00;              // PKT_RX_FIELD_1_LENGTH_12_8: 1 byte (containing variable packet length info)
	abApi_Write[5] = 0x02;              // PKT_RX_FIELD_1_LENGTH_7_0: 2 byte (containing variable packet length info)
	abApi_Write[6] = 0x00;              // PKT_RX_FIELD_1_CONFIG: No 4(G)FSK/Whitening/Manchester coding for this field
	abApi_Write[7] = 0x82;              // PKT_RX_FIELD_1_CRC_CONFIG: Start CRC calc. from this field, enable CRC calc.
	spi_write_cmd(8,abApi_Write);        // Send command to the radio IC
	// FIELD2 is variable length, contains the actual payload
//	vApi_WaitforCTS();                // Wait for CTS

	abApi_Write[0] = CMD_SET_PROPERTY;        // Use property command
	abApi_Write[1] = PROP_PKT_GROUP;        // Select property group
	abApi_Write[2] = 4;               // Number of properties to be written
	abApi_Write[3] = PROP_PKT_RX_FIELD_2_LENGTH_12_8; // Specify first property
	abApi_Write[4] = (MAX_PACKET_LENGTH >> 8) & 0xFF;     // PKT_RX_FIELD_2_LENGTH_12_8: max. field length (variable packet length)
	abApi_Write[5] = (MAX_PACKET_LENGTH >> 0) & 0xFF;     // PKT_RX_FIELD_2_LENGTH_7_0: max. field length (variable packet length)
	abApi_Write[6] = 0x00;              // PKT_RX_FIELD_2_CONFIG: No 4(G)FSK/Whitening/Manchester coding for this field
	abApi_Write[7] = 0x0A;              // PKT_RX_FIELD_2_CRC_CONFIG: check CRC at the end, enable CRC calc.
	spi_write_cmd(8,abApi_Write);        // Send command to the radio IC
//	vApi_WaitforCTS();                // Wait for CTS

	// Configure CRC polynomial and seed
//	abApi_Write[0] = CMD_SET_PROPERTY;        // Use property command
//	abApi_Write[1] = PROP_PKT_GROUP;        // Select property group
//	abApi_Write[2] = 1;               // Number of properties to be written
//	abApi_Write[3] = PROP_PKT_CRC_CONFIG;     // Specify property
//	abApi_Write[4] = 0x83;              // CRC seed: all `1`s, poly: No. 3, 16bit, Baicheva-16
//	spi_write_cmd(5,abApi_Write);        // Send command to the radio IC
//	vApi_WaitforCTS();                // Wait for CTS

	// Set the TX FIFO alsmot empty threshold and RX FIFO alsmost full threshold
//	abApi_Write[0] = CMD_SET_PROPERTY;	// Use property command
//	abApi_Write[1] = PROP_PKT_GROUP;		// Select property group
//	abApi_Write[2] = 2;				    // Number of properties to be written
//	abApi_Write[3] = PROP_PKT_TX_THRESHOLD;	      // Specify property
//	abApi_Write[4] = TX_THRESHOLD;
//	abApi_Write[5] = RX_THRESHOLD;
//	spi_write_cmd(6,abApi_Write);		// Send command to the radio IC
//	vApi_WaitforCTS();					// Wait for CTS
}


void fifo_reset(void)			// 复位发射和接收 FIFO
{
	unsigned char p[2];

	p[0] = FIFO_INFO;
	p[1] = 0x03;
	//  SendCmdReceiveAnswer(2, 3, p);
	spi_write_cmd(2, p);
}

void tx_fifo_reset(void)
{
	unsigned char p[2];

	p[0] = FIFO_INFO;
	p[1] = 0x01;
	//  SendCmdReceiveAnswer(2, 3, p);
	spi_write_cmd(2, p);
}

void clr_packet_sent_pend(void) {
	u8 p[2];
	u8 p2[3];
	p[0] = GET_PH_STATUS;
//	p[1] = 0xdf;
	p[1] = 0x10;//leave the PACKET_RX_PEND_CLR alone
	spi_write_cmd(2, p);
//	SendCmdReceiveAnswer(2,3,p,p2);
}

void clr_packet_rx_pend(void) {
	u8 p[2];
	u8 p2[3];
	p[0] = GET_PH_STATUS;
//	p[1] = 0xef;
	p[1] = 0x20;//leave the PACKET_SENT_PEND_CLR alone
//	spi_write_cmd(2, p);
	SendCmdReceiveAnswer(2,3,p,p2);
}

void clr_preamble_detect_pend(void){
	u8 p[2];
	p[0] = GET_MODEM_STATUS;
	p[1] = 0xfd;
	spi_write_cmd(2, p);
//	SendCmdReceiveAnswer(2,3,p,p2);
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

void get_interrupt_status(void)		// 中断
{
	unsigned char p[4];
	unsigned char p2[10];

	p[0] = GET_INT_STATUS;
	p[1] = 0xfb;
	p[2] = 0x7f;
	p[3] = 0x7f;
	SendCmdReceiveAnswer(4,9,p,p2);
//	spi_write_cmd(4, p);
	//spi_read(9,GET_INT_STATUS);
}

u16 get_packet_info()
{
	unsigned char p[6];
	unsigned char p2[3];
	u16 bLen;

	p[0] = CMD_PACKET_INFO;
	p[1] = 0x00;
	p[2] = 0x00;
	p[3] = 0x00;
	p[4] = 0x00;
	p[5] = 0x00;
	SendCmdReceiveAnswer(1,3,p,p2);
	bLen = ((u16)p2[1] << 8) | (u16)p2[2];
	return bLen;
}
/*
void clr_interrupt_nosleep(void)		// 清中断标志
{
	unsigned char p[4];

	p[0] = GET_INT_STATUS;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	//SendCmdReceiveAnswer(4,9,p);
	spi_write_cmd_nosleep(4, p);
	//spi_read(9,GET_INT_STATUS);
}
*/

/**
 * 0x00 INT_CTL_ENABLE: PH_INT_STATUS_EN
 * 0x01 INT_CTL_PH_ENABLE: PACKET_SENT_EN, PACKET_RX_EN, CRC_ERROR_EN
 * 0x02
 */
void enable_chip_irq(void) {
	unsigned char p[6];
	p[0] = 0x11;
	p[1] = 0x01;
	p[2] = 0x02;
	p[3] = 0x00;
	p[4] = 0x01;
	p[5] = 0x38;
	spi_write_cmd(6, p);
}

//void enable_tx_interrupt(void)		// 使能发射中断
//{
//	unsigned char p[6];
//
//	p[0] = 0x11;
//	p[1] = 0x01;
//	p[2] = 0x02;
//	p[3] = 0x00;
//	p[4] = 0x01;
//	p[5] = 0x20;
//	spi_write_cmd(6, p);
//}
//
//void enable_rx_interrupt(void)		// 使能接收中断
//{
//	unsigned char p[7];
//
//	p[0] = 0x11;
//	p[1] = 0x01;
//	p[2] = 0x03;
//	p[3] = 0x00;
//	p[4] = 0x03;
//	p[5] = 0x18;
//	p[6] = 0x00;
//	spi_write_cmd(7, p);
//}

void tx_start(void)					// 开始发射
{
	unsigned char p[5];

	p[0] = START_TX ;
	p[1] = freq_channel ;  			// 通道0

//	p[2] = 0x50;//TX_TUNE
//	p[2] = 0x60;//RX_TUNE
	p[2] = 0x30;//ready

	p[3] = 0x00;
	p[4] = 0x40;
	spi_write_cmd(5, p);
}

void tx_start_1B(void)					// 开始发射
{
	unsigned char p[1];

	p[0] = START_TX ;
	spi_write_cmd(1, p);
}

void tx_change_variable_len(u16 len)
{
	unsigned char p[6];
	// Set TX packet length
	p[0] = 0x11;        // Use property command
	p[1] = 0x12;          // Select property group
	p[2] =1;                       // Number of properties to be written
	p[3] = PROP_PKT_FIELD_1_LENGTH_7_0 ;            // Specify first property
	p[4] = len & 0xFF;  // Field length (variable packet length info)
//	p[5] = (len >> 0) & 0xFF;
//	printk(KERN_ALERT "tx_change_variable_len:p[4]:%x, p[5]:%x\n", p[4],p[5]);
	spi_write_cmd(5, p);
}

void rx_start(void)					// 开始接收
{
	unsigned char p[8];

	p[0] = START_RX ;
	p[1] = freq_channel ; 			// 通道0
	p[2] = 0x00;
	p[3] = 0x00;
	p[4] = 0x40;
	p[5] = 0x00;
	p[6] = 0x03;
	p[7] = 0x03;
	spi_write_cmd(8, p);///5
}

void change_state2tx_tune(void){
	unsigned char p[2];
	p[0] = CHANGE_STATE;
	p[1] = 0x05;//TX_TUNE
	spi_write_cmd(2, p);
}

void spi_write_fifo(unsigned char * data, int len) {
	int j;

	mutex_lock(&mutex_spi);

	cs_low();
	u8 cmd = WRITE_TX_FIFO;
	spidev_global.buffer = &cmd;
	spidev_sync_write(&spidev_global, 1);
	for (j = 0; j < len; j++) {
		cmd = data[j];
		spidev_sync_write(&spidev_global, 1);
	}
	cs_high();

	mutex_unlock(&mutex_spi);
}

void spi_read_fifo(unsigned char * st, int len) {
	int j;

	mutex_lock(&mutex_spi);

	cs_low();
	u8 cmd = READ_RX_FIFO;
//	u8 ret;
	spidev_global.buffer = &cmd;
	spidev_sync_write(&spidev_global, 1);
	cmd = 0xff;

	for (j = 0; j < len; j++) {
		spidev_sync_transfer(&spidev_global, &cmd, &(st[j]),  1);

	}
	cs_high();
	mutex_unlock(&mutex_spi);

	//Serial.println();
	//  unsigned char p[] = {READ_RX_FIFO};
	//  SendCmdReceiveAnswer(1,payload_length,p);
}

void get_fifo_info(u8 *rx)			// 复位发射和接收 FIFO
{
//	u8 rx[10];
	unsigned char p[2];

	p[0] = FIFO_INFO;
	p[1] = 0x00;
	SendCmdReceiveAnswer(2, 3, p, rx);
	//	spi_write(2,p);
}

void get_ph_status(u8 *rx){
	unsigned char p[2];

	p[0] = GET_PH_STATUS;
	p[1] = 0xff;
	SendCmdReceiveAnswer(2, 3, p, rx);
}

void get_modem_status(u8 *rx){
	unsigned char p[2];

	p[0] = GET_MODEM_STATUS;
	p[1] = 0xff;
	SendCmdReceiveAnswer(2, 4, p, rx);
}


void read_frr_a(u8 *value) {
	u8 cmd;
	u8 rx[3];
	int j;
	cmd = FRR_A_READ;

	mutex_lock(&mutex_spi);
	cs_low();
	spidev_global.buffer = &cmd;
	spidev_sync_write(&spidev_global, 1);
	cmd = 0x00;
	spidev_sync_transfer(&spidev_global, &cmd, value,  1);
	cs_high();
	mutex_unlock(&mutex_spi);
/*
	for(j=5; j>=0 && (*value == 0 || *value == 0xff); j--)
	{
//		mutex_lock(&mutex_spi);
//		get_ph_status(rx);
//		mutex_unlock(&mutex_spi);
//		*value = rx[1];
		printk(KERN_ALERT "read frra retry!\n");
		cmd = FRR_A_READ;

		mutex_lock(&mutex_spi);
		cs_low();
		spidev_global.buffer = &cmd;
		spidev_sync_write(&spidev_global, 1);
		cmd = 0x00;
		spidev_sync_transfer(&spidev_global, &cmd, value,  1);
		cs_high();
		mutex_unlock(&mutex_spi);

	}
*/
	return;
}

void read_frr_b(u8 *value) {
	u8 cmd;
	u8 rx[3];
	int j;
	cmd = FRR_B_READ;

	mutex_lock(&mutex_spi);
	cs_low();
	spidev_global.buffer = &cmd;
	spidev_sync_write(&spidev_global, 1);
	cmd = 0x00;
	spidev_sync_transfer(&spidev_global, &cmd, value,  1);
	cs_high();
	mutex_unlock(&mutex_spi);

//	while(*value == 0 || *value == 0xff)
//	{
//		mutex_lock(&mutex_spi);
//		get_ph_status(rx);
//		mutex_unlock(&mutex_spi);
//		*value = rx[1];
//	}

}

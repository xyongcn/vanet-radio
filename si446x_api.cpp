#include <SPI.h>


/* pecanpico2 Si446x Driver copyright (C) 2013  KT5TK
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "config.h"
#include <math.h>
#include "si446x_api.h"
#include "radio_config_si4463.h"




#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#define NOP 				0x00 
#define PART_INFO                       0x01
#define FUNC_INFO                       0x10
#define SET_PROPERTY                    0x11 
#define GET_PROPERTY                    0x12 
#define GPIO_PIN_CFG                    0x13
#define GET_ADC_READING                 0x14 
#define FIFO_INFO                       0x15
#define PACKET_INFO                     0x16
#define IRCAL                           0x17 
#define PROTOCOL_CFG                    0x18 
#define GET_INT_STATUS                  0x20
#define GET_PH_STATUS                   0x21
#define GET_MODEM_STATUS                0x22
#define GET_CHIP_STATUS                 0x23
#define START_TX                        0x31 
#define START_RX                        0x32 
#define REQUEST_DEVICE_STATE             0x33
#define CHANGE_STATE                    0x34 
#define READ_CMD_BUFF                   0x44 
#define FRR_A_READ                      0x50
#define FRR_B_READ                      0x51 
#define FRR_C_READ                      0x53 
#define FRR_D_READ                      0x57 
#define WRITE_TX_FIFO                   0x66 
#define READ_RX_FIFO                    0x77 
#define START_MFSK                      0x35 
#define RX_HOP                          0x36 

#define payload_length  				14
#define freq_channel		0 

const unsigned char RF_MODEM_CLKGEN_BAND_1_data[] = 		{RF_MODEM_CLKGEN_BAND_1};
const unsigned char RF_FREQ_CONTROL_INTE_8_data[] = 		{RF_FREQ_CONTROL_INTE_8};
const unsigned char RF_POWER_UP_data[] = 			   		{ RF_POWER_UP};
const unsigned char RF_GPIO_PIN_CFG_data[] = 			   	{ RF_GPIO_PIN_CFG}; 
const unsigned char RF_GLOBAL_XO_TUNE_1_data[] = 		   	{ RF_GLOBAL_XO_TUNE_1};
const unsigned char RF_GLOBAL_CONFIG_1_data[] = 		   	{ RF_GLOBAL_CONFIG_1}; 
const unsigned char RF_FRR_CTL_A_MODE_4_data[] = 		   	{ RF_FRR_CTL_A_MODE_4};
const unsigned char RF_PREAMBLE_TX_LENGTH_9_data[] = 		{ RF_PREAMBLE_TX_LENGTH_9};
const unsigned char RF_SYNC_CONFIG_5_data[] = 		 	   	{ RF_SYNC_CONFIG_5};
const unsigned char RF_PKT_CRC_CONFIG_1_data[] = 		   	{ RF_PKT_CRC_CONFIG_1};
const unsigned char RF_PKT_CONFIG1_1_data[] = 			   	{ RF_PKT_CONFIG1_1};
const unsigned char RF_PKT_LEN_3_data[] = 			   		{ RF_PKT_LEN_3};
const unsigned char RF_PKT_FIELD_1_LENGTH_12_8_12_data[] =	{ RF_PKT_FIELD_1_LENGTH_12_8_12};
const unsigned char RF_PKT_FIELD_4_LENGTH_12_8_8_data[] = 	{ RF_PKT_FIELD_4_LENGTH_12_8_8};
const unsigned char RF_MODEM_FREQ_DEV_0_1_data[] = 		   	{ RF_MODEM_FREQ_DEV_0_1};
const unsigned char RF_MODEM_AGC_CONTROL_1_data[] = 		{ RF_MODEM_AGC_CONTROL_1};
const unsigned char RF_MATCH_VALUE_1_12_data[] =            { RF_MATCH_VALUE_1_12};
const unsigned char RF_MODEM_RSSI_COMP_1_data[] = 			{ RF_MODEM_RSSI_COMP_1};
const unsigned char RF_MODEM_MOD_TYPE_12_data[]=			{RF_MODEM_MOD_TYPE_12};
const unsigned char RF_MODEM_TX_RAMP_DELAY_8_data[]=				{RF_MODEM_TX_RAMP_DELAY_8};
const unsigned char RF_MODEM_BCR_OSR_1_9_data[]=					{RF_MODEM_BCR_OSR_1_9};
const unsigned char RF_MODEM_AFC_GEAR_7_data[]=						{RF_MODEM_AFC_GEAR_7};
const unsigned char RF_MODEM_AGC_WINDOW_SIZE_9_data[]=				{RF_MODEM_AGC_WINDOW_SIZE_9};
const unsigned char RF_MODEM_OOK_CNT1_11_data[]=					{RF_MODEM_OOK_CNT1_11};
const unsigned char RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_data[]=	{RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12};
const unsigned char RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_data[]=	{RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12};
const unsigned char RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_data[]=	{RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12};
const unsigned char RF_SYNTH_PFDCP_CPFF_7_data[]=					{RF_SYNTH_PFDCP_CPFF_7};

const unsigned char tx_test_aa_data[14] = {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa};  
//const unsigned char tx_ph_data[14] = {'s','w','w','x',0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x6d};  // 每秒发射的固定内容的测试信号，第10个数据是前9个数据的校验和
const unsigned char tx_ph_data[19] = {'a','b','c','d','e',0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55};
//const unsigned char tx_ph_data[14] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};

unsigned int si446x_powerlevel = 0;
unsigned long active_freq = RADIO_FREQUENCY;

//#include <SoftwareSerial.h>
//SoftwareSerial mySerial(ADC2_PIN, ADC1_PIN); // RX, TX



//#include <SoftwareSerial.h>
//SoftwareSerial mySerial(ADC2_PIN, ADC1_PIN); // RX, TX

RadioSi446x::RadioSi446x(int SS){
  if (SS==0) {
    ss_pin = SSpin;
    nirq_pin = NIRQ;
  }else if (SS==1) {
    ss_pin = SSpin1;
    nirq_pin = NIRQ1;
  }else {
    ss_pin = 999;
    nirq_pin = 999;
  }
}


void RadioSi446x::SendCmdReceiveAnswer(int byteCountTx, int byteCountRx, const unsigned char* pData)
{
    
    /* There was a bug in A1 hardware that will not handle 1 byte commands. 
       It was supposedly fixed in B0 but the fix didn't make it at the last minute, so here we go again */
 //   if (byteCountTx == 1)
  //      byteCountTx++;
    
    digitalWrite(ss_pin,LOW);
    char answer;   
    
//    Serial.print("sent: ");
    
//    for (int j = 0; j < byteCountTx; j++) // Loop through the bytes of the pData
//    {
//      byte wordb = pData[j];
//      Serial.print(wordb,HEX);
//      Serial.print(" ");
//    } 
    
//    Serial.println();
    
    //发送命令
    for (int j = 0; j < byteCountTx; j++) // Loop through the bytes of the pData
    {
      byte wordb = pData[j];
      SPI.transfer(wordb);  
    } 
    
    digitalWrite(ss_pin,HIGH);

    delayMicroseconds(20);

    digitalWrite(ss_pin,LOW);   
    
    int reply = 0x00;
    while (reply != 0xFF)
    {       
       reply = SPI.transfer(0x44);
//       Serial.print(reply,HEX);
//       Serial.print(" ");
       if (reply != 0xFF)
       {
         digitalWrite(ss_pin,HIGH);
         delayMicroseconds(20);
         digitalWrite(ss_pin,LOW);   
       }
    }
    
    if (byteCountRx > 0) {
      Serial.println();
      Serial.print("rx: ");
    }

    for (int k = 0; k < byteCountRx; k++)
    {
      Serial.print(SPI.transfer(0x44), HEX);
      Serial.print(" ");
    }
       
    digitalWrite(ss_pin,HIGH);
    Serial.println();
    delay(100); // Wait half a second to prevent Serial buffer overflow
}



// Actions ===============================================================

// Config reset ----------------------------------------------------------
void RadioSi446x::reset(void) 
{
//  digitalWrite(VCXO_ENABLE_PIN,HIGH);
//  Serial.println("VCXO is enabled"); 
//  delay(600);
  
  



  // Start talking to the Si446X radio chip

  //divide VCXO_FREQ into its bytes; MSB first  
//  unsigned int x3 = VCXO_FREQ / 0x1000000;
//  unsigned int x2 = (VCXO_FREQ - x3 * 0x1000000) / 0x10000;
//  unsigned int x1 = (VCXO_FREQ - x3 * 0x1000000 - x2 * 0x10000) / 0x100;
//  unsigned int x0 = (VCXO_FREQ - x3 * 0x1000000 - x2 * 0x10000 - x1 * 0x100); 

//POWER_UP 
  //const unsigned char init_command[] = {0x02, 0x01, 0x01, x3, x2, x1, x0};// no patch, boot main app. img, FREQ_VCXO, return 1 byte
  const unsigned char init_command[] = {RF_POWER_UP};
  SendCmdReceiveAnswer(7, 1 ,init_command); 

  Serial.println("Radio booted"); 

  const unsigned char PART_INFO_command[] = {0x01}; // Part Info
  SendCmdReceiveAnswer(1, 9, PART_INFO_command);
  Serial.println("Part info was checked");  

  const unsigned char get_int_status_command[] = {0x20, 0x00, 0x00, 0x00}; //  Clear all pending interrupts and get the interrupt status back
  SendCmdReceiveAnswer(4, 9, get_int_status_command);


  Serial.println("Radio ready");
 
  //const char gpio_pin_cfg_command[] = {0x13, 0x02, 0x02, 0x02, 0x02, 0x08, 0x11, 0x00}; //  Set all GPIOs LOW; Link NIRQ to CTS; Link SDO to MISO; Max drive strength
  const unsigned char gpio_pin_cfg_command[] = {0x13, 0x14, 0x02, 0x21, 0x20, 0x27, 0x0b, 0x00};
  SendCmdReceiveAnswer(8, 8, gpio_pin_cfg_command);

  Serial.println("LEDs should be switched off at this point");
  
//  setFrequency(active_freq);
//  Serial.println("Frequency set");  
 
//  setModem(); 
//  Serial.println("CW mode set");  
  
//  tune_tx();
//  Serial.println("TX tune");  



}

void RadioSi446x::start_tx()
{
  unsigned char change_state_command[] = {0x34, 0x07}; //  Change to TX state
  SendCmdReceiveAnswer(2, 1, change_state_command);

}

void RadioSi446x::stop_tx()
{
  unsigned char change_state_command[] = {0x34, 0x03}; //  Change to Ready state
  SendCmdReceiveAnswer(2, 1, change_state_command);

}

void RadioSi446x::tune_tx()
{
  unsigned char change_state_command[] = {0x34, 0x05}; //  Change to TX tune state
  SendCmdReceiveAnswer(2, 1, change_state_command);

}




// Configuration parameter functions ---------------------------------------

void RadioSi446x::setModem()
{
  // Set to CW mode
  Serial.println("Setting modem into CW mode");  
  unsigned char set_modem_mod_type_command[] = {0x11, 0x20, 0x01, 0x00, 0x00};
  SendCmdReceiveAnswer(5, 1, set_modem_mod_type_command);
  
}



void RadioSi446x::setFrequency(unsigned long freq)
{ 
  
  // Set the output divider according to recommended ranges given in Si446x datasheet  
  int outdiv = 4;
  int band = 0;
  if (freq < 705000000UL) { outdiv = 6;  band = 1;};
  if (freq < 525000000UL) { outdiv = 8;  band = 2;};
  if (freq < 353000000UL) { outdiv = 12; band = 3;};
  if (freq < 239000000UL) { outdiv = 16; band = 4;};
  if (freq < 177000000UL) { outdiv = 24; band = 5;};
  
 
  unsigned long f_pfd = 2 * VCXO_FREQ / outdiv;
  
  unsigned int n = ((unsigned int)(freq / f_pfd)) - 1;
  
  float ratio = (float)freq / (float)f_pfd;
  float rest  = ratio - (float)n;
  

  unsigned long m = (unsigned long)(rest * 524288UL); 
 


// set the band parameter
  unsigned int sy_sel = 8; // 
  char unsigned set_band_property_command[] = {0x11, 0x20, 0x01, 0x51, (band + sy_sel)}; //   
  // send parameters
  SendCmdReceiveAnswer(5, 1, set_band_property_command);

// Set the pll parameters
  unsigned int m2 = m / 0x10000;
  unsigned int m1 = (m - m2 * 0x10000) / 0x100;
  unsigned int m0 = (m - m2 * 0x10000 - m1 * 0x100); 
  // assemble parameter string
  unsigned char set_frequency_property_command[] = {0x11, 0x40, 0x04, 0x00, n, m2, m1, m0};
  // send parameters
  SendCmdReceiveAnswer(8, 1, set_frequency_property_command);
  

}


// Public functions -----------------------------------------------------------

void RadioSi446x::setup()
{
// Not much radio configuration to do here
// because we initialize the transmitter each time right before we transmit a packet

// Just make sure that the VCXO is on and the transmitter chip is switched off at boot up
  pinMode(VCXO_ENABLE_PIN,  OUTPUT);
  digitalWrite(VCXO_ENABLE_PIN,HIGH);
  Serial.println("VCXO is enabled"); 

  //pinMode(NIRQ, INPUT);

  pinMode(RADIO_SDN_PIN, OUTPUT);
  delay(300);
  digitalWrite(RADIO_SDN_PIN, HIGH);  // active high = shutdown
  
  // Set up SPI  
  pinMode(SCKpin,  OUTPUT);
  pinMode(SSpin,   OUTPUT);
  pinMode(SSpin1,  OUTPUT);
  pinMode(MOSIpin, OUTPUT);
  pinMode(NIRQ, INPUT);
  pinMode(NIRQ1, INPUT);
  
  digitalWrite(SSpin, HIGH);  // ensure SS stays high for now
  digitalWrite(SSpin1, HIGH);
  
  // initialize SPI:
  SPI.begin(); 
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV8); // 8 MHz / 8 = 1 MHz
  delay(600);
 
  Serial.println("SPI is initialized"); 

}

void RadioSi446x::ptt_on()
{
  
  //digitalWrite(VCXO_ENABLE_PIN, HIGH);
  reset();
  // turn on the blue LED (GPIO2) to indicate TX
  //unsigned char gpio_pin_cfg_command2[] = {0x13, 0x02, 0x02, 0x03, 0x02, 0x08, 0x11, 0x00}; //  Set GPIO2 HIGH; Link NIRQ to CTS; Link SDO to MISO; Max drive strength
  //SendCmdReceiveAnswer(8, 1, gpio_pin_cfg_command2);

//  start_tx();
  si446x_powerlevel = 1023;
}

void RadioSi446x::ptt_off()
{
  stop_tx();
  si446x_powerlevel = 0;
  // turn off the blue LED (GPIO2)
  unsigned char gpio_pin_cfg_command0[] = {0x13, 0x02, 0x02, 0x02, 0x02, 0x08, 0x11, 0x00}; //  Set all GPIOs LOW; Link NIRQ to CTS; Link SDO to MISO; Max drive strength
  SendCmdReceiveAnswer(8, 1, gpio_pin_cfg_command0);

  digitalWrite(RADIO_SDN_PIN, HIGH);  // active high = shutdown
  //digitalWrite(VCXO_ENABLE_PIN, LOW); //keep enabled for now

}

void RadioSi446x::set_freq(unsigned long freq)
{
  active_freq = freq;
}


int RadioSi446x::get_powerlevel()
{
  return si446x_powerlevel;

}
void RadioSi446x::spi_write(int byteCountTx, const unsigned char* pData)
{
  SendCmdReceiveAnswer(byteCountTx, 0, pData);
  
}

void RadioSi446x::fifo_reset(void)			// 复位发射和接收 FIFO
{
	unsigned char p[2];
	
	p[0] = FIFO_INFO;
	p[1] = 0x03;   
//  SendCmdReceiveAnswer(2, 3, p);
	spi_write(2,p);
}

void RadioSi446x::clr_interrupt(void)		// 清中断标志
{
	unsigned char p[4];
	
	p[0] = GET_INT_STATUS;
	p[1] = 0;   
	p[2] = 0;   
	p[3] = 0; 
  SendCmdReceiveAnswer(4,9,p);
//	spi_write(4,p);
	//spi_read(9,GET_INT_STATUS); 
}

void RadioSi446x::enable_tx_interrupt(void)		// 使能发射中断
{	
	unsigned char p[6];

	p[0] = 0x11;
	p[1] = 0x01;
	p[2] = 0x02;
	p[3] = 0x00;
	p[4] = 0x01;
	p[5] = 0x20;
	spi_write(6, p);  
}

void RadioSi446x::enable_rx_interrupt(void)		// 使能接收中断
{
	unsigned char p[7];

	p[0] = 0x11;
	p[1] = 0x01;  
	p[2] = 0x03;  
	p[3] = 0x00;  
	p[4] = 0x03;   
	p[5] = 0x18; 
	p[6] = 0x00;
	spi_write(0x07, p);
}	

void RadioSi446x::tx_start(void)					// 开始发射
{
	unsigned char p[5];
	
	p[0] = START_TX ; 
	p[1] = freq_channel ;  			// 通道0
	p[2] = 0x30; 
	p[3] = 0;
	p[4] = 0; 
	spi_write(5, p);  
} 

void RadioSi446x::rx_start(void)					// 开始接收
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
	spi_write(8, p);  
}

void RadioSi446x::spi_write_fifo(void) {
  digitalWrite(ss_pin, LOW);
  SPI.transfer(WRITE_TX_FIFO);
  for (int j=0; j<payload_length; j++){
    SPI.transfer(tx_ph_data[j]);
  }
  digitalWrite(ss_pin,HIGH);  
}

void RadioSi446x::spi_read_fifo(void) {
 /*     int reply = 0x00;

  digitalWrite(ss_pin, HIGH);
  delayMicroseconds(20);
  digitalWrite(ss_pin, LOW);
  
      while (reply != 0xFF)
    {       
       reply = SPI.transfer(0x44);
//       Serial.print(reply,HEX);
//       Serial.print(" ");
       if (reply != 0xFF)
       {
         digitalWrite(ss_pin,HIGH);
         delayMicroseconds(20);
         digitalWrite(ss_pin,LOW);   
       }
    }

        */ 
  digitalWrite(ss_pin, LOW);
  SPI.transfer(READ_RX_FIFO);
  for (int j=0; j<payload_length; j++){
    Serial.print(SPI.transfer(0x00), HEX);
    Serial.print(" ");
  }
  digitalWrite(ss_pin,HIGH);
  Serial.println();  
//  unsigned char p[] = {READ_RX_FIFO};
//  SendCmdReceiveAnswer(1,payload_length,p);
}

void RadioSi446x::si4463_init(void)
{	
	unsigned char app_command_buf[20],i;
				
	//spi_write(0x07, RF_GPIO_PIN_CFG_data);   
	app_command_buf[0] = 0x13;			// 设置GPIO口
	app_command_buf[1]  = 0x14; 		// gpio 0,接收数据输出
	app_command_buf[2]  = 0x02;    		// gpio 1,输出低电平
	app_command_buf[3]  = 0x21;  		// gpio 2,接收天线开关
	app_command_buf[4]  = 0x20; 		// gpio 3,发射天线开关
	app_command_buf[5]  = 0x27;   		// nIRQ
	app_command_buf[6]  = 0x0b;  		// sdo
	spi_write(7, app_command_buf); 
		
	// spi_write(0x05, RF_GLOBAL_XO_TUNE_1_data); 
  //调整晶振,应该可以使用默认值
  
        app_command_buf[0] = 0x11;  
	app_command_buf[1]  = 0x00;    
	app_command_buf[2]  = 0x01;    
	app_command_buf[3]  = 0x00;  
	app_command_buf[4]  = 127;  			// 频率调整
	spi_write(5, app_command_buf);

	// spi_write(0x05, RF_GLOBAL_CONFIG_1_data);
  	app_command_buf[0] = 0x11;  
	app_command_buf[1]  = 0x00;
	app_command_buf[2]  = 0x01; 
	app_command_buf[3]  = 0x03; 
	app_command_buf[4]  = 0x40;  		// 收发缓冲区64字节,PH,高灵敏度模式
	spi_write(5, app_command_buf); 
    
    spi_write(0x08, RF_FRR_CTL_A_MODE_4_data);   
     
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
	spi_write(13, app_command_buf);	
	
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
    spi_write(9, app_command_buf);
       
	//  packet crc         
    app_command_buf[0] = 0x11;  
	app_command_buf[1]  = 0x12; 
	app_command_buf[2]  = 0x01; 
	app_command_buf[3]  = 0x00;
	app_command_buf[4]  = 0x81;							// CRC = itu-c, 使能crc
    spi_write(5, app_command_buf);  
        
	// packet   gernale configuration        
    app_command_buf[0] = 0x11;  
	app_command_buf[1]  = 0x12;
	app_command_buf[2]  = 0x01;
	app_command_buf[3]  = 0x06;
	app_command_buf[4]  = 0x02;							// CRC MSB， data MSB
    spi_write(5, app_command_buf);
        
  	// spi_write(0x07, RF_PKT_LEN_3_data);   
    app_command_buf[0] = 0x11;  
	app_command_buf[1]  = 0x12;
	app_command_buf[2]  = 0x03;
	app_command_buf[3]  = 0x08;
	app_command_buf[4]  = 0x00;
	app_command_buf[5]  = 0x00;
	app_command_buf[6]  = 0x00;						 
    spi_write(7, app_command_buf);         
 	
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
	spi_write(16, app_command_buf);						// 设置 Field 1--4长度
  
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
    spi_write(12, app_command_buf);
   
    spi_write(0x10, RF_MODEM_MOD_TYPE_12_data);        
	spi_write(0x05, RF_MODEM_FREQ_DEV_0_1_data);		 // 以上参数根据WDS配置得出	
	
    spi_write(0x0C, RF_MODEM_TX_RAMP_DELAY_8_data);    	
    spi_write(0x0D, RF_MODEM_BCR_OSR_1_9_data);			
    spi_write(0x0B, RF_MODEM_AFC_GEAR_7_data);			
    spi_write(0x05, RF_MODEM_AGC_CONTROL_1_data);		
    spi_write(0x0D, RF_MODEM_AGC_WINDOW_SIZE_9_data);	
    spi_write(0x0F, RF_MODEM_OOK_CNT1_11_data);			 // 以上参数根据WDS配置得出
    
	// spi_write(0x05, RF_MODEM_RSSI_COMP_1_data); //Modem
	app_command_buf[0] = 0x11;  
	app_command_buf[1] = 0x20;                                                     
	app_command_buf[2] = 0x01;                                                   
	app_command_buf[3] = 0x4e;             
	app_command_buf[4]  = 0x40;
    spi_write(5, app_command_buf);            	     
   
    spi_write(0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_data);
    spi_write(0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_data);
    spi_write(0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_data);       // 以上参数根据WDS配置得出
        
	// RF_PA
	app_command_buf[0] = 0x11;  
	app_command_buf[1]  = 0x22;                                                    
	app_command_buf[2]  = 0x04;                                               
	app_command_buf[3]  = 0x00;                                                     
	app_command_buf[4]  = 0x08;
	app_command_buf[5]  = 0x7f;							// 设置最大功率
	app_command_buf[6]  =0x00;
	app_command_buf[7]  = 0x3d;
    spi_write(8, app_command_buf);        
    
	spi_write(0x0B, RF_SYNTH_PFDCP_CPFF_7_data);
        
   	// header match
   	app_command_buf[0] = 0x11;  
	app_command_buf[1]  = 0x30;                                                     
	app_command_buf[2]  = 0x0c;                                                   
	app_command_buf[3]  = 0x00;                                                       
	app_command_buf[4]  = 'a';
	app_command_buf[5]  = 0xff;
      if(ss_pin == 1)
        app_command_buf[6]  = 0x40;
      else
        app_command_buf[6]  = 0x40;
	app_command_buf[7]  = 'b';                                      
	app_command_buf[8]  = 0xff;                                       
	app_command_buf[9]  = 0x01; 
	app_command_buf[10] = 'c';                                   
	app_command_buf[11]  =0xff;                                       
	app_command_buf[12]  =0x02;
	app_command_buf[13]  = 'd';                                  
	app_command_buf[14]  = 0xff;
	app_command_buf[15]  =0x03;
    spi_write(16, app_command_buf); 					// 配置头码
    
	spi_write(5, RF_MODEM_CLKGEN_BAND_1_data);
    spi_write(12, RF_FREQ_CONTROL_INTE_8_data); 	    // 设置频率  	
}    


/////////////////////////////////////////////////////
void RadioSi446x::request_device_state(void){
  unsigned char p[] = {REQUEST_DEVICE_STATE};
  SendCmdReceiveAnswer(1,3,p);
}

void RadioSi446x::get_pakcet_info(void){
	unsigned char p[6];
	
	p[0] = PACKET_INFO ;
	p[1] = 0x00 ; 			// 通道0
	p[2] = 0x00; 
	p[3] = 0x00;
	p[4] = 0x00;
	p[5] = 0x00; 
   	
	SendCmdReceiveAnswer(6,3,p);    
}


void RadioSi446x::get_fifo_info(void)			// 复位发射和接收 FIFO
{
	unsigned char p[2];
	
	p[0] = FIFO_INFO;
	p[1] = 0x00;   
  SendCmdReceiveAnswer(2, 3, p);
//	spi_write(2,p);
}

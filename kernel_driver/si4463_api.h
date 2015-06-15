#ifndef __RADIO_SI446X_H__
#define __RADIO_SI446X_H__


#include <linux/types.h>  /* size_t */



/*
 * FUNCTIONS
 */
void getCTS(void);
//Writing and Reading func
u8 * SendCmdReceiveAnswer(int byteCountTx, int byteCountRx, u8 * in_buff,
	u8 * out_buff);
//void spi_write(int byteCountTx, const unsigned char* pData);
void spi_write_fifo(unsigned char * data, int len);
void spi_read_fifo(unsigned char * st, int len);
void fifo_reset(void);
void read_frr_x(char no);    //no=a,b,c,d represent frr_a, frr_b, frr_c, frr_d

//Configure
void reset(void);
void setModem(void);
void start_tx(void);
void stop_tx(void);
void tune_tx(void);
void setFrequency(unsigned long freq); //Not in used
//void si4463_init(void);
void clr_interrupt(void);
void enable_tx_interrupt(void);		// 使能发射中断
void enable_rx_interrupt(void);		// 使能接收中断
void tx_start(void);
void rx_start(void);					// 开始接收
void tx_start_1B(void);

void set_frr_ctl(void);
//Status Getting Method
void request_device_state(void);
void get_pakcet_info(void);
void get_fifo_info(void);

#endif

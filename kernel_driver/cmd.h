#include "radio_config_si4463.h"
/*
 * CMD
 */
#define NOP 							0x00
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
#define REQUEST_DEVICE_STATE			0x33
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
//
//const unsigned char RF_MODEM_CLKGEN_BAND_1_data[] = { RF_MODEM_CLKGEN_BAND_1 };
//const unsigned char RF_FREQ_CONTROL_INTE_8_data[] = { RF_FREQ_CONTROL_INTE_8 };
//const unsigned char RF_POWER_UP_data[] = { RF_POWER_UP };
//const unsigned char RF_GPIO_PIN_CFG_data[] = { RF_GPIO_PIN_CFG };
//const unsigned char RF_GLOBAL_XO_TUNE_1_data[] = { RF_GLOBAL_XO_TUNE_1 };
//const unsigned char RF_GLOBAL_CONFIG_1_data[] = { RF_GLOBAL_CONFIG_1 };
//const unsigned char RF_FRR_CTL_A_MODE_4_data[] = { RF_FRR_CTL_A_MODE_4 };
//const unsigned char RF_PREAMBLE_TX_LENGTH_9_data[] = { RF_PREAMBLE_TX_LENGTH_9 };
//const unsigned char RF_SYNC_CONFIG_5_data[] = { RF_SYNC_CONFIG_5 };
//const unsigned char RF_PKT_CRC_CONFIG_1_data[] = { RF_PKT_CRC_CONFIG_1 };
//const unsigned char RF_PKT_CONFIG1_1_data[] = { RF_PKT_CONFIG1_1 };
//const unsigned char RF_PKT_LEN_3_data[] = { RF_PKT_LEN_3 };
//const unsigned char RF_PKT_FIELD_1_LENGTH_12_8_12_data[] = {
//		RF_PKT_FIELD_1_LENGTH_12_8_12 };
//const unsigned char RF_PKT_FIELD_4_LENGTH_12_8_8_data[] = {
//		RF_PKT_FIELD_4_LENGTH_12_8_8 };
//const unsigned char RF_MODEM_FREQ_DEV_0_1_data[] = { RF_MODEM_FREQ_DEV_0_1 };
//const unsigned char RF_MODEM_AGC_CONTROL_1_data[] = { RF_MODEM_AGC_CONTROL_1 };
//const unsigned char RF_MATCH_VALUE_1_12_data[] = { RF_MATCH_VALUE_1_12 };
//const unsigned char RF_MODEM_RSSI_COMP_1_data[] = { RF_MODEM_RSSI_COMP_1 };
//const unsigned char RF_MODEM_MOD_TYPE_12_data[] = { RF_MODEM_MOD_TYPE_12 };
//const unsigned char RF_MODEM_TX_RAMP_DELAY_8_data[] =
//		{ RF_MODEM_TX_RAMP_DELAY_8 };
//const unsigned char RF_MODEM_BCR_OSR_1_9_data[] = { RF_MODEM_BCR_OSR_1_9 };
//const unsigned char RF_MODEM_AFC_GEAR_7_data[] = { RF_MODEM_AFC_GEAR_7 };
//const unsigned char RF_MODEM_AGC_WINDOW_SIZE_9_data[] = {
//		RF_MODEM_AGC_WINDOW_SIZE_9 };
//const unsigned char RF_MODEM_OOK_CNT1_11_data[] = { RF_MODEM_OOK_CNT1_11 };
//const unsigned char RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_data[] = {
//		RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12 };
//const unsigned char RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_data[] = {
//		RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12 };
//const unsigned char RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_data[] = {
//		RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12 };
//const unsigned char RF_SYNTH_PFDCP_CPFF_7_data[] = { RF_SYNTH_PFDCP_CPFF_7 };

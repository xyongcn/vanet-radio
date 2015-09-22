#include "return_val.h"
#include "tal_internal.h"
#include "ieee_const.h"
#include "at86rf212.h"
#include <linux/jiffies.h>
#include <linux/delay.h>


/* === Tools ================================================================*/
uint16_t rand(void);
void  pal_timer_delay(uint16_t delay);
void ENTER_CRITICAL_REGION(void); //mutex_lock(&mutex_spi);
void LEAVE_CRITICAL_REGION(void); //mutex_unlock(&mutex_spi);



retval_t tal_init(void);
retval_t trx_reset(void);
tal_trx_status_t set_trx_state(trx_cmd_t trx_cmd);

void rf212_rx_begin(void);

void send_frame(uint8_t *frame_tx, bool use_csma, bool tx_retries);
void pal_trx_frame_read(uint8_t *data, uint8_t length);
void pal_trx_frame_write(uint8_t *data, uint8_t length);

void pal_trx_bit_write(uint8_t reg_addr, uint8_t mask, uint8_t pos, uint8_t new_value);
uint8_t pal_trx_bit_read(uint8_t addr, uint8_t mask, uint8_t pos);

void pal_trx_reg_write(uint8_t addr, uint8_t data);
uint8_t pal_trx_reg_read(uint8_t addr);

void rf212_clr_irq(void);


/* === Macros ============================================================== */
/*
 * Dummy value written in SPDR to retrieve data form it
 */
#define SPI_DUMMY_VALUE                 (0x00)


/*
 * Write access command of the transceiver
 */
#define WRITE_ACCESS_COMMAND            (0xC0)

/*
 * Read access command to the tranceiver
 */
#define READ_ACCESS_COMMAND             (0x80)

/*
 * Frame write command of transceiver
 */
#define TRX_CMD_FW                      (0x60)

/*
 * Frame read command of transceiver
 */
#define TRX_CMD_FR                      (0x20)

/*
 * SRAM write command of transceiver
 */
#define TRX_CMD_SW                      (0x40)

/*
 * SRAM read command of transceiver
 */
#define TRX_CMD_SR                      (0x00)

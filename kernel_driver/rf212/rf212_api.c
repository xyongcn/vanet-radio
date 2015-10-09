#include "rf212_api.h"
#include "../radio.h"
#include "tal_pib.h"

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/wait.h>


//================ GLOBAL ===================
extern struct spi_device *spi_save;
extern struct spidev_data spidev_global;
extern wait_queue_head_t spi_wait_queue;

/* CHIP STATUS */
extern tal_trx_status_t tal_trx_status;
extern tal_state_t tal_state;
extern uint8_t tal_pib_MaxCSMABackoffs;
extern bool tal_rx_on_required;
//===========================================

extern struct mutex mutex_spi;
/* === Tools ================================================================*/
uint16_t rand(){
	return jiffies;
}

/**
 * @brief Generates blocking delay
 *
 * This functions generates a blocking delay of specified time.
 *
 * @param delay in microseconds
 */
inline void pal_timer_delay(uint16_t delay)
{
        ndelay(delay);

}

inline void ENTER_CRITICAL_REGION(void)
{
//	printk(KERN_ALERT "LOCK\n");
//	mutex_lock(&mutex_spi);
}

inline void LEAVE_CRITICAL_REGION(void)
{
//	printk(KERN_ALERT "UNLOCK\n");
//	mutex_unlock(&mutex_spi);
}

/* ===  ================================================================*/

inline static u8 spi_put(u8 data){
	int ret;
	u8 rx;
	ret = spidev_sync_transfer(&spidev_global, &data, &rx, 1);
	if (ret <= 0)
		printk(KERN_ALERT "spi_put: spidev_sync_transfer error with code %d\n", ret);

  return rx;
}

inline static void pal_gpio_set(int pin, int state){
	gpio_set_value(pin, state);
}

inline static void PAL_WAIT_1_US(){
  udelay(1);
}

void rf212_clr_irq(){
	pal_trx_reg_read(RG_IRQ_STATUS);
}

void rf212_rx_begin(){
	pal_trx_reg_write(RG_TRX_STATE, CMD_RX_ON);
}
/**
 * @brief Writes data into a transceiver register
 *
 * This function writes a value into transceiver register.
 *
 * @param addr Address of the trx register
 * @param data Data to be written to trx register
 *
 */
void pal_trx_reg_write(uint8_t addr, uint8_t data)
{

    ENTER_CRITICAL_REGION();

    /* Prepare the command byte */
    addr |= WRITE_ACCESS_COMMAND;

    /* Start SPI transaction by pulling SEL low */
    SS_LOW();

    /* Send the Read command byte */
    spi_put(addr);

    /* Write the byte in the transceiver data register */
    spi_put(data);

    /* Stop the SPI transaction by setting SEL high */
    SS_HIGH();

    LEAVE_CRITICAL_REGION();
}
/**
 * @brief Subregister write
 *
 * @param[in]   addr  Offset of the register
 * @param[in]   mask  Bit mask of the subregister
 * @param[in]   pos   Bit position of the subregister
 * @param[out]  value  Data, which is muxed into the register
 */
void pal_trx_bit_write(uint8_t reg_addr, uint8_t mask, uint8_t pos, uint8_t new_value)
{
    uint8_t current_reg_value;

    current_reg_value = pal_trx_reg_read(reg_addr);
    current_reg_value &= ~mask;
    new_value <<= pos;
    new_value &= mask;
    new_value |= current_reg_value;

    pal_trx_reg_write(reg_addr, new_value);
}
/**
 * @brief Subregister read
 *
 * @param   Addr  offset of the register
 * @param   Mask  bit mask of the subregister
 * @param   Pos   bit position of the subregister
 * @return  Data  value of the read bit(s)
 */
uint8_t pal_trx_bit_read(uint8_t addr, uint8_t mask, uint8_t pos)
{
    uint8_t ret;

    ret = pal_trx_reg_read(addr);
    ret &= mask;
    ret >>= pos;

    return ret;
}

/**
 * @brief Reads current value from a transceiver register
 *
 * This function reads the current value from a transceiver register.
 *
 * @param addr Specifies the address of the trx register
 * from which the data shall be read
 *
 * @return value of the register read
 */
uint8_t pal_trx_reg_read(uint8_t addr)
{
    uint8_t register_value = 0;

    ENTER_CRITICAL_REGION();

    /* Prepare the command byte */
    addr |= READ_ACCESS_COMMAND;

    /* Start SPI transaction by pulling SEL low */
    SS_LOW();

    /* Send the Read command byte */
    spi_put(addr);

    /* Do dummy read for initiating SPI read */
    register_value = spi_put(SPI_DUMMY_VALUE);

    /* Stop the SPI transaction by setting SEL high */
    SS_HIGH();

    LEAVE_CRITICAL_REGION();

    return register_value;
}

/**
 * @brief Initializes the transceiver
 *
 * This function is called to initialize the transceiver.
 *
 * @return SUCCESS if the transceiver state is changed to TRX_OFF and the
 *                 current device part number and version number are correct;
 *         FAILURE otherwise
 */
static retval_t trx_init(void)
{
    tal_trx_status_t trx_status;
    uint8_t poll_counter = 0;

    pal_gpio_set(RST_PIN, LOW);
    pal_timer_delay(60);
    pal_gpio_set(RST_PIN, HIGH);
    pal_gpio_set(SLP_TR_PIN, LOW);

    pal_timer_delay(P_ON_TO_CLKM_AVAILABLE);   // wait until TRX_OFF can be written
    pal_trx_reg_write(RG_TRX_STATE, CMD_TRX_OFF);

    /* verify that trx has reached TRX_OFF */
    do
    {
        trx_status = (tal_trx_status_t)pal_trx_bit_read(SR_TRX_STATUS);
        poll_counter++;
        if (poll_counter == 0xFF)
        {
#if (DEBUG > 0)
            pal_alert();
#endif
            printk(KERN_ALERT "pal_trx_bit_read(SR_TRX_STATUS) failed! trx_status: %x\n", trx_status);
            return FAILURE;
        }
    } while (trx_status != TRX_OFF);

    tal_trx_status = TRX_OFF;

    /* Check if AT86RF212 is connected; omit manufacturer id check */
    if ((AT86RF212_PART_NUM != pal_trx_reg_read(RG_PART_NUM)) ||
        (AT86RF212_VERSION_NUM != pal_trx_reg_read(RG_VERSION_NUM)))
    {
    	printk(KERN_ALERT "AT86RF212_PART_NUM failed!\n");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Reset transceiver
 *
 * @return SUCCESS if the transceiver state is changed to TRX_OFF
 *         FAILURE otherwise
 */
retval_t trx_reset(void)
{
    tal_trx_status_t trx_status;
    uint8_t poll_counter = 0;
#ifdef EXTERN_EEPROM_AVAILABLE
    uint8_t xtal_trim_value;
#endif

    /* Get trim value for 16 MHz xtal; needs to be done before reset */
#ifdef EXTERN_EEPROM_AVAILABLE
    pal_ps_get(EXTERN_EEPROM, PS_XTAL_TRIM, &xtal_trim_value);
#endif

    /* trx might sleep, so wake it up */
    pal_gpio_set(SLP_TR_PIN, LOW);
    pal_timer_delay(SLEEP_TO_TRX_OFF_US);

    /* Apply reset pulse */
    pal_gpio_set(RST_PIN, LOW);
    pal_timer_delay(RST_PULSE_WIDTH_US);
    pal_gpio_set(RST_PIN, HIGH);

    /* verify that trx has reached TRX_OFF */
    do
    {
        trx_status = (tal_trx_status_t)pal_trx_bit_read(SR_TRX_STATUS);
        poll_counter++;
        if (poll_counter > 10)
        {
#if (DEBUG > 0)
            pal_alert();
#endif
            return FAILURE;
        }
    } while (trx_status != TRX_OFF);

    tal_trx_status = TRX_OFF;

    // Write 16MHz xtal trim value to trx.
    // It's only necessary if it differs from the reset value.
#ifdef EXTERN_EEPROM_AVAILABLE
    if (xtal_trim_value != 0x00)
    {
        pal_trx_bit_write(SR_XTAL_TRIM, xtal_trim_value);
    }
#endif

    return SUCCESS;
}

/**
 * @brief Configures the transceiver
 *
 * This function is called to configure the transceiver after reset.
 */
static void trx_config(void)
{
    uint16_t rand_value;

    /* Set pin driver strength */
    pal_trx_bit_write(SR_PAD_IO_CLKM, PAD_CLKM_2_MA);
    pal_trx_bit_write(SR_CLKM_SHA_SEL, CLKM_SHA_DISABLE);
    pal_trx_bit_write(SR_CLKM_CTRL, CLKM_1MHZ);

    /*
     * Init the SEED value of the CSMA backoff algorithm.
     * The rand algorithm gets some random seed initialization during timer initialization.
     */
    rand_value = (uint16_t)rand();
    pal_trx_reg_write(RG_CSMA_SEED_0, (uint8_t)rand_value);
    pal_trx_bit_write(SR_CSMA_SEED_1, (uint8_t)(rand_value >> 8));

    /*
     * Since the TAL currently supports MAC-2003 only, frames with version number
     * indicating MAC-2006 are not acknowledged.
     */
    pal_trx_bit_write(SR_AACK_FVN_MODE, FRAME_VERSION_0);
    pal_trx_bit_write(SR_AACK_SET_PD, PD_ACK_BIT_SET_ENABLE); /* ACKs for data requests, indicate pending data */
    pal_trx_bit_write(SR_RX_SAFE_MODE, RX_SAFE_MODE_ENABLE);    /* Enable buffer protection mode */
    pal_trx_bit_write(SR_IRQ_MASK_MODE, IRQ_MASK_MODE_ON); /* Enable poll mode */
    pal_trx_reg_write(RG_IRQ_MASK, TRX_IRQ_DEFAULT);    /* The TRX_END interrupt of the transceiver is enabled. */
    pal_trx_bit_write(SR_TX_AUTO_CRC_ON, TX_AUTO_CRC_ENABLE); /* Enable auto CRC calculation */
    //pal_trx_bit_write(SR_IRQ_2_EXT_EN, TIMESTAMPING_ENABLE);   /* Enable timestamping feature */
#ifdef ANTENNA_DIVERSITY
    pal_trx_bit_write(SR_IRQ_2_EXT_EN, 1); /* Enable antenna diversity. */
#endif
#ifdef ENABLE_TFA
#ifdef SPECIAL_PEER
    tfa_pib_set(TFA_PIB_RX_SENS, (uint8_t *)0x08);   // 0x08 = - 70 dBm; 0x0B = -61 dBm
#endif
#endif
}

/**
 * @brief Initializes the TAL
 *
 * This function is called to initialize the TAL. The transceiver is
 * initialized, the TAL PIBs are set to their default values, and the TAL state
 * machine is set to TAL_IDLE state.
 *
 * @return SUCCESS if the transceiver state is changed to TRX_OFF and the
 *                 current device part number and version number are correct;
 *         FAILURE otherwise
 */
retval_t tal_init(void)
{
    /* Init the PAL and by this means also the transceiver interface */
//    if (pal_init() != SUCCESS)
//    {
//        return FAILURE;
//    }

    if (trx_init() != SUCCESS)
    {
        return FAILURE;
    }

//#ifdef EXTERN_EEPROM_AVAILABLE
//    pal_ps_get(EXTERN_EEPROM, PS_IEEE_ADDR, &tal_pib_IeeeAddress);
//#else
//    pal_ps_get(INTERN_EEPROM, PS_IEEE_ADDR, &tal_pib_IeeeAddress);
//#endif
//
//#ifndef SNIFFER
//    // Check if a valid IEEE address is available.
//    if ((tal_pib_IeeeAddress == 0x0000000000000000) ||
//        (tal_pib_IeeeAddress == 0xFFFFFFFFFFFFFFFF))
//    {
//        return FAILURE;
//    }
//#endif

    if (trx_reset() != SUCCESS)
    {
        return FAILURE;
    }
    trx_config();

    pal_trx_reg_read(RG_IRQ_STATUS);    /* clear pending irqs, dummy read */

    /*
     * Configure interrupt handling.
     * Install a handler for the transceiver interrupt.
     */
//    pal_trx_irq_init(TRX_MAIN_IRQ_HDLR_IDX, (void *)trx_irq_handler_cb);
//    pal_trx_irq_enable(TRX_MAIN_IRQ_HDLR_IDX);     /* Enable transceiver interrupts. */

    /* Configure time stamp interrupt. */
    //pal_trx_irq_init(TRX_TSTAMP_IRQ_HDLR_IDX, (void *)trx_irq_timestamp_handler_cb);
    //pal_trx_irq_enable(TRX_TSTAMP_IRQ_HDLR_IDX);     /* Enable timestamp interrupts. */

    /* Initialize the buffer management module and get a buffer to store reveived frames. */
//    bmm_buffer_init();
//    tal_rx_buffer = bmm_buffer_alloc(LARGE_BUFFER_SIZE);
//
//    /* Init incoming frame queue */
//    qmm_queue_init(&tal_incoming_frame_queue, TAL_INCOMING_FRAME_QUEUE_CAPACITY);

    /* Handle TAL's PIB values */
    init_tal_pib(); /* implementation can be found in 'tal_pib.c' */
    write_all_tal_pib_to_trx();  /* implementation can be found in 'tal_pib.c' */

#ifdef ENABLE_TFA
    tfa_init();
#endif

    tal_state = TAL_IDLE;   /* reset TAL state machine */
#ifndef NOBEACON
    tal_csma_state = CSMA_IDLE;
#endif
    /* The receiver is not requested to be switched on after tal_init. */
    tal_rx_on_required = false;

    return SUCCESS;
} /* tal_init() */

/**
 * @brief Switches the PLL on
 */
static void switch_pll_on(void)
{
    trx_irq_reason_t irq_status;
	uint32_t start_time, now;

    /* Check if trx is in TRX_OFF; only from PLL_ON the following procedure is applicable */
    if (pal_trx_bit_read(SR_TRX_STATUS) != TRX_OFF)
    {
        ASSERT("Switch PLL_ON failed, because trx is not in TRX_OFF" == 0);
        return;
    }

	pal_trx_reg_read(RG_IRQ_STATUS);	/* clear PLL lock bit */
	/* Switch PLL on */
    pal_trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);

	/* Check if PLL has been locked. */
//	pal_get_current_time(&start_time);
//    while (1)
//    {
//		irq_status = (trx_irq_reason_t)pal_trx_reg_read(RG_IRQ_STATUS);
//        if (irq_status & TRX_IRQ_PLL_LOCK)
//        {
//            break;	// PLL is locked now
//        }
//
//		/* Check if polling needs too much time. */
//		pal_get_current_time(&now);
//		if (pal_sub_time_us(now, start_time) > (2 * PLL_LOCK_TIME_US))
//        {
//            /* leave poll loop and throw assertion */
//#if (DEBUG > 0)
//            ASSERT("PLL switch failed" == 0);
//			pal_alert();
//#endif
//            break;
//        }
//    }
}


/**
 * @brief Sets transceiver state
 *
 * @param trx_cmd needs to be one of the trx commands
 *
 * @return current trx state
 */
tal_trx_status_t set_trx_state(trx_cmd_t trx_cmd)
{
    if (tal_trx_status == TRX_SLEEP)
    {
        uint8_t bit_status;

        pal_gpio_set(SLP_TR_PIN, LOW);
        /* poll status register until TRX_OFF is reached */
        do
        {
            bit_status = pal_trx_bit_read(SR_TRX_STATUS);
        } while (bit_status != TRX_OFF);

#if (DEBUG > 0)
        pal_trx_reg_read(RG_IRQ_STATUS);    /* clear Wake irq, dummy read */
#endif

#ifdef ANTENNA_DIVERSITY
        pal_trx_bit_write(SR_IRQ_2_EXT_EN, 1); /* Enable antenna diversity. */
#endif

        if ((trx_cmd == CMD_TRX_OFF) || (trx_cmd == CMD_FORCE_TRX_OFF))
        {
            tal_trx_status = TRX_OFF;
            return TRX_OFF;
        }
    }

    tal_trx_status = (tal_trx_status_t)pal_trx_bit_read(SR_TRX_STATUS);

    switch (trx_cmd)    /* requested state */
    {
        case CMD_SLEEP:
            pal_trx_reg_write(RG_TRX_STATE, CMD_FORCE_TRX_OFF);
#ifdef ANTENNA_DIVERSITY
            pal_trx_bit_write(SR_IRQ_2_EXT_EN, 0); /* Disable antenna diversity */
#endif
            PAL_WAIT_1_US();
            pal_gpio_set(SLP_TR_PIN, HIGH);
            pal_timer_delay(TRX_OFF_TO_SLEEP_TIME);
            tal_trx_status = TRX_SLEEP;
            return TRX_SLEEP;   /* transceiver register cannot be read during TRX_SLEEP */

        case CMD_TRX_OFF:
            switch (tal_trx_status)
            {
                case TRX_OFF:
                    break;

                default:
                    pal_trx_reg_write(RG_TRX_STATE, CMD_TRX_OFF);
                    PAL_WAIT_1_US();
                    break;
            }
            break;

        case CMD_FORCE_TRX_OFF:
            switch (tal_trx_status)
            {
                case TRX_OFF:
                    break;

                default:
                    pal_trx_reg_write(RG_TRX_STATE, CMD_FORCE_TRX_OFF);
                    PAL_WAIT_1_US();
                    break;
            }
            break;

        case CMD_PLL_ON:
            switch (tal_trx_status)
            {
                case PLL_ON:
                    break;

                case TRX_OFF:
                    switch_pll_on();
                    break;

                case RX_ON:
                case RX_AACK_ON:
                case TX_ARET_ON:
                    pal_trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);
                    PAL_WAIT_1_US();
                    break;

                case BUSY_RX:
                case BUSY_TX:
                case BUSY_RX_AACK:
                case BUSY_TX_ARET:
                    /* do nothing if trx is busy */
                    break;

                default:
                    ASSERT("state transition not handled" == 0);
                    break;
            }
            break;

        case CMD_FORCE_PLL_ON:
            switch (tal_trx_status)
            {
                case TRX_OFF:
                    switch_pll_on();
                    break;

                case PLL_ON:
                    break;

                default:
                    pal_trx_reg_write(RG_TRX_STATE, CMD_FORCE_PLL_ON);
                    break;
            }
            break;

        case CMD_RX_ON:
            switch (tal_trx_status)
            {
                case RX_ON:
                    break;

                case PLL_ON:
                case RX_AACK_ON:
                case TX_ARET_ON:
                    pal_trx_reg_write(RG_TRX_STATE, CMD_RX_ON);
                    PAL_WAIT_1_US();
                    break;

                case TRX_OFF:
                    switch_pll_on();
                    pal_trx_reg_write(RG_TRX_STATE, CMD_RX_ON);
                    PAL_WAIT_1_US();
                    break;

                case BUSY_RX:
                case BUSY_TX:
                case BUSY_RX_AACK:
                case BUSY_TX_ARET:
                    /* do nothing if trx is busy */
                    break;

                default:
                    ASSERT("state transition not handled" == 0);
                    break;
            }
            break;

            case CMD_RX_AACK_ON:
                switch (tal_trx_status)
                {
                    case RX_AACK_ON:
                        break;

                    case PLL_ON:
                        pal_trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON);
                        PAL_WAIT_1_US();
                        break;

                    case TRX_OFF:
                        switch_pll_on();// state change from TRX_OFF to RX_AACK_ON can be done directly, too
                        pal_trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON);
                        PAL_WAIT_1_US();
                        break;

                    case TX_ARET_ON:
                    case RX_ON:
                        pal_trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);
                        PAL_WAIT_1_US();
                        // check if state change could be applied
                        tal_trx_status = (tal_trx_status_t)pal_trx_bit_read(SR_TRX_STATUS);
                        if (tal_trx_status != PLL_ON)
                        {
                            return tal_trx_status;
                        }
                        pal_trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON);
                        PAL_WAIT_1_US();
                        break;

                    case BUSY_RX:
                    case BUSY_TX:
                    case BUSY_RX_AACK:
                    case BUSY_TX_ARET:
                        /* do nothing if trx is busy */
                        break;

                    default:
                        ASSERT("state transition not handled" == 0);
                        break;
                }
                break;

            case CMD_TX_ARET_ON:
                switch (tal_trx_status)
                {
                    case TX_ARET_ON:
                        break;

                    case PLL_ON:
                        pal_trx_reg_write(RG_TRX_STATE, CMD_TX_ARET_ON);
                        PAL_WAIT_1_US();
                        break;

                    case RX_ON:
                    case RX_AACK_ON:
                        pal_trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);
                        PAL_WAIT_1_US();
                        // check if state change could be applied
                        tal_trx_status = (tal_trx_status_t)pal_trx_bit_read(SR_TRX_STATUS);
                        if (tal_trx_status != PLL_ON)
                        {
                            return tal_trx_status;
                        }
                        pal_trx_reg_write(RG_TRX_STATE, CMD_TX_ARET_ON);
                        PAL_WAIT_1_US();
                        break;

                    case TRX_OFF:
                        switch_pll_on();// state change from TRX_OFF to TX_ARET_ON can be done directly, too
                        pal_trx_reg_write(RG_TRX_STATE, CMD_TX_ARET_ON);
                        PAL_WAIT_1_US();
                        break;

                    case BUSY_RX:
                    case BUSY_TX:
                    case BUSY_RX_AACK:
                    case BUSY_TX_ARET:
                        /* do nothing if trx is busy */
                        break;

                    default:
                        ASSERT("state transition not handled" == 0);
                        break;
                }
                break;

            default:
                /* CMD_NOP, CMD_TX_START */
                ASSERT("trx command not handled" == 0);
                break;
        }

        do
        {
            tal_trx_status = (tal_trx_status_t)pal_trx_bit_read(SR_TRX_STATUS);
        } while (tal_trx_status == STATE_TRANSITION_IN_PROGRESS);

        return tal_trx_status;
    } /* set_trx_state() */



/**
 * @brief Sends frame
 *
 * @param frame_tx Pointer to prepared frame
 * @param use_csma Flag indicating if CSMA is requested
 * @param tx_retries Flag indicating if transmission retries are requested
 *                   by the MAC layer
 */
void send_frame(uint8_t *frame_tx, bool use_csma, bool tx_retries)
{
    tal_trx_status_t trx_status;

    // configure tx according to tx_retries
    if (tx_retries)
    {
        pal_trx_bit_write(SR_MAX_FRAME_RETRIES, aMaxFrameRetries);
    }
    else
    {
        pal_trx_bit_write(SR_MAX_FRAME_RETRIES, 0);
    }

    // configure tx according to csma usage
    if (use_csma)
    {
        pal_trx_bit_write(SR_MAX_CSMA_RETRIES, tal_pib_MaxCSMABackoffs);
    }
    else    // no csma is required
    {
        pal_trx_bit_write(SR_MAX_CSMA_RETRIES, 7);
    }

    do
    {
        trx_status = set_trx_state(CMD_TX_ARET_ON);
    } while (trx_status != TX_ARET_ON);

//    ENTER_CRITICAL_REGION();    // prevent from buffer underrun

    pal_trx_frame_write(frame_tx, frame_tx[0]);
    /* Toggle the SLP_TR pin triggering transmission. */
    pal_gpio_set(SLP_TR_PIN, HIGH);
    pal_gpio_set(SLP_TR_PIN, LOW);

    /*
     * Send the frame to the transceiver.
     * Note: The PhyHeader is the first byte of the frame to
     * be sent to the transceiver and this contains the frame
     * length.
     */

    tal_state = TAL_TX_AUTO;




//    LEAVE_CRITICAL_REGION();
}

/**
 * @brief Reads frame buffer of the transceiver
 *
 * This function reads the frame buffer of the transceiver.
 *
 * @param[out] data Pointer to the location to store frame
 * @param[in] length Number of bytes to be read from the frame
 * buffer.
 */
void pal_trx_frame_read(uint8_t *data, uint8_t length)
{
	int ret;
    ENTER_CRITICAL_REGION();

    /* Start SPI transaction by pulling SEL low */
    SS_LOW();

    /* Send the command byte */
    spi_put(TRX_CMD_FR);

    spidev_global.buffer = data;
    ret = spidev_sync_read(&spidev_global, length);
    if (ret != length) {
    	printk("*******pal_trx_frame_read error!, len: %d, read:%d********\n", length, ret);
    }
//    do
//    {
//        /* Do dummy read for initiating SPI read */
//        /* Upload the received byte in the user provided location */
//        *data++ = spi_put(SPI_DUMMY_VALUE);
//
//    } while (--length > 0);

    /* Stop the SPI transaction by setting SEL high */
    SS_HIGH();

    LEAVE_CRITICAL_REGION();
}

/**
 * @brief Writes data into frame buffer of the transceiver
 *
 * This function writes data into the frame buffer of the transceiver
 *
 * @param[in] data Pointer to data to be written into frame buffer
 * @param[in] length Number of bytes to be written into frame buffer
 */
void pal_trx_frame_write(uint8_t *data, uint8_t length)
{
    ENTER_CRITICAL_REGION();

    /* Start SPI transaction by pulling SEL low */
    SS_LOW();

    /* Send the command byte */
    spi_put(TRX_CMD_FW);

    spidev_global.buffer = data;
    int i;
//    for(i=0; i<length;i++) {
//    	if(data+i == NULL)
//    		printk(KERN_ALERT "NULLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\n");
//    }

    spidev_sync_write(&spidev_global, length);

//    do
//    {
//        /* Write the user provided data in the transceiver data register */
//        spi_put(*data++);
//
//    } while (--length > 0);

    /* Stop the SPI transaction by setting SEL high */
    SS_HIGH();

    LEAVE_CRITICAL_REGION();
}

/**
 * @file tal_pib.c
 *
 * @brief This file handles the TAL PIB attributes, set/get and initialization
 *
 * $Id: tal_pib.c 12253 2008-11-26 12:07:07Z uwalter $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2008, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> LICENSE.txt
 */

/* === INCLUDES ============================================================ */


#include "return_val.h"
#include "tal.h"
#include "ieee_const.h"
#include "tal_constants.h"
#include "tal_pib.h"
#include "rf212_api.h"
#include "at86rf212.h"
//#include "pal.h"
#include "tal_internal.h"
#ifdef SPECIAL_PEER
#include "private_const.h"
#endif /* SPECIAL_PEER */

/* === TYPES =============================================================== */


/* === MACROS ============================================================== */

/**
 * Tx power table
 * Table maps tx power value to register value
 */
#define TX_PWR_TABLE_NA \
/* Tx power, dBm        11    10     9     8     7     6     5     4     3     2     1     0    -1    -2    -3    -4    -5    -6    -7    -8    -9   -10   -11 */ \
/* Register value */  0xe0, 0xc1, 0xa1, 0x81, 0x82, 0x83, 0x60, 0x61, 0x41, 0x42, 0x22, 0x23, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c

#define TX_PWR_TABLE_EU1 \
/* Tx power, dBm        11    10     9     8     7     6     5     4     3     2     1     0    -1    -2    -3    -4    -5    -6    -7    -8    -9   -10   -11 */ \
/* Register value */  0xe0, 0xc1, 0xa1, 0x81, 0x82, 0x83, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x46, 0x26, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c

#define TX_PWR_TABLE_EU2 \
/* Tx power, dBm        11    10     9     8     7     6     5     4     3     2     1     0    -1    -2    -3    -4    -5    -6    -7    -8    -9   -10   -11 */ \
/* Register value */  0xe0, 0xc1, 0xa1, 0x81, 0x82, 0x83, 0xe7, 0xe8, 0xe9, 0xea, 0xab, 0x89, 0x66, 0x46, 0x26, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c

#define TX_PWR_TABLE_SIZE           (23)

#define EU_TX_PWR_TABLE             (1)     // Use power table #1 for EU mode

#if (EU_TX_PWR_TABLE == 1)
#define MAX_TX_PWR_BPSK20           (2)
#define MAX_TX_PWR_OQPSK            (-1)
#endif
#if (EU_TX_PWR_TABLE == 2)
#define MAX_TX_PWR_BPSK20           (5)
#define MAX_TX_PWR_OQPSK            (3)
#endif

/* === GLOBALS ============================================================= */

///**
// * Tx power table in flash.
// */
//FLASH_DECLARE (static uint8_t tx_pwr_table_NA[TX_PWR_TABLE_SIZE]) = { TX_PWR_TABLE_NA };
//#if (EU_TX_PWR_TABLE == 1)
//FLASH_DECLARE (static uint8_t tx_pwr_table_EU[TX_PWR_TABLE_SIZE]) = { TX_PWR_TABLE_EU1 };
//#endif
//#if (EU_TX_PWR_TABLE == 2)
//FLASH_DECLARE (static uint8_t tx_pwr_table_EU[TX_PWR_TABLE_SIZE]) = { TX_PWR_TABLE_EU2 };
//#endif

/* === PROTOTYPES ========================================================== */

static uint8_t limit_tx_pwr(uint8_t tal_pib_TransmitPower);
static uint8_t convert_phyTransmitPower_to_reg_value(uint8_t phyTransmitPower_value);
#ifdef TAL_2006
static bool apply_channel_page_configuration(uint8_t ch_page);
#endif

/* === IMPLEMENTATION ====================================================== */

/**
 * @brief Initialize the TAL PIB
 *
 * This function initializes the TAL information base attributes
 * to their default values.
 */
void init_tal_pib(void)
{
    tal_pib_MaxCSMABackoffs = TAL_MAX_CSMA_BACKOFFS_DEFAULT;
    tal_pib_MinBE = TAL_MINBE_DEFAULT;
    tal_pib_PANId = TAL_PANID_BC_DEFAULT;
    tal_pib_ShortAddress = TAL_SHORT_ADDRESS_DEFAULT;
    tal_pib_CurrentChannel = TAL_CURRENT_CHANNEL_DEFAULT;
    tal_pib_SupportedChannels = TRX_SUPPORTED_CHANNELS;
#ifdef TAL_2006
    tal_pib_CurrentPage = TAL_CURRENT_PAGE_DEFAULT;
#endif
    tal_pib_TransmitPower = limit_tx_pwr(TAL_TRANSMIT_POWER_DEFAULT);
    tal_pib_CCAMode = TAL_CCA_MODE_DEFAULT;
    tal_pib_BattLifeExt = TAL_BATTERY_LIFE_EXTENSION_DEFAULT;
    tal_pib_PrivatePanCoordinator = TAL_PAN_COORDINATOR_DEFAULT;
    tal_pib_PrivateAssociated = TAL_PRIVATE_ASSOCIATED_DEFAULT;
    tal_pib_BeaconOrder = TAL_BEACON_ORDER_DEFAULT;
    tal_pib_SuperFrameOrder = TAL_SUPERFRAME_ORDER_DEFAULT;
    tal_pib_BeaconTxTime = TAL_BEACON_TX_TIME_DEFAULT;
    tal_pib_ACKWaitDuration = macAckWaitDuration_def;

    tal_pib_PrivateAssociated = false;

#ifdef SPECIAL_PEER
    tal_pib_PrivateCCAFailure = TAL_PRIVATE_CCA_FAILURE_DEFAULT;
    tal_pib_PrivateDisableACK = TAL_PRIVATE_DISABLE_ACK_DEFAULT;
#endif /* SPECIAL_PEER */
}


/**
 * @brief Write all shadow PIB variables to the transceiver
 *
 * This function writes all shadow PIB variables to the transceiver.
 * It is assumed that the radio does not sleep.
 */
void write_all_tal_pib_to_trx(void)
{
    pal_trx_reg_write(RG_PAN_ID_0, (uint8_t)tal_pib_PANId);
    pal_trx_reg_write(RG_PAN_ID_1, (uint8_t)(tal_pib_PANId >> 8));
    pal_trx_reg_write(RG_IEEE_ADDR_0, (uint8_t)tal_pib_IeeeAddress);
    pal_trx_reg_write(RG_IEEE_ADDR_1, (uint8_t)(tal_pib_IeeeAddress >> 8));
    pal_trx_reg_write(RG_IEEE_ADDR_2, (uint8_t)(tal_pib_IeeeAddress >> 16));
    pal_trx_reg_write(RG_IEEE_ADDR_3, (uint8_t)(tal_pib_IeeeAddress >> 24));
    pal_trx_reg_write(RG_IEEE_ADDR_4, (uint8_t)(tal_pib_IeeeAddress >> 32));
    pal_trx_reg_write(RG_IEEE_ADDR_5, (uint8_t)(tal_pib_IeeeAddress >> 40));
    pal_trx_reg_write(RG_IEEE_ADDR_6, (uint8_t)(tal_pib_IeeeAddress >> 48));
    pal_trx_reg_write(RG_IEEE_ADDR_7, (uint8_t)(tal_pib_IeeeAddress >> 56));
    pal_trx_reg_write(RG_SHORT_ADDR_0, (uint8_t)tal_pib_ShortAddress);
    pal_trx_reg_write(RG_SHORT_ADDR_1, (uint8_t)(tal_pib_ShortAddress >> 8));

    /* configure TX_ARET; CSMA and CCA */
    pal_trx_bit_write(SR_CCA_MODE, tal_pib_CCAMode);
    pal_trx_bit_write(SR_MIN_BE, tal_pib_MinBE);

    pal_trx_bit_write(SR_AACK_I_AM_COORD, tal_pib_PrivatePanCoordinator);

    /* set phy parameter */
#ifdef TAL_2006
    apply_channel_page_configuration(tal_pib_CurrentPage);
#else   // channel page 0
    if (tal_pib_CurrentChannel == 0)
    {
        pal_trx_bit_write(SR_SUB_MODE, COMPLIANT_DATA_RATE_LOW);
    }
    else
    {
        pal_trx_bit_write(SR_SUB_MODE, COMPLIANT_DATA_RATE_HIGH);
    }

    pal_trx_bit_write(SR_BPSK_OQPSK, BPSK_MODE);
    pal_trx_bit_write(SR_GC_TX_OFFS, BPSK_TX_OFFSET);

#endif

    pal_trx_bit_write(SR_CHANNEL, tal_pib_CurrentChannel);

    {
        uint8_t reg_value;

        reg_value = convert_phyTransmitPower_to_reg_value(tal_pib_TransmitPower);
        pal_trx_reg_write(RG_PHY_TX_PWR, reg_value);
    }

#ifdef SPECIAL_PEER
    pal_trx_bit_write(SR_AACK_DIS_ACK, tal_pib_PrivateDisableACK);
#endif
}


/**
 * @brief Gets a TAL PIB attribute
 *
 * This function is called to retrieve the transceiver information base
 * attributes.
 *
 * @param[in] attribute TAL infobase attribute ID
 * @param[out] value TAL infobase attribute value
 *
 * @return UNSUPPORTED_ATTRIBUTE if the TAL infobase attribute is not found
 *         SUCCESS otherwise
 */
#if (HIGHEST_STACK_LAYER != MAC)
retval_t tal_pib_get(uint8_t attribute, void *value)
{
    switch (attribute)
    {
        case macMaxCSMABackoffs:
            *(uint8_t *)value = tal_pib_MaxCSMABackoffs;
            break;

        case macMinBE:
            *(uint8_t *)value = tal_pib_MinBE;
            break;

        case macPANId:
            *(uint16_t *)value = tal_pib_PANId;
            break;

        case macShortAddress:
            *(uint16_t *)value = tal_pib_ShortAddress;
            break;

        case phyCurrentChannel:
            *(uint8_t *)value = tal_pib_CurrentChannel;
            break;

        case phyChannelsSupported:
            *(uint32_t *)value = tal_pib_SupportedChannels;
            break;
#ifdef TAL_2006
        case phyCurrentPage:
            *(uint8_t *)value = tal_pib_CurrentPage;
            break;
#endif
        case phyTransmitPower:
            *(uint8_t *)value = tal_pib_TransmitPower;
            break;

        case phyCCAMode:
            *(uint8_t *)value = tal_pib_CCAMode;
            break;

        case macIeeeAddress:
            *(uint64_t *)value = tal_pib_IeeeAddress;
            break;
#ifdef SPECIAL_PEER
        case macPrivateCCAFailure:
            *(uint8_t *)value = tal_pib_PrivateCCAFailure;
            break;

        case macPrivateDisableACK:
            *(uint8_t *)value = tal_pib_PrivateDisableACK;
            break;
#endif
        case macBattLifeExt:
            *(bool *)value = tal_pib_BattLifeExt;
            break;

        case macBeaconOrder:
            *(uint8_t *)value = tal_pib_BeaconOrder;
            break;

        case macSuperframeOrder:
            *(uint8_t *)value = tal_pib_SuperFrameOrder;
            break;

        case macBeaconTxTime:
            *(uint32_t *)value = tal_pib_BeaconTxTime;
            break;

        case mac_i_pan_coordinator:
            *(bool *)value = tal_pib_PrivatePanCoordinator;
            break;

        case macPrivateAssociated:
            *(bool *)value = tal_pib_PrivateAssociated;
            break;

        case macAckWaitDuration:
            /*
             * AT86RF212 does not support changing this value w.r.t.
             * compliance operation.
             * Therefore the default value is returned depending on the currently used PHY.
             * The ACK timing can be reduced to 2 or 3 symbols using TFA function.
             */
            return UNSUPPORTED_ATTRIBUTE;

        default:
            /* Invalid attribute id */
            return UNSUPPORTED_ATTRIBUTE;
    }

    return SUCCESS;
} /* tal_pib_get() */
#endif  /* (HIGHEST_STACK_LAYER != MAC) */


///**
// * @brief Sets a TAL PIB attribute
// *
// * This function is called to set the transceiver information base
// * attributes.
// *
// * @param attribute TAL infobase attribute ID
// * @param value TAL infobase attribute value to be set
// *
// * @return UNSUPPORTED_ATTRIBUTE if the TAL info base attribute is not found
// *         BUSY if the TAL is not in TAL_IDLE state. An exception is
// *         macBeaconTxTime which can be accepted by TAL even if TAL is not
// *         in TAL_IDLE state.
// *         SUCCESS if the attempt to set the PIB attribute was successful
// *         TRX_ASLEEP if trx is in SLEEP mode and access to trx is required
// */
//retval_t tal_pib_set(uint8_t attribute, void *value)
//{
//    /*
//     * Do not allow any changes while ED or TX is done.
//     * We allow changes during RX, but it's on the user's own risk.
//     */
//#ifndef RFD
//    if (tal_state == TAL_ED_RUNNING)
//    {
//        ASSERT("TAL is busy" == 0);
//        return BUSY;
//    }
//#endif
//
//    /*
//     * Distinguish between PIBs that need to be changed in trx directly
//     * and those that are simple variable udpates.
//     * Ensure that the transceiver is not in SLEEP.
//     * If it is in SLEEP, change it to TRX_OFF.
//     */
//
//    switch (attribute)
//    {
//        case macBattLifeExt:
//            tal_pib_BattLifeExt = *((bool *)value);
//            break;
//
//        case macBeaconOrder:
//            tal_pib_BeaconOrder = *((uint8_t *)value);
//            break;
//
//        case macSuperframeOrder:
//            tal_pib_SuperFrameOrder = *((uint8_t *)value);
//            break;
//
//        case macBeaconTxTime:
//            tal_pib_BeaconTxTime = *((uint32_t *)value);
//            break;
//
//#ifdef SPECIAL_PEER
//        case macPrivateCCAFailure:
//            tal_pib_PrivateCCAFailure = *((uint8_t *)value);
//            break;
//#endif
//
//        case macPrivateAssociated:
//            tal_pib_PrivateAssociated = *((bool *)value);
//            break;
//
//        default:
//            /*
//             * Following PIBs require access to trx.
//             * Therefore trx must be at least in TRX_OFF.
//             */
//
//            if (tal_trx_status == TRX_SLEEP)
//            {
//                /* While trx is in SLEEP, register cannot be accessed. */
//                return TRX_ASLEEP;
//            }
//
//            switch (attribute)
//            {
//                case macMaxCSMABackoffs:
//                    tal_pib_MaxCSMABackoffs = *((uint8_t *)value);
//                    pal_trx_bit_write(SR_MAX_CSMA_RETRIES, tal_pib_MaxCSMABackoffs);
//                    break;
//
//                case macMinBE:
//                    tal_pib_MinBE = *((uint8_t *)value);
//                    pal_trx_bit_write(SR_MIN_BE, tal_pib_MinBE);
//                    break;
//
//                case macPANId:
//                    tal_pib_PANId = *((uint16_t *)value);
//#if (PAL_GENERIC_TYPE==AVR32)
//                    pal_trx_reg_write(RG_PAN_ID_1, (uint8_t)tal_pib_PANId);
//                    pal_trx_reg_write(RG_PAN_ID_0, (uint8_t)(tal_pib_PANId >> 8));
//#else
//                    pal_trx_reg_write(RG_PAN_ID_0, (uint8_t)tal_pib_PANId);
//                    pal_trx_reg_write(RG_PAN_ID_1, (uint8_t)(tal_pib_PANId >> 8));
//#endif
//                    break;
//
//                case macShortAddress:
//                    tal_pib_ShortAddress = *((uint16_t *)value);
//#if (PAL_GENERIC_TYPE==AVR32)
//                    pal_trx_reg_write(RG_SHORT_ADDR_1, (uint8_t)tal_pib_ShortAddress);
//                    pal_trx_reg_write(RG_SHORT_ADDR_0, (uint8_t)(tal_pib_ShortAddress >> 8));
//#else
//                    pal_trx_reg_write(RG_SHORT_ADDR_0, (uint8_t)tal_pib_ShortAddress);
//                    pal_trx_reg_write(RG_SHORT_ADDR_1, (uint8_t)(tal_pib_ShortAddress >> 8));
//#endif
//                    break;
//
//                case phyCurrentChannel:
//                    if (tal_state != TAL_IDLE)
//                    {
//                        return BUSY;
//                    }
//
//                    if ((uint32_t)TRX_SUPPORTED_CHANNELS & ((uint32_t)0x01 << *((uint8_t *)value)))
//                    {
//                        uint8_t previous_channel;
//
//                        previous_channel = tal_pib_CurrentChannel;
//                        tal_pib_CurrentChannel = *((uint8_t *)value);
//
//                        /* Check if frequency band is changed. */
//                        if ((tal_pib_CurrentChannel > 0) && (previous_channel > 0))
//                        {
//                            pal_trx_bit_write(SR_CHANNEL, tal_pib_CurrentChannel);
//                        }
//                        else
//                        {
//                            /* Switch to TRX_OFF quickly for modulation change */
//                            if (tal_trx_status == TRX_OFF)
//                            {
//                                pal_trx_bit_write(SR_CHANNEL, tal_pib_CurrentChannel);
//                            }
//                            else
//                            {
//                                /* Accelerate switching: keep regualtor on */
//                                pal_trx_bit_write(SR_TRX_OFF_AVDD_EN, 1);
//                                pal_trx_bit_write(SR_TRX_CMD, CMD_FORCE_TRX_OFF);
//                                pal_trx_bit_write(SR_CHANNEL, tal_pib_CurrentChannel);
//                            }
//
//                            /* Set modulation */
//                            if (tal_pib_CurrentChannel == 0)
//                            {
//                                pal_trx_bit_write(SR_SUB_MODE, COMPLIANT_DATA_RATE_LOW);
//                            }
//                            else
//                            {
//                                pal_trx_bit_write(SR_SUB_MODE, COMPLIANT_DATA_RATE_HIGH);
//                            }
//
//                            /* Store previous trx state */
//                            if (tal_trx_status != TRX_OFF)
//                            {
//                                set_trx_state(CMD_RX_AACK_ON);
//                                pal_trx_bit_write(SR_TRX_OFF_AVDD_EN, 0);   // switch regualtor off
//                            }
//                        }
//                    }
//                    else
//                    {
//                        return INVALID_PARAMETER;
//                    }
//
//                    break;
//#ifdef TAL_2006
//                case phyCurrentPage:
//                    if (tal_state != TAL_IDLE)
//                    {
//                        return BUSY;
//                    }
//                    else
//                    {
//                        tal_trx_status_t trx_state;
//                        // Changing the channel, channel page or modulation requires that TRX is in TRX_OFF.
//                        do
//                        {
//                            trx_state = set_trx_state(CMD_TRX_OFF);
//                        } while (trx_state != TRX_OFF);
//                    }
//
//                    {
//                        uint8_t page;
//
//                        page = *((uint8_t *)value);
//
//                        if (apply_channel_page_configuration(page))
//                        {
//                            tal_pib_CurrentPage = page;
//                        }
//                        else
//                        {
//                            return INVALID_PARAMETER;
//                        }
//                    }
//                    break;
//#endif
//                case phyTransmitPower:
//                    {
//                        uint8_t reg_value;
//
//                        tal_pib_TransmitPower = *((uint8_t *)value);
//
//                        /* Limit tal_pib_TransmitPower to max/min trx values */
//                        tal_pib_TransmitPower = limit_tx_pwr(tal_pib_TransmitPower);
//                        reg_value = convert_phyTransmitPower_to_reg_value(tal_pib_TransmitPower);
//                        pal_trx_reg_write(RG_PHY_TX_PWR, reg_value);
//                    }
//                    break;
//
//                case phyCCAMode:
//                    tal_pib_CCAMode = *((uint8_t *)value);
//                    pal_trx_bit_write(SR_CCA_MODE, tal_pib_CCAMode);
//                    break;
//
//                case macIeeeAddress:
//                    tal_pib_IeeeAddress = *((uint64_t *)value);
//#if (PAL_GENERIC_TYPE==AVR32)
//                    pal_trx_reg_write(RG_IEEE_ADDR_7, (uint8_t)tal_pib_IeeeAddress);
//                    pal_trx_reg_write(RG_IEEE_ADDR_6, (uint8_t)(tal_pib_IeeeAddress >> 8));
//                    pal_trx_reg_write(RG_IEEE_ADDR_5, (uint8_t)(tal_pib_IeeeAddress >> 16));
//                    pal_trx_reg_write(RG_IEEE_ADDR_4, (uint8_t)(tal_pib_IeeeAddress >> 24));
//                    pal_trx_reg_write(RG_IEEE_ADDR_3, (uint8_t)(tal_pib_IeeeAddress >> 32));
//                    pal_trx_reg_write(RG_IEEE_ADDR_2, (uint8_t)(tal_pib_IeeeAddress >> 40));
//                    pal_trx_reg_write(RG_IEEE_ADDR_1, (uint8_t)(tal_pib_IeeeAddress >> 48));
//                    pal_trx_reg_write(RG_IEEE_ADDR_0, (uint8_t)(tal_pib_IeeeAddress >> 56));
//#else
//                    pal_trx_reg_write(RG_IEEE_ADDR_0, (uint8_t)tal_pib_IeeeAddress);
//                    pal_trx_reg_write(RG_IEEE_ADDR_1, (uint8_t)(tal_pib_IeeeAddress >> 8));
//                    pal_trx_reg_write(RG_IEEE_ADDR_2, (uint8_t)(tal_pib_IeeeAddress >> 16));
//                    pal_trx_reg_write(RG_IEEE_ADDR_3, (uint8_t)(tal_pib_IeeeAddress >> 24));
//                    pal_trx_reg_write(RG_IEEE_ADDR_4, (uint8_t)(tal_pib_IeeeAddress >> 32));
//                    pal_trx_reg_write(RG_IEEE_ADDR_5, (uint8_t)(tal_pib_IeeeAddress >> 40));
//                    pal_trx_reg_write(RG_IEEE_ADDR_6, (uint8_t)(tal_pib_IeeeAddress >> 48));
//                    pal_trx_reg_write(RG_IEEE_ADDR_7, (uint8_t)(tal_pib_IeeeAddress >> 56));
//#endif
//                    break;
//
//#ifdef SPECIAL_PEER
//                case macPrivateDisableACK:
//                    tal_pib_PrivateDisableACK = *((uint8_t *)value);
//                    pal_trx_bit_write(SR_AACK_DIS_ACK, tal_pib_PrivateDisableACK);
//                    break;
//#endif
//
//                case mac_i_pan_coordinator:
//                    tal_pib_PrivatePanCoordinator = *((bool *)value);
//                    pal_trx_bit_write(SR_AACK_I_AM_COORD, tal_pib_PrivatePanCoordinator);
//                    break;
//
//                case macAckWaitDuration:
//                    /*
//                     * AT86RF212 does not support changing this value w.r.t.
//                     * compliance operation.
//                     */
//                    return UNSUPPORTED_ATTRIBUTE;
//
//                default:
//                    return UNSUPPORTED_ATTRIBUTE;
//            }
//
//            break; /* end of 'default' from 'switch (attribute)' */
//    }
//    return SUCCESS;
//} /* tal_pib_set() */


/**
 * @brief Limit the phyTransmitPower to the trx limits
 *
 * @param phyTransmitPower phyTransmitPower value
 *
 * @return limited tal_pib_TransmitPower
 */
static uint8_t limit_tx_pwr(uint8_t tal_pib_TransmitPower)
{
    int8_t dbm_value;

    dbm_value = CONV_phyTransmitPower_TO_DBM(tal_pib_TransmitPower);

    /* Limit to the transceiver's absolute maximum/minimum. */
    if (dbm_value > 11)
    {
        dbm_value = 11;
    }
    else if (dbm_value < -11)
    {
        dbm_value = -11;
    }

    /* Tx power limits depend on the currently used channel and channel page */
    if (tal_pib_CurrentChannel == 0)
    {
#ifdef TAL_2006
        if (tal_pib_CurrentPage == 0)
        {
            if (dbm_value > MAX_TX_PWR_BPSK20)
            {
                dbm_value = MAX_TX_PWR_BPSK20;
            }
        }
        else  // OQPSK
        {
            if (dbm_value > MAX_TX_PWR_OQPSK)
            {
                dbm_value = MAX_TX_PWR_OQPSK;
            }
        }
#else
        if (dbm_value > MAX_TX_PWR_BPSK20)
        {
            dbm_value = MAX_TX_PWR_BPSK20;
        }
#endif
    }

    tal_pib_TransmitPower = CONV_DBM_TO_phyTransmitPower(dbm_value);

    return (tal_pib_TransmitPower | TX_PWR_TOLERANCE);
}


/**
 * @brief Converts a phyTransmitPower value to a register value
 *
 * @param phyTransmitPower_value phyTransmitPower value
 *
 * @return register value
 */
static uint8_t convert_phyTransmitPower_to_reg_value(uint8_t phyTransmitPower_value)
{
//    int8_t dbm_value;
//    uint8_t trx_tx_level;
//
//    dbm_value = CONV_phyTransmitPower_TO_DBM(phyTransmitPower_value);
//
//    /* Select the corresponding tx_pwr_table */
//    if (tal_pib_CurrentChannel == 0)
//    {
//        trx_tx_level = PGM_READ_BYTE(&tx_pwr_table_EU[11 - dbm_value]);
//    }
//    else    // channels 1-10
//    {
//        trx_tx_level = PGM_READ_BYTE(&tx_pwr_table_NA[11 - dbm_value]);
//    }
//
//    return trx_tx_level;
	return 0x22;
}


#ifdef TAL_2006
/**
 * @brief Apply channel page configuartion to transceiver
 *
 * @param ch_page Channel page
 *
 * @return true if changes could be applied else false
 */
static bool apply_channel_page_configuration(uint8_t ch_page)
{
    if (tal_pib_CurrentChannel == 0)
    {
        pal_trx_bit_write(SR_SUB_MODE, COMPLIANT_DATA_RATE_LOW);
    }
    else
    {
        pal_trx_bit_write(SR_SUB_MODE, COMPLIANT_DATA_RATE_HIGH);
    }

    switch (ch_page)
    {
        case 0: /* BPSK */
                pal_trx_bit_write(SR_BPSK_OQPSK, BPSK_MODE);
                pal_trx_bit_write(SR_GC_TX_OFFS, BPSK_TX_OFFSET);
                // Compliant ACK timing
                pal_trx_bit_write(SR_AACK_ACK_TIME, AACK_ACK_TIME_12_SYMBOLS);
                break;

            case 2: /* O-QPSK */
                pal_trx_bit_write(SR_BPSK_OQPSK, OQPSK_MODE);
                pal_trx_bit_write(SR_GC_TX_OFFS, OQPSK_TX_OFFSET);
                pal_trx_bit_write(SR_OQPSK_DATA_RATE, ALTRATE_100_KBPS_OR_250_KBPS);
                // Compliant ACK timing
                pal_trx_bit_write(SR_AACK_ACK_TIME, AACK_ACK_TIME_12_SYMBOLS);
                break;
#ifdef HIGH_DATA_RATE_SUPPORT
            case 16:    /* non-compliant OQPSK mode 1 */
                pal_trx_bit_write(SR_BPSK_OQPSK, OQPSK_MODE);
                pal_trx_bit_write(SR_GC_TX_OFFS, OQPSK_TX_OFFSET);
                pal_trx_bit_write(SR_OQPSK_DATA_RATE, ALTRATE_200_KBPS_OR_500_KBPS);
                // Reduced ACK timing
                pal_trx_bit_write(SR_AACK_ACK_TIME, AACK_ACK_TIME_2_SYMBOLS);
                break;

            case 17:    /* non-compliant OQPSK mode 2 */
                pal_trx_bit_write(SR_BPSK_OQPSK, OQPSK_MODE);
                pal_trx_bit_write(SR_GC_TX_OFFS, OQPSK_TX_OFFSET);
                pal_trx_bit_write(SR_OQPSK_DATA_RATE, ALTRATE_400_KBPS_OR_1_MBPS);
                // Reduced ACK timing
                pal_trx_bit_write(SR_AACK_ACK_TIME, AACK_ACK_TIME_2_SYMBOLS);
                break;
#endif
            default:
                return false;
    }

    return true;
}
#endif

/* EOF */

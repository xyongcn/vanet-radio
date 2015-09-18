/**
 * @file tal_constants.h
 *
 * @brief This file contains constants used througthout the TAL.
 *
 * $Id: tal_constants.h 12372 2008-12-02 12:25:17Z uwalter $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2008, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> LICENSE.txt
 */

/* Prevent double inclusion */
#ifndef TAL_CONSTANTS_H
#define TAL_CONSTANTS_H

/* === INCLUDES ============================================================ */


/* === EXTERNALS =========================================================== */


/* === TYPES =============================================================== */


/* === MACROS ============================================================== */

/*
 * TAL PIB default values
 */

/*
 * Default value of ack wait duration
 */
#define TAL_ACK_WAIT_DURATION_DEFAULT       (0x36)

/*
 * Default value of maximum csma ca backoffs
 */
#define TAL_MAX_CSMA_BACKOFFS_DEFAULT       (0x04)

/*
 * Default value of backoff exponent used while performing csma ca
 */
#define TAL_MINBE_DEFAULT                   (0x03)

/*
 * Value of a broadcast PAN ID
 */
#define TAL_PANID_BC_DEFAULT                (0xFFFF)

/*
 * Default value of short address
 */
#define TAL_SHORT_ADDRESS_DEFAULT           (0xFFFF)

/*
 * Default value of current channel in TAL
 */
#define TAL_CURRENT_CHANNEL_DEFAULT         (0x01)

#ifdef TAL_2006
/*
 * Default value of custom TAL PIB channel page
 */
#ifdef OQPSK_TEST
#define TAL_CURRENT_PAGE_DEFAULT            (0x02)
#else
#define TAL_CURRENT_PAGE_DEFAULT            (0x00)
#endif  /* #ifdef OQPSK_TEST */
#endif  /* #ifdef TAL_2006 */

/*
 * Default value of transmit power of transceiver
 * using IEEE defined format of phyTransmitPower
 * Following value corresponds to +3 dBm
 */
#ifdef SPECIAL_PEER
// Use lowest available tx power
#define TAL_TRANSMIT_POWER_DEFAULT          (TX_PWR_TOLERANCE | 0x21)
#else
#define TAL_TRANSMIT_POWER_DEFAULT          (TX_PWR_TOLERANCE | 0x0B)
#endif

/*
 * Default value CCA mode
 */
#define TAL_CCA_MODE_DEFAULT                (CCA_MODE1)

/*
 * Default value beacon order set to 15
 */
#define TAL_BEACON_ORDER_DEFAULT            (15)

/*
 * Default value supeframe order set to 15
 */
#define TAL_SUPERFRAME_ORDER_DEFAULT        (15)

/*
 * Radio wakeuptime -> (880/16) = 55 symbols, for this radio
 */
//#define TAL_PIB_RADIO_WAKEUPTIME_DEFAULT    (0x37)

/*
 * Default value of BeaconTxTime
 */
#define TAL_BEACON_TX_TIME_DEFAULT          (0x00000000)

/*
 * Default value of BatteryLifeExtension.
 */
#define TAL_BATTERY_LIFE_EXTENSION_DEFAULT  (false)

/*
 * Default value of PAN Coordiantor custom TAL PIB
 */
#define TAL_PAN_COORDINATOR_DEFAULT         (false)

/*
 * Default value of Frame Pending custom TAL PIB
 */
//#define TAL_FRAME_PENDING_DEFAULT           (false)

/*
 * Default value of custom TAL PIB indicating associated
 */
#define TAL_PRIVATE_ASSOCIATED_DEFAULT      (false)


#ifdef SPECIAL_PEER
/*
 * Default value of CCA FAILURE scenario.
 */
#define TAL_PRIVATE_CCA_FAILURE_DEFAULT     (0x00)

/*
 * Default value of Disable Ack attribute
 */
#define TAL_PRIVATE_DISABLE_ACK_DEFAULT     (0x00)

#endif /* SPECIAL_PEER */

/*
 * Mask used, to check if the frame is Beacon frame
 */
#define BEACON_FRAME                        (0x0000)

/*
 * Mask used, to check if the frame is a Data frame
 */
#define DATA_FRAME                          (0x0001)

/*
 * Mask to check if the frame is a MAC Command frame
 */
#define MAC_CMD_FRAME                       (0x0003)

/*
 * Mask used, to obtain the frame type from the FCF field
 */
#define FRAME_TYPE_MASK                     (0x0007)

/*
 * Mask used, to check if frame has a short source address
 */
#define SRC_SHORT_ADDR_MODE                 (0x8000)

/*
 * Mask used, to check if frame has a long source address
 */
#define SRC_EXT_ADDR_MODE                   (0xC000)

/*
 * Mask used, to check if frame has a short destination address
 */
#define DEST_SHORT_ADDR_MODE                (0x0800)

/*
 * Mask used, to check if frame has a long destination address
 */
#define DEST_EXT_ADDR_MODE                  (0x0C00)

/*
 * Mask used, to check if the intra PAN bit is set or not
 */
#define INTRA_PAN_SUBFIELD                  (0x0040)

/*
 * Mask used, to obtain the destination address mode from the FCF field
 */
#define DEST_ADDR_MODE_MASK                 (0x0C00)

/*
 * Mask used, to obtain the source address mode from the FCF field
 */
#define SRC_ADDR_MODE_MASK                  (0xC000)

/*
 * Size of the length parameter
 */
#define LENGTH_FIELD_LEN                    (0x01)

/*
 * Length (in octets) of extended address
 */
#define EXT_ADDR_LEN                        (0x08)

/*
 * Length (in octets) of short address
 */
#define SHORT_ADDR_LEN                      (0x02)

/*
 * Length (in octets) of PAN ID
 */
#define PAN_ID_LEN                          (0x02)

/*
 * Length (in octets) of FCF
 */
#define FCF_LEN                             (0x02)

/*
 * Length (in octets) of FCS
 */
#define FCS_LEN                             (0x02)

/* === PROTOTYPES ========================================================== */

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TAL_CONSTANTS_H */

/* EOF */



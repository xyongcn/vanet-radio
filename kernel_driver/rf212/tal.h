/**
 * @file tal.h
 *
 * @brief This file contains TAL API function declarations
 *
 * $Id: tal.h 12183 2008-11-24 10:32:42Z uwalter $
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
#ifndef TAL_H
#define TAL_H

/* === INCLUDES ============================================================ */

//#include <stdint.h>
//#include <stdbool.h>
//#include "tal_config.h"
//#include "bmm.h"
//#include "stack_config.h"
#include "return_val.h"
//#include "platform_types.h"
//#include "tal_types.h"
#include <linux/types.h>


/* === EXTERNALS =========================================================== */

/* PIB values stored in TAL */
extern uint8_t tal_pib_CCAMode;
extern uint8_t tal_pib_ACKWaitDuration;
extern uint8_t tal_pib_CurrentChannel;
extern uint32_t tal_pib_SupportedChannels;
extern uint64_t tal_pib_IeeeAddress;
extern uint8_t tal_pib_MaxCSMABackoffs;
extern uint8_t tal_pib_MinBE;
extern uint16_t tal_pib_PANId;
extern bool tal_pib_PrivatePanCoordinator;
extern bool tal_pib_PrivateAssociated;
extern uint16_t tal_pib_ShortAddress;
extern uint8_t tal_pib_TransmitPower;
extern bool tal_pib_BattLifeExt;
extern uint8_t tal_pib_BeaconOrder;
extern uint8_t tal_pib_SuperFrameOrder;
extern uint32_t tal_pib_BeaconTxTime;

#ifdef TAL_2006
extern uint8_t tal_pib_CurrentPage;
#endif

#ifdef SPECIAL_PEER
extern uint8_t tal_pib_PrivateCCAFailure;
extern uint8_t tal_pib_PrivateDisableACK;
#endif /* SPECIAL_PEER */



/* === TYPES =============================================================== */

typedef enum
{
/* MAC Command Frames (table 67) */
    /* Command Frame Identifier for Association Request */
    ASSOCIATIONREQUEST          = (0x01),
    /* Command Frame Identifier for Association Response */
    ASSOCIATIONRESPONSE,
    /* Command Frame Identifier for Disassociation Notification */
    DISASSOCIATIONNOTIFICATION,
    /* Command Frame Identifier for Data Request */
    DATAREQUEST,
    /* Command Frame Identifier for PANID Conflict Notification */
    PANIDCONFLICTNOTIFICAION,
    /* Command Frame Identifier for Orphan Notification */
    ORPHANNOTIFICATION,
    /* Command Frame Identifier for Beacon Request */
    BEACONREQUEST,
    /* Command Frame Identifier for Coordinator Realignment */
    COORDINATORREALIGNMENT,

/*
 * These are not MAC command frames but listed here as they are needed
 * in the msgtype field
 */
    /* Message is a directed orphan realignment command */
    ORPHANREALIGNMENT,
    /* Message is a beacon frame (in response to a beacon request cmd) */
    BEACON_MESSAGE,
    /* Message type field value for implicite poll without request */
    DATAREQUEST_IMPL_POLL,
    /* Message type field value for Null frame */
    NULL_FRAME,
    /* Message type field value for MCPS message */
    MCPS_MESSAGE
} frame_msgtype_t;



/**
 * Globally used frame info structure
 */

typedef struct frame_info_tag
{
    /*
     * Although the size member parameter is never evaluated, it is required
     * since this complies to the message structure required for the
     * dispatcher; see other message types in msg_types.h).
     */
    uint8_t size;
    uint8_t msg_type;
    buffer_t *buffer_header;
    uint16_t frame_ctrl;
    uint8_t seq_num;
    uint16_t dest_panid;
    uint64_t dest_address;
    uint16_t src_panid;
    uint64_t src_address;
    uint8_t payload_length;
    uint32_t time_stamp;
    uint8_t *payload;
} frame_info_t;


/**
 * Sleep Mode supported by the transceiver
 */
typedef enum sleep_mode_tag
{
    SLEEP_MODE_1
} sleep_mode_t;


/* === MACROS ============================================================== */

/* RF bands: */
/**
 * 868 / 910 MHz (channels 0 through 10)
 * using BPSK
 */
#define BAND_900                            (0)

/**
 * 2.4 GHz (channels 11 through 26)
 */
#define BAND_2400                           (1)

/**
 * AT86RF230 operates in the 2.4GHz band
 */
#if ((TAL_TYPE == AT86RF230A) || (TAL_TYPE == AT86RF230B) || (TAL_TYPE == AT86RF231))
#define RF_BAND                             BAND_2400
#elif (TAL_TYPE == AT86RF212)
#define RF_BAND                             BAND_900
#else
#error "Missing RF_BAND define"
#endif

#if (RF_BAND == BAND_2400)
/**
 * 4 bits form one symbol since O-QPSK is used
 */
#define SYMBOLS_PER_OCTET                   (2)

#elif (RF_BAND == BAND_900)
/**
 * 1 bit forms one symbol since BPSK is used
 */
#define SYMBOLS_PER_OCTET                   (8)
#else
#error "Unsupported RF band"
#endif

/**
 * The maximum time in symbols for a 32 bit timer
 */
#define MAX_SYMBOL_TIME                     (0x0FFFFFFF)

/**
 * Symbol mask for ingnoring most significant nibble
 */
#define SYMBOL_MASK                         (0x0FFFFFFF)

/* Custom attribute used by TAL */

/**
 * Attribute id of mac_i_pan_coordinator PIB
 */
#define mac_i_pan_coordinator               (0x0B)

/**
 * Conversion of symbols to microseconds
 */
#if (RF_BAND == BAND_2400)
    #define TAL_CONVERT_SYMBOLS_TO_US(symbols)      ((uint32_t)(symbols) << 4)
#else   /* (RF_BAND == BAND_900) */
    #ifdef TAL_2006
        #define TAL_CONVERT_SYMBOLS_TO_US(symbols)                                                        \
            (tal_pib_CurrentPage == 0 ?                                                                   \
                (tal_pib_CurrentChannel == 0 ? ((uint32_t)(symbols) * 50) : ((uint32_t)(symbols) * 25)) : \
                (tal_pib_CurrentChannel == 0 ? ((uint32_t)(symbols) * 40) : ((uint32_t)(symbols) << 4))   \
            )
    #else
        #define TAL_CONVERT_SYMBOLS_TO_US(symbols)      (tal_pib_CurrentChannel == 0 ? ((uint32_t)(symbols) * 50) : ((uint32_t)(symbols) * 25))
    #endif  /* #ifdef TAL_2006 */
#endif  /* #if (RF_BAND == BAND_2400) */

/**
 * Conversion of microseconds to symbols
 */
#if (RF_BAND == BAND_2400)
    #define TAL_CONVERT_US_TO_SYMBOLS(time)         ((time) >> 4)
#else   /* (RF_BAND == BAND_900) */
    #ifdef TAL_2006
        #define TAL_CONVERT_US_TO_SYMBOLS(time)                                 \
            (tal_pib_CurrentPage == 0 ?                                         \
                (tal_pib_CurrentChannel == 0 ? ((time) / 50) : ((time) / 25)) : \
                (tal_pib_CurrentChannel == 0 ? ((time) / 40) : ((time) >> 4))   \
            )
    #else
        #define TAL_CONVERT_US_TO_SYMBOLS(time)         (tal_pib_CurrentChannel == 0 ? ((time) / 50) : ((time) / 25))
    #endif  /* #ifdef TAL_2006 */
#endif  /* #if (RF_BAND == BAND_2400) */

/**
 * Beacon Interval formula: BI = aBaseSuperframeDuration 2^BO\f$0
 * where \f$0 <= BO <= 14. Note: Beacon interval calculated is in
 * symbols.
 */
#define TAL_GET_BEACON_INTERVAL_TIME(BO) \
        ((1UL * aBaseSuperframeDuration) << (BO))

/**
 * Superframe Duration formula: \f$BI = aBaseSuperframeDuration 2^SO\f$
 * where \f$0 <= SO <= BO\f$
 */
#define TAL_GET_SUPERFRAME_DURATION_TIME(SO) \
        ((1UL * aBaseSuperframeDuration) << (SO))


/* === PROTOTYPES ========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

void tal_task(void);
retval_t tal_init(void);
retval_t tal_reset(bool set_default_pib);
#if (DEBUG > 0)
void tal_trx_state_reset(void);
#endif

#ifndef RFD
retval_t tal_ed_start(uint8_t scan_duration);
void tal_ed_end_cb(uint8_t energy_level);
#endif

#if (HIGHEST_STACK_LAYER != MAC)
retval_t tal_pib_get(uint8_t attribute, void *value);
#endif
retval_t tal_pib_set(uint8_t attribute, void *value);

uint8_t tal_rx_enable(uint8_t state);
void tal_rx_frame_cb(frame_info_t *mac_frame_info, uint8_t lqi);

#ifndef RFD
#ifndef NOBEACON
void tal_prepare_beacon(frame_info_t *mac_frame_info);
void tal_tx_beacon(void);
#endif
#endif

retval_t tal_tx_frame(frame_info_t *mac_frame_info, bool perform_csma_ca,
                      bool perform_frame_retry);
void tal_tx_frame_done_cb(retval_t status, frame_info_t *frame);

retval_t tal_trx_sleep(sleep_mode_t mode);
retval_t tal_trx_wakeup(void);


/**
 * @brief Adds two time values
 *
 * This function adds two time values
 *
 * @param a Time value 1
 * @param b Time value 2
 *
 * @return value of a + b
 */
static inline uint32_t tal_add_time_symbols(uint32_t a, uint32_t b)
{
    return ((a + b) & SYMBOL_MASK);
}


/**
 * @brief Subtract two time values
 *
 * This function subtracts two time values taking care of roll over.
 *
 * @param a Time value 1
 * @param b Time value 2
 *
 * @return value a - b
 */
static inline uint32_t tal_sub_time_symbols(uint32_t a, uint32_t b)
{
    if (a > b)
    {
        return ((a - b) & SYMBOL_MASK);
    }
    else
    {
        /* This is a roll over case */
        return (((MAX_SYMBOL_TIME - b) + a) & SYMBOL_MASK);
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TAL_H */
/* EOF */

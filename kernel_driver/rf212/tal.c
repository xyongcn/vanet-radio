/* === INCLUDES ============================================================ */
#include "rf212_api.h"
#include "return_val.h"

#include "tal_pib.h"
#include "tal.h"
#include "at86rf212.h"

//#include "tal_rx.h"
//#include "tal_tx.h"

//#ifndef RFD
//#include "ffd_data_structures.h"
//#endif  /* FFD */
#ifndef NOBEACON
#include "tal_slotted_csma.h"
#endif  /* BEACON */

/* === TYPES =============================================================== */


/* === MACROS ============================================================== */


/* === GLOBALS ============================================================= */

/*
 * TAL PIBs
 */
/**
 * The maximum number of back-offs the CSMA-CA algorithm will attempt
 * before declaring a CSMA_CA failure.
 */
uint8_t tal_pib_MaxCSMABackoffs;

/**
 * The minimum value of the backoff exponent BE in the CSMA-CA algorithm.
 */
uint8_t tal_pib_MinBE;

/**
 * 16-bit PAN ID.
 */
uint16_t tal_pib_PANId;

/**
 * Node's 16-bit short address.
 */
uint16_t tal_pib_ShortAddress;

/**
 * Node's 64-bit (IEEE) address.
 */
uint64_t tal_pib_IeeeAddress;

/**
 * Current RF channel to be used for all transmissions and receptions.
 */
uint8_t tal_pib_CurrentChannel;

/**
 * Supported channels
 */
uint32_t tal_pib_SupportedChannels;

#ifdef TAL_2006
/**
 * Current channel page; supported: page 0 and 2
 */
uint8_t tal_pib_CurrentPage;
#endif

/**
 * Default value of transmit power of transceiver
 * using IEEE defined format of phyTransmitPower
 */
uint8_t tal_pib_TransmitPower;

/**
 * CCA Mode.
 */
uint8_t tal_pib_CCAMode;

/**
 * macACKWaitDuration.
 */
uint8_t tal_pib_ACKWaitDuration;

/**
 * Indicates if the node is a PAN coordinator or not.
 */
bool tal_pib_PrivatePanCoordinator;

/**
 * Indicates if the node is associated.
 */
bool tal_pib_PrivateAssociated;

/**
 * Indication of whether battery life extension is enabled or not.
 */
bool tal_pib_BattLifeExt;

/**
 * Beacon order.
 */
uint8_t tal_pib_BeaconOrder;

/**
 * Superframe order.
 */
uint8_t tal_pib_SuperFrameOrder;

/**
 * Holds the time at which last beacon was transmitted or received.
 */
uint32_t tal_pib_BeaconTxTime;

#ifdef SPECIAL_PEER
/**
 * Private TAL PIB attribute to generate a CCA Channel Access Failure.
 * Value 0 implements normal CCA behaviour.
 * Value 1 leads to CCA Channel Access Failure.
 * Any other value will also implement normal CCA behaviour.
 */
uint8_t tal_pib_PrivateCCAFailure;

/**
 * Private TAL PIB attribute to disable ACK sending.
 * Value 0 implements normal ACK sending. Value 255 disables ACK
 * sending completely. Any other value will arrange for the
 * respective number of ACKs from being sent.
 */
uint8_t tal_pib_PrivateDisableACK;
#endif /* SPECIAL_PEER */



/*
 * Global TAL variables
 * These variables are only to be used by the TAL internally.
 */

/**
 * Current state of the TAL state machine.
 */
tal_state_t tal_state;

/**
 * Current state of the transceiver.
 */
tal_trx_status_t tal_trx_status;

/**
 * Indicates if the transceiver needs to switch on its receiver by tal_task(),
 * because it could not be switched on due to buffer shortage.
 */
bool tal_rx_on_required;

/**
 * Pointer to the 15.4 frame created by the TAL to be handed over
 * to the transceiver.
 */
uint8_t *tal_frame_to_tx;

/**
 * Pointer to receive buffer that can be used to upload a frame from the trx.
 */
buffer_t *tal_rx_buffer = NULL;

/**
 * Queue that contains all frames that are uploaded from the trx, but have not
 * be processed by the MCL yet.
 */
queue_t tal_incoming_frame_queue;

/**
 * Frame pointer for the frame structure provided by the MCL.
 */
frame_info_t *mac_frame_ptr;

/**
 * Timestamp
 */
uint32_t tal_rx_timestamp;

#ifndef NOBEACON
/**
 * CSMA state machine variable
 */
csma_state_t tal_csma_state;
#endif

#if ((!defined RFD) && (!defined NOBEACON))
/*
 * Flag indicating if beacon transmission is currently in progress.
 */
bool tal_beacon_transmission;
#endif

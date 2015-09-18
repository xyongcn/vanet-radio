/**
 * @file ieee_const.h
 *
 * @brief This header holds all IEEE 802.15.4 constants and attribute identifiers
 *
 * $Id: ieee_const.h 11699 2008-11-03 10:11:13Z uwalter $
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
#ifndef IEEE_CONST_H
#define IEEE_CONST_H

#define BAND_900                            (0)

#define RF_BAND BAND_900
//#if !defined(RF_BAND)
//#error "Please define RF_BAND to BAND_2400 or BAND_900."
//#endif /* !defined(RF_BAND) */

/* === Includes ============================================================= */


/* === Macros =============================================================== */

/**
 * Minimum size of a valid frame other than an Ack frame
 */
#define MIN_FRAME_LENGTH                (8)

/**
 * Maximum size of the management frame(Association Response frame)
 */
#define MAX_MGMT_FRAME_LENGTH           (30)

/* === MAC Constants ======================================================== */

/**
 * Maximum size of PHY packet
 * @ingroup apiMacConst
 */
#define aMaxPHYPacketSize               (127)

/**
 * Maximum turnaround Time of the radio to switch from Rx to Tx or Tx to Rx
 * in symbols
 * @ingroup apiMacConst
 */
#define aTurnaroundTime                 (12)

/* 7.4.1 MAC Layer Constants */

/**
 * The number of symbols forming a superframe slot
 * when the superframe order is equal to 0.
 * @ingroup apiMacConst
 */
#define aBaseSlotDuration               (60)

/**
 * The number of symbols forming a superframe when
 * the superframe order is equal to 0.
 * @ingroup apiMacConst
 */
#define aBaseSuperframeDuration         (aBaseSlotDuration * aNumSuperframeSlots)

/**
 * The maximum value of the backoff exponent in the CSMA-CA algorithm.
 * @ingroup apiMacConst
 */
#define aMaxBE                          (5)

/**
 * The maximum number of octets added by the MAC
 * sublayer to the payload of its beacon frame.
 * @ingroup apiMacConst
 */
#define aMaxBeaconOverhead              (75)

/**
 * The maximum size, in octets, of a beacon payload.
 * @ingroup apiMacConst
 */
#define aMaxBeaconPayloadLength         (aMaxPHYPacketSize - aMaxBeaconOverhead)

/**
 * The number of superframes in which a GTS descriptor
 * exists in the beacon frame of a PAN coordinator.
 * @ingroup apiMacConst
 */
#define aGTSDescPersistenceTime         (4)

/**
 * The maximum number of octets added by the MAC
 * sublayer to its payload without security. If security is
 * required on a frame, its secure processing may inflate
 * the frame length so that it is greater than this value. In
 * this case, an error is generated through the appropriate
 * confirm or MLME-COMM-STATUS.indication primitives.
 * @ingroup apiMacConst
 */
#define aMaxFrameOverhead               (25)

/**
 * The maximum number of CAP symbols in a beaconenabled PAN, or symbols in a
 * nonbeacon-enabled PAN, to wait for a frame intended as a response to a
 * data request frame.
 * @ingroup apiMacConst
 */
#define aMaxFrameResponseTime           (1220)

/**
 * The maximum number of retries allowed after a transmission failure.
 * @ingroup apiMacConst
 */
#define aMaxFrameRetries                (3)

/**
 * The number of consecutive lost beacons that will cause the MAC sublayer of
 * a receiving device to declare a loss of synchronization.
 * @ingroup apiMacConst
 */
#define aMaxLostBeacons                 (4)

/**
 * The maximum number of octets that can be
 * transmitted in the MAC frame payload field.
 * @ingroup apiMacConst
 */
#define aMaxMACFrameSize                (aMaxPHYPacketSize - aMaxFrameOverhead)

/**
 * The maximum size of an MPDU, in octets, that can be
 * followed by a short interframe spacing (SIFS) period.
 * @ingroup apiMacConst
 */
#define aMaxSIFSFrameSize               (18)

/**
 * The minimum number of symbols forming the CAP.
 * This ensures that MAC commands can still be
 * transferred to devices when GTSs are being used.
 * An exception to this minimum shall be allowed for the
 * accommodation of the temporary increase in the
 * beacon frame length needed to perform GTS
 * maintenance (see 7.2.2.1.3).
 * @ingroup apiMacConst
 */
#define aMinCAPLength                   (440)

/**
 * The minimum number of symbols forming a long
 * interframe spacing (LIFS) period.
 * @ingroup apiMacConst
 */
#define aMinLIFSPeriod                  (40)

/**
 * The minimum number of symbols forming a short
 * interframe spacing (SIFS) period.
 * @ingroup apiMacConst
 */
#define aMinSIFSPeriod                  (12)

/**
 * The number of slots contained in any superframe.
 * @ingroup apiMacConst
 */
#define aNumSuperframeSlots             (16)

/**
 * The maximum number of symbols a device shall wait for a response
 * command to be available following a request command.
 * @ingroup apiMacConst
 */
#define aResponseWaitTime               (32 * aBaseSuperframeDuration)

/**
 * The number of symbols forming the basic time period
 * used by the CSMA-CA algorithm.
 * @ingroup apiMacConst
 */
#define aUnitBackoffPeriod              (20)

/* PHY PIB Attributes */

/**
 * @ingroup apiPhyPib
 * @{
 */

/* Standard PIB attributes */

/**
 * The RF channel to use for all following transmissions and receptions.
 */
#define phyCurrentChannel               (0x00)

/**
 * The 5 most significant bits (MSBs) (b27, ..., b31) of phyChannelsSupported
 * shall be reserved and set to 0, and the 27 LSBs (b0, b1, ..., b26) shall
 * indicate the status (1 = available, 0 = unavailable) for each of the 27 valid
 * channels (bk shall indicate the status of channel k).
 */
#define phyChannelsSupported            (0x01)

/**
 * The 2 MSBs represent the tolerance on the transmit power: 00 = 1 dB
 * 01 = 3 dB 10 = 6 dB The 6 LSBs represent a signed integer in
 * twos-complement format, corresponding to the nominal transmit power of the
 * device in decibels relative to 1 mW. The lowest value of phyTransmitPower
 * shall be interpreted as less than or equal to 32 dBm.
 */
#define phyTransmitPower                (0x02)

/**
 * The CCA mode
 *  - CCA Mode 1: Energy above threshold. CCA shall report a busy medium
 * upon detecting any energy above the ED threshold.
 *  - CCA Mode 2: Carrier sense only. CCA shall report a busy medium only upon
 * the detection of a signal with the modulation and spreading characteristics
 * of IEEE 802.15.4. This signal may be above or below the ED threshold.
 *  - CCA Mode 3: Carrier sense with energy above threshold. CCA shall report a
 * busy medium only upon the detection of a signal with the modulation and
 * spreading characteristics of IEEE 802.15.4 with energy above the ED
 * threshold. */
#define phyCCAMode                      (0x03)

/**
 * phyCurrentPage
 * This is the current PHY channel page. This is used in conjunction with
 * phyCurrentChannel to uniquely identify the channel currently being used.
 * MAC-2006
 */
#define phyCurrentPage                  (0x04)

/**
 * Number of octets added by the PHY: 4 sync octets + SFD octet.
 */
#define PHY_OVERHEAD                    (5)

/*@}*//* apiPhyPib */

/* 7.4.2 MAC PIB Attributes */

/**
 * The maximum number of symbols to wait for an acknowledgment frame to arrive
 * following a transmitted data frame. This value is dependent on the currently
 * selected logical channel. For 0 <= phyCurrentChannel <= 10, this
 * value is equal to 120. For 11 <= phyCurrentChannel <= 26, this  value is
 * equal to 54.
 *
 * - @em Type: Integer
 * - @em Range: 54 or 120
 * - @em Default: 54
 * @ingroup apiMacPib
 */
#define macAckWaitDuration              (0x40)

#if RF_BAND == BAND_2400
/**
 * Default value for MIB macAckWaitDuration
 * @ingroup apiMacPibDef
 */
#define macAckWaitDuration_def          (54)
#elif RF_BAND == BAND_900
/**
 * Default value for MIB macAckWaitDuration
 * @ingroup apiMacPibDef
 */
#define macAckWaitDuration_def          (120)
#else   /* Can't happen because RF_BAND is checked above. */
#error "You have got no license for that RF band."
#endif /* RF_BAND */

/**
 * Indication of whether a coordinator is currently allowing association.
 * A value of true indicates that association is permitted.
 *
 * - @em Type: Boolean
 * - @em Range: true or false
 * - @em Default: false
 * @ingroup apiMacPib
 */
#define macAssociationPermit            (0x41)

/**
 * Default value for PIB macAssociationPermit
 * @ingroup apiMacPibDef
 */
#define macAssociationPermit_def        (false)

/**
 * Indication of whether a device automatically sends a data request command
 * if its address is listed in the beacon frame. A value of true indicates
 * that the data request command is automatically sent.
 *
 * - @em Type: Boolean
 * - @em Range: true or false
 * - @em Default: true
 * @ingroup apiMacPib
 */
#define macAutoRequest                  (0x42)

/**
 * Default value for PIB macAutoRequest
 * @ingroup apiMacPibDef
 */
#define macAutoRequest_def              (true)

/**
 * Indication of whether battery life extension, by reduction of coordinator
 * receiver operation time during the CAP, is enabled. A value of
 * true indicates that it is enabled.
 *
 * - @em Type: Boolean
 * - @em Range: true or false
 * - @em Default: false
 * @ingroup apiMacPib
 */
#define macBattLifeExt                  (0x43)

/**
 * Default value for PIB macBattLifeExt
 * @ingroup apiMacPibDef
 */
#define macBattLifeExt_def              (false)

/**
 * The number of backoff periods during which the receiver is enabled following
 * a beacon in battery life extension mode. This value is dependent on the
 * currently selected logical channel. For 0 <= * phyCurrentChannel <= 10, this
 * value is equal to 8. For 11 <= * phyCurrentChannel <= 26, this value
 * is equal to 6.
 *
 * - @em Type: Integer
 * - @em Range: 6 or 8
 * - @em Default: 6
 * @ingroup apiMacPib
 */
#define macBattLifeExtPeriods           (0x44)

/**
 * Default value for PIB macBattLifeExtPeriods
 * @ingroup apiMacPibDef
 */
#define macBattLifeExtPeriods_def       (6)

/**
 * The contents of the beacon payload.
 *
 * - @em Type: Set of octets
 * - @em Range: --
 * - @em Default: NULL
 * @ingroup apiMacPib
 */
#define macBeaconPayload                (0x45)

/**
 * The length, in octets, of the beacon payload.
 *
 * - @em Type: Integer
 * - @em Range: 0 - aMaxBeaconPayloadLength
 * - @em Default: 0
 * @ingroup apiMacPib
 */
#define macBeaconPayloadLength          (0x46)

/**
 * Default value for PIB macBeaconPayloadLength
 * @ingroup apiMacPibDef
 */
#define macBeaconPayloadLength_def      (0)

/**
 * Specification of how often the coordinator transmits a beacon.
 * The macBeaconOrder, BO, and the beacon interval, BI, are related as
 * follows: for 0 <= BO <= 14, BI = aBaseSuperframeDuration * 2^BO symbols.
 * If BO = 15, the coordinator will not transmit a beacon.
 *
 * - @em Type: Integer
 * - @em Range: 0 - 15
 * - @em Default: 15
 * @ingroup apiMacPib
 */
#define macBeaconOrder                  (0x47)

/**
 * Default value for PIB macBeaconOrder
 * @ingroup apiMacPibDef
 */
#define macBeaconOrder_def              (15)

/**
 * BO value for nonbeacon-enabled network
 */
#define NON_BEACON_NWK                  (0x0F)

/**
 * The time that the device transmitted its last beacon frame, in symbol
 * periods. The measurement shall be taken at the same symbol boundary within
 * every transmitted beacon frame, the location of which is implementation
 * specific. The precision of this value shall be a minimum of 20 bits, with
 * the lowest four bits being the least significant.
 *
 * - @em Type: Integer
 * - @em Range: 0x000000 - 0xffffff
 * - @em Default: 0x000000
 * @ingroup apiMacPib
 */
#define macBeaconTxTime                 (0x48)

/**
 * Default value for PIB macBeaconTxTime
 * @ingroup apiMacPibDef
 */
#define macBeaconTxTime_def             (0x000000)

/**
 * The sequence number added to the transmitted beacon frame.
 *
 * - @em Type: Integer
 * - @em Range: 0x00 - 0xff
 * - @em Default: Random value from within the range.
 * @ingroup apiMacPib
 */
#define macBSN                          (0x49)

/**
 * The 64 bit address of the coordinator with which the device is associated.
 *
 * - @em Type: IEEE address
 * - @em Range: An extended 64bit IEEE address
 * - @em Default: -
 * @ingroup apiMacPib
 */
#define macCoordExtendedAddress         (0x4A)

/**
 * The 16 bit short address assigned to the coordinator with which the device
 * is associated. A value of 0xfffe indicates that the coordinator is only
 * using its 64 bit extended address. A value of 0xffff indicates that this
 * value is unknown.
 *
 * - @em Type: Integer
 * - @em Range: 0x0000 - 0xffff
 * - @em Default: 0xffff
 * @ingroup apiMacPib
 */
#define macCoordShortAddress            (0x4B)

/**
 * Default value for PIB macCoordShortAddress
 * @ingroup apiMacPibDef
 */
#define macCoordShortAddress_def        (0xFFFF)

/**
 * The sequence number added to the transmitted data or MAC command frame.
 *
 * - @em Type: Integer
 * - @em Range: 0x00 - 0xff
 * - @em Default: Random value from within the range.
 * @ingroup apiMacPib
 */
#define macDSN                          (0x4C)

/**
 * macGTSPermit is true if the PAN coordinator is to accept GTS requests,
 * false otherwise.
 *
 * - @em Type: Boolean
 * - @em Range: true or false
 * - @em Default: true
 * @ingroup apiMacPib
 */
#define macGTSPermit                    (0x4D)

/**
 * Default value for PIB macGTSPermit
 * @ingroup apiMacPibDef
 */
#define macGTSPermit_def                (true)

/**
 * The maximum number of backoffs the CSMA-CA algorithm will
 * attempt before declaring a channel access failure.
 *
 * - @em Type: Integer
 * - @em Range: 0 - 5
 * - @em Default: 4
 * @ingroup apiMacPib
 */
#define macMaxCSMABackoffs              (0x4E)

/**
 * Default value for PIB macMaxCSMABackoffs
 * @ingroup apiMacPibDef
 */
#define macMaxCSMABackoffs_def          (4)

/**
 * The minimum value of the backoff exponent in the CSMA-CA algorithm.
 * Note that if this value is set to 0, collision avoidance is disabled
 * during the first iteration of the algorithm. Also note that for the
 * slotted version of the CSMACA algorithm with the battery life extension
 * enabled, the minimum value of the backoff exponent will be the lesser of
 * 2 and the value of macMinBE.
 *
 * - @em Type: Integer
 * - @em Range: 0 - 3
 * - @em Default: 3
 * @ingroup apiMacPib
 */
#define macMinBE                        (0x4F)

/**
 * Default value for PIB macMinBE
 * @ingroup apiMacPibDef
 */
#define macMinBE_def                    (3)

/**
 * The 16 bit identifier of the PAN on which the device is operating. If this
 * value is 0xffff, the device is not associated.
 *
 * - @em Type: Integer
 * - @em Range: 0x0000 - 0xffff
 * - @em Default: 0xffff
 * @ingroup apiMacPib
 */
#define macPANId                        (0x50)

/**
 * Default value for PIB macPANId
 * @ingroup apiMacPibDef
 */
#define macPANId_def                    (0xFFFF)

/**
 * This indicates whether the MAC sublayer is in a promiscuous (receive all)
 * mode. A value of true indicates that the MAC sublayer accepts all frames
 * received from the PHY.
 *
 * - @em Type: Boolean
 * - @em Range: true or false
 * - @em Default: false
 * @ingroup apiMacPib
 */
#define macPromiscuousMode              (0x51)

/**
 * Default value for PIB macPromiscuousMode
 * @ingroup apiMacPibDef
 */
#define macPromiscuousMode_def          (false)

/**
 * This indicates whether the MAC sublayer is to enable its receiver
 * during idle periods.
 *
 * - @em Type: Boolean
 * - @em Range: true or false
 * - @em Default: false
 * @ingroup apiMacPib
 */
#define macRxOnWhenIdle                 (0x52)

/**
 * Default value for PIB macRxOnWhenIdle
 * @ingroup apiMacPibDef
 */
#define macRxOnWhenIdle_def             (false)

/**
 * The 16 bit address that the device uses to communicate in the PAN.
 * If the device is a PAN coordinator, this value shall be chosen before a
 * PAN is started. Otherwise, the address is allocated by a coordinator
 * during association. A value of 0xfffe indicates that the device has
 * associated but has not been allocated an address. A value of 0xffff
 * indicates that the device does not have a short address.
 *
 * - @em Type: Integer
 * - @em Range: 0x0000 - 0xffff
 * - @em Default: 0xffff
 * @ingroup apiMacPib
 */
#define macShortAddress                 (0x53)

/**
 * Default value for PIB macShortAddress
 * @ingroup apiMacPibDef
 */
#define macShortAddress_def             (0xFFFF)

/**
 * This specifies the length of the active portion of the superframe, including
 * the beacon frame. The macSuperframeOrder, SO, and the superframe duration,
 * SD, are related as follows: for 0 <= SO <= BO <= 14, SD =
 * aBaseSuperframeDuration * 2SO symbols. If SO = 15, the superframe will
 * not be active following the beacon.
 *
 * - @em Type: Integer
 * - @em Range: 0 - 15
 * - @em Default: 15
 * @ingroup apiMacPib
 */
#define macSuperframeOrder              (0x54)

/**
 * Default value for PIB macSuperframeOrder
 * @ingroup apiMacPibDef
 */
#define macSuperframeOrder_def          (15)

/**
 * The maximum time (in superframe periods) that a transaction is stored by a
 * coordinator and indicated in its beacon.
 *
 * - @em Type: Integer
 * - @em Range: 0x0000 - 0xffff
 * - @em Default: 0x01f4
 * @ingroup apiMacPib
 */
#define macTransactionPersistenceTime   (0x55)

/**
 * Private MAC PIB attribute to allow setting the MAC address in test mode.
 * @todo numbering needs to alligned with other special speer attributes
 * @ingroup apiMacPib
 */
#define macIeeeAddress                  (0xF0)

/**
 * Private MAC PIB attribute indicationg that node is associated.
 * @todo numbering needs to alligned with other special speer attributes
 * @todo remove this TAL PIB attribute if MAC syncs before association
 * @ingroup apiMacPib
 */
#define macPrivateAssociated            (0xF8)

/**
 * Default value for PIB macTransactionPersistenceTime
 * @ingroup apiMacPibDef
 */
#define macTransactionPersistenceTime_def (0x01F4)


/**
 * @ingroup apiPhyConst
 *  @{
 */
/* 6.2.3 PHY Enumeration Definitions */
typedef enum phy_enum_tag
{
    /**
     * The CCA attempt has detected a busy channel.
     */
    PHY_BUSY                              = (0x00),

    /**
     * The transceiver is asked to change its state while receiving.
     */
    PHY_BUSY_RX                           = (0x01),

    /**
     * The transceiver is asked to change its state while transmitting.
     */
    PHY_BUSY_TX                           = (0x02),

    /**
     * The transceiver is to be switched off.
     */
    PHY_FORCE_TRX_OFF                     = (0x03),

    /**
     * The CCA attempt has detected an idle channel.
     */
    PHY_IDLE                              = (0x04),

    /**
     * A SET/GET request was issued with a parameter in the primitive that is out
     * of the valid range.
     */
    PHY_INVALID_PARAMETER                 = (0x05),

    /**
     * The transceiver is in or is to be configured into the receiver enabled
     * state.
     */
    PHY_RX_ON                             = (0x06),

    /**
     * A SET/GET, an ED operation, or a transceiver state change was successful.
     */
    PHY_SUCCESS                           = (0x07),

    /**
     * The transceiver is in or is to be configured into the transceiver disabled
     * state.
     */
    PHY_TRX_OFF                           = (0x08),

    /**
     * The transceiver is in or is to be configured into the transmitter enabled
     * state.
     */
    PHY_TX_ON                             = (0x09),

    /**
     * A SET/GET request was issued with the identifier of an attribute that is not
     * supported.
     */
    PHY_UNSUPPORTED_ATTRIBUTE             = (0x0A),

    /**
     * A SET/GET request was issued with the identifier of an attribute that is
     * read-only.
     */
    PHY_READ_ONLY                         = (0x0B)
} phy_enum_t;


/* Non-standard values / extensions */

/**
 * PHY_SUCCESS in phyAutoCSMACA when received ACK frame had the pending bit set
 */
#define PHY_SUCCESS_DATA_PENDING        (0x10)

/**
 * ED scan/sampling duration
 */
#define ED_SAMPLE_DURATION_SYM          (8)

/*@}*/ /* apiPhyConst */

/* === MAC Status Enumeration Definitions === */

/**
 * The requested operation was completed successfully. For a transmission
 * request, this value indicates a successful transmission.
 *
 * @ingroup apiMacReturn
 */
#define MAC_SUCCESS                     (0x00)

/**
 * The beacon was lost following a synchronization request.
 *
 * @ingroup apiMacReturn
 */
#define MAC_BEACON_LOSS                 (0xE0)

/**
 * A transmission could not take place due to activity on the channel, i.e.,
 * the CSMA-CA mechanism has failed.
 *
 * @ingroup apiMacReturn
 */
#define MAC_CHANNEL_ACCESS_FAILURE      (0xE1)

/**
 * The GTS request has been denied by the PAN coordinator.
 *
 * @ingroup apiMacReturn
 */
#define MAC_DENIED                      (0xE2)

/**
 * The attempt to disable the transceiver has failed.
 *
 * @ingroup apiMacReturn
 */
#define MAC_DISABLE_TRX_FAILURE         (0xE3)

/**
 * The received frame induces a failed security check according to the security
 * suite.
 *
 * @ingroup apiMacReturn
 */
#define MAC_FAILED_SECURITY_CHECK       (0xE4)

/**
 * The frame resulting from secure processing has a length that is greater than
 * aMACMaxFrameSize.
 *
 * @ingroup apiMacReturn
 */
#define MAC_FRAME_TOO_LONG              (0xE5)

/**
 * The requested GTS transmission failed because the specified GTS either did
 * not have a transmit GTS direction or was not defined.
 *
 * @ingroup apiMacReturn
 */
#define MAC_INVALID_GTS                 (0xE6)

/**
 * A request to purge an MSDU from the transaction queue was made using an MSDU
 * handle that was not found in the transaction table.
 *
 * @ingroup apiMacReturn
 */
#define MAC_INVALID_HANDLE              (0xE7)

/**
 * A parameter in the primitive is out of the valid range.
 *
 * @ingroup apiMacReturn
 */
#define MAC_INVALID_PARAMETER           (0xE8)

/**
 * No acknowledgment was received after aMaxFrameRetries.
 *
 * @ingroup apiMacReturn
 */
#define MAC_NO_ACK                      (0xE9)

/**
 * A scan operation failed to find any network beacons.
 *
 * @ingroup apiMacReturn
 */
#define MAC_NO_BEACON                   (0xEA)

/**
 * No response data were available following a request.
 *
 * @ingroup apiMacReturn
 */
#define MAC_NO_DATA                     (0xEB)

/**
 * The operation failed because a short address was not allocated.
 *
 * @ingroup apiMacReturn
 */
#define MAC_NO_SHORT_ADDRESS            (0xEC)

/**
 * A receiver enable request was unsuccessful because it could not be completed
 * within the CAP.
 *
 * @ingroup apiMacReturn
 */
#define MAC_OUT_OF_CAP                  (0xED)

/**
 * A PAN identifier conflict has been detected and communicated to the PAN
 * coordinator.
 *
 * @ingroup apiMacReturn
 */
#define MAC_PAN_ID_CONFLICT             (0xEE)

/**
 * A coordinator realignment command has been received.
 *
 * @ingroup apiMacReturn
 */
#define MAC_REALIGNMENT                 (0xEF)

/**
 * The transaction has expired and its information discarded.
 *
 * @ingroup apiMacReturn
 */
#define MAC_TRANSACTION_EXPIRED         (0xF0)

/**
 * There is no capacity to store the transaction.
 *
 * @ingroup apiMacReturn
 */
#define MAC_TRANSACTION_OVERFLOW        (0xF1)

/**
 * The transceiver was in the transmitter enabled state when the receiver was
 * requested to be enabled.
 *
 * @ingroup apiMacReturn
 */
#define MAC_TX_ACTIVE                   (0xF2)

/**
 * The appropriate key is not available in the ACL.
 *
 * @ingroup apiMacReturn
 */
#define MAC_UNAVAILABLE_KEY             (0xF3)

/**
 * A SET/GET request was issued with the identifier of a PIB attribute that is
 * not supported.
 *
 * @ingroup apiMacReturn
 */
#define MAC_UNSUPPORTED_ATTRIBUTE       (0xF4)

/**
 * This value is used to indicate that the sender of the data frame was not
 * found in the ACL.
 *
 * @ingroup apiMacReturn
 */
#define MAC_NOACLENTRYFOUND             (0x08)

/* MLME-SCAN.request type */

/**
 * @brief Energy scan (see @link wpan_mlme_scan_req() @endlink)
 * @ingroup apiConst
 */
#define MLME_SCAN_TYPE_ED               (0x00)

/**
 * @brief Active scan (see @link wpan_mlme_scan_req() @endlink)
 * @ingroup apiConst
 */
#define MLME_SCAN_TYPE_ACTIVE           (0x01)

/**
 * @brief Passive scan (see @link wpan_mlme_scan_req() @endlink)
 * @ingroup apiConst
 */
#define MLME_SCAN_TYPE_PASSIVE          (0x02)

/**
 * @brief Orphan scan (see @link wpan_mlme_scan_req() @endlink)
 * @ingroup apiConst
 */
#define MLME_SCAN_TYPE_ORPHAN           (0x03)

/* Various constants */

/**
 * Highest valid channel number
 * @ingroup apiConst
 */
#if (RF_BAND == BAND_2400)
#define MAX_CHANNEL                     (26)
#else   // 900 MHz
#define MAX_CHANNEL                     (10)
#endif

/**
 * First channel used for scan
 * @ingroup apiConst
 */
#if (RF_BAND == BAND_2400)
#define FIRST_SCAN_CHANNEL              (11)
#else   // 900 MHz
#define FIRST_SCAN_CHANNEL              (0)
#endif

/* Association status values from table 68 */

/**
 * Association status code value (see @link wpan_mlme_associate_resp() @endlink).
 * @ingroup apiConst
 */
#define ASSOCIATION_SUCCESSFUL          (0)

/**
 * Association status code value (see @link wpan_mlme_associate_resp() @endlink).
 * @ingroup apiConst
 */
#define PAN_AT_CAPACITY                 (1)

/**
 * Association status code value (see @link wpan_mlme_associate_resp() @endlink).
 * @ingroup apiConst
 */
#define PAN_ACCESS_DENIED               (2)

/**
 * Association status code value (see @link wpan_mlme_associate_resp() @endlink).
 * @ingroup apiConst
 */
#define ASSOCIATION_RESERVED            (3)

/**
 * Defines the beacon frame type. (Table 65 IEEE 802.15.4 Specification)
 */
#define FCF_FRAMETYPE_BEACON            (0x00)

/**
 * Define the data frame type. (Table 65 IEEE 802.15.4 Specification)
 */
#define FCF_FRAMETYPE_DATA              (0x01)

/**
 * Define the ACK frame type. (Table 65 IEEE 802.15.4 Specification)
 */
#define FCF_FRAMETYPE_ACK               (0x02)

/**
 * Define the command frame type. (Table 65 IEEE 802.15.4 Specification)
 */
#define FCF_FRAMETYPE_MAC_CMD           (0x03)

/**
 * A macro to set the frame type.
 */
#define FCF_SET_FRAMETYPE(x)            (x)

/**
 * The mask for the frame pending bit of the FCF
 */
#define FCF_FRAME_PENDING               (1 << 4)

/**
 * The mask for the ACK request bit of the FCF
 */
#define FCF_ACK_REQUEST                 (1 << 5)

/**
 * The mask for the intra PAN bit of the FCF
 */
#define FCF_INTRA_PAN                   (1 << 6)

/**
 * Address Mode: NO ADDRESS
 */
#define FCF_NO_ADDR                     (0x00)

/**
 * Address Mode: RESERVED
 */
#define FCF_RESERVED_ADDR               (0x01)

/**
 * Address Mode: SHORT
 */
#define FCF_SHORT_ADDR                  (0x02)

/**
 * Address Mode: LONG
 */
#define FCF_LONG_ADDR                   (0x03)

/**
 * Defines the offset of the source address
 */
#define FCF_SOURCE_ADDR_OFFSET          (14)

/**
 * Defines the offset of the destination address
 */
#define FCF_DEST_ADDR_OFFSET            (10)

/**
 * Macro to set the source address mode
 */
#define FCF_SET_SOURCE_ADDR_MODE(x)     ((x) << FCF_SOURCE_ADDR_OFFSET)

/**
 * Macro to set the destination address mode
 */
#define FCF_SET_DEST_ADDR_MODE(x)       ((x) << FCF_DEST_ADDR_OFFSET)

/**
 * Mask for the number of short addresses pending
 */
#define NUM_SHORT_PEND_ADDR(x)          ((x) & 0x07)

/**
 * Mask for the number of long addresses pending
 */
#define NUM_LONG_PEND_ADDR(x)           (((x) & 0x70) >> 4)

/**
 * Generic 16 bit broadcast address
 */
#define BROADCAST                       (0xFFFF)

/**
 * Unused EUI-64 address
 */
#define CLEAR_ADDR_64                   (0ULL)

/**
 * MAC is using long address (by now).
 */
#define MAC_NO_SHORT_ADDR_VALUE         (0xFFFE)

/**
 * Invalid short address
 */
#define INVALID_SHORT_ADDRESS           (0xFFFF)


/**
 * @brief Converts a phyTransmitPower value to a dBm value
 *
 * @param phyTransmitPower_value phyTransmitPower value
 *
 * @return dBm using signed integer format
 */
#define CONV_phyTransmitPower_TO_DBM(phyTransmitPower_value)                      \
            (                                                                     \
             ((phyTransmitPower_value & 0x20) == 0x00) ?                          \
              ((int8_t)(phyTransmitPower_value & 0x3F)) :                         \
              ((-1) * (int8_t)((~((phyTransmitPower_value & 0x1F) - 1)) & 0x1F))  \
            )


/**
 * @brief Converts a dBm value to a phyTransmitPower value
 *
 * @param dbm_value dBm value
 *
 * @return phyTransmitPower_value using IEEE-defined format
 */
#define CONV_DBM_TO_phyTransmitPower(dbm_value)                                         \
            (                                                                           \
                dbm_value < -32 ?                                                       \
                0x20 :                                                                  \
                (                                                                       \
                    dbm_value > 31 ?                                                    \
                    0x1F :                                                              \
                    (                                                                   \
                        dbm_value < 0 ?                                                 \
                        ( ((~(((uint8_t)((-1) * dbm_value)) - 1)) & 0x1F) | 0x20 ) :    \
                        (uint8_t)dbm_value                                              \
                    )                                                                   \
                )                                                                       \
            )                                                                           \


/* === Types ================================================================ */

#if !defined(DOXYGEN)
typedef enum trx_cca_mode_tag
{
    CCA_MODE0 = 0, /* used for RF231 */
    CCA_MODE1 = 1,
    CCA_MODE2 = 2,
    CCA_MODE3 = 3
}
/**
 * CCA Modes of the transceiver
 */
trx_cca_mode_t;
#endif

/* === Externals ============================================================ */


/* === Prototypes =========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IEEE_CONST_H */
/* EOF */

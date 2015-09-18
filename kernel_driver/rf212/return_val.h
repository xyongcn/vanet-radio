/**
 * @file return_val.h
 *
 * @brief Return values of APIs
 *
 * This header file has enumeration of return values.
 *
 * $Id: return_val.h 11056 2008-09-15 08:00:34Z uwalter $
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
#ifndef RETURN_VAL_H
#define RETURN_VAL_H

/* === Includes ============================================================ */
#include "../ringbuffer.h"

/* === Externals =========================================================== */


/* === Types =============================================================== */

/**
 * These are the return values of the PAL API.
 */
typedef enum
#if !defined(DOXYGEN)
retval_tag
#endif
{
    /** Return code for success */
    SUCCESS                     = 0x00,
/** @cond COMPLETE_PAL */
    BEACON_TX_SUCCESS           = 0x80,
    TRX_ASLEEP                  = 0x81,
    TRX_AWAKE                   = 0x82,
    CRC_CORRECT                 = 0x83,
    CRC_INCORRECT               = 0x84,
    FAILURE                     = 0x85,
    BUSY                        = 0x86,
    TAL_FRAME_PENDING           = 0x87,
/** @endcond */

    /** Return code, if the timer that should be started is already running */
    ALREADY_RUNNING             = 0x88,
    /** Return code, if the timer that should be stopped is not running */
    NOT_RUNNING                 = 0x89,
    /** Return code, if the requested timer ID is invalid */
    INVALID_ID                  = 0x8A,
    /** Return code, if the specified timeout is out of range or too short */
    INVALID_TIMEOUT             = 0x8B,
    /** Return code, if the parameter check failed */
    INVALID_PARAMETER           = 0xE8,
/** @cond COMPLETE_PAL */
    QUEUE_FULL                  = 0x8C,
    CSMA_CA_IN_PROGRESS         = 0x8D,
    NO_FRAME_TRANSMISSION       = 0x8E,
    CHANNEL_ACCESS_FAILURE      = 0xE1,
    NO_ACK                      = 0xE9,
    UNSUPPORTED_ATTRIBUTE       = 0xF4
/** @endcond */
} retval_t;

/* === Macros ============================================================== */


/* === Prototypes ========================================================== */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RETURN_VAL_H */
/* EOF */

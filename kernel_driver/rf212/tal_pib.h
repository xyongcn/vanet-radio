/**
 * @file tal_pib.h
 *
 * @brief This file contains the prototypes for TAL PIB functions.
 *
 * $Id: tal_pib.h 11056 2008-09-15 08:00:34Z uwalter $
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
#ifndef TAL_PIB_H
#define TAL_PIB_H

/* === INCLUDES ============================================================ */
#include "at86rf212.h"
#include "tal_internal.h"
#include <linux/types.h>

/* === EXTERNALS =========================================================== */
extern tal_trx_status_t tal_trx_status;
extern tal_state_t tal_state;

/* === TYPES =============================================================== */


/* === MACROS ============================================================== */


/* === PROTOTYPES ========================================================== */

#ifdef __cplusplus
extern "C" {
#endif


void init_tal_pib(void);
void write_all_tal_pib_to_trx(void);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* TAL_PIB_H */

/* EOF */

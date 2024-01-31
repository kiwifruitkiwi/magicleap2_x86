/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Platform driver for the CVIP SOC.
 *
 * Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
 */
#ifndef __CVIP_PLT_WDT_H__
#define __CVIP_PLT_WDT_H__
// Total number of Mero watchdogs
#define NUM_MERO_WATCHDOGS (2)
#define MERO_WDT_MDELAY_COUNT (100)

void wdt_keepalive_in_panic(void);
#endif


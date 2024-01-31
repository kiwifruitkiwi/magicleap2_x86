/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/**
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 */

#ifndef DIAG_H
#define DIAG_H

#define DIAG_MAX_REQ_SIZE	(16 * 1024)
#define DIAG_MAX_RSP_SIZE	(16 * 1024)

/**
 * In the worst case, the HDLC buffer can be atmost twice the size of the
 * original packet. Add 3 bytes for 16 bit CRC (2 bytes) and a delimiter
 * (1 byte)
 */
#define DIAG_MAX_HDLC_BUF_SIZE	((DIAG_MAX_REQ_SIZE * 2) + 3)

#define DIAG_EVENT_MASK     0xFF
#define DIAG_LOG_MASK       0xFF

/**
 * These masks are to be used for support of all legacy messages in the sw.
 * The user does not need to remember the names as they will be embedded in
 * the appropriate macros.
 */
#define MSG_LEGACY_LOW			MSG_MASK_0
#define MSG_LEGACY_MED			MSG_MASK_1
#define MSG_LEGACY_HIGH			MSG_MASK_2
#define MSG_LEGACY_ERROR		MSG_MASK_3
#define MSG_LEGACY_FATAL		MSG_MASK_4

/* Legacy Message Priorities */
#define MSG_LVL_FATAL    (MSG_LEGACY_FATAL)
#define MSG_LVL_ERROR    (MSG_LEGACY_ERROR | MSG_LVL_FATAL)
#define MSG_LVL_HIGH     (MSG_LEGACY_HIGH | MSG_LVL_ERROR)
#define MSG_LVL_MED      (MSG_LEGACY_MED | MSG_LVL_HIGH)
#define MSG_LVL_LOW      (MSG_LEGACY_LOW | MSG_LVL_MED)

#define MSG_SSID_WLAN                        4500
#define MSG_SSID_WLAN_LAST                   4583

#define MSG_MASK_0			(0x00000001)
#define MSG_MASK_1			(0x00000002)
#define MSG_MASK_2			(0x00000004)
#define MSG_MASK_3			(0x00000008)
#define MSG_MASK_4			(0x00000010)

int diag_loglevel_set(struct mhi_device *mhi_dev);
int diag_local_write(struct device *dev, unsigned char *buf, int len);

#endif    /* DIAG_H */

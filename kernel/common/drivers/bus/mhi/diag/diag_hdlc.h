/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/**
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 */

#ifndef DIAG_HDLC_H
#define DIAG_HDLC_H

#include <linux/types.h>

#define CONTROL_CHAR    0x7E
#define ESC_CHAR    0x7D
#define ESC_MASK    0x20

#define HDLC_INCOMPLETE    0
#define HDLC_COMPLETE      1

#define HDLC_FOOTER_LEN    3

enum diag_send_state_enum_type {
	DIAG_STATE_START,
	DIAG_STATE_BUSY,
	DIAG_STATE_CRC1,
	DIAG_STATE_CRC2,
	DIAG_STATE_TERM,
	DIAG_STATE_COMPLETE
};

struct diag_send_desc_type {
	const void *pkt;
	const void *last;         /* Address of last byte to send. */
	enum diag_send_state_enum_type state;
	/* True if this fragment terminates the packet */
	unsigned char terminate;
};

struct diag_hdlc_dest_type {
	void *dest;
	void *dest_last;
	u16 crc;
};

struct diag_hdlc_decode_type {
	u8 *src_ptr;
	unsigned int src_idx;
	unsigned int src_size;
	u8 *dest_ptr;
	unsigned int dest_idx;
	unsigned int dest_size;
	int          escaping;
};

/* ------ Public Function Declarations ------- */

void diag_hdlc_encode(struct diag_send_desc_type *src_desc,
		      struct diag_hdlc_dest_type *enc);
int diag_hdlc_decode(struct diag_hdlc_decode_type *hdlc);
int crc_check(u8 *buf, uint32_t len);

#endif    /* DIAG_HDLC_H */

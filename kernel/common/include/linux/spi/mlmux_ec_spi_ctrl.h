/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019, Magic Leap, Inc.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef _MLMUX_EC_SPI_CTRL
#define _MLMUX_EC_SPI_CTRL

#include <linux/types.h>

#define MLM_SPI_CRC8_POLYNOMIAL   (7)
#define MLM_SPI_MSG_Q_LEN         (8)
#define MLM_SPI_MAX_MSG_SIZE      (128)
#define MLM_SPI_TXRX_DATA_SIZE    (32)
#define MLM_SPI_TXRX_PKT_HDR_SIZE (sizeof(struct mlm_spi_packet) - \
				   MLM_SPI_TXRX_DATA_SIZE)
#define MLM_SPI_TXRX_PKT_SIZE     (sizeof(struct mlm_spi_packet))

#define MLM_SPI_MASTER_SOP_BYTE (0x99)
#define MLM_SPI_SLAVE_SOP_BYTE (0x66)


enum {
	MLM_SPI_FRAME_ID_WR,
	MLM_SPI_FRAME_ID_RD,
};

enum {
	MLM_SPI_PKT_ID_SOF,
	MLM_SPI_PKT_ID_DATA,
	MLM_SPI_PKT_ID_ACK,
};

/**
 * struct mlm_spi_sof_wr - write frame data format
 * @len: length in bytes of data to be transferred
 */
struct mlm_spi_sof_wr {
	__le16 len;
} __packed;

/**
 * struct mlm_spi_sof_rd - read frame data format
 * @data: reserved data
 */
struct mlm_spi_sof_rd {
	uint8_t data;
} __packed;

/**
 * struct mlm_spi_sof - beginning of frame struct
 * @frm_seq: which number frame this transaction is
 * @frm_id: what type of frame is this
 */
struct mlm_spi_sof {
	__le16 frm_seq;
	uint8_t frm_id;
	union {
		struct mlm_spi_sof_wr wr;
		struct mlm_spi_sof_rd rd;
	} t;
	/* rsvd up to MLM_SPI_TXRX_DATA_SIZE byte */
} __packed;

/**
 * struct mlm_spi_ack - ack data format
 * @id: ack id
 * @nack: non-0 if a nack
 * @cmd_ack: command id this response is for
 * @data: ack data. E.g. length in bytes slave wants to tx in case of
 *	  RD_CMD ack or amount of data being ack'ed.
 */
struct mlm_spi_ack {
	uint8_t nack : 1;
	uint8_t rsrv : 7;
	uint8_t pkt_id;
	__le16 data;
	/* rsvd up to MLM_SPI_TXRX_DATA_SIZE byte */
} __packed;

/**
 * struct mlm_spi_packet - ack data format
 * @sop: start of packet. Used to identify that this is a packet
 * @pkt_seq:
 * @pkt_id: identifies what type of packet this is
 * @p:	@sof:
 *	@data:
 *	@ack:
 * @crc: checksum
 */
struct mlm_spi_packet {
	uint8_t sop;
	uint8_t pkt_seq;
	uint8_t pkt_id;
	union {
		struct mlm_spi_sof sof;
		uint8_t data[MLM_SPI_TXRX_DATA_SIZE];
		struct mlm_spi_ack ack;
	} p;
	uint8_t crc;
} __packed;

#endif /* _MLMUX_EC_SPI_CTRL */


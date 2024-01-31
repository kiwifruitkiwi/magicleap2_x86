// SPDX-License-Identifier: GPL-2.0
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

#include <linux/acpi.h>
#include <linux/atomic.h>
#include <linux/crc8.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/ml-mux-ctrl.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spi/mlmux_ec_spi_ctrl.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define MLM_SPI_TX_RETRY	    (1000)
#define MLM_SPI_TX_PROTO_FAIL_RETRY (100)
#define MLM_SPI_WAIT_ACK_TIMEOUT  (250)
#define MIN(a, b) ((a < b) ? a : b)


/**
 * struct mlm_spi_wr_cmd - write command format
 * @table_msb: table containing all possible crc8's
 * @polynomial: polynomial used for table creation
 */
struct crc8_calc {
	u8 table_msb[CRC8_TABLE_SIZE];
	u8 polynomial;
};

/**
 * struct mlm_spi_xfer - struct to control transfer
 * @frame_seq: overall frame sequence number
 * @tx_pkt_seq: transmit packet number in the frame
 * @rx_pkt_seq: expected receive packet number in the frame
 * @tx_pkt: buffer for transmit packets
 * @rx_pkt: buffer for received packets
 * @crc: crc8 control struct
 * @rx_msg: receive buffer for the read data in a frame
 * @speed: current transaction speed
 * @lock: transfer mutex
 */
struct mlm_spi_xfer {
	uint16_t frame_seq;
	uint8_t tx_pkt_seq;
	uint8_t rx_pkt_seq;
	struct mlm_spi_packet tx_pkt;
	struct mlm_spi_packet rx_pkt;
	struct crc8_calc crc;
	uint8_t *rx_msg;
	uint32_t speed;
	struct mutex lock;
};

struct mlm_spi_ctrl_dev {
	struct device *dev;
	struct spi_device *spi;
	struct ml_mux_ctrl mlmux;
	struct gpio_desc *comm_rdy_gpio;
	struct gpio_desc *drdy_gpio;
	int drdy_irq;

	/* xfer controls */
	struct mlm_spi_xfer xfer;
	wait_queue_head_t enable_wq;
	atomic_t enable;
};

static int mlm_spi_tx_pkt(struct mlm_spi_ctrl_dev *sctrl,
			  struct mlm_spi_packet *pkt)
{
	struct spi_device *spi = sctrl->spi;
	struct spi_transfer xfer = {
		.bits_per_word = 8,
		.tx_buf = (uint8_t *)pkt,
		.rx_buf = NULL,
		.len = sizeof(struct mlm_spi_packet),
		.speed_hz = sctrl->xfer.speed,
	};
	int ret;

	/* TODO: once EC DMA is fixed */
	usleep_range(50, 100);

	ret = spi_sync_transfer(spi, &xfer, 1);
	if (ret)
		dev_err(sctrl->dev, "failed, ret=%d\n", ret);

	sctrl->xfer.tx_pkt_seq++;

	return ret;
}

static int mlm_spi_rx_pkt(struct mlm_spi_ctrl_dev *sctrl,
			   struct mlm_spi_packet *pkt)
{
	int ret;
	struct spi_device *spi = sctrl->spi;
	struct spi_transfer xfer = {
		.bits_per_word = 8,
		.tx_buf = NULL,
		.rx_buf = (uint8_t *)pkt,
		.len = sizeof(struct mlm_spi_packet),
		.speed_hz = sctrl->xfer.speed,
	};

	/* TODO: once EC DMA is fixed */
	usleep_range(50, 100);
	ret = spi_sync_transfer(spi, &xfer, 1);
	if (ret)
		dev_err(sctrl->dev, "failed, ret=%d\n", ret);

	return ret;
}

static bool mlm_spi_is_good_pkt(struct mlm_spi_ctrl_dev *sctrl,
			    struct mlm_spi_packet *pkt)
{
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	u8 pkt_crc = 0;

	if (pkt->sop != MLM_SPI_SLAVE_SOP_BYTE) {
		dev_err(sctrl->dev, "sop = %#04x, expected = %#04x\n",
			pkt->sop, MLM_SPI_SLAVE_SOP_BYTE);
		return false;
	}

	/* regardless if this is a real ack, we declare this as false because
	 * it's breaking protocol
	 */
	if (pkt->pkt_seq != xfer->rx_pkt_seq) {
		dev_warn(sctrl->dev, "out of sync rcv=%d expct=%d, reset?\n",
			 pkt->pkt_seq, xfer->rx_pkt_seq);
		return false;
	}

	pkt_crc = crc8(xfer->crc.table_msb, (u8 *)pkt,
		       (sizeof(*pkt) - 1), 0x00);
	if (pkt->crc != pkt_crc) {
		dev_err(sctrl->dev, "crc = %#04x, expected = %#04x\n",
			pkt->crc, pkt_crc);
		return false;
	}

	return true;
}

static int mlm_spi_wait_for_good_pkt(struct mlm_spi_ctrl_dev *sctrl,
				     struct mlm_spi_packet *pkt)
{
	unsigned long expiration;
	int ret;

	expiration = jiffies + msecs_to_jiffies(MLM_SPI_WAIT_ACK_TIMEOUT);
	do {
		ret = mlm_spi_rx_pkt(sctrl, pkt);
		if (ret)
			return ret;
		if (mlm_spi_is_good_pkt(sctrl, pkt))
			return 0;
		msleep(MLM_SPI_WAIT_ACK_TIMEOUT / 5);
	} while (time_before(jiffies, expiration));

	return -ETIMEDOUT;
}

/**
 * mlm_spi_wait_for_ack - busy loop waiting for ack from slave
 * @sctrl:	Pointer to the mlm spi ctrl data.
 * @ack_cmd_id:	cmd we're waiting to be ack'ed
 *
 */
static int mlm_spi_wait_for_ack(struct mlm_spi_ctrl_dev *sctrl,
				uint8_t ack_pkt_id)
{
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	struct mlm_spi_packet *ack_pkt = &xfer->rx_pkt;
	int ret = 0;

	/* If the buffers are still being prepared slave is to send
	 * blank or not good pkts. So, let's wait for a good pkt.
	 */
	ret = mlm_spi_wait_for_good_pkt(sctrl, ack_pkt);
	if (ret)
		goto exit;
	xfer->rx_pkt_seq++;

	if (ack_pkt->pkt_id != MLM_SPI_PKT_ID_ACK ||
	    ack_pkt->p.ack.nack ||
	    ack_pkt_id != ack_pkt->p.ack.pkt_id)
		ret = -EPROTO;

exit:
	return ret;
}

/**
 * mlm_spi_tx_ack - send a ack in response to some pkt
 * @sctrl:	Pointer to the mlm spi ctrl data
 * @len:	length of the pkt we've received or trying to receive
 * @pkt_id:	packet id for packet we're acking. Currently only data pkts.
 * @is_nack:	nack flag
 */
static int mlm_spi_tx_ack_pkt(struct mlm_spi_ctrl_dev *sctrl,
				uint16_t len,
				uint8_t pkt_id,
				bool is_nack)
{
	int ret = 0;
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	struct mlm_spi_packet *wr_pkt = &xfer->tx_pkt;

	wr_pkt->sop = MLM_SPI_MASTER_SOP_BYTE;
	wr_pkt->pkt_seq = xfer->tx_pkt_seq;
	wr_pkt->pkt_id = MLM_SPI_PKT_ID_ACK;
	wr_pkt->p.ack.pkt_id = pkt_id;
	wr_pkt->p.ack.nack = is_nack;
	wr_pkt->p.ack.data = cpu_to_le16(len);
	wr_pkt->crc = crc8(xfer->crc.table_msb, (u8 *)wr_pkt,
			   (sizeof(*wr_pkt) - 1), 0x00);

	ret = mlm_spi_tx_pkt(sctrl, wr_pkt);
	if (ret)
		goto exit;

exit:
	return ret;
}

/**
 * mlm_spi_tx_nack - send a nack in response to some pkt
 * @sctrl:	Pointer to the mlm spi ctrl data
 * @len:	length of the pkt we've received or trying to receive
 * @pkt_id:	pkt id we're nack'ing
 */
static inline int mlm_spi_tx_nack(struct mlm_spi_ctrl_dev *sctrl,
			   uint16_t len,
			   uint8_t pkt_id)
{
	return mlm_spi_tx_ack_pkt(sctrl, len, pkt_id, true);
}

/**
 * mlm_spi_tx_ack - send a ack in response to some pkt
 * @sctrl:	Pointer to the mlm spi ctrl data
 * @len:	length of the pkt we've received or trying to receive
 * @pkt_id:	pkt id we're ack'ing
 */
static inline int mlm_spi_tx_ack(struct mlm_spi_ctrl_dev *sctrl,
			  uint16_t len,
			  uint8_t pkt_id)
{
	return mlm_spi_tx_ack_pkt(sctrl, len, pkt_id, false);
}

int mlm_spi_sof_rd(struct mlm_spi_ctrl_dev *sctrl, uint16_t *len)
{
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	struct mlm_spi_packet *rd_sof_pkt = &xfer->tx_pkt;
	int ret = 0;

	/* All SOF assume restart of a transaction so pkt sequence
	 * is re-init to 0.
	 */
	xfer->tx_pkt_seq = 0;
	xfer->rx_pkt_seq = 0;
	rd_sof_pkt->sop = MLM_SPI_MASTER_SOP_BYTE;
	rd_sof_pkt->pkt_seq = xfer->tx_pkt_seq;
	rd_sof_pkt->pkt_id = MLM_SPI_PKT_ID_SOF;
	rd_sof_pkt->p.sof.frm_seq = cpu_to_le16(xfer->frame_seq);
	rd_sof_pkt->p.sof.frm_id = MLM_SPI_FRAME_ID_RD;
	rd_sof_pkt->crc = crc8(xfer->crc.table_msb, (u8 *)rd_sof_pkt,
			       (sizeof(*rd_sof_pkt) - 1), 0x00);
	xfer->frame_seq++;

	ret = mlm_spi_tx_pkt(sctrl, rd_sof_pkt);
	if (ret)
		goto exit;

	ret = mlm_spi_wait_for_ack(sctrl, MLM_SPI_PKT_ID_SOF);
	if (ret)
		goto exit;

	/* If good ack, then we can get len from current xfer */
	*len = (le16_to_cpu(xfer->rx_pkt.p.ack.data));

exit:
	return ret;
}

static int mlm_spi_sof_wr(struct mlm_spi_ctrl_dev *sctrl, uint16_t len)
{
	int ret = 0;
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	struct mlm_spi_packet *wr_sof_pkt = &xfer->tx_pkt;

	/* All commands assume restart of the rx and tx pkt sequence */
	xfer->tx_pkt_seq = 0;
	xfer->rx_pkt_seq = 0;
	wr_sof_pkt->sop = MLM_SPI_MASTER_SOP_BYTE;
	wr_sof_pkt->pkt_seq = xfer->tx_pkt_seq;
	wr_sof_pkt->pkt_id = MLM_SPI_PKT_ID_SOF;
	wr_sof_pkt->p.sof.frm_seq = cpu_to_le16(xfer->frame_seq);
	wr_sof_pkt->p.sof.frm_id = MLM_SPI_FRAME_ID_WR;
	wr_sof_pkt->p.sof.t.wr.len = cpu_to_le16(len);
	wr_sof_pkt->crc = crc8(xfer->crc.table_msb, (u8 *)wr_sof_pkt,
			       (sizeof(*wr_sof_pkt) - 1), 0x00);
	xfer->frame_seq++;

	ret = mlm_spi_tx_pkt(sctrl, wr_sof_pkt);
	if (ret)
		goto exit;

	ret = mlm_spi_wait_for_ack(sctrl, MLM_SPI_FRAME_ID_WR);
	if (ret)
		goto exit;

exit:
	return ret;
}

static int mlm_spi_fill_rx_msg(struct mlm_spi_ctrl_dev *sctrl,
				uint8_t *rx_msg,
				uint16_t len)
{
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	struct mlm_spi_packet *pkt = &xfer->rx_pkt;
	int ret = 0;
	int i = 0;

	for (i = 0; i < len; i += MLM_SPI_TXRX_DATA_SIZE) {
		ret = mlm_spi_rx_pkt(sctrl, pkt);
		if (ret)
			break;

		/* TODO: we could wait for a good pkt here? */
		if (!mlm_spi_is_good_pkt(sctrl, pkt)) {
			ret = -EIO;
			break;
		}
		xfer->rx_pkt_seq++;

		if (pkt->pkt_id != MLM_SPI_PKT_ID_DATA) {
			ret = -EPROTO;
			break;
		}

		memcpy(&rx_msg[i], &pkt->p.data,
		       MIN(len - i, MLM_SPI_TXRX_DATA_SIZE));
	}

	return ret;
}

static int mlm_spi_tx_data(struct mlm_spi_ctrl_dev *sctrl,
			   uint8_t *data,
			   uint16_t len)
{
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	struct mlm_spi_packet *pkt = &xfer->tx_pkt;
	int ret = 0;
	int i = 0;

	for (i = 0; i < len; i += MLM_SPI_TXRX_DATA_SIZE) {
		pkt->sop = MLM_SPI_MASTER_SOP_BYTE;
		pkt->pkt_seq = xfer->tx_pkt_seq;
		pkt->pkt_id = MLM_SPI_PKT_ID_DATA;
		memcpy(&pkt->p.data, &data[i],
		       MIN(len - i, MLM_SPI_TXRX_DATA_SIZE));
		pkt->crc = crc8(xfer->crc.table_msb, (u8 *)pkt,
				(sizeof(*pkt) - 1), 0x00);

		ret = mlm_spi_tx_pkt(sctrl, pkt);
		if (unlikely(ret))
			break;
	}

	return ret;
}

static void mlm_spi_tx(struct ml_mux_ctrl *ctrl, uint32_t len, void *data)
{
	struct mlm_spi_ctrl_dev *sctrl = container_of(ctrl,
						      struct mlm_spi_ctrl_dev,
						      mlmux);
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	int ret = 1;

	if (len > MLM_SPI_MAX_MSG_SIZE)
		return;

	wait_event(sctrl->enable_wq, atomic_read(&sctrl->enable));

	mutex_lock(&xfer->lock);
	while (ret) {
		ret = mlm_spi_sof_wr(sctrl, len);
		if (ret) {
			/* Keep trying until the slave accepts */
			dev_err(sctrl->dev, "tx sof fail err=%d", ret);
			msleep(MLM_SPI_TX_RETRY);
			continue;
		}

		ret = mlm_spi_tx_data(sctrl, (uint8_t *)data, len);
		if (ret) {
			dev_err(sctrl->dev, "TX FAIL! %d\n", ret);
			msleep(MLM_SPI_TX_PROTO_FAIL_RETRY);
			continue;
		}

		ret = mlm_spi_wait_for_ack(sctrl, MLM_SPI_PKT_ID_DATA);
		if (ret) {
			dev_err(sctrl->dev, "ACK FAIL! %d\n", ret);
			msleep(MLM_SPI_TX_PROTO_FAIL_RETRY);
			continue;
		}
	}
	mutex_unlock(&xfer->lock);
}

static irqreturn_t mlm_spi_rx(int irq, void *data)
{
	struct mlm_spi_ctrl_dev *sctrl = (struct mlm_spi_ctrl_dev *)data;
	struct mlm_spi_xfer *xfer = &sctrl->xfer;
	uint16_t len = 0;
	int ret = 1;

	if (!atomic_read(&sctrl->enable))
		return IRQ_HANDLED;

	mutex_lock(&xfer->lock);
	while (ret) {
		/* if the process fails in command tx phase we have to rely on
		 * slave to generated a new DRDY signal to restart xfer.
		 */
		ret = mlm_spi_sof_rd(sctrl, &len);
		if (ret) {
			dev_err(sctrl->dev, "sof_rd fail err=%d", ret);
			break;
		} else if (len > MLM_SPI_MAX_MSG_SIZE) {
			dev_err(sctrl->dev, "sof_rd fail len=%d > exp=%d",
				len, MLM_SPI_MAX_MSG_SIZE);
			break;
		}

		ret = mlm_spi_fill_rx_msg(sctrl, xfer->rx_msg, len);
		if (ret) {
			mlm_spi_tx_nack(sctrl, len, MLM_SPI_PKT_ID_DATA);
			dev_err(sctrl->dev, "fill rx fail err=%d", ret);
			break;
		}
		ret = mlm_spi_tx_ack(sctrl, len, MLM_SPI_PKT_ID_DATA);
		if (ret) {
			mlm_spi_tx_nack(sctrl, len, MLM_SPI_PKT_ID_DATA);
			dev_err(sctrl->dev, "tx ack fail err=%d", ret);
			break;
		}
		/* if the slave does not receive ack after successful send we
		 * might get a repeated message, not quite sure how to handle.
		 * Mlmux would warn us, but will process the msg regardless.
		 * This makes this the bottom line "error" and the choice is
		 * to repeat the message rather than lose it on the slave side.
		 * No amount of ack'ing acks here will help as this is a
		 * serialized communication we cannot establish a real end.
		 */
		mutex_unlock(&xfer->lock);
		ml_mux_msg_receive(&sctrl->mlmux, len, xfer->rx_msg);
		mutex_lock(&xfer->lock);
	}
	mutex_unlock(&xfer->lock);

	return IRQ_HANDLED;
}

static ssize_t speed_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf,
			    size_t count)
{
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct mlm_spi_ctrl_dev *sctrl = spi_get_drvdata(spi);
	uint8_t ret;
	uint32_t data;

	ret = kstrtou32(buf, 10, &data);
	if (ret) {
		dev_err(dev, "%s: failed to convert data value", __func__);
		goto exit;
	}
	mutex_lock(&sctrl->xfer.lock);
	sctrl->xfer.speed = data;
	mutex_unlock(&sctrl->xfer.lock);

exit:
	return ret ? ret : count;
}

static ssize_t speed_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct mlm_spi_ctrl_dev *sctrl = spi_get_drvdata(spi);

	return scnprintf(buf, PAGE_SIZE, "%d\n", sctrl->xfer.speed);
}
static DEVICE_ATTR_RW(speed);

static ssize_t enable_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf,
			    size_t count)
{
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct mlm_spi_ctrl_dev *sctrl = spi_get_drvdata(spi);
	uint8_t ret;
	uint32_t data;

	ret = kstrtou32(buf, 10, &data);
	if (ret) {
		dev_err(dev, "%s: failed to convert data value", __func__);
		goto exit;
	}
	atomic_set(&sctrl->enable, !!data);
	if (!data) {
		disable_irq(sctrl->drdy_irq);
	} else {
		enable_irq(sctrl->drdy_irq);
		wake_up_all(&sctrl->enable_wq);
		if (sctrl->comm_rdy_gpio)
			gpiod_set_value(sctrl->comm_rdy_gpio, 0);
	}
exit:
	return ret ? ret : count;
}

static ssize_t enable_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct mlm_spi_ctrl_dev *sctrl = spi_get_drvdata(spi);

	return scnprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&sctrl->enable));
}
static DEVICE_ATTR_RW(enable);


static struct attribute *mlm_spi_attributes[] = {
	&dev_attr_speed.attr,
	&dev_attr_enable.attr,
	NULL
};

static const struct attribute_group mlm_spi_attr_group = {
	.attrs = mlm_spi_attributes,
};

static void mlm_spi_deinit_xfer(struct mlm_spi_ctrl_dev *sctrl)
{
	struct mlm_spi_xfer *xfer = &sctrl->xfer;

	mutex_destroy(&xfer->lock);
}

static int mlm_spi_init_xfer(struct mlm_spi_ctrl_dev *sctrl)
{
	struct mlm_spi_xfer *xfer = &sctrl->xfer;

	xfer->speed = 1000000;
	xfer->frame_seq = 0;
	xfer->rx_msg = devm_kzalloc(sctrl->dev, MLM_SPI_MAX_MSG_SIZE,
				    GFP_KERNEL);
	if (!xfer->rx_msg)
		return -ENOMEM;

	xfer->crc.polynomial = MLM_SPI_CRC8_POLYNOMIAL;
	crc8_populate_msb(xfer->crc.table_msb, xfer->crc.polynomial);
	mutex_init(&xfer->lock);
	init_waitqueue_head(&sctrl->enable_wq);

	return 0;
}

static int mlm_spi_ctrl_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct mlm_spi_ctrl_dev *sctrl;
	int ret = 0;

	if (spi->irq <= 0) {
		dev_err(dev, "no irq defined\n");
		return -ENODEV;
	}

	sctrl = devm_kzalloc(dev, sizeof(struct mlm_spi_ctrl_dev), GFP_KERNEL);
	if (!sctrl)
		return -ENOMEM;

	sctrl->dev = dev;
	sctrl->spi = spi;
	atomic_set(&sctrl->enable, 0);

	ret = mlm_spi_init_xfer(sctrl);
	if (ret) {
		dev_err(dev, "failed xfer init err=%d\n", ret);
		goto exit;
	}
	spi_set_drvdata(spi, sctrl);

	strlcpy(sctrl->mlmux.name, "mlmux_spi_ec", sizeof(sctrl->mlmux.name));
	sctrl->mlmux.tx_data = mlm_spi_tx;
	sctrl->mlmux.queue_len = MLM_SPI_MSG_Q_LEN;
	sctrl->mlmux.max_tx_len = MLM_SPI_MAX_MSG_SIZE;
	sctrl->mlmux.max_rx_len = MLM_SPI_MAX_MSG_SIZE;
	sctrl->mlmux.dev = dev;

	sctrl->drdy_gpio = devm_gpiod_get(dev, "drdy", GPIOD_IN);
	if (IS_ERR(sctrl->drdy_gpio)) {
		ret = PTR_ERR(sctrl->drdy_gpio);
		dev_err(dev, "drdy get fail err=%d\n", ret);
		goto free_deinit_xfer;
	}

	sctrl->comm_rdy_gpio = devm_gpiod_get(dev, "comm_rdy", GPIOD_OUT_HIGH);
	if (IS_ERR(sctrl->comm_rdy_gpio)) {
		ret = PTR_ERR(sctrl->comm_rdy_gpio);
		dev_warn(dev, "comm_rdy get fail ret=%d\n", ret);
		sctrl->comm_rdy_gpio = NULL;
	}

	sctrl->drdy_irq = gpiod_to_irq(sctrl->drdy_gpio);
	if (sctrl->drdy_irq < 0) {
		ret = sctrl->drdy_irq;
		dev_err(dev, "drdy gpio to irq fail err=%d\n", ret);
		goto free_deinit_xfer;
	}

	ret = devm_request_threaded_irq(dev, sctrl->drdy_irq, NULL, mlm_spi_rx,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					"mlm_spi_irq", sctrl);
	if (ret) {
		dev_err(dev, "failed to request irq !\n");
		goto free_deinit_xfer;
	}

	ret = sysfs_create_group(&spi->dev.kobj, &mlm_spi_attr_group);
	if (ret < 0) {
		dev_err(dev, "failed to create sysfs node %d\n", ret);
		goto free_irq;
	}

	ret = ml_mux_ctrl_register(&sctrl->mlmux);
	if (ret)
		goto rem_sysgroup;

	if (device_property_present(sctrl->dev, "enable")) {
		atomic_set(&sctrl->enable, 1);
		wake_up_all(&sctrl->enable_wq);
		if (sctrl->comm_rdy_gpio)
			gpiod_set_value(sctrl->comm_rdy_gpio, 0);
	} else {
		disable_irq(sctrl->drdy_irq);
	}
	dev_info(dev, "Success!\n");

	return ret;
rem_sysgroup:
	sysfs_remove_group(&spi->dev.kobj, &mlm_spi_attr_group);
free_irq:
	devm_free_irq(dev, spi->irq, sctrl);
free_deinit_xfer:
	mlm_spi_deinit_xfer(sctrl);
	spi_set_drvdata(spi, NULL);
exit:
	return ret;
}

static int mlm_spi_ctrl_remove(struct spi_device *spi)
{
	struct mlm_spi_ctrl_dev *sctrl = spi_get_drvdata(spi);

	dev_info(&spi->dev, "%s: enter\n", __func__);
	ml_mux_ctrl_unregister(&sctrl->mlmux);
	sysfs_remove_group(&spi->dev.kobj, &mlm_spi_attr_group);
	devm_free_irq(&spi->dev, spi->irq, sctrl);
	mutex_destroy(&sctrl->xfer.lock);
	spi_set_drvdata(spi, NULL);

	return 0;
}

static const struct of_device_id mlm_spi_ctrl_match[] = {
	{ .compatible = "ml,mlm_ctrl_stm32" },
	{ },
};
MODULE_DEVICE_TABLE(of, mlm_spi_ctrl_match);

#ifdef CONFIG_ACPI
static const struct acpi_device_id mlm_spi_acpi_match[] = {
	{"STM32462", 0},
	{ },
};
MODULE_DEVICE_TABLE(acpi, mlm_spi_acpi_match);
#endif

static struct spi_driver mlm_spi_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "mlmux_spi_ctrl",
		.of_match_table = mlm_spi_ctrl_match,
		.acpi_match_table = ACPI_PTR(mlm_spi_acpi_match),
	},
	.probe = mlm_spi_ctrl_probe,
	.remove = mlm_spi_ctrl_remove,
};
module_spi_driver(mlm_spi_driver);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("Magic Leap MLMUX SPI controller");
MODULE_LICENSE("GPL v2");


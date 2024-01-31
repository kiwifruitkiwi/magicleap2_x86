/* Copyright (c) 2018, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/usb.h>
#include <asm/byteorder.h>

#define FTDI_FT4222_VID (0x0403)
#define FTDI_FT4222_PID (0x601c)

#define FT4222_OUT (USB_TYPE_VENDOR)
#define FT4222_IN  (USB_DIR_IN | USB_TYPE_VENDOR)

#define URB_CTRL_OUT	1
#define URB_CTRL_IN	0

#define FT4222_SPI_MAX_IO_SZ	254
#define FT4222_CTRL_MSG_TOUT	100
#define FT4222_BULK_MSG_TOUT	200
#define FT4222_BULK_FLUSH_TOUT	1

#define FT4222_SYS_CLK_HZ	60000000UL /* 60 MHz */
#define FT4222_CPOL		0x0100
#define FT4222_CPHA		0x0100

enum ft4222_cs {
	FTDI_CS1,
	FTDI_CS2,
	FTDI_CS3,
	FTDI_CS4,
	FTDI_CS_MAX,
};

struct ft4222_urb_ctrl_req {
	u8	request;
	u8	type;
	bool	dir_in;
	u16	value;
	u16	index;
	u16	length;
};

struct ft4222_urb_ctrl_resp {
	u8	*data;
	u16	length;
};

struct ft4222_usb2spi {
	struct usb_interface	*intf;
	struct spi_device	*spi_device;
	u32			speed_curr;
	u8			ep_out;
	u8			ep_in;
};

struct ft4222_spi_data {
	struct	usb_device	*usb_dev;
	struct	spi_master	*spim;
	u8			buf[FT4222_SPI_MAX_IO_SZ + 2];
	struct	ft4222_usb2spi	usb2spi[FTDI_CS_MAX];
};

static inline int ft4222_check_response(const char *buf)
{
	if (buf[0] != 0x02)
		return -EINVAL;

	/* The second byte of response has been observed to be either
	 * 0x60 or 0x00 depending on the platform.
	 */
	if (buf[1] != 0x60 && buf[1] != 0x00)
		return -EINVAL;

	return 0;
}

static int ft4222_send_ctrl_req(struct usb_device *usb_dev,
				const struct ft4222_urb_ctrl_req *req,
				struct ft4222_urb_ctrl_resp *resp, bool out)
{
	unsigned int pipe = (out ? usb_rcvctrlpipe(usb_dev, 0) :
				   usb_sndctrlpipe(usb_dev, 0));

	dev_dbg(&usb_dev->dev,
		"pipe 0x%08x, req %d, type 0x%02x, val 0x%04x, idx %d, len %d\n",
		pipe, req->request, req->type, req->value, req->index,
		req->length);
	resp->length = usb_control_msg(usb_dev, pipe,
				       req->request, req->type, req->value,
				       req->index, resp->data, req->length,
				       FT4222_CTRL_MSG_TOUT);
	if (resp->length != req->length) {
		dev_err(&usb_dev->dev, "%s: expected %d, received %d bytes\n",
			__func__, req->length, resp->length);
		return -EREMOTEIO;
	}
	print_hex_dump_debug("CTRL RX", DUMP_PREFIX_NONE, 16, 1, resp->data,
			     resp->length, 0);
	return 0;
}

static int ft4222_spi_detect(struct ft4222_spi_data *ft4222)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int i, rc;

	struct ft4222_urb_ctrl_req req = {
		.request = 144,
		.type = FT4222_IN,
		.value = 0,
		.length = 2,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = ft4222->buf,
		.length = 2,
	};

	for (i = 0; i < 64; i++) {
		req.index = i;
		rc = ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_IN);
		if (rc) {
			dev_err(&usb_dev->dev,
				"init sequence failed, idx = %d, %d\n", i, rc);
			return -EREMOTEIO;
		}
	}
	return 0;
}

static int ft4222_spi_list_dev_msg_out(struct ft4222_spi_data *ft4222, u8 cs)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	struct ft4222_urb_ctrl_req req = {
		.request = 0,
		.type = FT4222_OUT,
		.value = 0,
		.index = cs,
		.length = 0,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = NULL,
		.length = 0,
	};

	return ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);
}

static int ft4222_spi_list_dev_msg_in(struct ft4222_spi_data *ft4222, u8 cs)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int rc;

	struct ft4222_urb_ctrl_req req = {
		.request = 5,
		.type = FT4222_IN,
		.value = 0,
		.index = cs,
		.length = 2,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = &ft4222->buf[0],
		.length = 2,
	};

	rc = ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_IN);
	if (rc)
		return rc;

	if (ft4222_check_response(resp.data)) {
		dev_err(&usb_dev->dev, "cs = %d, invalid response %02x%02x\n",
			cs, resp.data[0], resp.data[1]);
		return -EREMOTEIO;
	}
	return 0;
}

static int ft4222_spi_list_dev(struct ft4222_spi_data *ft4222, u8 cs)
{
	int rc;

	rc = ft4222_spi_list_dev_msg_out(ft4222, cs);
	if (rc)
		goto failure;

	rc = ft4222_spi_list_dev_msg_in(ft4222, cs);
	if (rc)
		goto failure;

	return 0;

failure:
	dev_err(&ft4222->usb_dev->dev, "%s(cs = %d) failed, %d\n",
		__func__, cs, rc);
	return rc;
}

static int ft4222_spi_open_msg_out(struct ft4222_spi_data *ft4222, u8 cs)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	struct ft4222_urb_ctrl_req req = {
		.request = 0,
		.type = FT4222_OUT,
		.value = 0,
		.index = cs,
		.length = 0,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = NULL,
		.length = 0,
	};

	return ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);
}

static int ft4222_spi_open_msg_in(struct ft4222_spi_data *ft4222, u8 cs)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int rc;

	struct ft4222_urb_ctrl_req req = {
		.request = 5,
		.type = FT4222_IN,
		.value = 0,
		.index = cs,
		.length = 2,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = &ft4222->buf[0],
		.length = 2,
	};

	rc = ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_IN);
	if (rc)
		return rc;

	if (ft4222_check_response(resp.data)) {
		dev_err(&usb_dev->dev, "ss %d, invalid response %02x%02x\n",
			cs, resp.data[0], resp.data[1]);
		return -EREMOTEIO;
	}

	return 0;
}

/* This function is equivalent to FT_OpenEx() function */
static int ft4222_spi_open(struct ft4222_spi_data *ft4222, u8 cs)
{
	int rc = -EFAULT;

	if (!ft4222->usb2spi[cs].ep_out ||
	    !ft4222->usb2spi[cs].ep_in)
		goto failure;

	rc = ft4222_spi_open_msg_out(ft4222, cs);
	if (rc)
		goto failure;

	rc = ft4222_spi_open_msg_in(ft4222, cs);
	if (rc)
		goto failure;

	return 0;

failure:
	dev_err(&ft4222->usb_dev->dev, "%s(cs = %d) failed, %d\n",
		__func__, cs, rc);
	return rc;
}

static int ft4222_spi_init_msg1(struct ft4222_spi_data *ft4222, u8 cs)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	struct ft4222_urb_ctrl_req req = {
		.request = 32,
		.type = FT4222_IN,
		.value = 1,
		.index = cs,
		.length = 13,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = &ft4222->buf[0],
		.length = 13,
	};
	return ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_IN);
}

static int ft4222_spi_get_version(struct ft4222_spi_data *ft4222, u8 cs)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int rc;

	struct ft4222_urb_ctrl_req req = {
		.request = 32,
		.type = FT4222_IN,
		.value = 0,
		.index = cs,
		.length = 12,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = &ft4222->buf[0],
		.length = 12,
	};

	rc = ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_IN);
	if (rc)
		return rc;

	if (resp.data[0] != 0x42 || resp.data[1] != 0x22) {
		dev_err(&usb_dev->dev, "invalid version %02x%02x\n",
			resp.data[0], resp.data[1]);
		return -EREMOTEIO;
	}
	dev_info(&usb_dev->dev, "usb2spi hw %02x%02x\n",
		resp.data[0], resp.data[1]);
	return 0;
}

static int ft4222_spi_init_msg3(struct ft4222_spi_data *ft4222, u8 cs)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int rc;

	struct ft4222_urb_ctrl_req req = {
		.request = 34,
		.type = FT4222_IN,
		.value = 0x06b1,
		.index = cs,
		.length = 1,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = &ft4222->buf[0],
		.length = 1,
	};

	rc = ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_IN);
	if (rc)
		return rc;

	if (resp.data[0] != 0) {
		dev_err(&usb_dev->dev, "invalid response 0x%02x\n",
			resp.data[0]);
		return -EREMOTEIO;
	}
	return 0;
}

static int ft4222_spi_init_msg4(struct ft4222_spi_data *ft4222, u8 cs)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int rc;

	struct ft4222_urb_ctrl_req req = {
		.request = 32,
		.type = FT4222_IN,
		.value = 4,
		.index = cs,
		.length = 1,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = &ft4222->buf[0],
		.length = 1,
	};

	rc = ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_IN);
	if (rc)
		return rc;

	if (resp.data[0] != 0) {
		dev_err(&usb_dev->dev, "invalid response 0x%02x\n",
			resp.data[0]);
		return -EREMOTEIO;
	}
	return 0;
}

static int ft4222_spi_init_req33(struct ft4222_spi_data *ft4222,
				 const struct spi_device *spi)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	u8 clk_div;
	u8 cs = spi->chip_select;
	int rc;

	struct ft4222_urb_ctrl_req req = {
		.request = 33,
		.type = FT4222_OUT,
		.index = cs,
		.length = 0,
	};
	struct ft4222_urb_ctrl_resp resp = {
		.data = NULL,
		.length = 0,
	};

	req.value = 0x49;
	rc = ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);
	req.value = 0x142;
	rc += ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);

	for (clk_div = 0; clk_div < 9; clk_div++)
		if (ft4222->usb2spi[cs].speed_curr >= FT4222_SYS_CLK_HZ / BIT(clk_div))
			break;
	req.value = (clk_div << 8) | 0x44;
	dev_info(&usb_dev->dev, "cs = %d: set clk speed to %luHz\n",
		 cs, FT4222_SYS_CLK_HZ / BIT(clk_div));
	rc += ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);

	req.value = 0x45;
	if (spi->mode & SPI_CPOL)
		req.value |= FT4222_CPOL;
	rc += ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);

	req.value = 0x46;
	if (spi->mode & SPI_CPHA)
		req.value |= FT4222_CPHA;
	rc += ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);

	req.value = 0x43;
	rc += ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);

	req.value = 0x148;
	rc += ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);

	req.value = 0x305;
	rc += ft4222_send_ctrl_req(usb_dev, &req, &resp, URB_CTRL_OUT);

	return (rc != 0) ? -EREMOTEIO : 0;
}

static int ft4222_spi_init(struct ft4222_spi_data *ft4222,
			   const struct spi_device *spi)
{
	int rc;

	rc = ft4222_spi_init_msg1(ft4222, spi->chip_select);
	if (rc)
		goto failure;

	rc = ft4222_spi_get_version(ft4222, spi->chip_select);
	if (rc)
		goto failure;

	rc = ft4222_spi_init_msg3(ft4222, spi->chip_select);
	if (rc)
		goto failure;

	rc = ft4222_spi_init_msg4(ft4222, spi->chip_select);
	if (rc)
		goto failure;

	rc = ft4222_spi_init_req33(ft4222, spi);
	if (rc)
		goto failure;

	return 0;

failure:
	dev_err(&ft4222->usb_dev->dev, "%s(cs = %d) failed, %d\n",
		__func__, spi->chip_select, rc);
	return rc;
}

static int ft4222_spi_write(struct ft4222_spi_data *ft4222, u8 ep,
			    u8 *tx, u16 len)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int bytes_actual, rc;

	print_hex_dump_debug("BULK TX:", DUMP_PREFIX_NONE, 32, 1, tx, len, 0);
	rc = usb_bulk_msg(usb_dev, usb_sndbulkpipe(usb_dev, ep),
			  tx, len, &bytes_actual, FT4222_BULK_MSG_TOUT);
	if (rc) {
		dev_err(&usb_dev->dev, "usb_bulk_msg failed, %d\n", rc);
		return rc;
	}

	/* ToDo: Add a loop to send the rest of data */
	if (bytes_actual != len) {
		dev_err(&usb_dev->dev, "partial tx, %d out of %d bytes\n",
			bytes_actual, len);
		return -EREMOTEIO;
	}
	return 0;
}

static int ft4222_spi_read(struct ft4222_spi_data *ft4222, u8 ep,
			   u8 *rx, u16 len)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int rc, bytes_actual;
	int data_left = len;
	int retries = 3;

	if (len > FT4222_SPI_MAX_IO_SZ)
		return -EINVAL;

	memset(rx, 0, len);
	while (retries-- > 0) {
		rc = usb_bulk_msg(usb_dev, usb_rcvbulkpipe(usb_dev, ep),
				  &ft4222->buf[0], sizeof(ft4222->buf),
				  &bytes_actual, FT4222_BULK_MSG_TOUT);
		if (rc) {
			dev_err(&usb_dev->dev, "read failed, %d\n", rc);
			return rc;
		}

		if (bytes_actual < 2) {
			dev_dbg(&usb_dev->dev,
				"read failed, received %d bytes\n",
				bytes_actual);
			continue;
		}

		print_hex_dump_debug("BULK RX:", DUMP_PREFIX_NONE, 32, 1,
				     &ft4222->buf[0], bytes_actual, 0);

		/* The first 2 bytes is the error code */
		if (ft4222_check_response(ft4222->buf)) {
			dev_err(&usb_dev->dev,
				"invalid response, 0x%02x%02x\n",
				ft4222->buf[0], ft4222->buf[1]);
			return -EREMOTEIO;
		}
		bytes_actual -= 2;

		if (bytes_actual == 0)
			continue;

		if (bytes_actual > data_left) {
			dev_err(&usb_dev->dev,
				"%s: expected %d, received %d bytes\n",
				__func__, data_left, bytes_actual);
			return -EREMOTEIO;
		}

		memcpy(rx, &ft4222->buf[2], bytes_actual);
		rx += bytes_actual;
		data_left -= bytes_actual;

		if (!data_left)
			return 0;
	}
	return -ETIMEDOUT;
}

static void ft4222_spi_flush(struct ft4222_spi_data *ft4222, u8 ep)
{
	struct usb_device *usb_dev = ft4222->usb_dev;
	int rc = 0, bytes_actual;
	int retries = 3;

	while (retries-- > 0) {
		rc = usb_bulk_msg(usb_dev, usb_rcvbulkpipe(usb_dev, ep),
				  &ft4222->buf[0], sizeof(ft4222->buf),
				  &bytes_actual, FT4222_BULK_FLUSH_TOUT);
		if (!rc) {
			print_hex_dump_debug("FLUSH RX:", DUMP_PREFIX_NONE, 32,
					1, &ft4222->buf[0], bytes_actual, 0);
			continue;
		}

		if (rc == -ETIMEDOUT)
			break;
	}
}

static int ft4222_spi_setup(struct spi_device *spi)
{
	struct ft4222_spi_data *ft4222 = spi_master_get_devdata(spi->master);
	const u8 cs = spi->chip_select;
	int rc = -EFAULT;

	dev_info(&spi->dev, "%s(cs = %d), requested speed = %d, mode = %d\n",
		 __func__, cs, spi->max_speed_hz, spi->mode);
	if (cs == 0 || cs > FTDI_CS_MAX) {
		dev_err(&spi->dev, "%s: invalid cs = %d\n", __func__, cs);
		goto failure;
	}

	if (!ft4222->usb2spi[cs].ep_in ||
	    !ft4222->usb2spi[cs].ep_out) {
		dev_err(&spi->dev,
			"USB interface for EPs 0x%02x 0x%02x not registered\n",
			ft4222->usb2spi[cs].ep_in,
			ft4222->usb2spi[cs].ep_out);
		goto failure;
	}

	if (ft4222->usb2spi[cs].spi_device) {
		dev_err(&spi->dev,
			"SPI device for USB EPs 0x%02x 0x%02x already exists\n",
			ft4222->usb2spi[cs].ep_in,
			ft4222->usb2spi[cs].ep_out);
		goto failure;
	}

	rc = ft4222_spi_list_dev(ft4222, cs);
	if (rc) {
		dev_err(&spi->dev, "ft4222_spi_list_dev failed, %d\n", rc);
		goto failure;
	}

	rc = ft4222_spi_open(ft4222, cs);
	if (rc) {
		dev_err(&spi->dev, "ft4222_spi_open failed, %d\n", rc);
		goto failure;
	}

	rc = ft4222_spi_init(ft4222, spi);
	if (rc) {
		dev_err(&spi->dev, "ft4222_spi_init failed, %d\n", rc);
		goto failure;
	}

	ft4222->usb2spi[cs].speed_curr = spi->max_speed_hz;
	ft4222->usb2spi[cs].spi_device = spi;
	return 0;

failure:
	dev_err(&spi->dev, "%s(cs = %d) failed, %d\n", __func__, cs, rc);
	return rc;
}

static void ft4222_spi_cleanup(struct spi_device *spi)
{
	struct ft4222_spi_data *ft4222 = spi_master_get_devdata(spi->master);
	struct device *dev = &ft4222->usb_dev->dev;
	const u8 cs = spi->chip_select;

	dev_info(&spi->dev, "%s(cs = %d)\n", __func__, cs);
	if (cs == 0 || cs > FTDI_CS_MAX) {
		dev_err(dev, "%s: invalid cs = %d\n", __func__, cs);
		return;
	}
	ft4222->usb2spi[cs].spi_device = NULL;
}

static int ft4222_spi_transfer_one(struct ft4222_spi_data *ft4222,
				   const u8 cs, u8 ep_in, u8 ep_out,
				   struct spi_transfer *t)
{
	struct device *dev = &ft4222->usb_dev->dev;
	struct spi_device *spi = ft4222->usb2spi[cs].spi_device;
	u32 speed_hz = t->speed_hz;
	int rc = 0;

	if (t->len > FT4222_SPI_MAX_IO_SZ) {
		dev_err(dev, "spi io len = %d is greater than max len = %d\n",
			t->len, FT4222_SPI_MAX_IO_SZ);
		return -E2BIG;
	}

	if (!t->tx_buf)
		goto rx;

	if (!speed_hz || speed_hz > spi->max_speed_hz)
		speed_hz = spi->max_speed_hz;

	/* You are not guaranteed exact speed. speed = (FT4222_SYS_CLK_HZ / div) */
	if (speed_hz != ft4222->usb2spi[cs].speed_curr) {
		ft4222->usb2spi[cs].speed_curr = speed_hz;
		ft4222_spi_init_req33(ft4222, spi);
	}

	memcpy(&ft4222->buf[0], t->tx_buf, t->len);
	rc = ft4222_spi_write(ft4222, ep_out, &ft4222->buf[0], t->len);
	if (rc) {
		dev_err(dev, "tx %d bytes failed, %d\n", t->len, rc);
		return rc;
	}

	rc = ft4222_spi_write(ft4222, ep_out, NULL, 0);
	if (rc) {
		dev_err(dev, "finalize tx failed, %d\n", rc);
		return rc;
	}

rx:
	if (!t->rx_buf) {
		ft4222_spi_flush(ft4222, ep_in);
		return 0;
	}

	rc = ft4222_spi_read(ft4222, ep_in, t->rx_buf, t->len);
	if (rc)
		dev_err(dev, "rx %d bytes failed, %d\n", t->len, rc);

	return rc;
}

static int ft4222_spi_transfer_one_message(struct spi_master *spim,
					   struct spi_message *msg)
{
	struct ft4222_spi_data *ft4222 = spi_master_get_devdata(spim);
	struct spi_device *spi = msg->spi;
	struct spi_transfer *t;
	const u8 cs = spi->chip_select;
	u8 ep_in, ep_out;
	int rc = 0;

	dev_dbg(&spim->dev, "%s(cs = %d) enter\n", __func__, cs);
	if (cs == 0 || cs > FTDI_CS_MAX) {
		dev_err(&spim->dev, "%s: invalid cs = %d\n", __func__, cs);
		return -EINVAL;
	}

	ep_in = ft4222->usb2spi[spi->chip_select].ep_in;
	ep_out = ft4222->usb2spi[spi->chip_select].ep_out;

	list_for_each_entry(t, &msg->transfers, transfer_list) {
		rc = ft4222_spi_transfer_one(ft4222, cs, ep_in, ep_out, t);
		if (rc)
			goto exit;

		if (t->delay_usecs)
			udelay(t->delay_usecs);
	}

exit:
	dev_dbg(&spim->dev, "%s(cs = %d, len = %d) done, %d\n",
		__func__, cs, msg->actual_length, rc);
	msg->status = rc;
	spi_finalize_current_message(spim);
	return rc;
}

static int ft4222_populate_bulk_ep_info(struct usb_interface *intf,
					struct	ft4222_usb2spi *usb2spi)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct usb_host_interface *desc = intf->cur_altsetting;
	struct device *dev = &intf->dev;
	const u16 ver = le16_to_cpu(usb_dev->descriptor.bcdDevice);
	u8 i;

	dev_info(dev, "version %x.%02x found at bus %03d addr %03d\n",
		 ver >> 8, ver & 0xff, usb_dev->bus->busnum, usb_dev->devnum);

	for (i = 0; i < desc->desc.bNumEndpoints; i++) {
		struct usb_endpoint_descriptor *ep = &desc->endpoint[i].desc;
		u8 ep_addr = ep->bEndpointAddress;
		u8 cs = 0;

		switch (ep_addr) {
		case 0x02:
		case 0x81:
			cs = FTDI_CS1;
			break;
		case 0x04:
		case 0x83:
			cs = FTDI_CS2;
			break;
		case 0x06:
		case 0x85:
			cs = FTDI_CS3;
			break;
		case 0x08:
		case 0x87:
			cs = FTDI_CS4;
			break;
		default:
			WARN(1, "Unsupported USB EP 0x%02x", ep_addr);
			return -EINVAL;
		}

		usb_set_intfdata(intf, &usb2spi[cs]);
		usb2spi[cs].intf = intf;
		dev_info(&intf->dev, "EP address: 0x%02x\n", ep_addr);
		if (usb_endpoint_is_bulk_in(ep))
			usb2spi[cs].ep_in = ep->bEndpointAddress;
		else
			usb2spi[cs].ep_out = ep->bEndpointAddress;
	}

	return 0;
}

static int count_registered_eps(struct ft4222_spi_data *ft4222)
{
	int idx, cnt = 0;

	for (idx = 0; idx < FTDI_CS_MAX; idx++)
		if (ft4222->usb2spi[idx].ep_in && ft4222->usb2spi[idx].ep_out)
			cnt++;

	dev_info(&ft4222->usb_dev->dev, "%s return %d\n", __func__, cnt);
	return cnt;
}

static int ft4222_spi_probe(struct usb_interface *intf,
			    const struct usb_device_id *id)
{
	struct usb_device *usb_dev =  usb_get_dev(interface_to_usbdev(intf));
	struct ft4222_spi_data *ft4222 = dev_get_drvdata(&usb_dev->dev);
	struct spi_master *spim;
	int rc;

	dev_info(&intf->dev,
		 "%s: usb devnum = %d\n", __func__, usb_dev->devnum);
	if (!ft4222) {
		/* Allocate spi master controller */
		dev_info(&usb_dev->dev, "allocate spi master controller\n");
		spim = spi_alloc_master(&usb_dev->dev,
					sizeof(struct ft4222_spi_data));
		if (!spim) {
			rc = -ENOMEM;
			goto failure;
		}

		spim->mode_bits = SPI_CPOL | SPI_CPHA;
		spim->bits_per_word_mask = SPI_BPW_MASK(8) |
					   SPI_BPW_MASK(16) |
					   SPI_BPW_MASK(32);
		spim->setup = ft4222_spi_setup;
		spim->cleanup = ft4222_spi_cleanup;
		spim->transfer_one_message = ft4222_spi_transfer_one_message;
		spim->num_chipselect = FTDI_CS_MAX;
		spim->bus_num = -1;

		ft4222 = spi_master_get_devdata(spim);
		ft4222->usb_dev = usb_dev;
		ft4222->spim = spim;
		dev_set_drvdata(&usb_dev->dev, ft4222);

		rc = ft4222_spi_detect(ft4222);
		if (rc)
			goto failure;
	}

	rc = ft4222_populate_bulk_ep_info(intf, &ft4222->usb2spi[0]);
	if (rc)
		goto failure;

	/* After all EPs get registered register SPI master controller */
	if (count_registered_eps(ft4222) == FTDI_CS_MAX) {
		dev_set_drvdata(&usb_dev->dev, ft4222);
		ACPI_COMPANION_SET(&usb_dev->dev, ACPI_COMPANION(&intf->dev));
		rc = devm_spi_register_master(&usb_dev->dev, ft4222->spim);
		if (rc) {
			dev_err(&usb_dev->dev,
				"devm_spi_register_master failed, %d\n", rc);
			goto failure;
		}
		dev_info(&usb_dev->dev, "spi master bus = %d\n",
			 ft4222->spim->bus_num);
	}

	return 0;

failure:
	usb_put_dev(usb_dev);
	dev_err(&intf->dev, "%s failed, %d\n", __func__, rc);
	return rc;
}

static void ft4222_spi_disconnect(struct usb_interface *intf)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct ft4222_usb2spi *usb2spi = usb_get_intfdata(intf);

	dev_info(&intf->dev,
		 "%s: usb devnum = %d\n", __func__, usb_dev->devnum);
	if (usb2spi) {
		usb2spi->intf = NULL;
		usb2spi->spi_device = NULL;
		usb2spi->ep_in = 0;
		usb2spi->ep_out = 0;
		usb_set_intfdata(intf, NULL);
	}
	usb_put_dev(usb_dev);
}

static struct usb_device_id ft4222_id_table[] = {
	{ USB_DEVICE(FTDI_FT4222_VID, FTDI_FT4222_PID) },
	{},
};

static struct usb_driver ft4222_spi_driver = {
	.name = "spi_ft4222",
	.probe = ft4222_spi_probe,
	.disconnect = ft4222_spi_disconnect,
	.id_table = ft4222_id_table,
};

module_usb_driver(ft4222_spi_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("FT4222 SPI bus adapter");


// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for TI TPS6598x USB Power Delivery controller family
 *
 * Copyright (C) 2017, Intel Corporation
 * Modification Copyright (C) 2021, Advanced Micro Devices, Inc.
 * Author: Heikki Krogerus <heikki.krogerus@linux.intel.com>
 */

#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/usb/typec.h>
#include <linux/usb/role.h>
#ifdef CONFIG_SPI_TPS6598X_CLIENT
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/pd_vdo.h>
#include <linux/usb/typec_altmode.h>
#endif
/* Register offsets */
#define TPS_REG_VID			0x00
#define TPS_REG_MODE			0x03
#define TPS_REG_CMD1			0x08
#define TPS_REG_DATA1			0x09
#define TPS_REG_INT_EVENT1		0x14
#define TPS_REG_INT_EVENT2		0x15
#define TPS_REG_INT_MASK1		0x16
#define TPS_REG_INT_MASK2		0x17
#define TPS_REG_INT_CLEAR1		0x18
#define TPS_REG_INT_CLEAR2		0x19
#define TPS_REG_STATUS			0x1a
#define TPS_REG_SYSTEM_CONF		0x28
#define TPS_REG_CTRL_CONF		0x29
#define TPS_REG_POWER_STATUS		0x3f
#define TPS_REG_RX_IDENTITY_SOP		0x48
#define TPS_REG_DIP_STATUS		0x58
#define TPS_REG_EVENT_SIZE		11
/* TPS_REG_INT_* bits */
#define TPS_REG_INT_PLUG_EVENT		BIT(3)
#define TPS_DIP_STATUS			(0x3)
/* TPS_REG_STATUS bits */
#define TPS_STATUS_PLUG_PRESENT		BIT(0)
#define TPS_STATUS_ORIENTATION		BIT(4)
#define TPS_STATUS_PORTROLE(s)		(!!((s) & BIT(5)))
#define TPS_STATUS_DATAROLE(s)		(!!((s) & BIT(6)))
#define TPS_STATUS_VCONN(s)		(!!((s) & BIT(7)))

/* TPS_REG_SYSTEM_CONF bits */
#define TPS_SYSCONF_PORTINFO(c)		((c) & 7)

/* DP Connection and SOP defines*/
#define TPS6598x_CON	0x00
#define TPS6598x_SOP	0x01
#define STD_SVID		0xff01

enum {
	TPS_PORTINFO_SINK,
	TPS_PORTINFO_SINK_ACCESSORY,
	TPS_PORTINFO_DRP_UFP,
	TPS_PORTINFO_DRP_UFP_DRD,
	TPS_PORTINFO_DRP_DFP,
	TPS_PORTINFO_DRP_DFP_DRD,
	TPS_PORTINFO_SOURCE,
};

#ifdef CONFIG_SPI_TPS6598X_CLIENT

enum {
	MCU_OEM = 1,
	MCU_I2C1 = 0x10,
	MCU_I2C2,
	MCU_I2C3,
	MCU_I2C4
};

#pragma pack(push, 1)

struct spi_interface {
	u8	checksum;
	u8	flag;
	u8	function;
	u8	cmd;
	u8	txbytenb;
	u8	rxbytenb;
	u8	reg;
	u8	raw[64];
};

#pragma pack(pop)
#endif

/* TPS_REG_POWER_STATUS bits */
#define TPS_POWER_STATUS_SOURCESINK	BIT(1)
#define TPS_POWER_STATUS_PWROPMODE(p)	(((p) & GENMASK(3, 2)) >> 2)

/* TPS_REG_RX_IDENTITY_SOP */
struct tps6598x_rx_identity_reg {
	u8 status;
	struct usb_pd_identity identity;
	u32 vdo[3];
} __packed;

/* Standard Task return codes */
#define TPS_TASK_TIMEOUT		1
#define TPS_TASK_REJECTED		3

enum {
	TPS_MODE_APP,
	TPS_MODE_BOOT,
	TPS_MODE_BIST,
	TPS_MODE_DISC,
};

static const char *const modes[] = {
	[TPS_MODE_APP]	= "APP ",
	[TPS_MODE_BOOT]	= "BOOT",
	[TPS_MODE_BIST]	= "BIST",
	[TPS_MODE_DISC]	= "DISC",
};

/* Unrecognized commands will be replaced with "!CMD" */
#define INVALID_CMD(_cmd_)		(_cmd_ == 0x444d4321)

struct tps6598x {
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock; /* device lock */
	u8 i2c_protocol:1;

	struct typec_port *port;
	struct typec_partner *partner;
	struct usb_pd_identity partner_identity;
	struct usb_role_switch *role_sw;
#ifdef CONFIG_SPI_TPS6598X_CLIENT
	struct spi_device *client;
	int irq;
	int gpio_number;
	struct tps_dp_altmode *dp;
#endif
};

#ifdef CONFIG_SPI_TPS6598X_CLIENT
struct tps_dp_altmode {
	struct typec_displayport_data data;
	struct typec_altmode *alt;
	const struct typec_altmode *port;
	struct	typec_altmode  *part_alt;
	struct tps6598x *tps;
};
#endif
/*
 * Max data bytes for Data1, Data2, and other registers. See ch 1.3.2:
 * http://www.ti.com/lit/ug/slvuan1a/slvuan1a.pdf
 */
#define TPS_MAX_LEN	64
#ifdef CONFIG_SPI_TPS6598X_CLIENT

#define PD_I2C2_SLAVE_ADDR             (0x38 << 1)

static int tps6598x_register_altmodes(struct tps6598x *tps, u8 recipient);

static int tps6598x_exec_cmd(struct tps6598x *tps, const char *cmd,
			     size_t in_len, u8 *in_data,
			     size_t out_len, u8 *out_data);

static int tps6598x_displayport_enter(struct typec_altmode *alt, u32 *vdo);

static int tps6598x_displayport_exit(struct typec_altmode *alt);

static int init_gpio(struct tps6598x *tps)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_TPS6598X_GPIO
	tps->gpio_number = 347;  //AGPIO 91 of MCU (256+91))
#else
	tps->gpio_number = 0;
#endif
	if (!gpio_is_valid(tps->gpio_number)) {
		ret = tps->gpio_number;
		dev_err(tps->dev, "cannot get named GPIO Int_N, ret=%d", ret);
		return ret;
	}
	ret = devm_gpio_request(tps->dev, tps->gpio_number, "pd,int_n");
	if (ret < 0) {
		dev_err(tps->dev, "cannot request GPIO Int_N, ret=%d", ret);
		return ret;
	}
	ret = gpio_direction_input(tps->gpio_number);
	if (ret < 0) {
		dev_err(tps->dev,
			"cannot set GPIO Int_N to input, ret=%d", ret);
		return ret;
	}
	ret = gpio_to_irq(tps->gpio_number);

	if (ret < 0) {
		dev_err(tps->dev,
			"cannot request IRQ for GPIO Int_N, ret=%d", ret);
		return ret;
	}
	tps->irq = ret;
	return 0;
}

static int
tps6598x_block_write_amd(struct tps6598x *tps, u8 reg, void *val, size_t len)

{	struct spi_device *spi = tps->client;
	struct spi_interface sp;
	struct spi_transfer xfer[2];
	struct spi_message msg;
	unsigned char buf[100];
	int rc;

/* first do the SPI write to Inform MCU that this I2C write */
	memset(xfer, 0, sizeof(xfer));
	memset(buf, 0, sizeof(buf));
	sp.function = MCU_I2C2; //physical PD connection
	sp.reg = reg;
	sp.cmd =  PD_I2C2_SLAVE_ADDR;
	sp.txbytenb = len + 1;
	sp.rxbytenb = 0;
	sp.raw[0] = len;
	memcpy(&sp.raw[1], (unsigned char *)val, len);
	memcpy(buf, (unsigned char *)&sp, sizeof(struct spi_interface));
	xfer[0].tx_buf = (const void *)buf;
	xfer[0].len =  sizeof(struct spi_interface);
	spi_message_init(&msg);
	spi_message_add_tail(&xfer[0], &msg);
	rc = spi_sync(spi, &msg);
	if (rc)
		return rc;
	mdelay(20);

/* Now issue the dummy read */
	spi_message_init(&msg);
	xfer[1].rx_buf = (void *)buf;
	xfer[1].len =  sizeof(struct spi_interface);
	xfer[1].tx_buf = (void *)NULL;
	spi_message_add_tail(&xfer[1], &msg);
	rc = spi_sync(spi, &msg);
	mdelay(10);
	return rc;
}

static int
tps6598x_block_read_amd(struct tps6598x *tps, u8 reg, void *val, size_t len)

{	struct spi_device *spi = tps->client;
	struct spi_interface sp;
	struct spi_transfer xfer[2];
	struct spi_message msg;
	unsigned char buf[100];
	int rc;
/* first do the SPI write to Inform MCU that this I2C Read */
	memset(xfer, 0, sizeof(xfer));
	memset(buf, 0, sizeof(buf));
	sp.function = MCU_I2C2; //physical PD connection
	sp.reg = reg;
	sp.cmd =  PD_I2C2_SLAVE_ADDR;
	sp.txbytenb = 1;
	sp.rxbytenb = len + 1;
	memcpy(buf, (unsigned char *)&sp, sizeof(struct spi_interface));
	xfer[0].tx_buf = (const void *)buf;
	xfer[0].len =  sizeof(struct spi_interface);
	spi_message_init(&msg);
	spi_message_add_tail(&xfer[0], &msg);
	rc = spi_sync(spi, &msg);
	if (rc)
		return rc;
	mdelay(10);

	/*Now issue the SPI Read to get actual I2C Data */
	spi_message_init(&msg);
	xfer[1].rx_buf = (void *)buf;
	xfer[1].len =  sizeof(struct spi_interface);
	xfer[1].tx_buf = (const void *)NULL;
	spi_message_add_tail(&xfer[1], &msg);
	rc = spi_sync(spi, &msg);
	if (rc)
		return rc;
	memcpy((unsigned char *)val, &buf[8], len);
	mdelay(10);
	return rc;
}
#endif

static int
tps6598x_block_read(struct tps6598x *tps, u8 reg, void *val, size_t len)
{
#ifdef CONFIG_SPI_TPS6598X_CLIENT
	return tps6598x_block_read_amd(tps, reg, val, len);
#else
	u8 data[TPS_MAX_LEN + 1];
	int ret;

	if (WARN_ON(len + 1 > sizeof(data)))
		return -EINVAL;

	if (!tps->i2c_protocol)
		return regmap_raw_read(tps->regmap, reg, val, len);

	ret = regmap_raw_read(tps->regmap, reg, data, sizeof(data));
	if (ret)
		return ret;

	if (data[0] < len)
		return -EIO;

	memcpy(val, &data[1], len);
#endif
	return 0;
}

static int tps6598x_block_write(struct tps6598x *tps, u8 reg,
				const void *val, size_t len)
{
#ifdef CONFIG_SPI_TPS6598X_CLIENT
	return tps6598x_block_write_amd(tps, reg, (void *)val, len);
#else
	u8 data[TPS_MAX_LEN + 1];

	if (!tps->i2c_protocol)
		return regmap_raw_write(tps->regmap, reg, val, len);

	data[0] = len;
	memcpy(&data[1], val, len);

	return regmap_raw_write(tps->regmap, reg, data, sizeof(data));
#endif
}

static inline int tps6598x_read16(struct tps6598x *tps, u8 reg, u16 *val)
{
	return tps6598x_block_read(tps, reg, val, sizeof(u16));
}

static inline int tps6598x_read32(struct tps6598x *tps, u8 reg, u32 *val)
{
	return tps6598x_block_read(tps, reg, val, sizeof(u32));
}

static inline int tps6598x_read64(struct tps6598x *tps, u8 reg, u64 *val)
{
	return tps6598x_block_read(tps, reg, val, sizeof(u64));
}

static inline int tps6598x_write16(struct tps6598x *tps, u8 reg, u16 val)
{
	return tps6598x_block_write(tps, reg, &val, sizeof(u16));
}

static inline int tps6598x_write32(struct tps6598x *tps, u8 reg, u32 val)
{
	return tps6598x_block_write(tps, reg, &val, sizeof(u32));
}

static inline int tps6598x_write64(struct tps6598x *tps, u8 reg, u64 val)
{
	return tps6598x_block_write(tps, reg, &val, sizeof(u64));
}

static inline int
tps6598x_write_4cc(struct tps6598x *tps, u8 reg, const char *val)
{
	return tps6598x_block_write(tps, reg, val, 4);
}

static int tps6598x_read_partner_identity(struct tps6598x *tps)
{
	struct tps6598x_rx_identity_reg id;
	int ret;

	ret = tps6598x_block_read(tps, TPS_REG_RX_IDENTITY_SOP,
				  &id, sizeof(id));
	if (ret)
		return ret;

	tps->partner_identity = id.identity;

	return 0;
}

static void tps6598x_set_data_role(struct tps6598x *tps,
				   enum typec_data_role role, bool connected)
{
	enum usb_role role_val;

	if (role == TYPEC_HOST)
		role_val = USB_ROLE_HOST;
	else
		role_val = USB_ROLE_DEVICE;

	if (!connected)
		role_val = USB_ROLE_NONE;

	usb_role_switch_set_role(tps->role_sw, role_val);
	typec_set_data_role(tps->port, role);
}

static int tps6598x_connect(struct tps6598x *tps, u32 status)
{
	struct typec_partner_desc desc;
	enum typec_pwr_opmode mode;
	u16 pwr_status;
	int ret;

	if (tps->partner)
		return 0;

	ret = tps6598x_read16(tps, TPS_REG_POWER_STATUS, &pwr_status);
	if (ret < 0)
		return ret;

	mode = TPS_POWER_STATUS_PWROPMODE(pwr_status);

	desc.usb_pd = mode == TYPEC_PWR_MODE_PD;
	desc.accessory = TYPEC_ACCESSORY_NONE; /* XXX: handle accessories */
	desc.identity = NULL;

	if (desc.usb_pd) {
		ret = tps6598x_read_partner_identity(tps);
		if (ret)
			return ret;
		desc.identity = &tps->partner_identity;
	}

	typec_set_pwr_opmode(tps->port, mode);
	typec_set_pwr_role(tps->port, TPS_STATUS_PORTROLE(status));
	typec_set_vconn_role(tps->port, TPS_STATUS_VCONN(status));
	tps6598x_set_data_role(tps, TPS_STATUS_DATAROLE(status), true);

	tps->partner = typec_register_partner(tps->port, &desc);
	if (IS_ERR(tps->partner))
		return PTR_ERR(tps->partner);

	if (desc.identity)
		typec_partner_set_identity(tps->partner);

	return 0;
}

static void tps6598x_disconnect(struct tps6598x *tps, u32 status)
{

	if (!IS_ERR(tps->partner))
		typec_unregister_partner(tps->partner);
	tps->partner = NULL;
	typec_set_pwr_opmode(tps->port, TYPEC_PWR_MODE_USB);
	typec_set_pwr_role(tps->port, TPS_STATUS_PORTROLE(status));
	typec_set_vconn_role(tps->port, TPS_STATUS_VCONN(status));
	tps6598x_set_data_role(tps, TPS_STATUS_DATAROLE(status), false);
}

static int tps6598x_exec_cmd(struct tps6598x *tps, const char *cmd,
			     size_t in_len, u8 *in_data,
			     size_t out_len, u8 *out_data)
{
	unsigned long timeout;
	u32 val;
	int ret;

	ret = tps6598x_read32(tps, TPS_REG_CMD1, &val);
	if (ret)
		return ret;
	if (val && !INVALID_CMD(val))
		return -EBUSY;

	if (in_len) {
		ret = tps6598x_block_write(tps, TPS_REG_DATA1,
					   in_data, in_len);
		if (ret)
			return ret;
	}

	ret = tps6598x_write_4cc(tps, TPS_REG_CMD1, cmd);
	if (ret < 0)
		return ret;

	/* XXX: Using 1s for now, but it may not be enough for every command. */
	timeout = jiffies + msecs_to_jiffies(1000);

	do {
		ret = tps6598x_read32(tps, TPS_REG_CMD1, &val);
		if (ret)
			return ret;
		if (INVALID_CMD(val))
			return -EINVAL;

		if (time_is_before_jiffies(timeout))
			return -ETIMEDOUT;
	} while (val);

	if (out_len) {
		ret = tps6598x_block_read(tps, TPS_REG_DATA1,
					  out_data, out_len);
		if (ret)
			return ret;
		val = out_data[0];
	} else {
		ret = tps6598x_block_read(tps, TPS_REG_DATA1, &val, sizeof(u8));
		if (ret)
			return ret;
	}

	switch (val) {
	case TPS_TASK_TIMEOUT:
		return -ETIMEDOUT;
	case TPS_TASK_REJECTED:
		return -EPERM;
	default:
		break;
	}

	return 0;
}

static int tps6598x_dr_set(struct typec_port *port, enum typec_data_role role)
{
	const char *cmd = (role == TYPEC_DEVICE) ? "SWUF" : "SWDF";
	struct tps6598x *tps = typec_get_drvdata(port);
	u32 status;
	int ret;

	mutex_lock(&tps->lock);

	ret = tps6598x_exec_cmd(tps, cmd, 0, NULL, 0, NULL);
	if (ret)
		goto out_unlock;

	ret = tps6598x_read32(tps, TPS_REG_STATUS, &status);
	if (ret)
		goto out_unlock;

	if (role != TPS_STATUS_DATAROLE(status)) {
		ret = -EPROTO;
		goto out_unlock;
	}

	tps6598x_set_data_role(tps, role, true);

out_unlock:
	mutex_unlock(&tps->lock);

	return ret;
}

static int tps6598x_pr_set(struct typec_port *port, enum typec_role role)
{
	const char *cmd = (role == TYPEC_SINK) ? "SWSk" : "SWSr";
	struct tps6598x *tps = typec_get_drvdata(port);
	u32 status;
	int ret;

	mutex_lock(&tps->lock);

	ret = tps6598x_exec_cmd(tps, cmd, 0, NULL, 0, NULL);
	if (ret)
		goto out_unlock;

	ret = tps6598x_read32(tps, TPS_REG_STATUS, &status);
	if (ret)
		goto out_unlock;

	if (role != TPS_STATUS_PORTROLE(status)) {
		ret = -EPROTO;
		goto out_unlock;
	}

	typec_set_pwr_role(tps->port, role);

out_unlock:
	mutex_unlock(&tps->lock);

	return ret;
}

static const struct typec_operations tps6598x_ops = {
	.dr_set = tps6598x_dr_set,
	.pr_set = tps6598x_pr_set,
};

static irqreturn_t tps6598x_interrupt(int irq, void *data)
{
	struct tps6598x *tps = data;
	u64 event1;
	u64 event2;
	u64 dp_sid_status;
	u32 status;
	int ret;
	u8 clear[TPS_REG_EVENT_SIZE];

	memset(clear, 0xff, TPS_REG_EVENT_SIZE);
	mutex_lock(&tps->lock);

	ret = tps6598x_read64(tps, TPS_REG_INT_EVENT1, &event1);
	ret |= tps6598x_read64(tps, TPS_REG_INT_EVENT2, &event2);
	dev_dbg(tps->dev, "Enter event1:0x%llx event2:0x%llx\n", event1, event2);
	if (ret) {
		dev_err(tps->dev, "%s: failed to read events\n", __func__);
		goto err_unlock;
	}

	ret = tps6598x_read32(tps, TPS_REG_STATUS, &status);
	dev_dbg(tps->dev, "new interrupt handler  enter status =0x%xx\n", status);

	if (ret) {
		dev_err(tps->dev, "%s: failed to read status\n", __func__);
		goto err_clear_ints;
	}

	/* Handle plug insert or removal */
	if ((event1 | event2) & TPS_REG_INT_PLUG_EVENT) {
		if (status & TPS_STATUS_PLUG_PRESENT) {
			ret = tps6598x_connect(tps, status);
			if (ret)
				dev_err(tps->dev,
					"failed to register partner\n");
		} else {
			tps6598x_disconnect(tps, status);
		}
	}

#ifdef CONFIG_SPI_TPS6598X_CLIENT
	ret = tps6598x_read64(tps, TPS_REG_DIP_STATUS, &dp_sid_status);

	if (ret) {
		dev_err(tps->dev, "%s: failed to DP status events\n", __func__);
		goto err_clear_ints;
	}
	if ((dp_sid_status  & TPS_DIP_STATUS) == TPS_DIP_STATUS) {
		dev_dbg(tps->dev, " DP Mode is active\n");
		ret = tps6598x_register_altmodes(tps, TPS6598x_SOP);
		if (ret) {
			dev_err(tps->dev, "%s: failed to register partner altmode\n", __func__);
			goto err_clear_ints;
		}
	}
#endif

err_clear_ints:
	tps6598x_read64(tps, TPS_REG_INT_EVENT1, &event1);
	tps6598x_read64(tps, TPS_REG_INT_EVENT2, &event2);
	tps6598x_block_write(tps, TPS_REG_INT_CLEAR1, clear, TPS_REG_EVENT_SIZE);
	tps6598x_block_write(tps, TPS_REG_INT_CLEAR2, clear, TPS_REG_EVENT_SIZE);
	tps6598x_read64(tps, TPS_REG_INT_EVENT1, &event1);
	tps6598x_read64(tps, TPS_REG_INT_EVENT2, &event2);
	dev_dbg(tps->dev, "After clearing event1:0x%llx event2:0x%llx\n", event1, event2);
err_unlock:
	mutex_unlock(&tps->lock);

	return IRQ_HANDLED;
}

static int tps6598x_check_mode(struct tps6598x *tps)
{
	char mode[5] = { };
	int ret;

	ret = tps6598x_read32(tps, TPS_REG_MODE, (void *)mode);
	if (ret)
		return ret;

	switch (match_string(modes, ARRAY_SIZE(modes), mode)) {
	case TPS_MODE_APP:
		dev_info(tps->dev, "in App Mode\n");
		return 0;
	case TPS_MODE_BOOT:
		dev_warn(tps->dev, "dead-battery condition\n");
		return 0;
	case TPS_MODE_BIST:
	case TPS_MODE_DISC:
	default:
		dev_err(tps->dev, "controller in unsupported mode \"%s\"\n",
			mode);
		break;
	}

	return -ENODEV;
}

#ifndef CONFIG_SPI_TPS6598X_CLIENT
static const struct regmap_config tps6598x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x7F,
};

static int tps6598x_probe(struct i2c_client *client)
#else
static int tps6598x_probe(struct spi_device *client)
#endif
{
	struct typec_capability typec_cap = { };
	struct tps6598x *tps;
	u32 conf;
	u32 status;
	u32 vid;
	int ret;
	u64 dp_sid_status;

#ifdef CONFIG_SPI_TPS6598X_CLIENT
	u8 event1[TPS_REG_EVENT_SIZE];
	u8 event2[TPS_REG_EVENT_SIZE];
	const struct software_node *swnode;
#endif
	struct fwnode_handle *fwnode;

	tps = devm_kzalloc(&client->dev, sizeof(*tps), GFP_KERNEL);
	if (!tps)
		return -ENOMEM;

	mutex_init(&tps->lock);
	tps->dev = &client->dev;
#ifdef CONFIG_SPI_TPS6598X_CLIENT
	tps->client = client;
#else
	tps->regmap = devm_regmap_init_i2c(client, &tps6598x_regmap_config);
	if (IS_ERR(tps->regmap))
		return PTR_ERR(tps->regmap);
	/*
	 * Checking can the adapter handle SMBus protocol. If it can not, the
	 * driver needs to take care of block reads separately.
	 *
	 * FIXME: Testing with I2C_FUNC_I2C. regmap-i2c uses I2C protocol
	 * unconditionally if the adapter has I2C_FUNC_I2C set.
	 */
	if (i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		tps->i2c_protocol = true;

#endif

	ret = tps6598x_read32(tps, TPS_REG_VID, &vid);
	if (ret < 0 || !vid)
		return -ENODEV;

	/* Make sure the controller has application firmware running */
	ret = tps6598x_check_mode(tps);
	if (ret)
		return ret;

	ret = tps6598x_read32(tps, TPS_REG_STATUS, &status);
	if (ret < 0)
		return ret;

	ret = tps6598x_read32(tps, TPS_REG_SYSTEM_CONF, &conf);

	if (ret < 0)
		return ret;

#ifdef CONFIG_SPI_TPS6598X_CLIENT
	swnode = software_node_find_by_name(NULL, "amd-dwc3-usb-sw");

	if (!swnode)
		return -EPROBE_DEFER;
	fwnode = software_node_fwnode(swnode);
	tps->role_sw = usb_role_switch_find_by_fwnode(fwnode);
	if (IS_ERR(tps->role_sw)) {
		ret = PTR_ERR(tps->role_sw);
		goto err_fwnode_put;
	}
	ret = tps6598x_block_read_amd(tps, TPS_REG_INT_EVENT1, event1, TPS_REG_EVENT_SIZE);
	if (ret < 0)
		return ret;
	ret = tps6598x_block_read_amd(tps, TPS_REG_INT_EVENT2, event2, TPS_REG_EVENT_SIZE);
	if (ret < 0)
		return ret;
	ret  = tps6598x_block_write_amd(tps, TPS_REG_INT_CLEAR1, event1, TPS_REG_EVENT_SIZE);
	if (ret < 0)
		return ret;
	ret = tps6598x_block_write_amd(tps, TPS_REG_INT_CLEAR2, event2, TPS_REG_EVENT_SIZE);
	if (ret < 0)
		return ret;
	ret = tps6598x_block_read_amd(tps, TPS_REG_INT_EVENT1, event1, TPS_REG_EVENT_SIZE);
	if (ret < 0)
		return ret;
	ret = tps6598x_block_read_amd(tps, TPS_REG_INT_EVENT2, event2, TPS_REG_EVENT_SIZE);
	if (ret < 0)
		return ret;
#else
	fwnode = device_get_named_child_node(&client->dev, "connector");
	if (IS_ERR(fwnode))
		return PTR_ERR(fwnode);
	tps->role_sw = fwnode_usb_role_switch_get(fwnode);
	if (IS_ERR(tps->role_sw)) {
		ret = PTR_ERR(tps->role_sw);
		goto err_fwnode_put;
	}
#endif
	typec_cap.revision = USB_TYPEC_REV_1_2;
	typec_cap.pd_revision = 0x200;
	typec_cap.prefer_role = TYPEC_NO_PREFERRED_ROLE;
	typec_cap.driver_data = tps;
	typec_cap.ops = &tps6598x_ops;
	typec_cap.fwnode = fwnode;

	switch (TPS_SYSCONF_PORTINFO(conf)) {
	case TPS_PORTINFO_SINK_ACCESSORY:
	case TPS_PORTINFO_SINK:
		typec_cap.type = TYPEC_PORT_SNK;
		typec_cap.data = TYPEC_PORT_UFP;
		break;
	case TPS_PORTINFO_DRP_UFP_DRD:
	case TPS_PORTINFO_DRP_DFP_DRD:
		typec_cap.type = TYPEC_PORT_DRP;
		typec_cap.data = TYPEC_PORT_DRD;
		break;
	case TPS_PORTINFO_DRP_UFP:
		typec_cap.type = TYPEC_PORT_DRP;
		typec_cap.data = TYPEC_PORT_UFP;
		break;
	case TPS_PORTINFO_DRP_DFP:
		typec_cap.type = TYPEC_PORT_DRP;
		typec_cap.data = TYPEC_PORT_DFP;
		break;
	case TPS_PORTINFO_SOURCE:
		typec_cap.type = TYPEC_PORT_SRC;
		typec_cap.data = TYPEC_PORT_DFP;
		break;
	default:
		ret = -ENODEV;
		goto err_role_put;
	}

	tps->port = typec_register_port(&client->dev, &typec_cap);
	if (IS_ERR(tps->port)) {
		ret = PTR_ERR(tps->port);
		goto err_role_put;
	}

#ifdef CONFIG_SPI_TPS6598X_CLIENT
	ret = tps6598x_register_altmodes(tps, TPS6598x_CON);
	if (ret) {
		dev_err(tps->dev, "failed to register altmode\n");
		goto err_role_put;
	}
#endif

	if (status & TPS_STATUS_PLUG_PRESENT) {
		ret = tps6598x_connect(tps, status);
		if (ret) {
			dev_err(tps->dev, "failed to register partner\n");
			goto err_role_put;
		}
#ifdef CONFIG_SPI_TPS6598X_CLIENT
		ret = tps6598x_read64(tps, TPS_REG_DIP_STATUS, &dp_sid_status);
		if (ret) {
			dev_err(tps->dev, "failed to read the dp status register partner\n");
			goto err_role_put;
		}
		dev_dbg(tps->dev, " probe dp_sid_status =0x%llx\n", dp_sid_status);

		if ((dp_sid_status & TPS_DIP_STATUS) == TPS_DIP_STATUS) {
			dev_dbg(tps->dev, " DP Mode is active\n");
			ret =  tps6598x_register_altmodes(tps, TPS6598x_SOP);
			if (ret) {
				dev_err(tps->dev, "failed to register partner\n");
				goto err_role_put;
			}
		}
#endif
	}

#ifdef CONFIG_SPI_TPS6598X_CLIENT
	client->irq = 0;
	init_gpio(tps);
	client->irq = tps->irq;

	ret = devm_request_threaded_irq(&client->dev, client->irq, NULL,
					tps6598x_interrupt,
					IRQF_ONESHOT |	IRQF_TRIGGER_FALLING,
					dev_name(&client->dev), tps);
#else
	ret = devm_request_threaded_irq(&client->dev, client->irq, NULL,
					tps6598x_interrupt,
					IRQF_SHARED | IRQF_ONESHOT,
					dev_name(&client->dev), tps);
#endif
	if (ret) {
		tps6598x_disconnect(tps, 0);
		typec_unregister_port(tps->port);
		goto err_role_put;
	}
#ifdef CONFIG_SPI_TPS6598X_CLIENT
	fwnode_handle_put(fwnode);
	spi_set_drvdata(client, tps);
	return 0;
#else
	fwnode_handle_put(fwnode);
	i2c_set_clientdata(client, tps);
	return 0;
#endif

err_role_put:
	usb_role_switch_put(tps->role_sw);
err_fwnode_put:
	fwnode_handle_put(fwnode);

	return ret;
}

#ifdef CONFIG_SPI_TPS6598X_CLIENT
static const struct typec_altmode_ops tps6598x_displayport_ops = {
	.enter = tps6598x_displayport_enter,
	.exit = tps6598x_displayport_exit,
	.vdm = NULL,

};

static int tps6598x_enter_mode(struct tps6598x *tps)
{
	const char *cmd = "AMEn";
	int ret;
	u8 out_data = 0;
	u8 in_len;
	u8 out_len;
	u32 in_data;
	u64 dp_sid_status;

	ret = tps6598x_read64(tps, TPS_REG_DIP_STATUS, &dp_sid_status);
	if (ret) {
		dev_err(tps->dev, "%s: failed to DP status events\n", __func__);
		goto out;
	}
	dev_dbg(tps->dev, "before executing command  enter dp_sid_status =0x%llx\n", dp_sid_status);

	in_len = 3;
	out_len = 1;
	in_data = (STD_SVID << 8) | (0x01 << 5);
	ret = tps6598x_exec_cmd(tps, cmd, in_len, (u8 *)&in_data, out_len, &out_data);
	/* do not check for return status of AMEn command, temp workaround */
	ret = tps6598x_read64(tps, TPS_REG_DIP_STATUS, &dp_sid_status);
	if (ret) {
		dev_err(tps->dev, "%s: failed to DP status events\n", __func__);
		goto out;
	}
	dev_dbg(tps->dev, "after executing AMEn command  enter dp_sid_status =0x%llx\n", dp_sid_status);
	if ((dp_sid_status  & TPS_DIP_STATUS) == TPS_DIP_STATUS)
		ret = 0;
	else
		ret = -EINVAL;
out:
	return ret;
}

static int tps6598x_exit_mode(struct tps6598x *tps)
{
	const char *cmd = "AMEx";
	int ret;
	u8 out_data = 0;
	u8 in_len;
	u8 out_len;
	u32 in_data;
	u64 dp_sid_status;

	ret = tps6598x_read64(tps, TPS_REG_DIP_STATUS, &dp_sid_status);
	if (ret) {
		dev_err(tps->dev, "%s: failed to DP status events\n", __func__);
		goto out;
	}
	dev_dbg(tps->dev, "before executing command  enter dp_sid_status =0x%llx\n", dp_sid_status);

	in_data = (STD_SVID << 8) | (0x7 << 5);
	in_len = 3;
	out_len = 1;
	ret = tps6598x_exec_cmd(tps, cmd, in_len, (u8 *)&in_data, out_len, &out_data);
	/* do not check for return status of AMEx command, temp workaround */
	ret = tps6598x_read64(tps, TPS_REG_DIP_STATUS, &dp_sid_status);
	if (ret) {
		dev_err(tps->dev, "%s: failed to DP status events\n", __func__);
		goto out;
	}
	dev_dbg(tps->dev, "after executing AMEn command  enter dp_sid_status =0x%llx\n", dp_sid_status);

	if ((dp_sid_status  & TPS_DIP_STATUS) != TPS_DIP_STATUS)
		ret = 0;
	else
		ret = -EINVAL;
out:
	return ret;
}

static int tps6598x_displayport_enter(struct typec_altmode *alt, u32 *vdo)
{
	struct tps_dp_altmode *dp = typec_altmode_get_drvdata(alt);
	int ret = 0;
	u64 dp_sid_status;

	mutex_lock(&dp->tps->lock);
	dev_dbg(dp->tps->dev, "%s\n", __func__);

	tps6598x_read64(dp->tps, TPS_REG_DIP_STATUS, &dp_sid_status);

	if ((dp_sid_status  & TPS_DIP_STATUS) != TPS_DIP_STATUS)
		ret = tps6598x_enter_mode(dp->tps);
	else
		dev_dbg(dp->tps->dev, "Already in Enter Mode\n");

	if (ret) {
		dev_err(dp->tps->dev, "Failed to go to Enter Mode\n");
		goto out_unlock;
	}

	/*To update sys interface make driver owner NULL */
	dp->part_alt->dev.driver = NULL;
	typec_altmode_update_active(dp->part_alt, true);

out_unlock:
	mutex_unlock(&dp->tps->lock);
	return ret;
}

static int tps6598x_displayport_exit(struct typec_altmode *alt)
{
	struct tps_dp_altmode *dp = typec_altmode_get_drvdata(alt);
	int ret = 0;
	u64 dp_sid_status;

	mutex_lock(&dp->tps->lock);
	dev_dbg(dp->tps->dev, "%s\n", __func__);

	tps6598x_read64(dp->tps, TPS_REG_DIP_STATUS, &dp_sid_status);

	if ((dp_sid_status  & TPS_DIP_STATUS) != TPS_DIP_STATUS)
		dev_dbg(dp->tps->dev, "Already in Exit Mode\n");
	else
		ret = tps6598x_exit_mode(dp->tps);

	if (ret) {
		dev_err(dp->tps->dev, "Failed to go to Exit Mode\n");
		goto out_unlock;
	}

	/*To update sys interface make driver owner NULL */
	dp->part_alt->dev.driver = NULL;
	typec_altmode_update_active(dp->part_alt, false);

out_unlock:
	mutex_unlock(&dp->tps->lock);
	return 0;
}

static struct typec_altmode_desc desc;

static int tps6598x_register_altmodes(struct tps6598x *tps, u8 recipient)
{
	u8 all_assignments = BIT(DP_PIN_ASSIGN_C) | BIT(DP_PIN_ASSIGN_D) |
			     BIT(DP_PIN_ASSIGN_E);

	int ret;
	struct typec_altmode *alt;
	struct tps_dp_altmode *dp;

	switch (recipient) {
	case TPS6598x_CON:
		desc.vdo = 0x01;//alt[j].mid;
		desc.svid = STD_SVID;//alt[j].svid;
		desc.roles = TYPEC_PORT_DRD;
		desc.mode = 0x01;
		/* We can't rely on the firmware with the capabilities. */
		desc.vdo |= DP_CAP_DP_SIGNALING | DP_CAP_RECEPTACLE;

		/* Claiming that we support all pin assignments */
		desc.vdo |= all_assignments << 8;
		desc.vdo |= all_assignments << 16;

		alt = typec_port_register_altmode(tps->port, &desc);
		if (IS_ERR(alt))
			return -ENOMEM;

		dp = devm_kzalloc(&alt->dev, sizeof(*dp), GFP_KERNEL);

		if (!dp) {
			typec_unregister_altmode(alt);
			return -ENOMEM;
		}

		dp->alt = alt;
		alt->ops = &tps6598x_displayport_ops;
		tps->dp = dp;
		typec_altmode_set_drvdata(alt, dp);
		dp->tps = tps;

		break;

	case TPS6598x_SOP:
		if (!tps->dp->part_alt)	{
			alt = typec_partner_register_altmode(tps->partner, &desc);
				if (IS_ERR(alt)) {
					ret = PTR_ERR(alt);
					return ret;
				}
			tps->dp->part_alt = alt;
			dev_dbg(tps->dev, "tps6598x_partner_register_altmodes  alt=%p\n", alt);
		}

		break;
	}

	return 0;
}

static int tps6598x_remove(struct spi_device *client)
{
	struct tps6598x *tps = spi_get_drvdata(client);

	tps6598x_disconnect(tps, 0);
	if (tps->dp && tps->dp->alt)
		typec_unregister_altmode(tps->dp->alt);
	typec_unregister_port(tps->port);
	usb_role_switch_put(tps->role_sw);

	return 0;
}

static const struct spi_device_id tps6598x_id[] = {
	{ "tps6598x_new" },
	{ }
};
MODULE_DEVICE_TABLE(spi, tps6598x_id);

static struct spi_driver tps6598x_spi_driver = {
	.driver = {
		.name = "tps6598x_new",
	},
	.probe = tps6598x_probe,
	.remove = tps6598x_remove,
	.id_table = tps6598x_id,
};

module_spi_driver(tps6598x_spi_driver);
#else
static int tps6598x_remove(struct i2c_client *client)
{
	struct tps6598x *tps = i2c_get_clientdata(client);

	tps6598x_disconnect(tps, 0);
	typec_unregister_port(tps->port);
	usb_role_switch_put(tps->role_sw);
	return 0;
}

static const struct of_device_id tps6598x_of_match[] = {
	{ .compatible = "ti,tps6598x", },
	{}
};
MODULE_DEVICE_TABLE(of, tps6598x_of_match);

static const struct i2c_device_id tps6598x_id[] = {
	{ "tps6598x" },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tps6598x_id);

static struct i2c_driver tps6598x_i2c_driver = {
	.driver = {
		.name = "tps6598x",
		.of_match_table = tps6598x_of_match,
	},
	.probe_new = tps6598x_probe,
	.remove = tps6598x_remove,
	.id_table = tps6598x_id,
};
module_i2c_driver(tps6598x_i2c_driver);
#endif

MODULE_AUTHOR("Heikki Krogerus <heikki.krogerus@linux.intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("TI TPS6598x USB Power Delivery Controller Driver");

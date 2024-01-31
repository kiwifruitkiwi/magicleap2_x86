/* Copyright (c) 2019, Magic Leap, Inc. All rights reserved.
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
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <asm/fpu/api.h>
#include <linux/byteorder/generic.h>

#define DEVICE_NAME     "icp101xx"
#define CMD_MSG_SIZE    2
static const uint8_t icp_cmd_reset[CMD_MSG_SIZE] = { 0x80, 0x5d };
static const uint8_t icp_cmd_read_id[CMD_MSG_SIZE] = { 0xef, 0xc8 };
static const uint8_t icp_cmd_read_otp[CMD_MSG_SIZE] = { 0xc7, 0xf7 };
/* normal mode */
static const uint8_t icp_cmd_meas_n[CMD_MSG_SIZE] = { 0x48, 0xa3 };
/* low power mode */
static const uint8_t icp_cmd_meas_lp[CMD_MSG_SIZE] = { 0x40, 0x1a };
/* low noise mode */
static const uint8_t icp_cmd_meas_ln[CMD_MSG_SIZE] = { 0x50, 0x59 };
/* ultra low noise mode */
static const uint8_t icp_cmd_meas_uln[CMD_MSG_SIZE] = { 0x58, 0xe0 };
static const uint8_t icp_cmd_read_otp_mode[] = { 0xc5, 0x95, 0x00, 0x66, 0x9c };

enum icp_dev_meas_modes {
	NORMAL = 0,
	LOW_POWER,
	LOW_NOISE,
	ULTRA_LOW_NOISE,
};

static const uint8_t *device_mode[] = {
	[NORMAL] = icp_cmd_meas_n,
	[LOW_POWER] = icp_cmd_meas_lp,
	[LOW_NOISE] = icp_cmd_meas_ln,
	[ULTRA_LOW_NOISE] = icp_cmd_meas_uln,
};

/* ICP-101xx Available modes (Shown using sysfs entry to user) */
static const char * const meas_modes[] = {
	"NORMAL",
	"LOW_POWER",
	"LOW_NOISE",
	"ULTRA_LOW_NOISE"
};

struct icp101xx_dev {
	struct i2c_client *client;
	struct mutex icp_lock;
	int32_t sensor_const[4];
	uint16_t dev_id;
	const uint8_t *current_meas_mode;
	enum icp_dev_meas_modes meas_mode;
};

static int icp101xx_i2c_send(const struct i2c_client *client, const char *buf,
		int count)
{
	int rc;

	rc = i2c_master_send(client, buf, count);
	if (rc < 0) {
		dev_err(&client->dev, "I2C send failed, rc %d\n", rc);
	} else if (rc != count) {
		dev_err(&client->dev, "I2C send failed, wrote %d\n", rc);
		rc = -EIO;
	} else {
		rc = 0;
	}
	return rc;
}

static int icp101xx_i2c_recv(const struct i2c_client *client, char *buf,
		int count)
{
	int rc;

	rc = i2c_master_recv(client, buf, count);
	if (rc < 0) {
		dev_err(&client->dev, "I2C recv failed, rc %d\n", rc);
	} else if (rc != count) {
		dev_err(&client->dev, "I2C recv failed, read %d\n", rc);
		rc = -EIO;
	} else {
		rc = 0;
	}
	return rc;
}

static int icp101xx_soft_reset_device(struct icp101xx_dev *data)
{
	int rc;
	struct i2c_client *client = data->client;

	mutex_lock(&data->icp_lock);
	rc = icp101xx_i2c_send(client, (char *) icp_cmd_reset,
			ARRAY_SIZE(icp_cmd_reset));
	if (rc) {
		dev_err(&client->dev, "Send failed reset, %d\n", rc);
		goto exit;
	}

	msleep_interruptible(200);
	dev_dbg(&client->dev, "%s Reset successful\n", DEVICE_NAME);

exit:
	mutex_unlock(&data->icp_lock);
	return rc;
}

static int icp101xx_get_device_id(struct icp101xx_dev *data)
{
	uint16_t buf;
	int rc;
	struct i2c_client *client = data->client;

	mutex_lock(&data->icp_lock);
	rc = icp101xx_i2c_send(client, (char *) icp_cmd_read_id,
			ARRAY_SIZE(icp_cmd_read_id));
	if (rc) {
		dev_err(&client->dev, "Send failed read_id, %d\n", rc);
		goto exit;
	}

	rc = icp101xx_i2c_recv(client, (char *) &buf, sizeof(buf));
	if (rc) {
		dev_err(&client->dev, "Recv failed read_id, %d\n", rc);
		goto exit;
	}

	data->dev_id = (be16_to_cpup((__le16 *) &buf) & 0x3F);
	dev_dbg(&client->dev, "Device ID is : 0x%x for %s\n",
				data->dev_id, DEVICE_NAME);

exit:
	mutex_unlock(&data->icp_lock);
	return rc;
}

static int icp101xx_get_calibration_data(struct icp101xx_dev *data)
{
	uint16_t buf;
	int rc, i;
	struct i2c_client *client = data->client;

	mutex_lock(&data->icp_lock);
	rc = icp101xx_i2c_send(client, (char *) icp_cmd_read_otp_mode,
			ARRAY_SIZE(icp_cmd_read_otp_mode));
	if (rc) {
		dev_err(&client->dev, "Send failed read_otp_mode, %d\n", rc);
		goto exit;
	}

	for (i = 0; i < 4; i++) {
		msleep_interruptible(100);
		rc = icp101xx_i2c_send(client, (char *) icp_cmd_read_otp,
				ARRAY_SIZE(icp_cmd_read_otp));
		if (rc) {
			dev_err(&client->dev, "Send failed read_otp, %d\n",
					rc);
			goto exit;
		}

		msleep_interruptible(100);
		rc = icp101xx_i2c_recv(client, (char *) &buf, sizeof(buf));
		if (rc) {
			dev_err(&client->dev, "Recv failed read_otp, %d\n",
					rc);
			goto exit;
		}
		data->sensor_const[i] = be16_to_cpup((__le16 *) &buf);
	}

exit:
	mutex_unlock(&data->icp_lock);
	return rc;
}

static int icp101xx_get_measurement(struct icp101xx_dev *data, long mask,
		int *val)
{
	int rc;
	uint8_t buf[9];
	struct i2c_client *client = data->client;

	mutex_lock(&data->icp_lock);

	rc = icp101xx_i2c_send(client, data->current_meas_mode, CMD_MSG_SIZE);
	if (rc) {
		dev_err(&client->dev, "Send failed current_meas_mode, %d\n",
				rc);
		goto exit;
	}

	msleep_interruptible(100);
	rc = icp101xx_i2c_recv(client, buf, sizeof(buf));
	if (rc) {
		dev_err(&client->dev, "Recv failed get_measurement, %d\n", rc);
		goto exit;
	}

	msleep_interruptible(100);
	if (mask == IIO_PRESSURE)
		*val = be32_to_cpu((buf[0] << 8) |
				   (buf[1] << 16) |
				    buf[3] << 24);
	else if (mask == IIO_TEMP)
		*val = be32_to_cpu(buf[6] << 16 | buf[7] << 24);

exit:
	mutex_unlock(&data->icp_lock);
	return rc;
}

static int icp101xx_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	struct icp101xx_dev *data = iio_priv(indio_dev);
	int rc = -EINVAL;

	if (mask == IIO_CHAN_INFO_RAW)
		rc = icp101xx_get_measurement(data, chan->type, val);

	return rc == 0 ? IIO_VAL_INT : rc;
}

static ssize_t icp101xx_cur_meas_mode(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct icp101xx_dev *data = iio_priv(dev_to_iio_dev(dev));

	return sprintf(buf, "%s\n", meas_modes[data->meas_mode]);
}

static ssize_t icp101xx_show_meas_modes(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s %s %s\n", meas_modes[NORMAL],
			meas_modes[LOW_POWER],
			meas_modes[LOW_NOISE],
			meas_modes[ULTRA_LOW_NOISE]);
}

static ssize_t icp101xx_store_meas_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct icp101xx_dev *data = iio_priv(dev_to_iio_dev(dev));
	int i = 0;

	mutex_lock(&data->icp_lock);
	for (i = 0; i < ARRAY_SIZE(meas_modes); i++) {
		if (strncmp(buf, meas_modes[i], strlen(meas_modes[i])) == 0) {
			data->meas_mode = i;
			data->current_meas_mode = device_mode[data->meas_mode];
			dev_dbg(dev, "Measurement mode set to %s\n",
				meas_modes[data->meas_mode]);
			break;
		}
	}
	mutex_unlock(&data->icp_lock);
	return count;
}

static ssize_t icp101xx_device_reset(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct icp101xx_dev *data = iio_priv(dev_to_iio_dev(dev));

	if (*buf == '0')
		return -EINVAL;

	if (*buf == '1')
		icp101xx_soft_reset_device(data);
	else
		dev_err(dev, "Inappropriate value to reset device\n");
	return count;
}

static ssize_t icp101xx_show_constants(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct icp101xx_dev *data = iio_priv(dev_to_iio_dev(dev));
	int ret;

	ret = icp101xx_get_calibration_data(data);
	if (ret) {
		dev_err(dev, "Failed to update sensor_const, ret %d\n", ret);
		return ret;
	}

	dev_dbg(dev, "sensor_const updated\n");

	return sprintf(buf, "%d %d %d %d\n", data->sensor_const[0],
					     data->sensor_const[1],
					     data->sensor_const[2],
					     data->sensor_const[3]);
}

static ssize_t icp101xx_show_device_id(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct icp101xx_dev *data = iio_priv(dev_to_iio_dev(dev));

	return sprintf(buf, "0x%02x\n", data->dev_id);
}

static IIO_DEVICE_ATTR(cur_meas_mode, 0444, icp101xx_cur_meas_mode, NULL, 0);
static IIO_DEVICE_ATTR(get_meas_modes, 0444, icp101xx_show_meas_modes, NULL, 0);
static IIO_DEVICE_ATTR(set_meas_mode, 0644, NULL, icp101xx_store_meas_mode, 0);
static IIO_DEVICE_ATTR(reset, 0644, NULL, icp101xx_device_reset, 0);
static IIO_DEVICE_ATTR(sensor_const, 0444, icp101xx_show_constants, NULL, 0);
static IIO_DEVICE_ATTR(device_id, 0444, icp101xx_show_device_id, NULL, 0);

#define ICP101xx_DEV_ATTR(name) (&iio_dev_attr_##name.dev_attr.attr)

static struct attribute *icp101xx_attributes[] = {
	ICP101xx_DEV_ATTR(get_meas_modes),
	ICP101xx_DEV_ATTR(set_meas_mode),
	ICP101xx_DEV_ATTR(cur_meas_mode),
	ICP101xx_DEV_ATTR(reset),
	ICP101xx_DEV_ATTR(sensor_const),
	ICP101xx_DEV_ATTR(device_id),
	NULL,
};

static const struct attribute_group icp101xx_attribute_group = {
	.attrs = icp101xx_attributes,
};

static const struct iio_info icp101xx_info = {
	.read_raw = &icp101xx_read_raw,
	.attrs = &icp101xx_attribute_group,
};

static const struct iio_chan_spec icp101xx_channels[] = {
	{
		.type = IIO_PRESSURE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},
};

static int icp101xx_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct icp101xx_dev *icp_dev;
	struct iio_dev *indio_dev;
	int ret;

	dev_info(&client->dev, "%s: enter\n", __func__);

	if (!i2c_check_functionality(client->adapter,
					 I2C_FUNC_SMBUS_READ_WORD_DATA |
					 I2C_FUNC_SMBUS_WRITE_BYTE |
					 I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		dev_err(&client->dev,
			"Adapter does not support i2c_check_functionality\n");
		return -EOPNOTSUPP;
	}

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*icp_dev));
	if (!indio_dev)
		return -ENOMEM;

	icp_dev = iio_priv(indio_dev);
	icp_dev->client = client;
	icp_dev->meas_mode = NORMAL;
	icp_dev->current_meas_mode = device_mode[NORMAL];

	indio_dev->name = id->name;
	indio_dev->dev.parent = &client->dev;
	indio_dev->info = &icp101xx_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = icp101xx_channels;
	indio_dev->num_channels = ARRAY_SIZE(icp101xx_channels);

	i2c_set_clientdata(client, indio_dev);

	if (icp101xx_soft_reset_device(icp_dev) != 0)
		dev_err(&client->dev, "%s Reset Failed\n", DEVICE_NAME);
	else
		dev_dbg(&client->dev, "%s Reset Successful\n", DEVICE_NAME);

	if (icp101xx_get_device_id(icp_dev) != 0)
		dev_err(&client->dev, "%s failed\n", DEVICE_NAME);
	else
		dev_dbg(&client->dev, "%s successful\n", DEVICE_NAME);

	mutex_init(&icp_dev->icp_lock);
	ret = devm_iio_device_register(&client->dev, indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "Could not register IIO device\n");
		return ret;
	}

	dev_info(&client->dev, "%s: Success\n", __func__);
	return 0;
}

static const struct i2c_device_id icp101xx_i2c_id[] = {
	{ "icp10101", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, icp101xx_i2c_id);

static const struct of_device_id icp101xx_of_match[] = {
	{ .compatible = "invensense,icp10101", .data = "icp10101" },
	{ },
};
MODULE_DEVICE_TABLE(of, icp101xx_of_match);

static struct acpi_device_id icp101xx_acpi_match[] = {
	{ "ICP10101", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, icp101xx_acpi_match);

static struct i2c_driver icp101xx_driver = {
	.probe = icp101xx_probe,
	.id_table = icp101xx_i2c_id,
	.driver = {
		.name = DEVICE_NAME,
		.of_match_table = icp101xx_of_match,
		.acpi_match_table = ACPI_PTR(icp101xx_acpi_match),
		},
};

module_i2c_driver(icp101xx_driver);

MODULE_DESCRIPTION("TDK/Invensense ICP101xx pressure & temperature driver");
MODULE_AUTHOR("Priyank Rathod <prathod@magicleap.com>");
MODULE_LICENSE("GPL v2");

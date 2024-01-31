// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
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
#include <linux/hwmon.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/hwmon-sysfs.h>
#include "ml_pwr_mon.h"

#define PWR_MON_CHAN_NAME		"POWER_MON_CHAN"
#define PWR_MON_SUSPEND_TIMEOUT		(500)
#define PWR_MON_RESUME_TIMEOUT		(500)
#define PWR_MON_SUSPEND_RESUME_DATA	(1)
#define PWR_MON_FILE_MODE_READ_ONLY	(0444)
#define PWR_MON_FILE_MODE_READ_WRITE	(0644)
#define PWR_MON_READ_TIMEOUT 		(2000)
#define PWR_MON_MAX_NUM_SAMPLES		(1000)

/* See https://www.kernel.org/doc/html/latest/hwmon/hwmon-kernel-api.html
 * The three hwmon sensors being used are:
 * hwmon_in: Voltage sensor
 * hwmon_curr: Current sensor
 * hwmon_power: Power sensor
 */

static int pwr_mon_send_msg(struct pwr_mon_dev_data *pmdata,
			    struct pwr_mon_tx_msg *tx_msg,
			    uint32_t len)
{
	int ret;

	if (!pmdata->ch_open) {
		dev_err(pmdata->dev, "channel not open\n");
		return -ENOTCONN;
	}

	ret = ml_mux_send_msg(pmdata->client.ch, len, tx_msg);
	if (ret)
		dev_err(pmdata->dev, "send err=%d\n", ret);

	return ret;
}

static int pwr_mon_read_in(struct pwr_mon *pm, u32 attr,
			   int channel, long *val)
{
	struct pwr_mon_tx_msg tx_msg = {.type = PWR_MON_TX_UPDATE};
	unsigned long jiffies_left;
	int t = msecs_to_jiffies(PWR_MON_READ_TIMEOUT);
	int ret;

	switch (attr) {
	case hwmon_in_input:
		tx_msg.u.msg.id = pm->id;
		tx_msg.u.msg.data = channel;
		tx_msg.u.msg.chan_type = PWR_MON_TYPE_VOLT;
		ret = pwr_mon_send_msg(pm->pmdata, &tx_msg,
				(uint32_t)PWR_MON_TX_MSG_SIZE(msg));
		if (ret)
			return -ENODATA;

		reinit_completion(&pm->pmdata->read);
		jiffies_left = wait_for_completion_timeout(&pm->pmdata->read, t);
		if (!jiffies_left)
			return -ETIMEDOUT;

		if (pm->inputs[channel].disconnected ||
				!pm->inputs[channel].valid)
			return -ENODATA;

		*val = pm->inputs[channel].volt;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int pwr_mon_read_curr(struct pwr_mon *pm, u32 attr,
			     int channel, long *val)
{
	struct pwr_mon_tx_msg tx_msg = {.type = PWR_MON_TX_UPDATE};
	unsigned long jiffies_left;
	int t = msecs_to_jiffies(PWR_MON_READ_TIMEOUT);
	int ret;

	switch (attr) {
	case hwmon_curr_input:
		tx_msg.u.msg.id = pm->id;
		tx_msg.u.msg.data = channel;
		tx_msg.u.msg.chan_type = PWR_MON_TYPE_CURR;
		ret = pwr_mon_send_msg(pm->pmdata, &tx_msg,
				(uint32_t)PWR_MON_TX_MSG_SIZE(msg));
		if (ret)
			return -ENODATA;

		reinit_completion(&pm->pmdata->read);
		jiffies_left = wait_for_completion_timeout(&pm->pmdata->read, t);
		if (!jiffies_left)
			return -ETIMEDOUT;

		if (pm->inputs[channel].disconnected ||
				!pm->inputs[channel].valid)
			return -ENODATA;

		*val = pm->inputs[channel].curr;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int pwr_mon_read_power(struct pwr_mon *pm, u32 attr,
			      int channel, long *val)
{
	struct pwr_mon_tx_msg tx_msg = {.type = PWR_MON_TX_UPDATE};
	unsigned long jiffies_left;
	int t = msecs_to_jiffies(PWR_MON_READ_TIMEOUT);
	int ret;

	switch (attr) {
	case hwmon_power_average:
		tx_msg.type = PWR_MON_TX_GET_AVG_PWR;
		/* Fall through */
	case hwmon_power_input:
		tx_msg.u.msg.id = pm->id;
		tx_msg.u.msg.data = channel;
		tx_msg.u.msg.chan_type = PWR_MON_TYPE_PWR;
		ret = pwr_mon_send_msg(pm->pmdata, &tx_msg,
				(uint32_t)PWR_MON_TX_MSG_SIZE(msg));
		if (ret)
			return -ENODATA;

		reinit_completion(&pm->pmdata->read);
		jiffies_left = wait_for_completion_timeout(&pm->pmdata->read, t);
		if (!jiffies_left)
			return -ETIMEDOUT;

		if (pm->inputs[channel].disconnected ||
				!pm->inputs[channel].valid)
			return -ENODATA;

		*val = pm->inputs[channel].power * 1000;
		break;
	case hwmon_power_crit:
		*val = pm->pwr_overlimit * 1000;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int pwr_mon_read(struct device *dev, enum hwmon_sensor_types type,
			u32 attr, int channel, long *val)
{
	int ret;
	struct pwr_mon *pm = dev_get_drvdata(dev);

	switch (type) {
	case hwmon_in:
		/* 0-align channel ID */
		ret = pwr_mon_read_in(pm, attr, channel - 1, val);
		break;
	case hwmon_curr:
		ret = pwr_mon_read_curr(pm, attr, channel, val);
		break;
	case hwmon_power:
		ret = pwr_mon_read_power(pm, attr, channel, val);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

static int pwr_mon_write_power(struct pwr_mon *pm, u32 attr,
			      int channel, long val)
{
	struct pwr_mon_tx_msg tx_msg;
	unsigned long jiffies;
	int t = msecs_to_jiffies(PWR_MON_READ_TIMEOUT);
	int ret;

	switch (attr) {
	case hwmon_power_crit:
		tx_msg.type = PWR_MON_TX_SET_PWR_LMT;
		tx_msg.u.msg.id = pm->id;
		tx_msg.u.msg.data = val;
		ret = pwr_mon_send_msg(pm->pmdata, &tx_msg,
				(uint32_t)PWR_MON_TX_MSG_SIZE(msg));
		if (ret)
			return -ENODATA;

		reinit_completion(&pm->pmdata->read);
		jiffies = wait_for_completion_timeout(&pm->pmdata->read, t);
		if (!jiffies)
			return -ETIMEDOUT;
		pm->pwr_overlimit = val;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int pwr_mon_write(struct device *dev, enum hwmon_sensor_types type,
			u32 attr, int channel, long val)
{
	int ret;
	struct pwr_mon *pm = dev_get_drvdata(dev);

	switch (type) {
	case hwmon_power:
		ret = pwr_mon_write_power(pm, attr, channel, val);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

static int pwr_mon_read_string(struct device *dev,
			       enum hwmon_sensor_types type, u32 attr,
			       int channel, const char **str)
{
	struct pwr_mon *pm = dev_get_drvdata(dev);
	int index;

	index = (type == hwmon_in) ? channel - 1 : channel;

	*str = pm->inputs[index].label;

	return 0;
}

static ssize_t pwr_mon_avg_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct pwr_mon *pm = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", pm->num_avg);
}

static ssize_t pwr_mon_avg_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf,
				 size_t count)
{
	struct pwr_mon *pm = dev_get_drvdata(dev);
	struct pwr_mon_tx_msg tx_msg = {.type = PWR_MON_TX_SET_AVG};
	unsigned long jiffies_left;
	int t = msecs_to_jiffies(PWR_MON_READ_TIMEOUT);
	u32 value;
	int ret;

	ret = kstrtou32(buf, 10, &value);
	if (ret)
		return ret;

	if (!value)
		return -EINVAL;

	if (value > PWR_MON_MAX_NUM_SAMPLES)
		value = PWR_MON_MAX_NUM_SAMPLES;

	tx_msg.u.msg.id = pm->id;
	tx_msg.u.msg.data = value;
	ret = pwr_mon_send_msg(pm->pmdata, &tx_msg,
				(uint32_t)PWR_MON_TX_MSG_SIZE(msg));
	if (ret)
		return -ENODATA;

	reinit_completion(&pm->pmdata->read);
	jiffies_left = wait_for_completion_timeout(&pm->pmdata->read, t);
	if (!jiffies_left)
		return -ETIMEDOUT;

	pm->num_avg = value;
	return count;
}

static SENSOR_DEVICE_ATTR_2_RW(num_avg, pwr_mon_avg, 0, 0);

static ssize_t pwr_mon_boot_cur_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct pwr_mon *pm = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", pm->boot_current);
}

static SENSOR_DEVICE_ATTR_2_RO(boot_current, pwr_mon_boot_cur, 0, 0);

static umode_t pwr_mon_attributes_visible(struct kobject *kobj,
					  struct attribute *attr, int index)
{
	struct device *dev = kobj_to_dev(kobj);
	const struct pwr_mon *pm = dev_get_drvdata(dev);

	if (attr == &sensor_dev_attr_num_avg.dev_attr.attr)
		return (pm->num_avg) ? PWR_MON_FILE_MODE_READ_WRITE : 0;

	if (attr == &sensor_dev_attr_boot_current.dev_attr.attr)
		return (pm->boot_current) ? PWR_MON_FILE_MODE_READ_ONLY : 0;

	return 0;
}

static struct attribute *pwr_mon_attributes[] = {
	&sensor_dev_attr_num_avg.dev_attr.attr,
	&sensor_dev_attr_boot_current.dev_attr.attr,
	NULL
};

static const struct attribute_group pwr_mon_attrgroup = {
	.attrs = pwr_mon_attributes,
	.is_visible = pwr_mon_attributes_visible,
};

static const struct attribute_group *hwmon_groups[] = {
	&pwr_mon_attrgroup,
	NULL
};

static umode_t pwr_mon_is_visible(const void *data,
				  enum hwmon_sensor_types type,
				  u32 attr, int channel)
{
	const struct pwr_mon *pm = (struct pwr_mon *)data;
	const struct pwr_mon_input *input = NULL;
	int on = 1;
	int mode = PWR_MON_FILE_MODE_READ_ONLY;

	switch (type) {
	case hwmon_in:
		/* Ignore in0_ so sysfs entries start with 1 as index */
		if (channel == 0)
			return 0;

		switch (attr) {
		case hwmon_in_label:
		case hwmon_in_input:
			if (channel - 1 < PWR_MON_NUM_CHANNELS)
				input = &pm->inputs[channel - 1];
			/* Hide label node if label is not provided */
			return (input && !input->disconnected) ?
				PWR_MON_FILE_MODE_READ_ONLY : 0;
		default:
			return 0;
		}
	case hwmon_curr:
		switch (attr) {
		case hwmon_curr_label:
		case hwmon_curr_input:
			if (channel < PWR_MON_NUM_CHANNELS)
				input = &pm->inputs[channel];
			/* Hide label node if label is not provided */
			return (input && !input->disconnected) ?
				PWR_MON_FILE_MODE_READ_ONLY : 0;
		default:
			return 0;
		}

	case hwmon_power:
		switch (attr) {
		case hwmon_power_average:
		case hwmon_power_crit:
			if (attr == hwmon_power_average) {
				on = !!pm->num_avg;
				mode = PWR_MON_FILE_MODE_READ_ONLY;
			} else {
				on = !!pm->pwr_overlimit;
				mode = PWR_MON_FILE_MODE_READ_WRITE;
			}
			/* Fall through */
		case hwmon_power_label:
		case hwmon_power_input:
			if (channel < PWR_MON_NUM_CHANNELS)
				input = &pm->inputs[channel];
			/* Hide label node if label is not provided */
			return (input && !input->disconnected && on) ?
				mode : 0;
		default:
			return 0;
		}
	default:
		return 0;
	}
}

static const struct hwmon_channel_info *pwr_mon_info[] = {
	HWMON_CHANNEL_INFO(in,
			   /* 0: dummy, skipped in is_visible so that
			    * voltage attributes will start with 1 so
			    * will be same as current and power.
			    */
			   HWMON_I_INPUT,
			   /* 1-4: input voltage Channels */
			   HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL),
	HWMON_CHANNEL_INFO(curr,
			   HWMON_C_INPUT | HWMON_C_LABEL,
			   HWMON_C_INPUT | HWMON_C_LABEL,
			   HWMON_C_INPUT | HWMON_C_LABEL,
			   HWMON_C_INPUT | HWMON_C_LABEL),
	HWMON_CHANNEL_INFO(power,
			   HWMON_P_INPUT | HWMON_P_LABEL |
			   HWMON_P_AVERAGE | HWMON_P_CRIT,
			   HWMON_P_INPUT | HWMON_P_LABEL |
			   HWMON_P_AVERAGE | HWMON_P_CRIT,
			   HWMON_P_INPUT | HWMON_P_LABEL |
			   HWMON_P_AVERAGE | HWMON_P_CRIT,
			   HWMON_P_INPUT | HWMON_P_LABEL |
			   HWMON_P_AVERAGE | HWMON_P_CRIT),
	NULL
};

static const struct hwmon_ops pwr_mon_hwmon_ops = {
	.is_visible = pwr_mon_is_visible,
	.read_string = pwr_mon_read_string,
	.read = pwr_mon_read,
	.write = pwr_mon_write,
};

static const struct hwmon_chip_info pwr_mon_chip_info = {
	.ops = &pwr_mon_hwmon_ops,
	.info = pwr_mon_info,
};

static inline void pwr_mon_destroy(struct pwr_mon_dev_data *pmdata,
				   struct pwr_mon *pm)
{
	hwmon_device_unregister(pm->hwmon_dev);
	idr_remove(&pmdata->pm_idr, pm->id);
	kfree(pm);
}

static void pwr_mon_unregister_all(struct pwr_mon_dev_data *pmdata)
{
	struct pwr_mon *pm;
	int i;

	mutex_lock(&pmdata->lock);
	if (!idr_is_empty(&pmdata->pm_idr))
		idr_for_each_entry(&pmdata->pm_idr, pm, i)
			pwr_mon_destroy(pmdata, pm);
	mutex_unlock(&pmdata->lock);
}

static void pwr_mon_reg_work(struct work_struct *work)
{
	struct pwr_mon_work *pm_work;
	struct pwr_mon_dev_data *pmdata;
	struct pwr_mon_rx_reg *reg_info;
	struct pwr_mon *pm;
	struct pwr_mon_tx_msg tx_msg;
	int ret;
	int i;

	pm_work = container_of(work, struct pwr_mon_work, work);
	pmdata = pm_work->pmdata;
	reg_info = (struct pwr_mon_rx_reg *)pm_work->data;

	mutex_lock(&pmdata->lock);
	/* fill in id in tx reg ack later, depending on pass or fail */
	tx_msg.type = PWR_MON_TX_REG_ACK;
	strncpy(tx_msg.u.msg.name, reg_info->pm_name, PWR_MON_NAME_SIZE);

	pm = kzalloc(sizeof(*pm), GFP_KERNEL);
	if (!pm)
		goto error;

	strncpy(pm->name, reg_info->pm_name, PWR_MON_NAME_SIZE);

	ret = idr_alloc(&pmdata->pm_idr, pm, 0, 32, GFP_KERNEL);
	if (ret < 0) {
		dev_err(pmdata->dev, "fail: idr_alloc, %d\n", ret);
		kfree(pm);
		goto error;
	}

	pm->id = ret;
	pm->num_avg = reg_info->num_avg;
	pm->pwr_overlimit = reg_info->pwr_overlimit;
	pm->boot_current = reg_info->boot_current;
	tx_msg.u.msg.id = ret;
	dev_info(pmdata->dev, "id is %d", ret);
	ret = pwr_mon_send_msg(pmdata, &tx_msg,
			       (uint32_t)PWR_MON_TX_MSG_SIZE(msg));
	if (ret) {
		dev_err(pmdata->dev, "fail to send reg ack\n");
		idr_remove(&pmdata->pm_idr, pm->id);
		kfree(pm);
		goto unlock;
	}
	for (i = 0; i < PWR_MON_NUM_CHANNELS; i++) {
		if (reg_info->chan_name[i][0] != '\0') {
			strncpy(pm->inputs[i].label,
				reg_info->chan_name[i],
				PWR_MON_CHAN_NAME_SIZE);
			pm->inputs[i].disconnected = false;
		} else {
			pm->inputs[i].disconnected = true;
		}
	}

	pm->hwmon_dev = hwmon_device_register_with_info(pmdata->dev,
			pm->name, pm, &pwr_mon_chip_info, hwmon_groups);

	if (IS_ERR(pm->hwmon_dev)) {
		dev_err(pmdata->dev, "reg power mon failed=%ld\n",
			PTR_ERR(pm->hwmon_dev));
		kfree(pm);
		goto error;
	}

	pm->pmdata = pmdata;
	goto unlock;

error:
	tx_msg.u.msg.id = -1;
	ret = pwr_mon_send_msg(pmdata, &tx_msg,
			       (uint32_t)PWR_MON_TX_MSG_SIZE(msg));
	if (ret)
		dev_err(pmdata->dev, "fail to send reg ack id -1\n");
unlock:
	mutex_unlock(&pmdata->lock);
	devm_kfree(pmdata->dev, pm_work->data);
	devm_kfree(pmdata->dev, pm_work);
}

static void pwr_mon_update_work(struct work_struct *work)
{
	struct pwr_mon_work *pm_work;
	struct pwr_mon_dev_data *pmdata;
	struct pwr_mon_rx_update *rx_update;
	struct pwr_mon *pm;
	int i;

	pm_work = container_of(work, struct pwr_mon_work, work);
	pmdata = pm_work->pmdata;
	rx_update = (struct pwr_mon_rx_update *)pm_work->data;

	mutex_lock(&pmdata->lock);
	pm = (struct pwr_mon *)idr_find(&pmdata->pm_idr, rx_update->id);
	if (!pm) {
		dev_err(pmdata->dev, "Can't find device %d\n",
			rx_update->id);
		goto unlock;
	}

	i = rx_update->channel;
	if (pm->inputs[i].disconnected)
		goto unlock;

	pm->inputs[i].valid = rx_update->valid;
	if (!rx_update->valid)
		goto unlock;

	dev_dbg(pmdata->dev, "id=%d ch=%d chan_type=%d data=%d\n",
			rx_update->id,
			i,
			rx_update->chan_type,
			rx_update->data);
	switch (rx_update->chan_type) {
	case PWR_MON_TYPE_VOLT:
		pm->inputs[i].volt = rx_update->data;
		break;
	case PWR_MON_TYPE_CURR:
		pm->inputs[i].curr = rx_update->data;
		break;
	case PWR_MON_TYPE_PWR:
		pm->inputs[i].power = rx_update->data;
		break;
	default:
		dev_warn(pmdata->dev, "Invalid chan_type %d\n",
			rx_update->chan_type);
	}

unlock:
	complete(&pmdata->read);
	mutex_unlock(&pmdata->lock);
	devm_kfree(pmdata->dev, pm_work->data);
	devm_kfree(pmdata->dev, pm_work);
}

static int pwr_mon_queue_msg(struct pwr_mon_dev_data *pmdata,
			     const void *data, size_t size,
			     void (*work_func)(struct work_struct *))
{
	int ret = 0;
	struct pwr_mon_work *pm_work;

	if (!size || !data) {
		dev_err(pmdata->dev, "queue data is corrupted\n");
		ret = -EINVAL;
		goto exit;
	}

	pm_work = devm_kzalloc(pmdata->dev, sizeof(*pm_work), GFP_KERNEL);
	if (!pm_work) {
		ret = -ENOMEM;
		goto exit;
	}

	pm_work->data = devm_kzalloc(pmdata->dev, size, GFP_KERNEL);
	if (!pm_work->data) {
		ret = -ENOMEM;
		devm_kfree(pmdata->dev, pm_work);
		goto exit;
	}
	memcpy(pm_work->data, data, size);
	pm_work->pmdata = pmdata;

	INIT_WORK(&pm_work->work, work_func);
	queue_work(pmdata->work_q, &pm_work->work);

exit:
	return ret;
}

static inline int mlmux_size_check(struct pwr_mon_dev_data *pmdata,
				   uint32_t len, size_t exp)
{
	if (len != exp) {
		dev_err(pmdata->dev, "Unexpected length %d vs %zu\n", len, exp);
		return -EMSGSIZE;
	}
	return 0;
}

static void pwr_mon_recv(struct ml_mux_client *cli, uint32_t len, void *msg)
{
	struct pwr_mon_dev_data *pmdata;
	struct pwr_mon_rx_msg *rx_msg = (struct pwr_mon_rx_msg *)msg;
	int ret;
	struct pwr_mon *pm;

	pmdata = container_of(cli, struct pwr_mon_dev_data, client);

	switch (rx_msg->type) {
	case PWR_MON_RX_UPDATE:
		if (mlmux_size_check(pmdata, len, PWR_MON_RX_SIZE(update)))
			return;

		ret = pwr_mon_queue_msg(pmdata, &rx_msg->u.update, len - 1,
					pwr_mon_update_work);
		if (ret)
			dev_err(pmdata->dev,
				"fail queue update work, err=%d\n", ret);
		break;
	case PWR_MON_RX_REG:
		if (mlmux_size_check(pmdata, len, PWR_MON_RX_SIZE(reg)))
			return;

		ret = pwr_mon_queue_msg(pmdata, &rx_msg->u.reg, len - 1,
					pwr_mon_reg_work);
		if (ret)
			dev_err(pmdata->dev,
				"fail queue reg work, err=%d\n", ret);
		break;
	case PWR_MON_RX_SUSPEND_ACK:
		if (mlmux_size_check(pmdata, len, PWR_MON_RX_SIZE(data)))
			return;

		if (rx_msg->u.data)
			dev_err(pmdata->dev, "suspend failed ret = %d",
				rx_msg->u.data);

		/* complete on pass or fail */
		complete(&pmdata->suspend);
		break;
	case PWR_MON_RX_RESUME_ACK:
		if (mlmux_size_check(pmdata, len, PWR_MON_RX_SIZE(data)))
			return;

		if (rx_msg->u.data)
			dev_err(pmdata->dev, "resume failed ret = %d",
				rx_msg->u.data);

		/* complete on pass or fail */
		complete(&pmdata->resume);
		break;
	case PWR_MON_RX_SET_AVG_ACK:
		if (mlmux_size_check(pmdata, len, PWR_MON_RX_SIZE(avg_data)))
			return;

		if (rx_msg->u.avg_data.data)
			dev_err(pmdata->dev, "set avg failed ret = %d",
				rx_msg->u.avg_data.data);

		/* complete on pass or fail */
		complete(&pmdata->read);
		break;
	case PWR_MON_RX_GET_AVG_PWR_ACK:
		if (mlmux_size_check(pmdata, len, PWR_MON_RX_SIZE(avg_data)))
			return;

		pm = (struct pwr_mon *)idr_find(&pmdata->pm_idr,
			rx_msg->u.avg_data.id);
		if (!pm) {
			dev_err(pmdata->dev, "Can't find device %d\n",
				rx_msg->u.avg_data.id);
			return;
		}
		pm->inputs[rx_msg->u.avg_data.channel].power =
			rx_msg->u.avg_data.data;
		pm->inputs[rx_msg->u.avg_data.channel].valid = 1;

		/* complete on pass or fail */
		complete(&pmdata->read);
		break;
	case PWR_MON_RX_SET_PWR_LMT_ACK:
		if (mlmux_size_check(pmdata, len, PWR_MON_RX_SIZE(data)))
			return;

		if (rx_msg->u.data)
			dev_err(pmdata->dev, "set pwr limit failed ret = %d",
				rx_msg->u.data);
		else
			/* complete on pass */
			complete(&pmdata->read);
		break;
	default:
		dev_err(pmdata->dev, "Unknown cmd type %u\n", rx_msg->type);
		return;
	}
}

static void pwr_mon_open_work(struct work_struct *work)
{
	struct pwr_mon_dev_data *pmdata;
	struct pwr_mon_work *pm_work;
	bool chan_open;

	pm_work = container_of(work, struct pwr_mon_work, work);
	pmdata = pm_work->pmdata;
	chan_open = *(bool *)pm_work->data;

	if (pmdata->ch_open == chan_open) {
		dev_warn(pmdata->dev, "power mon chan already %s\n",
			 chan_open ? "opened" : "closed");
		goto exit;
	}
	pmdata->ch_open = chan_open;

	dev_info(pmdata->dev, "power mon chan is %s\n",
		 pmdata->ch_open ? "opened" : "closed");

	if (!pmdata->ch_open)
		pwr_mon_unregister_all(pmdata);

exit:
	devm_kfree(pmdata->dev, pm_work->data);
	devm_kfree(pmdata->dev, pm_work);
}

static void pwr_mon_open(struct ml_mux_client *cli, bool is_open)
{
	struct pwr_mon_dev_data *pmdata;
	int ret;

	pmdata = container_of(cli, struct pwr_mon_dev_data, client);

	ret = pwr_mon_queue_msg(pmdata, &is_open, sizeof(is_open),
				pwr_mon_open_work);
	if (ret)
		dev_err(pmdata->dev, "fail queue open work, err=%d\n", ret);
}

#ifdef CONFIG_OF
static int pwr_mon_parse_dt(struct pwr_mon_dev_data *pmdata)
{
	struct device_node *np = pmdata->dev->of_node;

	if (of_property_read_string(np, "ml,chan-name", &pmdata->chan_name)) {
		dev_err(pmdata->dev, "ml,chan-name undefined\n");
		return -EINVAL;
	}

	return 0;
}
#else
static inline int pwr_mon_parse_dt(struct pwr_mon_dev_data *pmdata)
{
	return -ENODEV;
}
#endif

#ifdef CONFIG_ACPI
static int pwr_mon_process_acpi(struct pwr_mon_dev_data *pmdata)
{
	pmdata->chan_name = PWR_MON_CHAN_NAME;

	return 0;
}
#else
static inline int pwr_mon_process_acpi(struct pwr_mon_dev_data *pmdata)
{
	return -ENODEV;
}
#endif

static int pwr_mon_get_config(struct pwr_mon_dev_data *pmdata)
{
	if (pmdata->dev->of_node)
		return pwr_mon_parse_dt(pmdata);
	else
		return pwr_mon_process_acpi(pmdata);
}

static int pwr_mon_probe(struct platform_device *pdev)
{
	struct pwr_mon_dev_data *pmdata = NULL;
	int ret = 0;
	int status = 0;
	bool ch_open = true;

	dev_info(&pdev->dev, "Enter\n");
	pmdata = devm_kzalloc(&pdev->dev, sizeof(*pmdata), GFP_KERNEL);
	if (!pmdata) {
		ret = -ENOMEM;
		goto exit;
	}
	pmdata->dev = &pdev->dev;
	platform_set_drvdata(pdev, pmdata);

	ret = pwr_mon_get_config(pmdata);
	if (ret)
		goto exit;

	pmdata->client.dev = pmdata->dev;
	pmdata->client.notify_open = pwr_mon_open;
	pmdata->client.receive_cb = pwr_mon_recv;

	init_completion(&pmdata->suspend);
	init_completion(&pmdata->resume);
	init_completion(&pmdata->read);
	mutex_init(&pmdata->lock);
	idr_init(&pmdata->pm_idr);

	pmdata->work_q = alloc_workqueue(pmdata->chan_name, WQ_UNBOUND, 1);
	if (!pmdata->work_q) {
		dev_err(pmdata->dev, "Failed to create workqueue\n");
		ret = -ENOMEM;
		goto exit_idr_destroy;
	}

	status = ml_mux_request_channel(&pmdata->client,
					(char *)pmdata->chan_name);
	/* If ML_MUX_CH_REQ_SUCCESS, will not queue msg since remote side
	 * is not open yet
	 */
	if (status == ML_MUX_CH_REQ_OPENED) {
		ret = pwr_mon_queue_msg(pmdata, &ch_open, sizeof(ch_open),
					pwr_mon_open_work);
		if (ret) {
			dev_err(pmdata->dev, "fail queue open msg err=%d\n",
				ret);
			goto exit_wq_destroy;
		}
	} else if (status < 0) {
		ret = status;
		goto exit_wq_destroy;
	}

	dev_info(pmdata->dev, "Success!\n");
	return 0;

exit_wq_destroy:
	destroy_workqueue(pmdata->work_q);
exit_idr_destroy:
	idr_destroy(&pmdata->pm_idr);
	mutex_destroy(&pmdata->lock);
exit:
	return ret;
}

int pwr_mon_remove(struct platform_device *pdev)
{
	struct pwr_mon_dev_data *pmdata = platform_get_drvdata(pdev);

	ml_mux_release_channel(pmdata->client.ch);
	destroy_workqueue(pmdata->work_q);
	pwr_mon_unregister_all(pmdata);
	idr_destroy(&pmdata->pm_idr);
	mutex_destroy(&pmdata->lock);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pwr_mon_suspend(struct device *dev)
{
	int ret;
	unsigned long jiffies_left;
	struct pwr_mon_dev_data *pmdata = dev_get_drvdata(dev);
	struct pwr_mon_tx_msg tx_msg;
	int t = msecs_to_jiffies(PWR_MON_SUSPEND_TIMEOUT);

	reinit_completion(&pmdata->suspend);
	tx_msg.type = PWR_MON_TX_SUSPEND;
	tx_msg.u.data = PWR_MON_SUSPEND_RESUME_DATA;
	ret = pwr_mon_send_msg(pmdata, &tx_msg,
			       (uint32_t)PWR_MON_TX_MSG_SIZE(data));
	if (ret)
		return ret;

	jiffies_left = wait_for_completion_timeout(&pmdata->suspend, t);
	if (!jiffies_left)
		return -ETIMEDOUT;

	dev_dbg(pmdata->dev, "suspend\n");

	return 0;
}

static int pwr_mon_resume(struct device *dev)
{
	int ret;
	unsigned long jiffies_left;
	struct pwr_mon_dev_data *pmdata = dev_get_drvdata(dev);
	struct pwr_mon_tx_msg tx_msg;
	int t = msecs_to_jiffies(PWR_MON_RESUME_TIMEOUT);

	reinit_completion(&pmdata->resume);
	tx_msg.type = PWR_MON_TX_RESUME;
	tx_msg.u.data = PWR_MON_SUSPEND_RESUME_DATA;
	ret = pwr_mon_send_msg(pmdata, &tx_msg,
			       (uint32_t)PWR_MON_TX_MSG_SIZE(data));
	if (ret)
		return ret;

	jiffies_left = wait_for_completion_timeout(&pmdata->resume, t);
	if (!jiffies_left)
		return -ETIMEDOUT;

	dev_dbg(pmdata->dev, "resume\n");

	return 0;
}

static const struct dev_pm_ops pwr_mon_pm = {
	.suspend = pwr_mon_suspend,
	.resume = pwr_mon_resume,
};
#define PWR_MON_PM_OPS	(&pwr_mon_pm)
#else
#define PWR_MON_PM_OPS	NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id ml_pwr_mon_match_table[] = {
	{ .compatible = "ml,ml_pwr_mon", },
	{ },
};
MODULE_DEVICE_TABLE(of, ml_pwr_mon_match_table);
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id ml_pwr_mon_acpi_match[] = {
	{"MXIN1111", 0},
	{ },
};
MODULE_DEVICE_TABLE(acpi, ml_pwr_mon_acpi_match);
#endif

static struct platform_driver ml_pwr_mon_driver = {
	.driver = {
		.name = "ml_pwr_mon",
		.of_match_table = of_match_ptr(ml_pwr_mon_match_table),
		.pm = PWR_MON_PM_OPS,
		.acpi_match_table = ACPI_PTR(ml_pwr_mon_acpi_match),
	},
	.probe = pwr_mon_probe,
	.remove = pwr_mon_remove,
};

module_platform_driver(ml_pwr_mon_driver);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("ML Power Monitor");
MODULE_LICENSE("GPL v2");

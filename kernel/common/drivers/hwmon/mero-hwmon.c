/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 */

#include <linux/module.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/miscdevice.h>
#include <linux/smu_protocol.h>

#define MERO_HWMON_NAME "merohwmon"
enum res_id {
	VOL_CPU0,
	VOL_CPU1,
	VOL_CPU2,
	VOL_CPU3,
	VOL_GFX,
	VOL_SOC,
	VOL_CVIP,
	CUR_CPU,
	CUR_GFX,
	CUR_SOC,
	CUR_CVIP,
	A55CLK,
	A55TWOCLK,
	C5CLK,
	Q6CLK,
	NIC400CLK,
	POWER_CPU,
	POWER_GFX,
	POWER_SOC,
	POWER_CVIP,
	TEM_CPU0,
	TEM_CPU1,
	TEM_CPU2,
	TEM_CPU3,
	TEM_GFX,
	TEM_SOC,
	TEM_CVIP,
	POWER_TIME,
	TEM_TIME,
	MIG_END_HYS,
	RES_NUM,
};

enum value_t {
	CUR_VAL,
	MIN_VAL,
	MAX_VAL,
	AVE_VAL,
	LIMT_VAL,
	TYPE_NUM,
};

struct mero_hwmon_data {
	const struct attribute_group *groups[1];
	struct device *hwmon_dev;
};

static ssize_t voltage_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct smu_msg msg;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	dev_dbg(dev, "%s: %d-%d\n", __func__, sattr->nr, sattr->index);
	if (unlikely(sattr->index != CUR_VAL)) {
		dev_err(dev, "%s: invalid value type\n", __func__);
		return -EIO;
	}

	switch (sattr->nr) {
	case VOL_CPU0:
		smu_msg(&msg, CVSMC_MSG_getcurrentvoltage, IP_CPU0);
		break;
	case VOL_CPU1:
		smu_msg(&msg, CVSMC_MSG_getcurrentvoltage, IP_CPU1);
		break;
	case VOL_CPU2:
		smu_msg(&msg, CVSMC_MSG_getcurrentvoltage, IP_CPU2);
		break;
	case VOL_CPU3:
		smu_msg(&msg, CVSMC_MSG_getcurrentvoltage, IP_CPU3);
		break;
	case VOL_GFX:
		smu_msg(&msg, CVSMC_MSG_getcurrentvoltage, IP_GFX);
		break;
	case VOL_SOC:
		smu_msg(&msg, CVSMC_MSG_getcurrentvoltage, IP_SOC);
		break;
	case VOL_CVIP:
		smu_msg(&msg, CVSMC_MSG_getcurrentvoltage, IP_CVIP);
		break;
	default:
		dev_err(dev, "invalid voltage id\n");
		return -EINVAL;
	}
	if (unlikely(!smu_send_single_msg(&msg))) {
		dev_err(dev, "%s: send smu msg failed\n", __func__);
		return -EBUSY;
	}
	return sprintf(buf, "%u\n", msg.resp);
}

static ssize_t current_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct smu_msg msg;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	dev_dbg(dev, "%s: %d-%d\n", __func__, sattr->nr, sattr->index);
	if (unlikely(sattr->index != CUR_VAL)) {
		dev_err(dev, "%s: invalid value type\n", __func__);
		return -EIO;
	}

	switch (sattr->nr) {
	case CUR_CPU:
		smu_msg(&msg, CVSMC_MSG_getcurrentcurrent, RAIL_CPU);
		break;
	case CUR_GFX:
		smu_msg(&msg, CVSMC_MSG_getcurrentcurrent, RAIL_GFX);
		break;
	case CUR_SOC:
		smu_msg(&msg, CVSMC_MSG_getcurrentcurrent, RAIL_SOC);
		break;
	case CUR_CVIP:
		smu_msg(&msg, CVSMC_MSG_getcurrentcurrent, RAIL_CVIP);
		break;
	default:
		dev_err(dev, "invalid current id\n");
		return -EINVAL;
	}

	if (unlikely(!smu_send_single_msg(&msg))) {
		dev_err(dev, "%s: send smu msg failed\n", __func__);
		return -EBUSY;
	}
	return sprintf(buf, "%u\n", msg.resp);
}

static ssize_t freq_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct smu_msg msg;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	dev_dbg(dev, "%s: %d-%d\n", __func__, sattr->nr, sattr->index);
	if (unlikely(sattr->index != CUR_VAL)) {
		dev_err(dev, "%s: invalid value type\n", __func__);
		return -EIO;
	}

	switch (sattr->nr) {
	case A55CLK:
		smu_msg(&msg, CVSMC_MSG_getcurrentfreq, A55CLK_ID);
		break;
	case A55TWOCLK:
		smu_msg(&msg, CVSMC_MSG_getcurrentfreq, A55TWOCLK_ID);
		break;
	case C5CLK:
		smu_msg(&msg, CVSMC_MSG_getcurrentfreq, C5CLK_ID);
		break;
	case Q6CLK:
		smu_msg(&msg, CVSMC_MSG_getcurrentfreq, Q6CLK_ID);
		break;
	case NIC400CLK:
		smu_msg(&msg, CVSMC_MSG_getcurrentfreq, NIC400CLK_ID);
		break;
	default:
		dev_err(dev, "invalid FRQ id\n");
		return -EINVAL;
	}

	if (unlikely(!smu_send_single_msg(&msg))) {
		dev_err(dev, "%s: send smu msg failed\n", __func__);
		return -EBUSY;
	}
	return sprintf(buf, "%u\n", msg.resp);
}

static ssize_t power_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct smu_msg msg;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	dev_dbg(dev, "%s: %d-%d\n", __func__, sattr->nr, sattr->index);
	if (unlikely(sattr->index != CUR_VAL && sattr->index != AVE_VAL)) {
		dev_err(dev, "%s: invalid value type\n", __func__);
		return -EIO;
	}

	switch (sattr->nr) {
	case POWER_CPU:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrentpower, RAIL_CPU);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragepower, RAIL_CPU);
		break;
	case POWER_GFX:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrentpower, RAIL_GFX);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragepower, RAIL_GFX);
		break;
	case POWER_SOC:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrentpower, RAIL_SOC);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragepower, RAIL_SOC);
		break;
	case POWER_CVIP:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrentpower, RAIL_CVIP);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragepower, RAIL_CVIP);
		break;
	default:
		dev_err(dev, "invalid power id\n");
		return -EINVAL;
	}

	if (unlikely(!smu_send_single_msg(&msg))) {
		dev_err(dev, "%s: send smu msg failed\n", __func__);
		return -EBUSY;
	}
	return sprintf(buf, "%u\n", msg.resp);
}

static ssize_t temp_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct smu_msg msg;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	dev_dbg(dev, "%s: %d-%d\n", __func__, sattr->nr, sattr->index);
	if (unlikely(sattr->index != CUR_VAL && sattr->index != AVE_VAL)) {
		dev_err(dev, "%s: invalid value type\n", __func__);
		return -EIO;
	}
	switch (sattr->nr) {
	case TEM_CPU0:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrenttemperature, IP_CPU0);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragetemperature, IP_CPU0);
		break;
	case TEM_CPU1:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrenttemperature, IP_CPU1);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragetemperature, IP_CPU1);
		break;
	case TEM_CPU2:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrenttemperature, IP_CPU2);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragetemperature, IP_CPU2);
		break;
	case TEM_CPU3:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrenttemperature, IP_CPU3);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragetemperature, IP_CPU3);
		break;
	case TEM_GFX:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrenttemperature, IP_GFX);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragetemperature, IP_GFX);
		break;
	case TEM_SOC:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrenttemperature, IP_SOC);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragetemperature, IP_SOC);
		break;
	case TEM_CVIP:
		if (sattr->index == CUR_VAL)
			smu_msg(&msg, CVSMC_MSG_getcurrenttemperature, IP_CVIP);
		else
			smu_msg(&msg, CVSMC_MSG_getaveragetemperature, IP_CVIP);
		break;
	default:
		dev_err(dev, "invalid temperature id\n");
		return -EINVAL;
	}

	if (unlikely(!smu_send_single_msg(&msg))) {
		dev_err(dev, "%s: send smu msg failed\n", __func__);
		return -EBUSY;
	}
	return sprintf(buf, "%u\n", msg.resp);
}

static ssize_t power_time_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	int ret;
	u32 time;
	struct smu_msg msg;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	dev_dbg(dev, "%s: %d-%d\n", __func__, sattr->nr, sattr->index);
	if (unlikely(sattr->index != AVE_VAL)) {
		dev_err(dev, "%s: invalid value type\n", __func__);
		return -EIO;
	}

	ret = kstrtouint(buf, 0, &time);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: get parameter failed\n", __func__);
		return ret;
	}

	switch (sattr->nr) {
	case POWER_TIME:
		smu_msg(&msg, CVSMC_MSG_setaveragepowertimeconstant, time);
		break;
	default:
		dev_err(dev, "invalid id\n");
		return -EINVAL;
	}

	if (unlikely(!smu_send_single_msg(&msg))) {
		dev_err(dev, "%s: send smu msg failed\n", __func__);
		return -EBUSY;
	}
	return count;
}

static ssize_t temp_time_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	int ret;
	u32 time;
	struct smu_msg msg;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	dev_dbg(dev, "%s: %d-%d\n", __func__, sattr->nr, sattr->index);
	if (unlikely(sattr->index != AVE_VAL)) {
		dev_err(dev, "%s: invalid value type\n", __func__);
		return -EIO;
	}

	ret = kstrtouint(buf, 0, &time);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: get parameter failed\n", __func__);
		return ret;
	}

	switch (sattr->nr) {
	case TEM_TIME:
		smu_msg(&msg, CVSMC_MSG_setaveragetemperaturetimeconstant,
			time);
		break;
	default:
		dev_err(dev, "invalid id\n");
		return -EINVAL;
	}

	if (unlikely(!smu_send_single_msg(&msg))) {
		dev_err(dev, "%s: send smu msg failed\n", __func__);
		return -EBUSY;
	}
	return count;
}

static ssize_t mtg_end_hys_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	int ret;
	u32 time;
	struct smu_msg msg;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	dev_dbg(dev, "%s: %d-%d\n", __func__, sattr->nr, sattr->index);
	if (unlikely(sattr->index != CUR_VAL)) {
		dev_err(dev, "%s: invalid value type\n", __func__);
		return -EIO;
	}

	ret = kstrtouint(buf, 0, &time);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: get parameter failed\n", __func__);
		return ret;
	}

	switch (sattr->nr) {
	case MIG_END_HYS:
		smu_msg(&msg, CVSMC_MSG_setmitigationendhysteresis, time);
		break;
	default:
		dev_err(dev, "invalid id\n");
		return -EINVAL;
	}

	if (unlikely(!smu_send_single_msg(&msg))) {
		dev_err(dev, "%s: send smu msg failed\n", __func__);
		return -EBUSY;
	}
	return count;
}

static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CPU0, voltage, VOL_CPU0, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CPU1, voltage, VOL_CPU1, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CPU2, voltage, VOL_CPU2, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CPU3, voltage, VOL_CPU3, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_GFX, voltage, VOL_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_SOC, voltage, VOL_SOC, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CVIP, voltage, VOL_CVIP, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_cur_CPU, current, CUR_CPU, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_cur_GFX, current, CUR_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_cur_SOC, current, CUR_SOC, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_cur_CVIP, current, CUR_CVIP, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_freq_A55CLK, freq, A55CLK, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_freq_A55TWOCLK, freq, A55TWOCLK, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_freq_C5CLK, freq, C5CLK, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_freq_Q6CLK, freq, Q6CLK, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_freq_NIC400CLK, freq, NIC400CLK, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_pow_CPU, power, POWER_CPU, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_pow_CPU, power, POWER_CPU, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_pow_GFX, power, POWER_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_pow_GFX, power, POWER_GFX, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_pow_SOC, power, POWER_SOC, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_pow_SOC, power, POWER_SOC, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_pow_CVIP, power, POWER_CVIP, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_pow_CVIP, power, POWER_CVIP, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CPU0, temp, TEM_CPU0, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CPU0, temp, TEM_CPU0, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CPU1, temp, TEM_CPU1, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CPU1, temp, TEM_CPU1, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CPU2, temp, TEM_CPU2, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CPU2, temp, TEM_CPU2, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CPU3, temp, TEM_CPU3, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CPU3, temp, TEM_CPU3, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_GFX, temp, TEM_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_GFX, temp, TEM_GFX, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_SOC, temp, TEM_SOC, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_SOC, temp, TEM_SOC, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CVIP, temp, TEM_CVIP, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CVIP, temp, TEM_CVIP, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_WO(ave_pow_time, power_time, POWER_TIME, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_WO(ave_tem_time, temp_time, TEM_TIME, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_WO(mtg_end_hys, mtg_end_hys, MIG_END_HYS, CUR_VAL);

static struct attribute *cvip_hwmon_attrs[] = {
	&sensor_dev_attr_cur_vol_CPU0.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CPU1.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CPU2.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CPU3.dev_attr.attr,
	&sensor_dev_attr_cur_vol_GFX.dev_attr.attr,
	&sensor_dev_attr_cur_vol_SOC.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CVIP.dev_attr.attr,
	&sensor_dev_attr_cur_cur_CPU.dev_attr.attr,
	&sensor_dev_attr_cur_cur_GFX.dev_attr.attr,
	&sensor_dev_attr_cur_cur_SOC.dev_attr.attr,
	&sensor_dev_attr_cur_cur_CVIP.dev_attr.attr,
	&sensor_dev_attr_cur_freq_A55CLK.dev_attr.attr,
	&sensor_dev_attr_cur_freq_A55TWOCLK.dev_attr.attr,
	&sensor_dev_attr_cur_freq_C5CLK.dev_attr.attr,
	&sensor_dev_attr_cur_freq_Q6CLK.dev_attr.attr,
	&sensor_dev_attr_cur_freq_NIC400CLK.dev_attr.attr,
	&sensor_dev_attr_cur_pow_CPU.dev_attr.attr,
	&sensor_dev_attr_ave_pow_CPU.dev_attr.attr,
	&sensor_dev_attr_cur_pow_GFX.dev_attr.attr,
	&sensor_dev_attr_ave_pow_GFX.dev_attr.attr,
	&sensor_dev_attr_cur_pow_SOC.dev_attr.attr,
	&sensor_dev_attr_ave_pow_SOC.dev_attr.attr,
	&sensor_dev_attr_cur_pow_CVIP.dev_attr.attr,
	&sensor_dev_attr_ave_pow_CVIP.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CPU0.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CPU0.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CPU1.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CPU1.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CPU2.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CPU2.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CPU3.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CPU3.dev_attr.attr,
	&sensor_dev_attr_cur_tem_GFX.dev_attr.attr,
	&sensor_dev_attr_ave_tem_GFX.dev_attr.attr,
	&sensor_dev_attr_cur_tem_SOC.dev_attr.attr,
	&sensor_dev_attr_ave_tem_SOC.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CVIP.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CVIP.dev_attr.attr,
	&sensor_dev_attr_ave_pow_time.dev_attr.attr,
	&sensor_dev_attr_ave_tem_time.dev_attr.attr,
	&sensor_dev_attr_mtg_end_hys.dev_attr.attr,
	NULL
};

static const struct attribute_group cvip_attr_group = {
	.attrs = cvip_hwmon_attrs
};

static struct miscdevice mero_hwmon_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MERO_HWMON_NAME,
};

static int __init mero_hwmon_init(void)
{
	int ret;
	struct device *dev;
	struct device *hw_dev;
	struct mero_hwmon_data *data;

	ret = misc_register(&mero_hwmon_misc);
	if (ret) {
		pr_err("%s: misc dev register fail\n", __func__);
		return ret;
	}
	dev = mero_hwmon_misc.this_device;
	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "out of memory\n");
		ret = -ENOMEM;
		goto err;
	}
	data->groups[0] = &cvip_attr_group;
	/* register device with all the acquired attributes */
	hw_dev = devm_hwmon_device_register_with_groups(dev,
							mero_hwmon_misc.name,
							data,
							data->groups);
	if (IS_ERR(hw_dev)) {
		dev_err(dev, "hwmon device register fail\n");
		ret = PTR_ERR(hw_dev);
		goto err;
	}
	data->hwmon_dev = hw_dev;
	return ret;

err:
	misc_deregister(&mero_hwmon_misc);
	return ret;
}

static void __exit mero_hwmon_exit(void)
{
	return misc_deregister(&mero_hwmon_misc);
}

/* late_initcall to make sure smu initialized*/
late_initcall(mero_hwmon_init);
module_exit(mero_hwmon_exit);
MODULE_DESCRIPTION("mero hwmon driver");

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
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/acpi.h>
#include <linux/ml-mux-client.h>
#include <linux/timer.h>
#include <linux/thermal.h>
#include "mlmux-power.h"

#define CREATE_TRACE_POINTS
#include <trace/events/mlmux_power.h>

#define DUMMY_BATTERY_FULL_CHARGE_IN_MICROVOLTS 871200
#define PSY_INIT_DELAY_MS 10000
#define MLMUX_POWER_SUSPEND_TIMEOUT_MS 2000
#define MLMUX_POWER_RESUME_TIMEOUT_MS 2000

#define MLMUX_POWER_FLAG_CHANGED(old, new, flag) \
	(GET_FLAG(old, flag) != GET_FLAG(new, flag))

#define MLMUX_POWER_FIELD_CHANGED(old, new, field) \
	(GET_FIELD(old, field) != GET_FIELD(new, field))

#define TS1_TZ "ts1"
#define TS2_TZ "ts2"

struct __mlmux_power_state {
	struct {
		uint8_t flags;
		uint8_t soc;
		uint16_t vbat;
		uint16_t vbat_adc;
		uint16_t vbat_max;
		int16_t batt_curr;
		int16_t batt_temp;
		int16_t ts1_temp;
		int16_t ts2_temp;
		uint32_t batt_id;
		int32_t power;
		uint8_t health;
		uint8_t status;
		uint16_t remaining_capacity;
		uint16_t full_charge_capacity;
		uint16_t design_capacity;
	} battery;

	struct {
		uint16_t flags;
		uint16_t vbus;
		uint16_t adpt_curr;
		uint16_t adap_curr_max;
		int16_t chrg_temp;
		uint16_t adap_volt_limit;
		uint16_t adap_volt_max;
		uint8_t charger_health;
	} charger;
};

/**
 * This structure matches mlmux_power_ext_state
 * in the header. Refer to the comment there for
 * more details.
 */
struct __mlmux_power_state_ext {
	uint16_t chgr_std;
	uint8_t rsvd;
	int16_t batt_curr_avg;
	uint16_t cycle_cnt;
	uint8_t epb_soc;
	uint32_t unused[28];
};

struct __mlmux_power_init {
	uint16_t batt_capacity;
	uint32_t batt_id_res;
	uint32_t update_period_ms;
	uint32_t unused[28];
};

enum mlmux_power_event_type {
	MLMUX_POWER_EVENT_OPEN,
	MLMUX_POWER_EVENT_CLOSE,
	MLMUX_POWER_EVENT_INIT,
	MLMUX_POWER_EVENT_STATUS,
	MLMUX_POWER_EVENT_STATUS_EXT,
	MLMUX_POWER_EVENT_UPDATE,
};

struct mlmux_power_event {
	struct list_head list;
	enum mlmux_power_event_type type;
	size_t len;
	uint8_t payload[0];
};

struct mlmux_power_data {
	struct platform_device *pdev;
	struct power_supply *battery;
	struct power_supply *usb;
	struct __mlmux_power_init init;
	struct __mlmux_power_state status;
	struct __mlmux_power_state_ext status_ext;
	struct ml_mux_client client;
	struct work_struct event_work;
	struct list_head event_queue;
	spinlock_t event_lock;
	struct timer_list psy_check_timer;
	uint64_t psy_check_period_ms;
	struct work_struct psy_alert_work;
	spinlock_t psy_check_lock;
	struct completion suspend;
	struct completion resume;
	spinlock_t psy_timer_en_lock;
	uint8_t psy_timer_en;
	bool batt_uevent_update;
	bool chrg_uevent_update;
	bool chrg_ext_uevent_update;
	struct timer_list psy_count_timer;
	struct work_struct psy_count_work;
	spinlock_t psy_count_lock;
	uint8_t psy_msg_count;
	struct thermal_zone_device *ts1;
	struct thermal_zone_device *ts2;
};

static enum power_supply_property mlmux_battery_props[] = {
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_AUTHENTIC,
	POWER_SUPPLY_PROP_TEMP_TS1,
	POWER_SUPPLY_PROP_TEMP_TS2,
	POWER_SUPPLY_PROP_POWER_NOW,
	POWER_SUPPLY_PROP_BATT_ID,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_TEMP_TS_EXT,
};

static enum power_supply_property mlmux_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CAP_MISMATCH,
	POWER_SUPPLY_PROP_CHARGER_INSUFFICIENT,
	POWER_SUPPLY_PROP_CHARGER_TYPE,
	POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT,
	POWER_SUPPLY_PROP_CHARGER_ADAPTER_ALLOWED,
	POWER_SUPPLY_PROP_CHARGER_STANDARD,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_EXTERNAL_POWER_BANK_SOC,
};

static ssize_t resistance_id_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct mlmux_power_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", data->init.batt_id_res);
}
static DEVICE_ATTR_RO(resistance_id);

static struct attribute *mlmux_power_sysfs_attrs[] = {
	&dev_attr_resistance_id.attr,
	NULL,
};
ATTRIBUTE_GROUPS(mlmux_power_sysfs);

static int mlmux_power_get_ts_temp(struct mlmux_power_data *data,
	struct thermal_zone_device *ts_zone_dev, int *temp)
{
	int tz_ret, ts_temp;

	if (!temp)
		return -EINVAL;

	if (!ts_zone_dev) {
		dev_err(&data->pdev->dev, "Invalid Thermal Zone passed\n");
		return -EINVAL;
	}

	tz_ret = thermal_zone_get_temp(ts_zone_dev, &ts_temp);
	if (tz_ret) {
		dev_err(&data->pdev->dev, "Unable to get temp_%s: %d\n",
			ts_zone_dev->type, tz_ret);
		return -EAGAIN;
	}

	/*
	 * Convert down from mC (kernel thermal spec) to dC
	 * (kernel power supply spec).
	 */
	*temp = ts_temp / 100;

	return 0;
}

static int mlmux_power_get_temp(struct mlmux_power_data *data, int *temp)
{
	int ret, temp_ts1, temp_ts2;

	if (!data->ts1) {
		data->ts1 = thermal_zone_get_zone_by_name(TS1_TZ);
		if (IS_ERR(data->ts1))
			data->ts1 = NULL;
	}

	if (!data->ts2) {
		data->ts2 = thermal_zone_get_zone_by_name(TS2_TZ);
		if (IS_ERR(data->ts2))
			data->ts2 = NULL;
	}

	ret = mlmux_power_get_ts_temp(data, data->ts1, &temp_ts1);
	if (ret)
		return ret;

	ret = mlmux_power_get_ts_temp(data, data->ts2, &temp_ts2);
	if (ret)
		return ret;

	*temp = temp_ts1 > temp_ts2 ? temp_ts1 : temp_ts2;

	return 0;
}

static int mlmux_battery_get_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
	struct mlmux_power_data *data = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = data->status.battery.soc;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = data->init.batt_capacity * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		/*
		 * TODO: Update when EE specifies how often to
		 * sample voltage readings.
		 */
		val->intval = data->status.battery.vbat * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = data->status.battery.batt_curr * 1000;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = data->status.battery.vbat * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = data->status.battery.vbat_max * 1000;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = mlmux_power_get_temp(data, &val->intval);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = GET_FLAG(data->status.battery.flags,
				MLMUX_POWER_BAT_FLAG_IS_PRESENT);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = data->status.battery.health;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = data->status.battery.status;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = data->status.battery.full_charge_capacity * 1000;
		break;
	case POWER_SUPPLY_PROP_AUTHENTIC:
		val->intval = GET_FLAG(data->status.battery.flags,
				MLMUX_POWER_BAT_FLAG_IS_AUTH);
		break;
	case POWER_SUPPLY_PROP_TEMP_TS1:
		if (!data->ts1) {
			data->ts1 = thermal_zone_get_zone_by_name(TS1_TZ);
			if (IS_ERR(data->ts1))
				data->ts1 = NULL;
		}
		ret = mlmux_power_get_ts_temp(data, data->ts1, &val->intval);
		break;
	case POWER_SUPPLY_PROP_TEMP_TS2:
		if (!data->ts2) {
			data->ts2 = thermal_zone_get_zone_by_name(TS2_TZ);
			if (IS_ERR(data->ts2))
				data->ts2 = NULL;
		}
		ret = mlmux_power_get_ts_temp(data, data->ts2, &val->intval);
		break;
	case POWER_SUPPLY_PROP_POWER_NOW:
		val->intval = data->status.battery.power * 1000;
		break;
	case POWER_SUPPLY_PROP_BATT_ID:
		val->intval = data->status.battery.batt_id;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = data->status.battery.remaining_capacity * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = data->status_ext.batt_curr_avg * 1000;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		val->intval = data->status_ext.cycle_cnt;
		break;
	case POWER_SUPPLY_PROP_TEMP_TS_EXT:
		val->intval = data->status.battery.batt_temp;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int mlmux_usb_get_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
	struct mlmux_power_data *data = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = GET_FLAG(data->status.charger.flags,
				MLMUX_POWER_CHARGE_FLAG_CABLE_CONN);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = data->status.charger.adap_curr_max * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = data->status.charger.adpt_curr * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = data->status.charger.adap_volt_max * 1000;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = data->status.charger.charger_health;
		break;
	case POWER_SUPPLY_PROP_CAP_MISMATCH:
		val->intval = GET_FLAG(data->status.charger.flags,
				MLMUX_POWER_CHARGE_FLAG_CAP_MISMATCH);
		break;
	case POWER_SUPPLY_PROP_CHARGER_INSUFFICIENT:
		val->intval = GET_FLAG(data->status.charger.flags,
				MLMUX_POWER_CHARGE_FLAG_INSUFFICIENT);
		break;
	case POWER_SUPPLY_PROP_CHARGER_TYPE:
		val->intval = GET_FIELD(data->status.charger.flags,
				MLMUX_POWER_CHARGE_FLAG_PSY_TYPE);
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		val->intval = data->status.charger.adap_volt_limit * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGER_ADAPTER_ALLOWED:
		val->intval = GET_FLAG(data->status.charger.flags,
				MLMUX_POWER_CHARGE_ADAPTER_ALLOWED);
		break;
	case POWER_SUPPLY_PROP_CHARGER_STANDARD:
		val->intval = data->status_ext.chgr_std;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = data->status.charger.vbus * 1000;
		break;
	case POWER_SUPPLY_PROP_EXTERNAL_POWER_BANK_SOC:
		val->intval = data->status_ext.epb_soc;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct power_supply_desc battery_desc = {
	.properties	= mlmux_battery_props,
	.num_properties	= ARRAY_SIZE(mlmux_battery_props),
	.get_property	= mlmux_battery_get_property,
	.name		= "battery",
	.type		= POWER_SUPPLY_TYPE_BATTERY,
};

static const struct power_supply_desc usb_desc = {
	.properties	= mlmux_usb_props,
	.num_properties	= ARRAY_SIZE(mlmux_usb_props),
	.get_property	= mlmux_usb_get_property,
	.name		= "usb_pd",
	.type		= POWER_SUPPLY_TYPE_USB_PD,
};

static int mlmux_power_supply_register(struct mlmux_power_data *data)
{
	struct power_supply_config psy_cfg = {};
	int ret;

	psy_cfg.drv_data = data;
	psy_cfg.attr_grp = mlmux_power_sysfs_groups;

	if (!data->battery) {
		data->battery = power_supply_register(&data->pdev->dev,
							&battery_desc,
							&psy_cfg);

		if (IS_ERR(data->battery)) {
			ret = PTR_ERR(data->battery);
			data->battery = NULL;
			dev_err(&data->pdev->dev, "Power supply battery register fail: %d\n",
				ret);
			goto psy_reg_fail;
		}
	}

	if (!data->usb) {
		data->usb = power_supply_register(&data->pdev->dev, &usb_desc,
							&psy_cfg);

		if (IS_ERR(data->usb)) {
			ret = PTR_ERR(data->usb);
			data->usb = NULL;
			dev_err(&data->pdev->dev, "Power supply usb charger register fail: %d\n",
				ret);
			goto psy_reg_fail;
		}
	}

	return 0;

psy_reg_fail:
	queue_work(system_power_efficient_wq, &data->psy_alert_work);
	return ret;
}

static int mlmux_power_supply_unregister(struct mlmux_power_data *data)
{
	if (data->battery) {
		power_supply_unregister(data->battery);
		data->battery = NULL;
	}

	if (data->usb) {
		power_supply_unregister(data->usb);
		data->usb = NULL;
	}

	return 0;
}

static bool mlmux_power_battery_changed(struct mlmux_power_data *data,
					struct __mlmux_power_state *new)
{
	if (data->status.battery.soc != new->battery.soc)
		return true;

	/* Charge to discharge or discharge to charge? */
	if ((data->status.battery.batt_curr ^ new->battery.batt_curr) < 0)
		return true;

	if (data->status.battery.health != new->battery.health)
		return true;

	if (data->status.battery.vbat_max != new->battery.vbat_max)
		return true;

	if (data->status.battery.status != new->battery.status)
		return true;

	return false;
}

static bool mlmux_power_usb_changed(struct mlmux_power_data *data,
					struct __mlmux_power_state *new)
{
	uint16_t old_flags = data->status.charger.flags;
	uint16_t new_flags = new->charger.flags;

	if (MLMUX_POWER_FLAG_CHANGED(old_flags, new_flags,
		MLMUX_POWER_CHARGE_FLAG_CABLE_CONN))
		return true;

	if (MLMUX_POWER_FIELD_CHANGED(old_flags, new_flags,
		MLMUX_POWER_CHARGE_FLAG_PSY_TYPE))
		return true;

	if (data->status.charger.adap_curr_max != new->charger.adap_curr_max)
		return true;

	if (MLMUX_POWER_FLAG_CHANGED(old_flags, new_flags,
		MLMUX_POWER_CHARGE_ADAPTER_ALLOWED))
		return true;

	if (MLMUX_POWER_FLAG_CHANGED(old_flags, new_flags,
		MLMUX_POWER_CHARGE_FLAG_INSUFFICIENT))
		return true;

	return false;
}

static int mlmux_power_send_msg(struct mlmux_power_data *data,
	enum mlmux_power_tx_type msg_type)
{
	struct mlmux_power_tx_pkt msg;
	uint32_t msg_body_len = 0;

	msg.hdr.msg_type = msg_type;
	if (msg_type == MLMUX_POWER_TX_POWER_UPDATE_ACK)
		msg_body_len = sizeof(msg.ack);

	return ml_mux_send_msg(data->client.ch,
		sizeof(msg.hdr) + msg_body_len, &msg);
}

static void mlmux_power_update_ack(struct mlmux_power_data *data)
{
	int ret;

	/*
	 * Don't acknowlege or restart timer neither power supply didn't
	 * initialize. Even if messages are received from mlmux, nothing
	 * meaningful can be done if power supply core can't be used to
	 * notify user space of any updates.
	 */
	if (!data->battery || !data->usb)
		return;

	ret = mlmux_power_send_msg(data, MLMUX_POWER_TX_POWER_UPDATE_ACK);
	if (ret)
		dev_err(&data->pdev->dev, "Power update ACK send failed");

	mod_timer(&data->psy_check_timer,
		jiffies + msecs_to_jiffies(data->psy_check_period_ms));
}

static void mlmux_power_event_status(struct mlmux_power_data *data,
				     struct mlmux_power_event *event)
{
	struct mlmux_power_state *state =
		(struct mlmux_power_state *)event->payload;
	struct __mlmux_power_state new;

	new.battery.flags = state->battery.flags;
	new.battery.soc = state->battery.soc;
	new.battery.vbat = __le16_to_cpu(state->battery.vbat);
	new.battery.vbat_adc = __le16_to_cpu(state->battery.vbat_adc);
	new.battery.vbat_max = __le16_to_cpu(state->battery.vbat_max);
	new.battery.batt_curr = __le16_to_cpu(state->battery.batt_curr);
	new.battery.batt_temp = __le16_to_cpu(state->battery.batt_temp);
	new.battery.ts1_temp = __le16_to_cpu(state->battery.ts1_temp);
	new.battery.ts2_temp = __le16_to_cpu(state->battery.ts2_temp);
	new.battery.batt_id = __le32_to_cpu(state->battery.batt_id);
	new.battery.power =  __le32_to_cpu(state->battery.power);
	new.battery.health = state->battery.health;
	new.battery.status = state->battery.status;
	new.battery.remaining_capacity = __le16_to_cpu(
		state->battery.remaining_capacity);
	new.battery.full_charge_capacity = __le16_to_cpu(
		state->battery.full_charge_capacity);
	new.battery.design_capacity = __le16_to_cpu(
		state->battery.design_capacity);

	new.charger.flags = __le16_to_cpu(state->charger.flags);
	new.charger.vbus = __le16_to_cpu(state->charger.vbus);
	new.charger.adpt_curr = __le16_to_cpu(state->charger.adpt_curr);
	new.charger.adap_curr_max = __le16_to_cpu(state->charger.adap_curr_max);
	new.charger.chrg_temp = __le16_to_cpu(state->charger.chrg_temp);
	new.charger.adap_volt_limit = __le16_to_cpu(
		state->charger.adap_volt_limit);
	new.charger.adap_volt_max = __le16_to_cpu(state->charger.adap_volt_max);
	new.charger.charger_health = state->charger.charger_health;

	data->batt_uevent_update = mlmux_power_battery_changed(data, &new);
	data->chrg_uevent_update = mlmux_power_usb_changed(data, &new);

	data->status = new;

	mlmux_power_update_ack(data);
}

static int mlmux_power_event_post(struct mlmux_power_data *data,
				  enum mlmux_power_event_type type,
				  size_t len, void *payload)
{
	struct mlmux_power_event *event;

	event = kzalloc(sizeof(*event) + len, GFP_KERNEL);
	if (!event)
		return -ENOMEM;

	event->type = type;
	event->len = len;
	if (len && data)
		memcpy(event->payload, payload, len);

	spin_lock(&data->event_lock);
	list_add_tail(&event->list, &data->event_queue);
	spin_unlock(&data->event_lock);

	queue_work(system_power_efficient_wq, &data->event_work);

	return 0;
}

static bool mlmux_power_ext_state_changed(struct mlmux_power_data *data,
				struct __mlmux_power_state_ext *new_state_ext)
{
	if (new_state_ext->chgr_std != data->status_ext.chgr_std)
		return true;

	if (new_state_ext->epb_soc != data->status_ext.epb_soc)
		return true;

	return false;
}

static void mlmux_power_event_status_ext(struct mlmux_power_data *data,
				     struct mlmux_power_event *event)
{
	struct mlmux_power_state_ext *state_ext =
		(struct mlmux_power_state_ext *)event->payload;
	struct __mlmux_power_state_ext new_state_ext;

	new_state_ext.chgr_std = __le16_to_cpu(state_ext->chgr_std);
	new_state_ext.batt_curr_avg = __le16_to_cpu(state_ext->batt_curr_avg);
	new_state_ext.cycle_cnt = __le16_to_cpu(state_ext->cycle_cnt);
	new_state_ext.epb_soc = state_ext->epb_soc;

	data->chrg_ext_uevent_update = mlmux_power_ext_state_changed(
		data, &new_state_ext);

	data->status_ext = new_state_ext;

	mlmux_power_update_ack(data);

	/*
	 * At this point all parts of the power status message
	 * (battery, charger, extended charger) have been received.
	 * Check if uevent needs to be issued based on changes to
	 * these fields.
	 */
	mlmux_power_event_post(data, MLMUX_POWER_EVENT_UPDATE, 0, NULL);
}

static void mlmux_power_event_status_init(struct mlmux_power_data *data,
				     struct mlmux_power_event *event)
{
	struct mlmux_power_init *init =
		(struct mlmux_power_init *)event->payload;
	struct __mlmux_power_init new_init;

	new_init.batt_capacity = __le16_to_cpu(init->batt_capacity);
	new_init.batt_id_res = __le32_to_cpu(init->batt_id_res);
	new_init.update_period_ms = __le32_to_cpu(init->update_period_ms);

	data->init = new_init;

	spin_lock(&data->psy_check_lock);
	data->psy_check_period_ms = data->init.update_period_ms;
	spin_unlock(&data->psy_check_lock);

	if (data->battery)
		power_supply_changed(data->battery);

	mlmux_power_update_ack(data);

	dev_dbg(&data->pdev->dev, "New PSY mlmux status check interval: %d\n",
				data->psy_check_period_ms);
	mod_timer(&data->psy_count_timer,
		jiffies + msecs_to_jiffies(data->psy_check_period_ms));
}

static void mlmux_power_event_status_update(struct mlmux_power_data *data)
{
	if (data->batt_uevent_update && data->battery)
		power_supply_changed(data->battery);

	if ((data->chrg_ext_uevent_update || data->chrg_uevent_update)
		&& data->usb)
		power_supply_changed(data->usb);

	spin_lock(&data->psy_count_lock);
	data->psy_msg_count++;
	spin_unlock(&data->psy_count_lock);
}

static void mlmux_power_event_worker(struct work_struct *work)
{
	struct mlmux_power_data *data =
		container_of(work, struct mlmux_power_data, event_work);
	struct mlmux_power_event *event, *e;

	spin_lock(&data->event_lock);

	list_for_each_entry_safe(event, e, &data->event_queue, list) {

		list_del(&event->list);
		spin_unlock(&data->event_lock);

		switch (event->type) {
		case MLMUX_POWER_EVENT_OPEN:
			mlmux_power_supply_register(data);
			break;
		case MLMUX_POWER_EVENT_CLOSE:
			mlmux_power_supply_unregister(data);
			break;
		case MLMUX_POWER_EVENT_STATUS:
			mlmux_power_event_status(data, event);
			break;
		case MLMUX_POWER_EVENT_STATUS_EXT:
			mlmux_power_event_status_ext(data, event);
			break;
		case MLMUX_POWER_EVENT_INIT:
			mlmux_power_event_status_init(data, event);
			break;
		case MLMUX_POWER_EVENT_UPDATE:
			mlmux_power_event_status_update(data);
			break;
		default:
			dev_err(&data->pdev->dev, "Unknown event %d\n",
				event->type);
			break;
		}

		kfree(event);
		spin_lock(&data->event_lock);
	}
	spin_unlock(&data->event_lock);
}

static void mlmux_power_open(struct ml_mux_client *cli, bool is_open)
{
	struct mlmux_power_data *data =
		container_of(cli, struct mlmux_power_data, client);

	if (is_open)
		mlmux_power_event_post(data, MLMUX_POWER_EVENT_OPEN, 0, NULL);
	else
		mlmux_power_event_post(data, MLMUX_POWER_EVENT_CLOSE, 0, NULL);
}

static void mlmux_power_recv(struct ml_mux_client *cli, uint32_t len, void *msg)
{
	struct mlmux_power_data *data =
		container_of(cli, struct mlmux_power_data, client);
	struct mlmux_power_rx_pkt *pkt = msg;

	if (len < sizeof(struct mlmux_power_hdr) || !msg)
		return;

	switch (pkt->hdr.msg_type) {
	case MLMUX_POWER_RX_INIT:
		if (len < (sizeof(pkt->hdr) + sizeof(pkt->init)))
			break;

		trace_psy_init(&pkt->init);

		mlmux_power_event_post(data, MLMUX_POWER_EVENT_INIT,
				       sizeof(pkt->state), &pkt->state);
		break;
	case MLMUX_POWER_RX_STATE:
		if (len < (sizeof(pkt->hdr) + sizeof(pkt->state)))
			break;
		trace_battery_status(&pkt->state.battery);
		trace_charger_status(&pkt->state.charger);

		mlmux_power_event_post(data, MLMUX_POWER_EVENT_STATUS,
				       sizeof(pkt->state), &pkt->state);
		break;
	case MLMUX_POWER_RX_EXT_STATE:
		if (len < (sizeof(pkt->hdr) + sizeof(pkt->state_ext)))
			break;

		trace_ext_status(&pkt->state_ext);

		mlmux_power_event_post(data, MLMUX_POWER_EVENT_STATUS_EXT,
				       sizeof(pkt->state_ext), &pkt->state_ext);
		break;
	case MLMUX_POWER_RX_SUSPEND_ACK:
		if (len < (sizeof(pkt->hdr) + sizeof(pkt->ack)))
			break;

		complete(&data->suspend);
		break;
	case MLMUX_POWER_RX_RESUME_ACK:
		if (len < (sizeof(pkt->hdr) + sizeof(pkt->ack)))
			break;

		complete(&data->resume);
		break;
	default:
		dev_err(&data->pdev->dev, "Unknown msg type: %d, len: %d\n",
			pkt->hdr.msg_type, len);
		break;
	}
}

struct device_type ml_power_supply_device_type = {
	.name = "ml_power_supply",
};

static void mlmux_power_psy_check_timer_fn(struct timer_list *timer)
{
	struct mlmux_power_data *data = container_of(timer,
		struct mlmux_power_data, psy_check_timer);

	dev_dbg(&data->pdev->dev, "psy check timer timed out\n");
	queue_work(system_power_efficient_wq, &data->psy_alert_work);
}

static void mlmux_power_psy_count_timer_fn(struct timer_list *timer)
{
	struct mlmux_power_data *data = container_of(timer,
		struct mlmux_power_data, psy_count_timer);

	queue_work(system_power_efficient_wq, &data->psy_count_work);
}

static void mlmux_power_psy_disc_worker(struct work_struct *work)
{
	struct mlmux_power_data *data = container_of(work,
		struct mlmux_power_data, psy_alert_work);
	char *psy_unavailable_envp[2] = { "PSY_UNAVAILABLE=1", NULL };

	kobject_uevent_env(&data->pdev->dev.kobj, KOBJ_CHANGE,
		psy_unavailable_envp);

	/*
	 * Re-arm to keep notifying user space of power supply disconnect
	 * in case user space is not in state to handle it. Also provides
	 * time for connection to be restored.
	 */
	spin_lock(&data->psy_timer_en_lock);
	if (data->psy_timer_en)
		mod_timer(&data->psy_check_timer,
			jiffies + msecs_to_jiffies(data->psy_check_period_ms));
	spin_unlock(&data->psy_timer_en_lock);
}

static void mlmux_power_psy_count_worker(struct work_struct *work)
{
	struct mlmux_power_data *data = container_of(work,
		struct mlmux_power_data, psy_count_work);
	uint8_t msg_count;
	uint64_t check_period_ms;

	spin_lock(&data->psy_count_lock);
	msg_count = data->psy_msg_count;
	data->psy_msg_count = 0;
	spin_unlock(&data->psy_count_lock);

	spin_lock(&data->psy_check_lock);
	check_period_ms = data->psy_check_period_ms;
	spin_unlock(&data->psy_check_lock);

	dev_dbg(&data->pdev->dev,
		"num status msgs received in last %lu ms: %d",
		check_period_ms, msg_count);

	mod_timer(&data->psy_count_timer,
		jiffies + msecs_to_jiffies(check_period_ms));
}

static int mlmux_power_suspend(struct device *dev)
{
	struct mlmux_power_data *data = dev_get_drvdata(dev);
	int ret;
	unsigned long jiffies_left;

	reinit_completion(&data->suspend);
	ret = mlmux_power_send_msg(data, MLMUX_POWER_TX_SUSPEND);
	if (ret)
		return ret;

	jiffies_left = wait_for_completion_timeout(&data->suspend,
		msecs_to_jiffies(MLMUX_POWER_SUSPEND_TIMEOUT_MS));
	if (!jiffies_left)
		return -ETIMEDOUT;

	del_timer(&data->psy_check_timer);
	del_timer(&data->psy_count_timer);

	dev_dbg(&data->pdev->dev, "suspend\n");

	return 0;
}

static int mlmux_power_resume(struct device *dev)
{
	struct mlmux_power_data *data = dev_get_drvdata(dev);
	int ret;
	unsigned long jiffies_left;
	uint64_t resume_period_ms;

	reinit_completion(&data->resume);
	ret = mlmux_power_send_msg(data, MLMUX_POWER_TX_RESUME);
	if (ret)
		return ret;

	spin_lock(&data->psy_check_lock);
	resume_period_ms = data->psy_check_period_ms;
	spin_unlock(&data->psy_check_lock);

	spin_lock(&data->psy_count_lock);
	data->psy_msg_count = 0;
	spin_unlock(&data->psy_count_lock);

	jiffies_left = wait_for_completion_timeout(&data->resume,
		msecs_to_jiffies(MLMUX_POWER_RESUME_TIMEOUT_MS));

	mod_timer(&data->psy_check_timer,
		jiffies + msecs_to_jiffies(resume_period_ms));

	mod_timer(&data->psy_count_timer,
		jiffies + msecs_to_jiffies(resume_period_ms));

	if (!jiffies_left)
		return -ETIMEDOUT;

	dev_dbg(&data->pdev->dev, "resume\n");

	return 0;
}

static const struct dev_pm_ops mlmux_power_pm = {
	.suspend = mlmux_power_suspend,
	.resume = mlmux_power_resume,
};

static int mlmux_power_probe(struct platform_device *pdev)
{
	int ret;
	struct mlmux_power_data *data;
	struct timer_list *timer;
	struct timer_list *count_timer;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);

	INIT_WORK(&data->event_work, mlmux_power_event_worker);
	spin_lock_init(&data->event_lock);
	INIT_LIST_HEAD(&data->event_queue);

	pdev->dev.type = &ml_power_supply_device_type;
	data->pdev = pdev;
	data->client.dev = &pdev->dev;
	data->client.notify_open = mlmux_power_open;
	data->client.receive_cb = mlmux_power_recv;
	data->psy_check_period_ms = PSY_INIT_DELAY_MS;
	data->psy_msg_count = 0;

	INIT_WORK(&data->psy_alert_work, mlmux_power_psy_disc_worker);
	spin_lock_init(&data->psy_check_lock);
	init_completion(&data->suspend);
	init_completion(&data->resume);

	spin_lock_init(&data->psy_timer_en_lock);
	data->psy_timer_en = 1;

	timer = &data->psy_check_timer;
	timer_setup(timer, mlmux_power_psy_check_timer_fn, 0);
	mod_timer(timer,
		jiffies + msecs_to_jiffies(data->psy_check_period_ms));

	INIT_WORK(&data->psy_count_work, mlmux_power_psy_count_worker);
	spin_lock_init(&data->psy_count_lock);

	count_timer = &data->psy_count_timer;
	timer_setup(count_timer, mlmux_power_psy_count_timer_fn, 0);

	ret = ml_mux_request_channel(&data->client, MLMUX_POWER_CHANNEL_NAME);

	if (ret == ML_MUX_CH_REQ_OPENED)
		ret = mlmux_power_event_post(data, MLMUX_POWER_EVENT_OPEN, 0,
					     NULL);

	return ret;
}

static int mlmux_power_remove(struct platform_device *pdev)
{
	struct mlmux_power_data *data = platform_get_drvdata(pdev);

	ml_mux_release_channel(data->client.ch);

	flush_work(&data->event_work);

	spin_lock(&data->psy_timer_en_lock);
	data->psy_timer_en = 0;
	spin_unlock(&data->psy_timer_en_lock);

	del_timer_sync(&data->psy_check_timer);
	flush_work(&data->psy_alert_work);

	del_timer_sync(&data->psy_count_timer);
	flush_work(&data->psy_count_work);

	mlmux_power_supply_unregister(data);

	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id mlmux_power_acpi_match[] = {
	{ "MXPM1111", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, mlmux_power_acpi_match);
#endif

static struct platform_driver mlmux_power_driver = {
	.probe = mlmux_power_probe,
	.remove = mlmux_power_remove,
	.driver = {
		.name = "mlmux-power",
		.acpi_match_table = ACPI_PTR(mlmux_power_acpi_match),
		.pm = &mlmux_power_pm,
	}
};
module_platform_driver(mlmux_power_driver);


MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Power driver for Magic Leap MUX");

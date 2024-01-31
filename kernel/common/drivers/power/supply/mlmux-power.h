/* SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
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
#ifndef MLMUX_POWER_H
#define MLMUX_POWER_H

#define GET_FIELD(_v, _f) (((_v) >>  _f##_LSB) & (_f##_MASK))
#define GET_FLAG(_v, _f) (!!((_v) & (_f)))

#define MLMUX_POWER_CHANNEL_NAME	"POWER_STATUS"

enum mlmux_power_rx_type {
	MLMUX_POWER_RX_INIT = 0,
	MLMUX_POWER_RX_STATE,
	MLMUX_POWER_RX_SUSPEND_ACK,
	MLMUX_POWER_RX_RESUME_ACK,
	MLMUX_POWER_RX_EXT_STATE,
};

enum mlmux_power_tx_type {
	MLMUX_POWER_TX_CHRG_CURR = 0,
	MLMUX_POWER_TX_VSYS,
	MLMUX_POWER_TX_ADPT_CURR,
	MLMUX_POWER_TX_UPDATE_NOW,
	MLMUX_POWER_TX_UPDATE_RATE,
	MLMUX_POWER_TX_SUSPEND,
	MLMUX_POWER_TX_RESUME,
	MLMUX_POWER_TX_POWER_UPDATE_ACK,
	MLMUX_POWER_TX_POWER_UPDATE_NACK,
};

enum mlmux_power_supply_chrg_state {
	MLMUX_POWER_SUPPLY_STATE_INIT,
	MLMUX_POWER_SUPPLY_STATE_UVLO_CHRG,
	MLMUX_POWER_SUPPLY_STATE_TRKL_CHRG,
	MLMUX_POWER_SUPPLY_STATE_LOW_PWR,
	MLMUX_POWER_SUPPLY_STATE_GOOD_PWR,
	MLMUX_POWER_SUPPLY_STATE_FAST_CHRG,
	MLMUX_POWER_SUPPLY_STATE_FACTORY,
	MLMUX_POWER_SUPPLY_STATE_BAT_UNKNOWN,
	MLMUX_POWER_SUPPLY_STATE_MAX,
};

enum mlmux_power_supply_type {
	MLMUX_POWER_SUPPLY_TYPE_UNKNOWN = 0,
	MLMUX_POWER_SUPPLY_TYPE_BATTERY,
	MLMUX_POWER_SUPPLY_TYPE_UPS,
	MLMUX_POWER_SUPPLY_TYPE_MAINS,
	MLMUX_POWER_SUPPLY_TYPE_USB, /* Standard Downstream Port */
	MLMUX_POWER_SUPPLY_TYPE_USB_DCP, /* Dedicated Charging Port */
	MLMUX_POWER_SUPPLY_TYPE_USB_CDP, /* Charging Downstream Port */
	MLMUX_POWER_SUPPLY_TYPE_USB_ACA, /* Accessory Charger Adapters */
	MLMUX_POWER_SUPPLY_TYPE_USB_TYPE_C, /* Type C Port */
	MLMUX_POWER_SUPPLY_TYPE_USB_PD, /* Power Delivery Port */
	MLMUX_POWER_SUPPLY_TYPE_USB_PD_DRP, /* PD Dual Role Port */
	MLMUX_POWER_SUPPLY_TYPE_APPLE_BRICK_ID, /* Apple Charging Method */
};

/**
 * struct mlmux_power_hdr - Header of the message
 *
 * @msg_type: Incoming message type
 */
struct mlmux_power_hdr {
	u8 msg_type;
} __packed;

/**
 * struct mlmux_power_init - Initial message from remote
 *
 * @rsvd: Reserved
 * @batt_capacity: Battery capacity (in mAh)
 * @batt_id_res: Battery ID resistor
 * @update_period_ms: Battery update period (in ms)
 *
 * Initial message from remote about battery
 */
struct mlmux_power_init {
	u8 rsvd;
	__le16 batt_capacity;
	__le32 batt_id_res;
	__le32 update_period_ms;
	__le32 unused[28];
} __packed;


/**
 * Bit 0 - Is battery present
 * Bit 1 - Is battery authenticated
 */
#define MLMUX_POWER_BAT_FLAG_IS_PRESENT			BIT(0)
#define MLMUX_POWER_BAT_FLAG_IS_AUTH			BIT(1)

/**
 * struct mlmux_power_status_battery - Battery status
 *
 * @flags: Flags about battery
 * @rsvd: Reserved
 * @soc: Battery state of charge (0-100%)
 * @vbat: Battery voltage (in mV)
 * @vbat_adc: battery ADC value
 * @vbat_max: Battery max voltage (in mV)
 * @batt_curr: Battery Current (in mA)
 * @batt_temp: Battery temperature (in decicelsius)
 * @ts1_temp: battery ts1 temperature (in decicelsius)
 * @ts2_temp: battery ts2 temperature (in decicelsius)
 * @batt_id: battery id resistor value (in ohms)
 * @power: vbat * current (in milliwatts)
 * @health: health of battery
 * @status: state of battery (e.g. charging, discharging)
 * @remaining_capacity: remaining capacity of battery (in mAh)
 * @full_charge_capacity: full charge capacity of battery (in mAh)
 * @design_capacity: design capacity (in mAh)
 */
struct mlmux_power_status_battery {
	u8 flags;
	u8 rsvd;
	u8 soc;
	__le16 vbat;
	__le16 vbat_adc;
	__le16 vbat_max;
	__le16 batt_curr;
	__le16 batt_temp;
	__le16 ts1_temp;
	__le16 ts2_temp;
	__le32 batt_id;
	__le32 power;
	u8 health;
	u8 status;
	__le16 remaining_capacity;
	__le16 full_charge_capacity;
	__le16 design_capacity;
} __packed;

/**
 * Bit 0 - Is charge enabled
 * Bit 1:4 - Charge State @enum mlmux_power_supply_chrg_state
 * Bit 5 - Charger capability mismatch
 * Bit 6 - Charger insufficient
 * Bit 7 - Cable connected state
 * Bit 8 - Cable orientation
 * Bit 9 - Request shutdown
 * Bit 10:13 - Power Suply Type @enum mlmux_power_supply_type
 * Bit 14 - Is DFP (Downstream Facing Port)
 */
#define MLMUX_POWER_CHARGE_FLAG_IS_ENABLED		BIT(0)
#define MLMUX_POWER_CHARGE_FLAG_STATE_MASK		(0x000F)
#define MLMUX_POWER_CHARGE_FLAG_STATE_LSB		(1)
#define MLMUX_POWER_CHARGE_FLAG_CAP_MISMATCH		BIT(5)
#define MLMUX_POWER_CHARGE_FLAG_INSUFFICIENT		BIT(6)
#define MLMUX_POWER_CHARGE_FLAG_CABLE_CONN		BIT(7)
#define MLMUX_POWER_CHARGE_FLAG_CABLE_ORIENT		BIT(8)
#define MLMUX_POWER_CHARGE_FLAG_REQ_SHUTDOWN		BIT(9)
#define MLMUX_POWER_CHARGE_FLAG_PSY_TYPE_MASK		(0x000F)
#define MLMUX_POWER_CHARGE_FLAG_PSY_TYPE_LSB		(10)
#define MLMUX_POWER_CHARGE_FLAG_IS_DFP			BIT(14)
#define MLMUX_POWER_CHARGE_ADAPTER_ALLOWED		BIT(15)

/**
 * struct mlmux_power_status_charger - Charger status
 *
 * @flags: Cherger related flags
 * @vbus: VBUS (in mV)
 * @adpt_curr: Adapter current (in mA)
 * @chrg_curr_max: Maximum charger current (in mA)
 * @chrg_temp: Charger temperature (in decicelsius)
 * @adpt_volt: adapter voltage (in mV)
 * @chrg_volt_max: maximum charger voltage (in mV)
 * @charger_health: health of charger
 */
struct mlmux_power_status_charger {
	__le16 flags;
	__le16 vbus;
	__le16 adpt_curr;
	__le16 adap_curr_max;
	__le16 chrg_temp;
	__le16 adap_volt_limit;
	__le16 adap_volt_max;
	u8 charger_health;
} __packed;


/**
 * struct mlmux_power_state - Battery, charger status
 * @battery: Battery status
 * @charger: Charger status
 */
struct mlmux_power_state {
	struct mlmux_power_status_battery battery;
	struct mlmux_power_status_charger charger;
} __packed;

/**
 * struct mlmux_power_ext_state - Extended battery/charger status
 * Whenever fields are added to this structure, the minimum
 * number of integers required to hold the fields must be
 * subtracted from the unused bank of 31 integers. This bank
 * started out at 124 available bytes; as that is the max
 * payload size allowed by mlmux. For example, if the existing
 * fields take up 8 bytes, then the unused field size should
 * be already be declared as unused[29]. If another byte, word,
 * or integer is to be added, the unused bank needs to be further
 * reduced to unused[28].
 *
 * @chgr_std: charger standard
 * @rsvd: reserved
 * @unused: unused
 */
struct mlmux_power_state_ext {
	__le16 chgr_std;
	u8 rsvd;
	__le16 batt_curr_avg;
	__le16 cycle_cnt;
	u8 epb_soc;
	__le32 unused[28];
} __packed;

/**
 * struct mlmux_power_ack - Power request ack
 *
 * @resp_code: Request success or failure
 * @rsvd: reserved
 * @unused: unused
 *
 * Response coming from remote for suspend or resume requests.
 */
struct mlmux_power_ack {
	u8 resp_code;
	u8 rsvd[2];
	__le32 unused[30];
} __packed;

struct mlmux_power_rx_pkt {
	struct mlmux_power_hdr hdr;
	union {
		struct mlmux_power_init init;
		struct mlmux_power_ack ack;
		struct mlmux_power_state state;
		struct mlmux_power_state_ext state_ext;
	} __packed;
} __packed;

struct mlmux_power_tx_pkt {
	struct mlmux_power_hdr hdr;
	union {
		struct mlmux_power_ack ack;
	} __packed;
} __packed;

#endif /* MLMUX_POWER_H */

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
#undef TRACE_SYSTEM
#define TRACE_SYSTEM mlmux_power

#if !defined(_TRACE_MLMUX_POWER_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MLMUX_POWER_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(battery_status,
	TP_PROTO(struct mlmux_power_status_battery *battery),
	TP_ARGS(battery),

	TP_STRUCT__entry(
		__field(uint8_t, flags)
		__field(uint8_t, soc)
		__field(uint16_t, vbat)
		__field(uint16_t, vbat_adc)
		__field(uint16_t, vbat_max)
		__field(uint16_t, batt_curr)
		__field(uint16_t, batt_temp)
	),

	TP_fast_assign(
		__entry->flags = battery->flags;
		__entry->soc = battery->soc;
		__entry->vbat = battery->vbat;
		__entry->vbat_adc = battery->vbat_adc;
		__entry->vbat_max = battery->vbat_max;
		__entry->batt_curr = battery->batt_curr;
		__entry->batt_temp = battery->batt_temp;
	),

	TP_printk("flags=0x%02x soc=%u vbat=%u vbat_adc=%u vbat_max=%u batt_curr=%d batt_temp=%d",
		  __entry->flags,
		  __entry->soc,
		  __entry->vbat,
		  __entry->vbat_adc,
		  __entry->vbat_max,
		  __entry->batt_curr,
		  __entry->batt_temp
	)

);

DEFINE_EVENT(battery_status, battery_status,
	TP_PROTO(struct mlmux_power_status_battery *battery),
	TP_ARGS(battery)
);

DECLARE_EVENT_CLASS(charger_status,
	TP_PROTO(struct mlmux_power_status_charger *charger),
	TP_ARGS(charger),

	TP_STRUCT__entry(
		__field(uint16_t, flags)
		__field(uint16_t, vbus)
		__field(uint16_t, adpt_curr)
		__field(uint16_t, adap_curr_max)
		__field(uint16_t, chrg_temp)
	),

	TP_fast_assign(
		__entry->flags = charger->flags;
		__entry->vbus = charger->vbus;
		__entry->adpt_curr = charger->adpt_curr;
		__entry->adap_curr_max = charger->adap_curr_max;
		__entry->chrg_temp = charger->chrg_temp;
	),

	TP_printk("flags=0x%04x vbus=%u adpt_curr=%u adap_curr_max=%u chrg_temp=%d",
		  __entry->flags,
		  __entry->vbus,
		  __entry->adpt_curr,
		  __entry->adap_curr_max,
		  __entry->chrg_temp
	)

);

DEFINE_EVENT(charger_status, charger_status,
	TP_PROTO(struct mlmux_power_status_charger *charger),
	TP_ARGS(charger)
);

DECLARE_EVENT_CLASS(ext_status,
	TP_PROTO(struct mlmux_power_state_ext *state_ext),
	TP_ARGS(state_ext),

	TP_STRUCT__entry(
		__field(uint16_t, chgr_std)
	),

	TP_fast_assign(
		__entry->chgr_std = state_ext->chgr_std;
	),

	TP_printk("chgr_std=%u",
		  __entry->chgr_std
	)

);

DEFINE_EVENT(ext_status, ext_status,
	TP_PROTO(struct mlmux_power_state_ext *state_ext),
	TP_ARGS(state_ext)
);

DECLARE_EVENT_CLASS(psy_init,
	TP_PROTO(struct mlmux_power_init *psy_init),
	TP_ARGS(psy_init),

	TP_STRUCT__entry(
		__field(uint16_t, batt_capacity)
		__field(uint32_t, batt_id_res)
		__field(uint32_t, update_period_ms)
	),

	TP_fast_assign(
		__entry->batt_capacity = psy_init->batt_capacity;
		__entry->batt_id_res = psy_init->batt_id_res;
		__entry->update_period_ms = psy_init->update_period_ms;
	),

	TP_printk("batt_capacity=%u batt_id_res=0x%04x update_period_ms=%u",
		  __entry->batt_capacity, __entry->batt_id_res,
		  __entry->update_period_ms
	)

);

DEFINE_EVENT(psy_init, psy_init,
	TP_PROTO(struct mlmux_power_init *psy_init),
	TP_ARGS(psy_init)
);

#endif /* _TRACE_MLMUX_POWER_H */

/* This part must be outside protection */
#include <trace/define_trace.h>


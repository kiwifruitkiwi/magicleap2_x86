/*
 * Copyright (C) 2010 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __CVIP_HARDWARE_L310_H
#define __CVIP_HARDWARE_L310_H

#define L310_CTRL			0x100
#define L310_AUX_CTRL			0x104
#define L310_TAG_LATENCY_CTRL		0x108
#define L310_DATA_LATENCY_CTRL		0x10C
#define L310_INTR_MASK			0x214
#define L310_MASKED_INTR_STAT		0x218
#define L310_RAW_INTR_STAT		0x21C
#define L310_INTR_CLEAR			0x220
#define L310_ADDR_FILTER_START		0xC00
#define L310_ADDR_FILTER_END		0xC04
#define L310_DEBUG_CTRL			0xF40
#define L310_PREFETCH_CTRL		0xF60
#define L310_POWER_CTRL			0xF80

#define L310_CTRL_EN				BIT(0)

#define L310_AUX_CTRL_FULL_LINE_ZERO		BIT(0)
#define L310_AUX_CTRL_HIGHPRIO_SO_DEV		BIT(10)
#define L310_AUX_CTRL_STORE_LIMITATION		BIT(11)
#define L310_AUX_CTRL_EXCLUSIVE_CACHE		BIT(12)
#define L310_AUX_CTRL_SHARED_INV		BIT(13)
#define L310_AUX_CTRL_ASSOC_MASK		BIT(16)
#define L310_AUX_CTRL_ASSOCIATIVITY		BIT(16)
#define L310_AUX_CTRL_WAY_SIZE_SHIFT		17
#define L310_AUX_CTRL_WAY_SIZE_MASK		(7 << 17)
#define L310_AUX_CTRL_WAY_SIZE(n)		((n) << 17)
#define L310_AUX_CTRL_EVTMON_ENABLE		BIT(20)
#define L310_AUX_CTRL_PARITY_ENABLE		BIT(21)
#define L310_AUX_CTRL_SHARED_OVERRIDE		BIT(22)
#define L310_AUX_CTRL_WR_ALLOCATE(n)		((n) << 23)
#define L310_AUX_CTRL_CACHE_REPLACE_RR		BIT(25)
#define L310_AUX_CTRL_NS_LOCKDOWN		BIT(26)
#define L310_AUX_CTRL_NS_INT_CTRL		BIT(27)
#define L310_AUX_CTRL_DATA_PREFETCH		BIT(28)
#define L310_AUX_CTRL_INSTR_PREFETCH		BIT(29)
#define L310_AUX_CTRL_EARLY_BRESP		BIT(30)

#define L310_LATENCY_CTRL_SETUP(n)		((n) << 0)
#define L310_LATENCY_CTRL_RD(n)			((n) << 4)
#define L310_LATENCY_CTRL_WR(n)			((n) << 8)

#define L310_DCL				BIT(0)
#define L310_DWB				BIT(1)
#define L310_SPNIDEN				BIT(2)

#define L310_PREFETCH_CTRL_OFFSET_MASK		0x1f
#define L310_PREFETCH_CTRL_DBL_LINEFILL_INCR	BIT(23)
#define L310_PREFETCH_CTRL_PREFETCH_DROP	BIT(24)
#define L310_PREFETCH_CTRL_DBL_LINEFILL_WRAP	BIT(27)
#define L310_PREFETCH_CTRL_DATA_PREFETCH	BIT(28)
#define L310_PREFETCH_CTRL_INSTR_PREFETCH	BIT(29)
#define L310_PREFETCH_CTRL_DBL_LINEFILL		BIT(30)

#define L310_ADDR_FILTER_EN			BIT(0)
#define L310_FILTER_START(n)			((n) << 20)
#define L310_FILTER_END(n)			((n) << 20)

#define L310_DYNAMIC_CLK_GATING_EN		BIT(1)
#define L310_STNDBY_MODE_EN			BIT(0)

#define L310_WAY_SIZE_SHIFT		3

#define L310_INTR_NUM			0x9
#define L310_INTR_STAT_MASK		((1 << L310_INTR_NUM) - 1)

#endif


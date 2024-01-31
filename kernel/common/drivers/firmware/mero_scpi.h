/*
 * Mero SCPI header file
 *
 * Copyright (C) 2010 ARM Ltd.
 * Modification Copyright (C) 2021, Advanced Micro Devices, Inc.
 * All rights reserved.
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

#ifndef __CVIP_SCPI_H__
#define __CVIP_SCPI_H__

#define MAX_DVFS_DOMAINS	8
#define SCPI_1MHZ		1000000

#define CORESIGHTCLK_RATE	100
#define CLOCK_OFF		0
#define DECOMPRESS_DPM0         1
#define DECOMPRESS_DPM1         2
#define DECOMPRESS_FREQ_DPM0    200
#define DECOMPRESS_FREQ_DPM1    600

enum cvip_clkids {
	A55DSUCLK = 0x0,
	A55TWOCLK,
	C5CLK,
	Q6CLK,
	NIC400CLK,
	CORESIGHTCLK,
	DECOMPRESSCLK,
	MAX_CVIPCLKS,
};

enum x86_clkids {
	CCLK0	= 0x0,
	CCLK1,
	CCLK2,
	CCLK3,
	GFXCLK,
	MAX_X86CLKS,
};

/**
 * struct scpi_drvinfo - mero scpi protocol data
 * @commands:   scpi commands
 * @scpi_ops:   mero scpi ops
 * @dvfs:       mero scpi dvfs tables
 * @dvfs_index: current dvfs opp index
 * @scpi_base:  scpi base address
 * @clkreq_list: clk request list
 * @evtlogid:   CVIP Event logging ID
 * @np:         device node of scpi
 */
struct scpi_drvinfo {
	int *commands;
	struct scpi_ops *scpi_ops;
	struct scpi_dvfs_info *dvfs[MAX_DVFS_DOMAINS];
	int dvfs_index[MAX_DVFS_DOMAINS];
	void __iomem *scpi_base;
	struct list_head clkreq_list;
	int evtlogid;
	struct device_node *np;
};

/*
 * struct scpi_clkreq - mero scpi clk request node
 * @mid:    clk request msg id
 * @arg:    argument for the clk request
 * @list:   list node to clkreq_list
 */
struct scpi_clkreq {
	u32 mid;
	u32 arg;
	struct list_head list;
};

#ifdef CONFIG_MERO_SCPI_PROTOCOL
int is_smu_dpm_ready(void);
void set_smu_dpm_ready(int state);
void set_smu_dpm_throttle(u32 event, u32 type);
#else
static inline int is_smu_dpm_ready(void)
{
	return 0;
}

static inline void set_smu_dpm_ready(int state)
{
}

static inline void set_smu_dpm_thottle(int event)
{
}
#endif

#endif /* __CVIP_SCPI_H__ */

/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "hal.h"
#include "log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][hal]"

#if !defined(ISP3_SILICON)
unsigned int isp_hw_reg_read32(unsigned int addr)
{
	uint32_t regVal = 0;

	if (addr & 0x3) {
		ISP_PR_ERR("%x fail align to 4B\n",
			addr);
	}

	if ((addr < 0x60000) ||
	((addr >= 0x70000) && (addr < 0xFFFFF))) {
		g_cgs_srv->ops->write_register(g_cgs_srv,
			((mmISP_IND_INDEX) >> 2), addr);
		regVal = g_cgs_srv->ops->read_register(g_cgs_srv,
			((mmISP_IND_DATA) >> 2));
	} else {
		regVal = g_cgs_srv->ops->read_register(g_cgs_srv,
			((addr & 0x0fffffff) >> 2));
	}

	return regVal;
};
void isp_hw_reg_write32(unsigned int addr, unsigned int value)
{
	if (addr == mmISP_POWER_STATUS) {
		ISP_PR_ERR("write mmISP_POWER_STATUS to 0x%x\n",
			value);
	}
	if (addr & 0x3) {
		ISP_PR_ERR("%x fail align to 4B\n",
			addr);
	}

	if ((addr < 0x60000)
	|| ((addr >= 0x70000) && (addr < 0xFFFFF))) {
		g_cgs_srv->ops->write_register(g_cgs_srv,
			((mmISP_IND_INDEX) >> 2), addr);
		g_cgs_srv->ops->write_register(g_cgs_srv,
			((mmISP_IND_DATA) >> 2), value);
	} else {
		g_cgs_srv->ops->write_register(g_cgs_srv,
			((addr & 0x0fffffff) >> 2), value);
		return;
	}
}
#endif

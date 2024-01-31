/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef ISP_PWR_H
#define ISP_PWR_H

#include "os_base_type.h"
#include "os_advance_type.h"

#define MS_TO_TIME_TICK(X) (((long long)(X)) * 10000)

#define PIXEL_SIZE_2M	(2000000)
#define PIXEL_SIZE_5M	(5000000)
#define PIXEL_SIZE_8M	(8000000)
#define PIXEL_SIZE_12M	(12000000)
#define PIXEL_SIZE_16M	(16000000)

struct isp_dpm_value {
	unsigned int soc_clk;
	unsigned int isp_iclk;
	unsigned int isp_xclk;
};

enum isp_dpm_level {
	ISP_DPM_LEVEL_0,
	ISP_DPM_LEVEL_1,
	ISP_DPM_LEVEL_2,
	ISP_DPM_LEVEL_3,
	ISP_DPM_LEVEL_4,
	ISP_DPM_LEVEL_5,
	ISP_DPM_LEVEL_6,
	ISP_DPM_LEVEL_7
};

#define DPM0_SOCCLK	400
#define DPM0_ISPXCLK	400
#define DPM0_ISPICLK	300

#define DPM1_SOCCLK	600
#define DPM1_ISPXCLK	600
#define DPM1_ISPICLK	513

#define DPM2_SOCCLK	745
#define DPM2_ISPXCLK	745
#define DPM2_ISPICLK	600

#define DPM3_SOCCLK	863
#define DPM3_ISPXCLK	863
#define DPM3_ISPICLK	800

#define DPM4_SOCCLK	965
#define DPM4_ISPXCLK	965
#define DPM4_ISPICLK	965

#define DPM5_SOCCLK	1093
#define DPM5_ISPXCLK	1093
#define DPM5_ISPICLK	1093

#define DPM6_SOCCLK	1171
#define DPM6_ISPXCLK	1171
#define DPM6_ISPICLK	1171

enum isp_pwr_unit_status {
	ISP_PWR_UNIT_STATUS_OFF,
	ISP_PWR_UNIT_STATUS_ON
};

/*isp power status set return*/
enum isp_pwr_ss_ret {
	ISP_PWR_SS_RET_SUCC_GO_ON,
	ISP_PWR_SS_RET_SUCC_NO_FURTHER,
	ISP_PWR_SS_RET_FAIL
};

struct isp_pwr_unit {
	enum isp_pwr_unit_status pwr_status;
	struct mutex pwr_status_mutex;
	long long on_time;
	long long idle_start_time;
};

void isp_pwr_unit_init(struct isp_pwr_unit *unit);
int isp_pwr_unit_should_pwroff(struct isp_pwr_unit *unit,
				unsigned int delay /*ms */);
int isp_ip_pwr_on(struct isp_context *isp, enum camera_id cid,
		unsigned int index, int hdr_enable);
int isp_ip_pwr_off(struct isp_context *isp);
int isp_req_clk(struct isp_context *isp, unsigned int sclk /*Mhz */,
		 unsigned int iclk /*Mhz */, unsigned int xclk /*Mhz */);
unsigned int isp_calc_clk_reg_value(unsigned int clk /*in Mhz */);
int isp_clk_change(struct isp_context *isp, enum camera_id cid,
			unsigned int index, int hdr_enable, int on);
int isp_dphy_pwr_on(struct isp_context *isp);
int isp_dphy_pwr_off(struct isp_context *isp);
void isp_acpi_fch_clk_enable(struct isp_context *isp, bool enable);
extern struct s_properties *g_prop;
#endif

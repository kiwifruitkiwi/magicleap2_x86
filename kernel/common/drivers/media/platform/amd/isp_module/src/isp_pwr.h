/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************************
 */

#ifndef ISP_PWR_H
#define ISP_PWR_H

#include "os_base_type.h"
#include "os_advance_type.h"

#define MS_TO_TIME_TICK(X) (((isp_time_tick_t)(X)) * 10000)

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
	isp_mutex_t pwr_status_mutex;
	isp_time_tick_t on_time;
	isp_time_tick_t idle_start_time;
};

void isp_pwr_unit_init(struct isp_pwr_unit *unit);
bool_t isp_pwr_unit_should_pwroff(struct isp_pwr_unit *unit,
				uint32 delay /*ms */);
result_t isp_ip_pwr_on(struct isp_context *isp, enum camera_id cid,
		uint32 index, bool_t hdr_enable);
result_t isp_ip_pwr_off(struct isp_context *isp);
result_t isp_req_clk(struct isp_context *isp, uint32 iclk /*Mhz */,
		uint32 sclk /*Mhz */, uint32 *actual_iclk,
		uint32 *actual_sclk);
uint32 isp_calc_clk_reg_value(uint32 clk /*in Mhz */);
result_t isp_clk_change(struct isp_context *isp, enum camera_id cid,
			uint32 index, bool_t hdr_enable, bool_t on);
result_t isp_dphy_pwr_on(struct isp_context *isp);
result_t isp_dphy_pwr_off(struct isp_context *isp);
void isp_acpi_fch_clk_enable(struct isp_context *isp, bool enable);

#endif

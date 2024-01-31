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

#include "isp_common.h"
#include "isp_pwr.h"
#include "log.h"
#include "isp_module_if_imp.h"
#include "cgs_common.h"
#include "cgs_linux.h"
#include <linux/acpi.h>
#include "i2c.h"

#define ISP_PGFSM_CONFIG 0x1BF00
#define ISP_PGFSM_READ_TILE1 0x1BF08
#define ISP_POWER_STATUS 0x60000

/*todocalculate the real clock needed in Mhz*/
void isp_clk_calculate(struct isp_context *isp, enum camera_id cid,
		 uint32 index, bool_t hdr_enable, bool_t other_cid_is_on,
		 uint32 *iclk, uint32 *sclk)
{
	if (iclk == NULL || sclk == NULL)
		return;
	switch (g_isp_env_setting) {
	case ISP_ENV_ALTERA:
		*iclk = 66;
		*sclk = 200;
		return;
	case ISP_ENV_MAXIMUS:
		*iclk = 100;
		*sclk = 200;
		return;
	default:
		break;
	};
#ifndef ENABLE_ISP_DYNAMIC_CLOCK
	isp = isp;
	cid = cid;
	index = index;
	hdr_enable = hdr_enable;
	other_cid_is_on = other_cid_is_on;
	*iclk = 685;
	*sclk = 685;
	return;
#else
	// Dual camera case
	if (other_cid_is_on) {
		*iclk = 685;
		*sclk = 685;
		return;
	}
	if (isp->sensor_info[cid].cam_type == CAMERA_TYPE_MEM) {//lookback case
		*iclk = 685;
		*sclk = 685;
		return;
	}


	uint32 pix_size = isp->sensor_info[cid].res_fps.res_fps[index].width
			*isp->sensor_info[cid].res_fps.res_fps[index].height;
//	uint32 fps = isp->sensor_info[cid].res_fps.res_fps[index].fps;

	hdr_enable = hdr_enable;
	if (pix_size < 3000000) {//2M sensor
		*iclk = 200;
		*sclk = 200;
	} else if (pix_size < 6000000) {//5M sensor
		*iclk = 200;
		*sclk = 200;
	} else if (pix_size < 9000000) {//8M sensor
		*iclk = 300;
		*sclk = 200;
	} else {
		*iclk = 480;
		*sclk = 200;
	}
	return;
#endif
}

result_t isp_clk_change(struct isp_context *isp, enum camera_id cid,
			uint32 index, bool_t hdr_enable, bool_t on)
{
	enum camera_id ocid;
	struct sensor_info *sif;
	struct sensor_info *osif;
	uint32 iclk;
	uint32 sclk;
	uint32 actual_iclk;
	uint32 actual_sclk;
	result_t ret = RET_SUCCESS;
	struct isp_pwr_unit *pwr_unit;

	pwr_unit = &isp->isp_pu_isp;
	switch (cid) {
	case CAMERA_ID_REAR:
		ocid = CAMERA_ID_FRONT_LEFT;
		break;
	case CAMERA_ID_FRONT_LEFT:
		ocid = CAMERA_ID_REAR;
		break;
	default:
		isp_dbg_print_err
			("-><- %s fail bad cid %u\n", __func__, cid);
		return RET_SUCCESS;
	};
	sif = &isp->sensor_info[cid];
	osif = &isp->sensor_info[ocid];
	if (!on && !osif->stream_inited) {
		isp_dbg_print_info
			("-><- %s suc do none cid:%u for pwr off\n",
			__func__, cid);
		return RET_SUCCESS;
	} else if (on && osif->stream_inited)
		isp_clk_calculate
			(isp, cid, index, hdr_enable, true, &iclk, &sclk);
	else if (on && !osif->stream_inited)
		isp_clk_calculate
			(isp, cid, index, hdr_enable, false, &iclk, &sclk);
	else {//!on && osif->stream_inited
		index =
		get_index_from_res_fps_id(isp, ocid, osif->cur_res_fps_id);
		isp_clk_calculate
		    (isp, ocid, index, osif->hdr_enable, false, &iclk, &sclk);
	};

	if ((isp->iclk == iclk) && (isp->sclk == sclk)
	&& (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON)) {
		isp_dbg_print_info
		("-><- %s, cid:%u succ no need set clk\n", __func__, cid);
		return RET_SUCCESS;
	};

	isp_dbg_print_info
		("-> %s, cid:%u set iclk:slck to %u,%u\n",
			__func__, cid, iclk, sclk);

	actual_iclk = iclk;
	actual_sclk = sclk;
	if (isp_req_clk(isp, iclk, sclk,
			&actual_iclk, &actual_sclk) != RET_SUCCESS) {
#ifdef IGNORE_PWR_REQ_FAIL
		isp_dbg_print_info
		("in %s, isp_req_clk fail ignore\n", __func__);
		{
			uint32 reg_sclk;
			uint32 reg_iclk;

			reg_iclk = isp_calc_clk_reg_value(iclk);
			reg_sclk = isp_calc_clk_reg_value(sclk);
			isp_hw_reg_write32(CLK0_CLK1_DFS_CNTL_n0, reg_sclk);
			isp_hw_reg_write32(CLK0_CLK3_DFS_CNTL_n1, reg_iclk);
			isp_dbg_print_info
			("in %s,CLK0_CLK1_DFS_CNTL_n0 to 0x%x\n",
			__func__, reg_sclk);
			isp_dbg_print_info
			("in %s,CLK0_CLK3_DFS_CNTL_n1 to 0x%x\n",
			__func__, reg_iclk);

		}
		isp_dbg_print_err
		("<- %s, fail set clk,but treat it as success\n", __func__);
#else
		ret = RET_FAILURE;
		isp_dbg_print_err("<- %s, fail set clk\n", __func__);
#endif

	} else {
		if (abs(iclk - actual_iclk) > (int32) (iclk / 10)) {
			isp_dbg_print_err("fail actual iclk too small\n");
		};
		if (abs(sclk - actual_sclk) > (int32) (sclk / 10)) {
			isp_dbg_print_err("fail actual sclk too small\n");
		};
		isp_dbg_print_info("<- %s suc\n", __func__);
	};
	isp->sclk = actual_sclk;
	isp->iclk = actual_iclk;
	isp->refclk = 24;//actual_iclk;
	return ret;

}


result_t isp_req_clk(struct isp_context *isp, uint32 iclk /*Mhz */,
		 uint32 sclk /*Mhz */, uint32 *actual_iclk,
		 uint32 *actual_sclk)
{
	//#define mmCLK0_CLK3_DFS_CNTL(0x0005B97C)
	uint32_t regVal = 0;
	uint32_t pgfsm_value;

	// Change iclk by changing register clk0_clk3_dfs_cntl_n1.CLK3_DIVIDER
	// The bigger of this value, the lower of the ICLK. 0x64= 100MHz
	// 0x64 - 100
	// 0x40 - 225
	// 0x30 - 300
	// 0x24 - 400
	// 0x20 - 450
	// 0x18 - 600
	// 0x15 - 685.7

	iclk = iclk * 1000;// change iclk to Mhz
	if ((iclk > 0) && (iclk <= 100*1000))
		regVal = 0x64;
	else if ((iclk > 100*1000) && (iclk <= 225*1000))
		regVal = 0x40;
	else if ((iclk > 225*1000) && (iclk <= 300*1000))
		regVal = 0x30;
	else if ((iclk > 300*1000) && (iclk <= 400*1000))
		regVal = 0x24;
	else if ((iclk > 400*1000) && (iclk <= 450*1000))
		regVal = 0x20;
	else if ((iclk > 450*1000) && (iclk <= 600*1000))
		regVal = 0x18;
	else if (iclk > 600*1000)
		regVal = 0x15;
	else {
		isp_dbg_print_err("%s: Invalid iclk = %d\n", __func__, iclk);
		return RET_INVALID_PARM;
	}
	//isp_hw_reg_write32(mmCLK0_CLK3_DFS_CNTL, regVal);

	isp_hw_reg_write32(CLK0_CLK1_DFS_CNTL_n0, 0x30); //regVal
	isp_hw_reg_write32(CLK0_CLK3_DFS_CNTL_n1, 0x30); //regVal
	pgfsm_value = isp_hw_reg_read32(CLK0_CLK3_DFS_CNTL_n1);

	isp_dbg_print_err
	("pgfsm write1 iclk CLK0_CLK3_DFS_CNTL_n1,0x%x, 0x%x\n",
							pgfsm_value, regVal);

	return RET_SUCCESS;
}

result_t isp_dphy_pwr_on(struct isp_context *isp)
{
	struct isp_pwr_unit *pwr_unit;

	pwr_unit = &isp->isp_pu_dphy;
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON) {
		isp_dbg_print_info("-><- %s succ, do none\n", __func__);
		return RET_SUCCESS;
	}

	{
		/*todo */
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;
		isp_dbg_print_info("-><- %s succ, todo later\n", __func__);
		return RET_SUCCESS;
	}
};

result_t isp_dphy_pwr_off(struct isp_context *isp)
{
	struct isp_pwr_unit *pwr_unit;

	pwr_unit = &isp->isp_pu_dphy;
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		isp_dbg_print_info("-><- %s succ, do none\n", __func__);
		return RET_SUCCESS;
	}

	{
		/*todo */
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
	}
	return RET_SUCCESS;
};

result_t isp_req_pwr(struct isp_context *isp, bool_t tilea_on, bool_t tileb_on)
{

#define RSMU_PGFSM_CMD_MASK 0x0000000e
#define RSMU_PGFSM_CMD_POWER_UP 0x2
	uint32 regVal = isp_hw_reg_read32(mmRSMU_PGFSM_CONTROL_ISP);

	if ((regVal & RSMU_PGFSM_CMD_MASK) != RSMU_PGFSM_CMD_POWER_UP) {
		// RSMU ISP Hard reset, through Addr/Data paired registers
		isp_hw_reg_write32
		(mmRSMU_SW_MMIO_PUB_IND_ADDR_0_ISP, mmRSMU_HARD_RESETB_ISP);
		isp_hw_reg_write32(mmRSMU_SW_MMIO_PUB_IND_DATA_0_ISP, 0);
		usleep_range(10000, 11000);
		isp_hw_reg_write32(mmRSMU_SW_MMIO_PUB_IND_DATA_0_ISP, 1);
		usleep_range(10000, 11000);

		// RSMU Power ON
		isp_hw_reg_write32(mmRSMU_PGFSM_CONTROL_ISP, 0x00000303);
		usleep_range(10000, 11000);

		// ISPPG ISP Power Status
		isp_hw_reg_write32(mmISP_POWER_STATUS, 0);
		usleep_range(10000, 11000);
	}

	return RET_SUCCESS;
}

/*refer to aidt_api_reset_all*/
void isp_hw_reset_all(struct isp_context *isp)
{
	//volatile uint32 regVal;
	//TODO FPGA2.0, add isp register reset
	uint32 regVal;

	isp_dbg_print_info("-> %s\n", __func__);
#define STALL_BIT_VAL 0x1
#define BUS_CLEAN_VAL 0x10
	//todo add later
	/*
	 *if (!isp_is_fpga())
	 *isp_hw_reg_write32(mmISP_POWER_STATUS, 0);
	 */

	//TODO FPGA2.0, add isp register reset
	regVal = isp_hw_reg_read32(mmISP_CORE_ISP_CTRL);
	regVal &= ~0x10;	//Disable input formater
	isp_hw_reg_write32(mmISP_CORE_ISP_CTRL, regVal);

	isp_hw_ccpu_disable(isp);

	isp_hw_reg_write32(mmISP_CORE_VI_IRCL, 0x1fff);
	isp_hw_reg_write32(mmISP_MIPI_VI_IRCL, 0xffff);
	isp_hw_reg_write32(mmISP_CORE_VI_IRCL, 0x0);
	isp_hw_reg_write32(mmISP_MIPI_VI_IRCL, 0x0);
	usleep_range(1000, 2000);

#define STALL_BIT_VAL 0x1
#define BUS_CLEAN_VAL 0x10
	isp_hw_reg_write32(mmISP_CORE_MI_MP_STALL, STALL_BIT_VAL);
	isp_hw_reg_write32(mmISP_CORE_MI_VP_STALL, STALL_BIT_VAL);
	isp_hw_reg_write32(mmISP_CORE_MI_PP_STALL, STALL_BIT_VAL);
	isp_hw_reg_write32(mmISP_CORE_MI_RD_DMA_STALL, STALL_BIT_VAL);
	isp_hw_reg_write32(mmISP_MIPI_MI_MIPI0_STALL, STALL_BIT_VAL);
	//isp_hw_reg_write32(mmISP_MIPI_MI_MIPI1_STALL, STALL_BIT_VAL);

	isp_dbg_print_info("<- %s\n", __func__);
}

uint32 isp_calc_clk_reg_value(uint32 clk /*in Mhz */)
{
	if (clk < 101)
		return 0x64;
	else if (clk < 219)
		return 0x41;
	else if (clk < 226)
		return 0x40;
	else if (clk < 253)
		return 0x39;
	else if (clk < 301)
		return 0x30;
	else if (clk < 401)
		return 0x24;
	else if (clk < 412)
		return 0x23;
	else if (clk < 424)
		return 0x22;
	else if (clk < 451)
		return 0x20;
	else if (clk < 481)
		return 0x1E;
	else if (clk < 515)
		return 0x1C;
	else if (clk < 555)
		return 0x1A;
	else if (clk < 601)
		return 0x18;
	else
		return 0x15;

}

result_t isp_ip_pwr_on(struct isp_context *isp, enum camera_id cid,
		 uint32 index, bool_t hdr_enable)
{
	result_t ret;
	struct isp_pwr_unit *pwr_unit;

	if (isp == NULL) {
		isp_dbg_print_err("-><- %s fail for null isp\n", __func__);
		return RET_INVALID_PARAM;
	}
	pwr_unit = &isp->isp_pu_isp;
	isp_dbg_print_info("-> %s\n", __func__);

	isp_mutex_lock(&pwr_unit->pwr_status_mutex);

	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		uint32 reg_value;
		uint32 pgfsm_value;

//test beging
//		isp_hw_reg_write32(mmRSMU_SW_MMIO_PUB_IND_ADDR_0_ISP,
//				 mmRSMU_HARD_RESETB_ISP);
		isp_hw_reg_write32(mmRSMU_HARD_RESETB_ISP, 0);
		usleep_range(1000, 2000);
		isp_hw_reg_write32(mmRSMU_HARD_RESETB_ISP, 1);
		usleep_range(1000, 2000);

		// RSMU Power ON
		isp_hw_reg_write32(mmRSMU_PGFSM_CONTROL_ISP, 0x00000303);
		usleep_range(1000, 2000);

		usleep_range(1000, 2000);
		pgfsm_value = isp_hw_reg_read32(mmRSMU_PGFSM_CONTROL_ISP);
		if (pgfsm_value != 0x303) {
			isp_dbg_print_err
				("pgfsm write1 fail,0x%x,should 0x303\n",
				pgfsm_value);
		};

//test end
		// ISPPG ISP Power Status
		isp_hw_reg_write32(mmISP_POWER_STATUS, 0);
		reg_value = isp_hw_reg_read32(mmISP_POWER_STATUS);
		if (reg_value != 0) {
			isp_dbg_print_err
			("in %s,fail update mmISP_POWER_STATUS %u, should 0\n",
			__func__, reg_value);
		} else {
			isp_dbg_print_info
			("in %s,update mmISP_POWER_STATUS to 0\n", __func__);
		}
	};
	if (isp_clk_change(isp, cid, index, hdr_enable, true) != RET_SUCCESS) {
		ret = RET_FAILURE;
		isp_dbg_print_info
				("<- %s, fail by isp_clk_change\n", __func__);
		goto quit;
	};
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		isp_acpi_fch_clk_enable(isp, 1);
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;
	}
	/*isp_test_reg_rw(isp); */
	if (ISP_GET_STATUS(isp) == ISP_STATUS_PWR_OFF) {
		isp_hw_reset_all(isp);
		isp_i2c_init(isp->iclk);
		isp_get_cur_time_tick(&pwr_unit->on_time);
		pwr_unit->idle_start_time =
				pwr_unit->on_time + MS_TO_TIME_TICK(2000);

		ISP_SET_STATUS(isp, ISP_STATUS_PWR_ON);
	}
	ret = RET_SUCCESS;
	isp_dbg_print_info("<- %s, succ\n", __func__);
quit:

	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	return ret;
};

result_t isp_ip_pwr_off(struct isp_context *isp)
{
	result_t ret;
	struct isp_pwr_unit *pwr_unit;
	uint32 reg_val;

	if (isp == NULL) {
		isp_dbg_print_err("-><- %s fail for null isp\n", __func__);
		return RET_INVALID_PARAM;
	}
	pwr_unit = &isp->isp_pu_isp;
	isp_dbg_print_info("-> %s\n", __func__);
	isp_unregister_isr(isp);
	isp_mutex_lock(&pwr_unit->pwr_status_mutex);
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		ret = RET_SUCCESS;
		isp_dbg_print_info("<- %s, succ do none\n", __func__);
	} else {
		isp_hw_ccpu_stall(isp);
		usleep_range(1000, 2000);
		/*hold ccpu reset */
		reg_val = isp_hw_reg_read32(mmISP_SOFT_RESET);
		reg_val |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
		isp_hw_reg_write32(mmISP_SOFT_RESET, 0x0);

		isp_acpi_fch_clk_enable(isp, 0);
		isp_hw_reg_write32(mmISP_POWER_STATUS, 1);

		isp_req_pwr(isp, 0, 0);
		//isp_req_clk(isp, 0, 0);
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		ISP_SET_STATUS(isp, ISP_STATUS_PWR_OFF);
		isp->sclk = 0;
		isp->iclk = 0;
		isp->refclk = 0;
		isp_dbg_print_info("<- %s, succ\n", __func__);
		ret = RET_SUCCESS;
	}
	isp->clk_info_set_2_fw = 0;
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	return ret;
};
void isp_acpi_fch_clk_enable(struct isp_context *isp, bool enable)
{
	uint32_t acpi_method;
	uint32_t acpi_function;
	uint32_t input_arg = 1;
	enum cgs_result result;

	//(uint32_t)COS_ACPI_TARGET_DISPLAY_ADAPTER;
	acpi_function = (uint32_t)0;
	if (enable)
		acpi_method = FOURCC('I', 'C', 'L', 'E');
	else
		acpi_method = FOURCC('I', 'C', 'L', 'D');
	result = g_cgs_srv->ops->call_acpi_method(g_cgs_srv, acpi_method,
		acpi_function, &input_arg, NULL, 0, 1/*sizeof(input_arg)*/, 0);
	if (result == CGS_RESULT__OK) {
		isp_dbg_print_info
		("-><- %s suc,enable %u\n", __func__, enable);
	} else {
		isp_dbg_print_err
		("-><- %s fail with %x,enable %u\n",
		__func__, result, enable);
	}
}

void isp_pwr_unit_init(struct isp_pwr_unit *unit)
{
	if (unit) {
		unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		isp_mutex_init(&unit->pwr_status_mutex);
		unit->on_time = 0;
		unit->on_time = MAX_ISP_TIME_TICK;
	}
}

bool_t isp_pwr_unit_should_pwroff(struct isp_pwr_unit *unit,
				  uint32 delay /*ms */)
{
	isp_time_tick_t tick;
	bool_t ret;

	if (unit == NULL)
		return false;
	isp_get_cur_time_tick(&tick);
	isp_mutex_lock(&unit->pwr_status_mutex);
	if (unit->pwr_status != ISP_PWR_UNIT_STATUS_ON) {
		ret = false;
	} else {
		if ((tick - MS_TO_TIME_TICK(delay)) > unit->idle_start_time)
			ret = true;
		else
			ret = false;
	}

	isp_mutex_unlock(&unit->pwr_status_mutex);
	return ret;
}

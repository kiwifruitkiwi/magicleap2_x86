/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "isp_pwr.h"
#include "log.h"
#include "isp_module_if_imp.h"
#include "cgs_common.h"
#include "cgs_linux.h"
#include <linux/acpi.h>
#include "i2c.h"
#include "porting.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][isp_pwr]"

#define ISP_PGFSM_CONFIG 0x1BF00
#define ISP_PGFSM_READ_TILE1 0x1BF08
#define ISP_POWER_STATUS 0x60000

struct isp_dpm_value dpm_value[] = {
	{DPM0_SOCCLK, DPM0_ISPICLK, DPM0_ISPXCLK},
	{DPM1_SOCCLK, DPM1_ISPICLK, DPM1_ISPXCLK},
	{DPM2_SOCCLK, DPM2_ISPICLK, DPM2_ISPXCLK},
	{DPM3_SOCCLK, DPM3_ISPICLK, DPM3_ISPXCLK},
	{DPM4_SOCCLK, DPM4_ISPICLK, DPM4_ISPXCLK},
	{DPM5_SOCCLK, DPM5_ISPICLK, DPM5_ISPXCLK},
	{DPM6_SOCCLK, DPM6_ISPICLK, DPM6_ISPXCLK}
};

/*calculate the real clock needed in Mhz*/
int isp_clk_calculate(struct isp_context __maybe_unused *isp,
	enum camera_id __maybe_unused cid,
	unsigned int __maybe_unused index,
	enum camera_id __maybe_unused ocid,
	unsigned int *sclk,
	unsigned int *iclk,
	unsigned int *xclk)
{
	unsigned int pix_size, opix_size;
	unsigned int fps, ofps;
	unsigned int ret = 0;
	unsigned int oindex;
	enum isp_dpm_level isp_dpm = ISP_DPM_LEVEL_0;

	if (!isp || !iclk || !xclk || !sclk) {
		ISP_PR_ERR("%s fail bad param\n", __func__);
		return RET_INVALID_PARAM;
	}

	switch (g_isp_env_setting) {
	case ISP_ENV_ALTERA:
		*iclk = 66;
		*xclk = 66;
		*sclk = 200;
		return ret;
	case ISP_ENV_MAXIMUS:
		*iclk = 100;
		*xclk = 100;
		*sclk = 100;
		return ret;
	default:
		break;
	}

#ifndef ENABLE_ISP_DYNAMIC_CLOCK
	pix_size = 0;
	opix_size = 0;
	fps = 0;
	ofps = 0;
	oindex = 0;
	isp_dpm = ISP_DPM_LEVEL_0;
	*iclk = DPM0_ISPICLK;
	*xclk = DPM0_ISPXCLK;
	*sclk = DPM0_SOCCLK;
	return ret;
#else
	pix_size =
			isp->sensor_info[cid].res_fps.res_fps[index].width
			* isp->sensor_info[cid].res_fps.res_fps[index].height;
	fps = isp->sensor_info[cid].res_fps.res_fps[index].fps;

	oindex = get_index_from_res_fps_id(isp, ocid,
				isp->sensor_info[ocid].cur_res_fps_id);
	opix_size =
			isp->sensor_info[ocid].res_fps.res_fps[oindex].width
			* isp->sensor_info[ocid].res_fps.res_fps[oindex].height;
	ofps = isp->sensor_info[ocid].res_fps.res_fps[oindex].fps;

	//single camera case and stream <= 30 fps
	if (!isp->sensor_info[ocid].stream_inited && fps <= 30) {
		if (pix_size < PIXEL_SIZE_8M)
			isp_dpm = ISP_DPM_LEVEL_0;
		else if (pix_size < PIXEL_SIZE_16M)
			isp_dpm = ISP_DPM_LEVEL_1;
		else
			isp_dpm = ISP_DPM_LEVEL_6;
	} else if (!isp->sensor_info[ocid].stream_inited && fps <= 60) {
		//single camera case and stream <= 60fps
		if (pix_size < PIXEL_SIZE_2M)
			isp_dpm = ISP_DPM_LEVEL_0;
		else if (pix_size < PIXEL_SIZE_5M)
			isp_dpm = ISP_DPM_LEVEL_1;
		else
			isp_dpm = ISP_DPM_LEVEL_6;
	} else if (!isp->sensor_info[ocid].stream_inited && fps > 60) {
		ISP_PR_ERR("%s don't support fps > 60\n", __func__);
		isp_dpm = ISP_DPM_LEVEL_6;
	}

	//multiple cameras case
	if (isp->sensor_info[ocid].stream_inited) {
		if (pix_size >= PIXEL_SIZE_12M)
			isp_dpm = ISP_DPM_LEVEL_6;
		else if (opix_size >= PIXEL_SIZE_12M)
			isp_dpm = ISP_DPM_LEVEL_6;
		else
			isp_dpm = ISP_DPM_LEVEL_4;
	}

	*sclk = dpm_value[isp_dpm].soc_clk;
	*iclk = dpm_value[isp_dpm].isp_iclk;
	*xclk = dpm_value[isp_dpm].isp_xclk;

	return ret;
#endif
}

int isp_clk_change(struct isp_context *isp, enum camera_id cid,
			unsigned int index, int hdr_enable, int on)
{
	enum camera_id ocid;
	unsigned int iclk;
	unsigned int sclk;
	unsigned int xclk;
	struct isp_pwr_unit *pwr_unit;
	int ret = RET_SUCCESS;

	pwr_unit = &isp->isp_pu_isp;
	switch (cid) {
	case CAMERA_ID_REAR:
		ocid = CAMERA_ID_FRONT_LEFT;
		break;
	case CAMERA_ID_FRONT_LEFT:
		ocid = CAMERA_ID_REAR;
		break;
	default:
		ISP_PR_ERR("fail bad cid %u\n", cid);
		return RET_SUCCESS;
	}
	if (on) {
		if (isp_clk_calculate
			(isp, cid, index, ocid, &sclk, &iclk, &xclk)) {
			ISP_PR_ERR("cid:%d, fail to calc isp clk\n", cid);
			return RET_FAILURE;
		}
	} else {
		sclk = 0;
		iclk = 0;
		xclk = 0;
	}
	if (isp->iclk == iclk &&
	    isp->sclk == sclk &&
	    pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON) {
		ISP_PR_INFO("cid:%u suc no need set clk\n", cid);
		return RET_SUCCESS;
	};

	ISP_PR_PC("cid:%u set sclk:iclk:xlck to %u,%u,%u\n",
		cid, sclk, iclk, xclk);

	if (isp_req_clk(isp, sclk, iclk, xclk) != RET_SUCCESS) {
#ifdef IGNORE_PWR_REQ_FAIL
		ISP_PR_INFO("isp_req_clk fail ignore\n");
		{
			unsigned int reg_sclk;
			unsigned int reg_iclk;
			unsigned int reg_xclk;

			reg_sclk = isp_calc_clk_reg_value(sclk);
			reg_iclk = isp_calc_clk_reg_value(iclk);
			reg_xclk = isp_calc_clk_reg_value(xclk);

#if	defined(ISP3_SILICON)
			isp_hw_reg_write32(CLK0_CLK2_BYPASS_CNTL_n0, 0x00);
			isp_hw_reg_write32(CLK0_CLK2_SLICE_CNTL_n0, 0x00001000);
			isp_hw_reg_write32(CLK0_CLK_DFSBYPASS_CONTROL_n0,
					0x00040000);
			isp_hw_reg_write32(CLK0_CLK3_BYPASS_CNTL_n0,
					0x00000000);
			isp_hw_reg_write32(CLK0_CLK3_SLICE_CNTL_n0, 0x00001000);
			isp_hw_reg_write32(CLK0_CLK_DFSBYPASS_CONTROL_n0,
					0x00080000);
			isp_hw_reg_write32(CLK0_CLK1_DFS_CNTL_n0, 0x14);//sclk
			isp_hw_reg_write32(CLK0_CLK2_DFS_CNTL_n0, 0x28);//iclk
			isp_hw_reg_write32(CLK0_CLK3_DFS_CNTL_n0, 0x28);//xclk
#else
			isp_hw_reg_write32(CLK0_CLK1_DFS_CNTL_n0, reg_sclk);
			isp_hw_reg_write32(CLK0_CLK3_DFS_CNTL_n1, reg_iclk);
			ISP_PR_DBG("reg_sclk to 0x%x\n",
				reg_sclk);
			ISP_PR_DBG("reg_iclk to 0x%x\n",
				reg_iclk);
#endif
		}
		ISP_PR_ERR("fail set clk,but treat it as success\n");
#else
		ret = RET_FAILURE;
		ISP_PR_ERR("fail set clk\n");
#endif
	}

	isp->sclk = sclk;
	isp->iclk = iclk;
	isp->xclk = xclk;
	isp->refclk = 24; //reference input;
	return ret;

}

int isp_set_clock_by_smu(unsigned int sclk, unsigned int iclk,
		unsigned int xclk)
{
	int ret = 0;

	if (g_swisp_svr.set_isp_clock) {
		ret = g_swisp_svr.set_isp_clock(sclk, iclk, xclk);
		if (ret != 0) {
			ISP_PR_ERR("%s fail! ret:%d, sclk:%d, iclk:%d, xclk:%d",
				__func__, ret, sclk, iclk, xclk);
		}
	} else {
		ret = -1;
		ISP_PR_ERR("%s don't support, sclk:%d, iclk:%d, xclk:%d",
			__func__, sclk, iclk, xclk);
	}

	return ret;
}

int isp_req_clk(struct isp_context *isp, unsigned int sclk /*Mhz */,
		 unsigned int iclk /*Mhz */, unsigned int xclk /*Mhz */)
{
#if	defined(ISP3_SILICON)
	if (isp_set_clock_by_smu(sclk, iclk, xclk)) {
		ISP_PR_ERR("set clock fail, sclk:%d, iclk:%d, xclk:%d\n",
			sclk, iclk, xclk);
		return RET_FAILURE;
	}
#else
	unsigned int regVal = 0;
	unsigned int pgfsm_value;

	isp_hw_reg_write32(CLK0_CLK1_DFS_CNTL_n0, 0x30); //regVal
	isp_hw_reg_write32(CLK0_CLK3_DFS_CNTL_n1, 0x30); //regVal

	pgfsm_value = isp_hw_reg_read32(CLK0_CLK3_DFS_CNTL_n1);

	ISP_PR_DBG("pgfsm write1 iclk CLK0_CLK3_DFS_CNTL_n1,0x%x, 0x%x\n",
		pgfsm_value, regVal);
#endif

	return RET_SUCCESS;
}

int isp_dphy_pwr_on(struct isp_context *isp)
{
	struct isp_pwr_unit *pwr_unit;

	pwr_unit = &isp->isp_pu_dphy;
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON) {
		ISP_PR_INFO("suc, do none\n");
		return RET_SUCCESS;
	}

	{
		/*todo */
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;
		ISP_PR_WARN("not implement yet\n");
		return RET_SUCCESS;
	}
};

int isp_dphy_pwr_off(struct isp_context *isp)
{
	struct isp_pwr_unit *pwr_unit;

	pwr_unit = &isp->isp_pu_dphy;
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		ISP_PR_INFO("suc, do none\n");
		return RET_SUCCESS;
	}

	{
		/*todo */
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
	}
	return RET_SUCCESS;
};

int isp_req_pwr(struct isp_context *isp, int tilea_on, int tileb_on)
{
	return RET_SUCCESS;
}

/*refer to aidt_api_reset_all*/
void isp_hw_reset_all(struct isp_context *isp)
{
	ENTER();

	aidt_api_disable_ccpu();

	aidt_hal_write_reg(mmISP_MIPI_IRCL, ~0);
	aidt_hal_write_reg(mmISP_MIPI_IRCL, 0x0);
	usleep_range(10000, 11000);

	/* enable ISP clock */
	SET_REG(ISP_CLOCK_GATING_A, ~0);
	SET_REG(ISP_MIPI_ICCL, ~0);
	SET_REG(ISP_MIPI_PIPE0_ICCL, ~0);
	SET_REG(ISP_MIPI_PIPE1_ICCL, ~0);
	SET_REG(ISP_MIPI_PIPE2_ICCL, ~0);
	usleep_range(10000, 11000);

	EXIT();
}


unsigned int isp_calc_clk_reg_value(unsigned int clk /*in Mhz */)
{
	if (clk < 101)
		return 0x78;
	else if (clk < 219)
		return 0x36;
	else if (clk < 226)
		return 0x35;
	else if (clk < 253)
		return 0x2f;
	else if (clk < 301)
		return 0x28;
	else if (clk < 401)
		return 0x1e;
	else if (clk < 412)
		return 0x1d;
	else if (clk < 424)
		return 0x1c;
	else if (clk < 451)
		return 0x1a;
	else if (clk < 481)
		return 0x18;
	else if (clk < 515)
		return 0x17;
	else if (clk < 555)
		return 0x15;
	else if (clk < 601)
		return 0x14;
	else if (clk < 746)
		return 0x10;
	else if (clk < 801)
		return 0x0f;
	else if (clk < 864)
		return 0x0d;
	else if (clk < 966)
		return 0x0c;
	else
		return 0x0a;

}

int isp_ip_power_set_by_smu(uint8_t enable)
{
	int ret = 0;

	if (g_swisp_svr.set_isp_power) {
		ret = g_swisp_svr.set_isp_power(enable);
		if (ret != 0) {
			ISP_PR_ERR("%s fail! ret: %d, enable:%d",
				__func__, ret, enable);
		}
	} else {
		ret = -1;
		ISP_PR_ERR("%s don't support, enable:%d",
				__func__, enable);
	}

	return ret;
}

int isp_ip_power_on(void)
{
	uint32_t regVal = 0;

	//reset ISP fuse
	regVal = 0x13;
	isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX),
			RSMU_SMS_FUSE_CFG_ISP_rsmuISP);
	isp_hw_reg_write32((NBIF_GPU_PCIE_DATA),
			regVal);

	regVal = 0x12;
	isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX),
			RSMU_SMS_FUSE_CFG_ISP_rsmuISP);
	isp_hw_reg_write32((NBIF_GPU_PCIE_DATA),
			regVal);

	//set HDP_HOST_PATH_CNTL/HDP_XDP_CHKN
	regVal = 0x80680000;
	isp_hw_reg_write32(HDP_HOST_PATH_CNTL, regVal);

	regVal = 0x48584454;
	isp_hw_reg_write32(HDP_XDP_CHKN, regVal);

	//Initial Power status check
	ISP_PR_INFO("Initial Power status check\n");

	isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX),
			RSMU_PGFSM_STATUS_ISP_rsmuISP);
	regVal = isp_hw_reg_read32(NBIF_GPU_PCIE_DATA);
	ISP_PR_INFO("read RSMU_PGFSM_STATUS_ISP_rsmuISP(0x%x)[0x%x]\n",
		RSMU_PGFSM_STATUS_ISP_rsmuISP, regVal);

	do {
		//msleep(30);
		//Power on 1st time
		ISP_PR_INFO("Power on 1st time\n");
		regVal = 0x1;
		isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX),
				RSMU_PGFSM_CONTROL_ISP_rsmuISP);
		isp_hw_reg_write32((NBIF_GPU_PCIE_DATA),
				regVal);

		regVal = 0x1f01;
		isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX),
				RSMU_PGFSM_CONTROL_ISP_rsmuISP);
		isp_hw_reg_write32((NBIF_GPU_PCIE_DATA),
				regVal);

		regVal = 0x1f03;
		isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX),
				RSMU_PGFSM_CONTROL_ISP_rsmuISP);
		isp_hw_reg_write32((NBIF_GPU_PCIE_DATA),
				regVal);

		//Power status check
		ISP_PR_INFO("Power status check\n");

		isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX),
				RSMU_PGFSM_STATUS_ISP_rsmuISP);
		regVal = isp_hw_reg_read32(NBIF_GPU_PCIE_DATA);
		ISP_PR_DBG("read RSMU_PGFSM_STATUS_ISP_rsmuISP(0x%x)[0x%x]\n",
			RSMU_PGFSM_STATUS_ISP_rsmuISP, regVal);
	} while (regVal != 0x00);

	regVal = 0x3;
	isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX), RSMU_HARD_RESETB_ISP_rsmuISP);
	isp_hw_reg_write32((NBIF_GPU_PCIE_DATA), regVal);

	regVal = 0x1;
	isp_hw_reg_write32((NBIF_GPU_PCIE_INDEX), RSMU_COLD_RESETB_ISP_rsmuISP);
	isp_hw_reg_write32((NBIF_GPU_PCIE_DATA), regVal);

	return 0;
}
int isp_ip_pwr_on(struct isp_context *isp, enum camera_id cid,
		 unsigned int index, int hdr_enable)
{
	int ret;
	struct isp_pwr_unit *pwr_unit;

	if (isp == NULL) {
		ISP_PR_ERR("fail for null isp\n");
		return RET_INVALID_PARAM;
	}
	pwr_unit = &isp->isp_pu_isp;

	if (isp_start_resp_proc_threads(isp) != RET_SUCCESS) {
		ISP_PR_ERR("isp_start_resp_proc_threads fail\n");
		return RET_FAILURE;
	}
	ISP_PR_INFO("isp start resp proc threads suc\n");

	isp_mutex_lock(&pwr_unit->pwr_status_mutex);

	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		//unsigned int reg_value;
	//	unsigned int pgfsm_value;
#if	defined(ISP3_SILICON)
		isp_ip_power_set_by_smu(true);
#endif
		// ISPPG ISP Power Status
		isp_hw_reg_write32(mmISP_POWER_STATUS, 0x07);
		if (1) {
			unsigned int ver = isp_hw_reg_read32(mmISP_VERSION);

			ISP_PR_INFO("hw ver 0x%x\n", ver);
			//isp_hw_reg_write32(mmISP_STATUS, 0xffffffff);

			ver = isp_hw_reg_read32(mmISP_STATUS);

			ISP_PR_DBG("mmISP status  0x%x\n", ver);
		}
	};

	if (isp_clk_change(isp, cid, index, hdr_enable, true) != RET_SUCCESS) {
		ret = RET_FAILURE;
		ISP_PR_ERR("fail by isp_clk_change\n");
		goto quit;
	};
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		isp_acpi_fch_clk_enable(isp, 1);
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;
	}

	if (ISP_GET_STATUS(isp) == ISP_STATUS_PWR_OFF) {
		isp_hw_reset_all(isp);
		//should be 24M
		if (isp->refclk != 24) {
			ISP_PR_ERR("fail isp->refclk %u should be 24\n",
				isp->refclk);
		}
		//isp_i2c_init(isp->iclk);
		//change to following according to aidt
		isp_i2c_init(isp->refclk);
		isp_get_cur_time_tick(&pwr_unit->on_time);
		pwr_unit->idle_start_time =
				pwr_unit->on_time + MS_TO_TIME_TICK(2000);

		ISP_SET_STATUS(isp, ISP_STATUS_PWR_ON);
	}
	ret = RET_SUCCESS;
	ISP_PR_PC("ISP Power on suc!\n");
quit:

	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	return ret;
};

int isp_ip_pwr_off(struct isp_context *isp)
{
	int ret;
	struct isp_pwr_unit *pwr_unit;
	unsigned int reg_val;

	if (isp == NULL) {
		ISP_PR_ERR("fail for null isp\n");
		return RET_INVALID_PARAM;
	}
	pwr_unit = &isp->isp_pu_isp;
//	isp_unregister_isr(isp);

	isp_stop_resp_proc_threads(isp);
	ISP_PR_INFO("isp stop resp proc streads suc");

	isp_mutex_lock(&pwr_unit->pwr_status_mutex);
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		ret = RET_SUCCESS;
		ISP_PR_INFO("suc do none\n");
	} else {
		//isp_hw_ccpu_stall(isp);
		//msleep(1);
		/*hold ccpu reset */
		reg_val = isp_hw_reg_read32(mmISP_SOFT_RESET);
		reg_val |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
		isp_hw_reg_write32(mmISP_SOFT_RESET, 0x0);

		isp_acpi_fch_clk_enable(isp, 0);
		isp_hw_reg_write32(mmISP_POWER_STATUS, 0);

		if (g_prop && (g_prop->disable_isp_power_tile == 1)) {
			ISP_PR_PC("disable isp power tile");
			isp_ip_power_set_by_smu(false);
		}

		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		ISP_SET_STATUS(isp, ISP_STATUS_PWR_OFF);
		isp->sclk = 0;
		isp->iclk = 0;
		isp->xclk = 0;
		isp->refclk = 0;
		ISP_PR_PC("ISP Power off suc!\n");
		ret = RET_SUCCESS;
	}
	isp->clk_info_set_2_fw = 0;
	isp->snr_info_set_2_fw = 0;
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	return ret;
};
void isp_acpi_fch_clk_enable(struct isp_context *isp, bool enable)
{
	uint32_t acpi_method;
	uint32_t acpi_function;
//	uint32_t input_arg = 1;
//	enum cgs_result result;

	if (isp_is_fpga()) {
		ISP_PR_INFO("skip for fpga, suc");
		return;
	}

	//(uint32_t)COS_ACPI_TARGET_DISPLAY_ADAPTER;
	acpi_function = (uint32_t)0;
	if (enable)
		acpi_method = FOURCC('I', 'C', 'L', 'E');
	else
		acpi_method = FOURCC('I', 'C', 'L', 'D');
//comment this out since the acpi interfaces not supported yet.
//	result = g_cgs_srv->ops->call_acpi_method(g_cgs_srv, acpi_method,
//		acpi_function, &input_arg, NULL, 0, 1, 0);
//	if (result == CGS_RESULT__OK) {
		//add this line to trick CP
//		ISP_PR_INFO("suc,enable %u\n", enable);
//	} else {
		//add this line to trick CP
//		ISP_PR_ERR("fail with %x,enable %u\n", result, enable);
//	}
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

int isp_pwr_unit_should_pwroff(struct isp_pwr_unit *unit,
				  unsigned int delay /*ms */)
{
	long long tick;
	int ret;

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

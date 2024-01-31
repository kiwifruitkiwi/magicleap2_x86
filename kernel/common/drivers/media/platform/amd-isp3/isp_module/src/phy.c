/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "phy.h"
#include "porting.h"
#include "log.h"
#include "hal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][phy]"

//refer to start_internal_synopsys_phy
#define ENABLE_MIPI_PHY_DDL_TUNING
#define MIPI_PHY0_REG_BASE (mmISP_MIPI_PHY0_REG0)
#define MIPI_PHY1_REG_BASE (mmISP_MIPI_PHY1_REG0)

enum _mipi_phy_id_t {
	MIPI_PHY_ID_0 = 0,
	MIPI_PHY_ID_1 = 1,
	MIPI_PHY_ID_MAX
};

struct _mipi_phy_reg_t {
	unsigned int isp_phy_cfg_reg0;
	unsigned int isp_phy_cfg_reg1;
	unsigned int isp_phy_cfg_reg2;
	unsigned int isp_phy_cfg_reg3;
	unsigned int isp_phy_cfg_reg4;
	unsigned int isp_phy_cfg_reg5;
	unsigned int isp_phy_cfg_reg6;
	unsigned int isp_phy_cfg_reg7;
	unsigned int isp_phy_testinterf_cmd;
	unsigned int isp_phy_testinterf_ack;
};

struct _mipi_phy_reg_t *g_isp_mipi_phy_reg_base[MIPI_PHY_ID_MAX] = {
	(struct _mipi_phy_reg_t *) MIPI_PHY0_REG_BASE,
	(struct _mipi_phy_reg_t *) MIPI_PHY1_REG_BASE,
};

const uint32_t g_isp_mipi_status_mask[4] = { 0x1, 0x3, 0x7, 0xf };

#define MODIFYFLD(var, reg, field, val) \
	(var = (var & (~reg##__##field##_MASK)) | \
	(((unsigned int)(val) << reg##__##field##__SHIFT) &\
	reg##__##field##_MASK))


//refer to start_internal_synopsys_phy
//bit_rate is in unit of Mbit/s
#define g_mipi_phy_reg_base g_isp_mipi_phy_reg_base
#define mipi_status_mask g_isp_mipi_status_mask
int start_internal_synopsys_phy(
	struct isp_context __maybe_unused *pcontext,
	unsigned int cam_id,
	unsigned int bit_rate/*in Mbps */,
	unsigned int lane_num)
{
	uint32_t regVal;
	uint32_t timeout = 0;
	uint32_t phy_reg_check_timeout = 10;	// 1 second
	uint32_t cmd_data;
	uint32_t mipi_phy_id;
	uint32_t rxclk_caldone;
	uint32_t rxclk_errcal;
	u32 force_value;
	//this value is used to change the Phy impedance,
	//it can be change from 0x0 to 0xF

	//=================================================
	//   Start Programming
	//=================================================

	// program PHY0
	if (cam_id == CAMERA_ID_REAR)
		mipi_phy_id = 0;
	else
		mipi_phy_id = 1;

	if (mipi_phy_id == 0) {
		// shutdown MIPI PHY
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG0, shutdownz, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG0, rstz, 0x0);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg0), regVal);

		// set testclr to 0x1
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG7, testclr, 0x1);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg7), regVal);

		usleep_range(5000, 6000);

		// set testclr to 0x0
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG7, testclr, 0x0);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg7), regVal);
	} else if (mipi_phy_id == 1) {
		// shutdown MIPI PHY
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY1_REG0, shutdownz, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY1_REG0, rstz, 0x0);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg0), regVal);

		// set testclr to 0x1
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY1_REG7, testclr, 0x1);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg7), regVal);

		usleep_range(5000, 6000);

		// set testclr to 0x0
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY1_REG7, testclr, 0x0);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg7), regVal);
	} else {
		ISP_PR_ERR("MIPI PHY %d not supported yet\n", mipi_phy_id);
		return RET_FAILURE;
	}
	// program hsfreqrange
	regVal = 0x0;
	if (bit_rate >= 80 && bit_rate <= 107.625)
		regVal = 0b0010000; //90   Mbps 432 osc_freq_target
	else if (bit_rate >= 83.125 && bit_rate <= 118.125)
		regVal = 0b0100000; //100  Mbps 432
	else if (bit_rate >= 92.625 && bit_rate <= 128.625)
		regVal = 0b0110000; //110  Mbps 432
	else if (bit_rate >= 102.125 && bit_rate <= 139.125)
		regVal = 0b0000001; //120  Mbps 432
	else if (bit_rate >= 111.625 && bit_rate <= 149.625)
		regVal = 0b0010001; //130  Mbps 432
	else if (bit_rate >= 121.125 && bit_rate <= 160.125)
		regVal = 0b0100001; //140  Mbps 432
	else if (bit_rate >= 130.625 && bit_rate <= 170.625)
		regVal = 0b0110001; //150  Mbps 432
	else if (bit_rate >= 140.125 && bit_rate <= 181.125)
		regVal = 0b0000010; //160  Mbps 432
	else if (bit_rate >= 149.625 && bit_rate <= 191.625)
		regVal = 0b0010010; //170  Mbps 432
	else if (bit_rate >= 159.125 && bit_rate <= 202.125)
		regVal = 0b0100010; //180  Mbps 432
	else if (bit_rate >= 168.625 && bit_rate <= 212.625)
		regVal = 0b0110010; //190  Mbps 432
	else if (bit_rate >= 182.875 && bit_rate <= 228.375)
		regVal = 0b0000011; //205  Mbps 432
	else if (bit_rate >= 197.125 && bit_rate <= 244.125)
		regVal = 0b0010011; //220  Mbps 432
	else if (bit_rate >= 211.375 && bit_rate <= 259.875)
		regVal = 0b0100011; //235  Mbps 432
	else if (bit_rate >= 225.625 && bit_rate <= 275.625)
		regVal = 0b0110011; //250  Mbps 432
	else if (bit_rate >= 249.375 && bit_rate <= 301.875)
		regVal = 0b0000100; //275  Mbps 432
	else if (bit_rate >= 273.125 && bit_rate <= 328.125)
		regVal = 0b0010100; //300  Mbps 432
	else if (bit_rate >= 296.875 && bit_rate <= 354.375)
		regVal = 0b0100101; //325  Mbps 432
	else if (bit_rate >= 320.625 && bit_rate <= 380.625)
		regVal = 0b0110101; //350  Mbps 432
	else if (bit_rate >= 368.125 && bit_rate <= 433.125)
		regVal = 0b0000101; //400  Mbps 432
	else if (bit_rate >= 415.625 && bit_rate <= 485.625)
		regVal = 0b0010110; //450  Mbps 432
	else if (bit_rate >= 463.125 && bit_rate <= 538.125)
		regVal = 0b0100110; //500  Mbps 432
	else if (bit_rate >= 510.625 && bit_rate <= 590.625)
		regVal = 0b0110111; //550  Mbps 432
	else if (bit_rate >= 558.125 && bit_rate <= 643.125)
		regVal = 0b0000111; //600  Mbps 432
	else if (bit_rate >= 605.625 && bit_rate <= 695.625)
		regVal = 0b0011000; //650  Mbps 432
	else if (bit_rate >= 653.125 && bit_rate <= 748.125)
		regVal = 0b0101000; //700  Mbps 432
	else if (bit_rate >= 700.625 && bit_rate <= 800.625)
		regVal = 0b0111001; //750  Mbps 432
	else if (bit_rate >= 748.125 && bit_rate <= 853.125)
		regVal = 0b0001001; //800  Mbps 432
	else if (bit_rate >= 795.625 && bit_rate <= 905.625)
		regVal = 0b0011001; //850  Mbps 432
	else if (bit_rate >= 843.125 && bit_rate <= 958.125)
		regVal = 0b0101001; //900  Mbps 432
	else if (bit_rate >= 890.625 && bit_rate <= 1010.625)
		regVal = 0b0111010; //950  Mbps 432
	else if (bit_rate >= 938.125 && bit_rate <= 1063.125)
		regVal = 0b0001010; //1000 Mbps 432
	else if (bit_rate >= 985.625 && bit_rate <= 1115.625)
		regVal = 0b0011010; //1050 Mbps 432
	else if (bit_rate >= 1033.125 && bit_rate <= 1168.125)
		regVal = 0b0101010; //1100 Mbps 432
	else if (bit_rate >= 1080.625 && bit_rate <= 1220.625)
		regVal = 0b0111011; //1150 Mbps 432
	else if (bit_rate >= 1128.125 && bit_rate <= 1273.125)
		regVal = 0b0001011; //1200 Mbps 432
	else if (bit_rate >= 1175.625 && bit_rate <= 1325.625)
		regVal = 0b0011011; //1250 Mbps 432
	else if (bit_rate >= 1223.125 && bit_rate <= 1378.125)
		regVal = 0b0101011; //1300 Mbps 432
	else if (bit_rate >= 1270.625 && bit_rate <= 1430.625)
		regVal = 0b0111100; //1350 Mbps 432
	else if (bit_rate >= 1318.125 && bit_rate <= 1483.125)
		regVal = 0b0001100; //1400 Mbps 432
	else if (bit_rate >= 1365.625 && bit_rate <= 1535.625)
		regVal = 0b0011100; //1450 Mbps 432
	else if (bit_rate >= 1413.125 && bit_rate <= 1588.125)
		regVal = 0b0101100; //1500 Mbps 432
	else {
		ISP_PR_ERR("Invalid data bitrate for sensor\n");
		return RET_FAILURE;
	}
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg2), regVal);

#ifdef ENABLE_MIPI_PHY_DDL_TUNING
	// config DDL tuning
	if ((mipi_phy_id == 0 && lane_num == 4) ||
		(mipi_phy_id == 1 && lane_num == 4)) {
		// read 0xE4 register

		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xE4);
		aidt_hal_write_reg((uint64_t)(
	&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd), regVal);

		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}
			usleep_range(9000, 10000);
			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when reading register 0xE4");
			return RET_FAILURE;
		}
		cmd_data = regVal &
			ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// write 0xE4 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xE4);
		MODIFYFLD(regVal,
		ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA, cmd_data | 0x1);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);
			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0xE4");
			return RET_FAILURE;
		}

		// write 0xE2 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_ADDR, 0xE2);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_WDATA, 0xB0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);
			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0xE2");
			return RET_FAILURE;
		}

		// write 0xE3 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xE3);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x1);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0xE3");
			return RET_FAILURE;
		}
	} else if ((mipi_phy_id == 0 && lane_num == 2) ||
		(mipi_phy_id == 1 && lane_num == 2)) {
		// case for MIPI PHY0/PHY1 with 2-lane enabled

		// read 0x60C register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_ADDR, 0x60C);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when reading register 0x60C");
			return RET_FAILURE;
		}
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// write 0x60C register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
				0x60C);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA,
		cmd_data | 0x1);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0x60C");
			return RET_FAILURE;
		}

		// write 0x60A register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_ADDR, 0x60A);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_WDATA, 0xB0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0x60A");
			return RET_FAILURE;
		}

		// write 0x60B register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60B);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x1);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0x60B");
			return RET_FAILURE;
		}

		// read 0x80C register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x80C);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when reading register 0x80C");
			return RET_FAILURE;
		}
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// write 0x80C register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x80C);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA,
		cmd_data | 0x1);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0x80C");
			return RET_FAILURE;
		}

		// write 0x80A register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x80A);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA,
		0xb0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0x80A");
			return RET_FAILURE;
		}

		// write 0x80B register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x80B);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x1);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR(
		"Timeout when writing register 0x80B");
			return RET_FAILURE;
		}
	} else if (lane_num == 1) {
	// case for PHY0/PHY1 with 1-lane enabled

		// read 0x60C register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x60C);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
			ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when reading register 0x60C");
			return RET_FAILURE;
		}
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// write 0x60C register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60C);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA,
			cmd_data | 0x1);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0x60C");
			return RET_FAILURE;
		}

		// write 0x60A register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_ADDR, 0x60A);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_WDATA, 0xb0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0x60A");
			return RET_FAILURE;
		}

		// write 0x60B register
		regVal = 0x0;
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x60B);
		MODIFYFLD(regVal,
			ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x1);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				ISP_PR_INFO(
		"Cmd finished with read count = %d\n", timeout);
				break;
			}

			usleep_range(9000, 10000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			ISP_PR_ERR("Timeout when writing register 0x60B");
			return RET_FAILURE;
		}
	} else {
		ISP_PR_ERR("Invalid lane num configuration");
		return RET_FAILURE;
	}
#endif

	//read register 0x8
	regVal = 0x0;
	MODIFYFLD(regVal,
		ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
	MODIFYFLD(regVal,
		ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x08);
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

	while (timeout < phy_reg_check_timeout) {
		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		if (regVal & ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
			ISP_PR_INFO("Cmd finished with read count = %d\n",
				timeout);
			break;
		}
		aidt_sleep(10);
		timeout = timeout + 1;
	}
	if (timeout == phy_reg_check_timeout) {
		ISP_PR_ERR("Timeout when reading register 0xE4");
		return RET_FAILURE;
	}
	cmd_data = regVal & ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

	// write 0x08 register
	regVal = 0x0;
	MODIFYFLD(regVal,
		ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
	MODIFYFLD(regVal,
		ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x08);
	MODIFYFLD(regVal,
		ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WDATA,
		cmd_data | 0x20);
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

	// program cfgclkfreqrange
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg3),
		0x1C);

	// program basedir_0 = 1;
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg1),
		0x01);

	// program forcerxmode = 0
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg5),
		0x0f);

	// enable the clock lane and corresponding data lanes
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enableclk, 0x1);
	if (lane_num == 1) {
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_0, 0x1);
	} else if (lane_num == 2) {
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_0, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_1, 0x1);
	} else if (lane_num == 3) {
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_0, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_1, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_2, 0x1);
	} else if (lane_num == 4) {
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_0, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_1, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_2, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG4, enable_3, 0x1);
	} else {
		ISP_PR_ERR("Lane num error\n");
		return RET_FAILURE;
	}

	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg4),
		regVal);
	aidt_sleep(10); // wait 10ms

	// kick off MIPI PHY by 2 steps
	// release shutdownz first
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_MIPI_PHY0_REG0, shutdownz, 0x1);
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg0), regVal);

	// release rstz secondly
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_MIPI_PHY0_REG0, shutdownz, 0x1);
	MODIFYFLD(regVal, ISP_MIPI_PHY0_REG0, rstz, 0x1);
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg0), regVal);

	aidt_sleep(10); // wait 10ms

	// program forcerxmode = 0
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg5),
		0x0);

	//=============================================
	// Check PHY auto calibration status with 4 steps:
	// 1. check termination calibration sequence with rext
	// 2. check clock lane offset calibration
	// 3. check data lane 0/1/2/3 offset calibration
	// 4. check DDL tuning calibration for lane 0/1/2/3
	// !!!No need to do force calibration since we suppose
	//PHY auto calibration
	// must be successful. Otherwise, MIPI PHY should have
	//some design issue and
	// need the PHY designer to investigate the issue. We can
	//duduce that PHY has
	// functional issue and ISP cannot work with sensor input.
	//==============================================

	//==== 1. check termination calibration status
	// read 0x221 register
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
	MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x221);
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

	regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
	cmd_data = regVal & ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;
	// check calibration done status. cmd_data[7] == 0x1
	if ((cmd_data & 0x80) == 0x80) {
		// read 0x222 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x222);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;
		if (0x0 == (cmd_data & 0x1)) {
			//read 0x220 register
			regVal = 0x0;
			MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
				CMD_WRITE, 0x0);
			MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
				CMD_ADDR, 0x220);
			aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;
			ISP_PR_INFO("MIPI PHY cali passes [0x%x]\n",
				(cmd_data >> 2) & 0xf);
		} else {
			ISP_PR_ERR("MIPI PHY cali fails!\n");
		}
	}

	/******add interface for manual calibration*******/
	force_value = (g_prop ? g_prop->phy_cali_value : 0x10);

	if (force_value <= 0xf) {
		ISP_PR_PC("Manual cali enabled, force_value[%d]", force_value);

		regVal = (force_value << 4);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_ADDR, 0x209);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = 0x1;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_ADDR, 0x20a);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);
	}
	/***********************************************/

	//==== 2. check clock lane offset calibration status
	// read 0x39D register
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
	MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x39D);
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

	regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
	cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;
	rxclk_caldone = cmd_data & 0x1;
	rxclk_errcal = (cmd_data >> 1) & 0xf;

	// read 0x39E register
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
	MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x39E);
	aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);
	regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
	cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

	// chack calibration done status. cmd_data[0] == 0x1
	if (rxclk_caldone) {
		if ((rxclk_errcal == 0) || (rxclk_errcal == 1))
			ISP_PR_INFO(
			"PHY clock passes, Value(0x39e): 0x%x\n",
			cmd_data & 0x3f)
		else
			ISP_PR_ERR("MIPI PHY clock lane offset cali fails!\n");
	} else {
		ISP_PR_ERR("MIPI PHY clock lane offset cali fails!\n");
		return RET_FAILURE;
	}

	//==== 3. check data lane offset calibration status
	// read 0x59F register
	if (lane_num == 1) {
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x59F);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
			ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if ((cmd_data & 0x4) == 0x0) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x5a1);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane0 offset cali pass reg0x5a1[0x%x]\n",
				cmd_data & 0x3f);
			} else {
				ISP_PR_ERR("data lane offset cali failed!");
			}
		}
	} else if (lane_num == 2) {
		// check data lane 0 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x59F);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if (0x0 == (cmd_data & 0x4)) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x5a1);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane0 offset cali pass reg0x5a1[0x%x]\n",
				cmd_data & 0x3f);
			} else {
				ISP_PR_ERR("data lane0 offset cali failed!\n");
			}
		}

		// check data lane 1 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x79F);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if (0x0 == (cmd_data & 0x4)) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x7a1);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane1 offset cali pass reg0x7a1[0x%x]\n",
				cmd_data & 0x3f);
			} else {
				ISP_PR_ERR("data lane1 offset cali failed!\n");
			}
		}
	} else if (lane_num == 4) {
		// check data lane 0 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x59F);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if (0x0 == (cmd_data & 0x4)) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x5a1);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane0 offset cali pass reg0x5a1[0x%x]\n",
				cmd_data & 0x3f);
			} else {
				ISP_PR_ERR("data lane0 offset cali failed!\n");
			}
		}

		// check data lane 1 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x79F);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if (0x0 == (cmd_data & 0x4)) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x7a1);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane 1 offset cali pass reg0x7a1[0x%x]\n",
				cmd_data & 0x3f);
			} else {
				ISP_PR_ERR("data lane1 offset cali failed!\n");
			}
		}

		// check data lane 2 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x99F);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if (0x0 == (cmd_data & 0x4)) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x9a1);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane2 offset cali pass reg0x9a1[0x%x]\n",
				cmd_data & 0x3f)
			} else {
				ISP_PR_ERR("data lane 2 offset cali failed!\n");
			}
		}

		// check data lane 3 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0xB9F);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if (0x0 == (cmd_data & 0x4)) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0xba1);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane3 offset cali pass reg0xba1[0x%x]\n",
				cmd_data & 0x3f)
			} else {
				ISP_PR_ERR("data lane3 offset cali failed!\n");
			}
		}
	}

	//==== 4. check DDL tuning calibration status
	// read 0x5E0 register
	if (lane_num == 1) {
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x5E0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if (0x0 == (cmd_data & 0x2)) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x5e5);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane 0 DDL cali pass reg0xba1[0x%x]\n",
				cmd_data & 0x1f)
			} else {
				ISP_PR_ERR("data lane 0 DDL cali failed!\n");
			}
		}
	} else if (lane_num == 2) {
		// check data lane 0 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x5E0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x5e5);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane 0 DDL cali pass reg0xba1[0x%x]\n",
				cmd_data & 0x1f)
			} else {
				ISP_PR_ERR("data lane 0 DDL cali failed");
			}
		}

		// check data lane 1 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
			CMD_ADDR, 0x7E0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x7e5);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane 1 DDL cali pass reg0x7e5[0x%x]\n",
				cmd_data & 0x1f)
			} else {
				ISP_PR_ERR("data lane 1 DDL cali failed!");
			}
		}
	} else if (lane_num == 4) {
		// check data lane 0 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x5E0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x5e5);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane 0 DDL cali pass reg0x5e5[0x%x]\n",
				cmd_data & 0x1f)
			} else {
				ISP_PR_ERR("data lane 0 DDL cali failed!");
			}
		}

		// check data lane 1 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x7E0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
		ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x7e5);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane 1 DDL cali pass reg0x7e5[0x%x]\n",
				cmd_data & 0x1f)
			} else {
				ISP_PR_ERR("data lane 1 DDL cali failed!");
			}
		}

		// check data lane 2 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0x9E0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
			ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0x9e5);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane 2 DDL cali pass reg0x9e5[0x%x]\n",
				cmd_data & 0x1f)
			} else {
				ISP_PR_ERR("data lane 2 DDL cali failed!");
			}
		}

		// check data lane 3 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD, CMD_ADDR,
		0xBE0);
		aidt_hal_write_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

		regVal = aidt_hal_read_reg((uint64_t)(
		&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
		cmd_data = regVal &
			ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if (0x0 == (cmd_data & 0x2)) {
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_WRITE, 0x0);
				MODIFYFLD(regVal, ISP_MIPI_PHY0_TESTINTERF_CMD,
					CMD_ADDR, 0xbe5);
				aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
		regVal);

				regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

				cmd_data = regVal &
				ISP_MIPI_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

				ISP_PR_INFO(
				"data lane 3 DDL cali pass reg0xbe5[0x%x]\n",
				cmd_data & 0x1f)
			} else {
				ISP_PR_ERR("data lane 3 DDL cali failed!");
			}
		}
	}

	ISP_PR_PC("MIPI PHY is enable!");
	return RET_SUCCESS;
}

int shutdown_internal_synopsys_phy(uint32_t device_id,
		uint32_t lane_num)
{
	uint32_t regVal;
//	uint32_t timeout = 0;
//	uint32_t phy_reg_check_timeout = 10; // 1 second


	//=================================================
	// SNPS PHY shutdown sequence
	//=================================================
	if (device_id == 0) {
		// step 1: check the stop status
		usleep_range(9000, 10000);
/*
 *		while (timeout < phy_reg_check_timeout) {
 *			regVal = aidt_hal_read_reg(mmISP_MIPI_CSI0_STATUS);
 *			if ((regVal &
 *		ISP_MIPI_CSI0_STATUS__MIPI_S_STOPSTATE_CLK_MASK) &&
 *		((regVal & (mipi_status_mask[lane_num - 1] <<
 *		ISP_MIPI_CSI0_STATUS__MIPI_STOPSTATE__SHIFT)) ==
 *		(mipi_status_mask[lane_num - 1] <<
 *		ISP_MIPI_CSI0_STATUS__MIPI_STOPSTATE__SHIFT))) {
 *				ISP_PR_WARN("MIPI PHY 0 has been stopped\n");
 *				break;
 *			}
 *
 *			usleep_range(9000, 10000);
 *
 *			timeout = timeout + 1;
 *		}
 *		if (timeout == phy_reg_check_timeout) {
 *			ISP_PR_ERR("Timeout in shutting down MIPI PHY 0");
 *			return RET_FAILURE;
 *		}
 */
		// step 2: shutdown MIPI PHY
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG0, shutdownz, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG0, rstz, 0x0);
		aidt_hal_write_reg(mmISP_MIPI_PHY0_REG0, regVal);

		// step 3: assert testclr signal
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY0_REG7, testclr, 0x1);
		aidt_hal_write_reg(mmISP_MIPI_PHY0_REG7, regVal);
	} else if (device_id == 1) {
		// step 1: check the stop status
		usleep_range(9000, 10000);
/*
 *		while (timeout < phy_reg_check_timeout) {
 *			regVal = aidt_hal_read_reg(mmISP_MIPI_CSI1_STATUS);
 *			if ((regVal &
 *		ISP_MIPI_CSI1_STATUS__MIPI_S_STOPSTATE_CLK_MASK) &&
 *		((regVal & (mipi_status_mask[lane_num - 1] <<
 *		ISP_MIPI_CSI1_STATUS__MIPI_STOPSTATE__SHIFT)) ==
 *		(mipi_status_mask[lane_num - 1] <<
 *		ISP_MIPI_CSI1_STATUS__MIPI_STOPSTATE__SHIFT))) {
 *				ISP_PR_WARN("MIPI PHY 1 has been stopped\n");
 *				break;
 *			}
 *
 *			usleep_range(9000, 10000);
 *
 *			timeout = timeout + 1;
 *		}
 *		if (timeout == phy_reg_check_timeout) {
 *			ISP_PR_ERR("Timeout in shutting down MIPI PHY 1");
 *			return RET_FAILURE;
 *		}
 */

		// step 2: shutdown MIPI PHY
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY1_REG0, shutdownz, 0x0);
		MODIFYFLD(regVal, ISP_MIPI_PHY1_REG0, rstz, 0x0);
		aidt_hal_write_reg(mmISP_MIPI_PHY1_REG0, regVal);

		// step 3: assert testclr signal
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_MIPI_PHY1_REG7, testclr, 0x1);
		aidt_hal_write_reg(mmISP_MIPI_PHY1_REG7, regVal);
	}

	return RET_SUCCESS;
}

void reset_sensor_phy(enum camera_id device_id, bool on,
		bool fpga_env, uint32_t lane_num)
{
	uint32_t regVal;
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		return;
	}

	if (device_id == 0) {
		if (fpga_env == true) {
			// for phy reset
			aidt_hal_write_reg(0xc00, 0x0);
			usleep_range(9000, 10000);
			aidt_hal_write_reg(0xc00, 0x1);
			usleep_range(9000, 10000);
			aidt_hal_write_reg(0xc00, 0x3);
		} else {
			if (on == false) {
				// for snps phy shutdown
				shutdown_internal_synopsys_phy(device_id,
					lane_num);
			}
		}

		if (on == true) {
			// for sensor reset
			regVal = aidt_hal_read_reg(mmISP_GPIO_SHUTDOWN_SENSOR0);
			regVal &= (~(0x1 << 4));
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR0, regVal);
			//The min interval of this is zero, but 1ms delay seems
			//more stable for the rising.
			usleep_range(1000, 2000);
			regVal |= (0x1 << 4);
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR0, regVal);
		} else {
			// for sensor shutdown
			regVal = aidt_hal_read_reg(mmISP_GPIO_SHUTDOWN_SENSOR0);
			regVal &= (~(0x1 << 4));
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR0, regVal);
		}
	} else if (device_id == 1) {
		if (fpga_env == true) {
			// for phy reset
			aidt_hal_write_reg(0xc08, 0x0);
			usleep_range(9000, 10000);
			aidt_hal_write_reg(0xc08, 0x1);
			usleep_range(9000, 10000);
			aidt_hal_write_reg(0xc08, 0x3);
		} else {
			if (on == false) {
				// for snps phy shutdown
				shutdown_internal_synopsys_phy(device_id,
					lane_num);
			}
		}
		if (on == true) {
			// for sensor reset
			regVal = aidt_hal_read_reg(mmISP_GPIO_SHUTDOWN_SENSOR1);
			regVal &= (~(0x1 << 4));
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR1, regVal);
			//The min interval of this is zero, but 1ms delay seems
			//more stable for the rising.
			usleep_range(1000, 2000);
			regVal |= (0x1 << 4);
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR1, regVal);
		} else {
			// for sensor shutdown
			regVal = aidt_hal_read_reg(mmISP_GPIO_SHUTDOWN_SENSOR1);
			regVal &= (~(0x1 << 4));
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR1, regVal);
		}
	} else {
		ISP_PR_ERR
		("device_id %d unsupported\n", device_id);
	}
}


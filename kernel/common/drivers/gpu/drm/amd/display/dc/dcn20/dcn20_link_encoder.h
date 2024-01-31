/*
 * Copyright (C) 2012-2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: AMD
 *
 */

#ifndef __DC_LINK_ENCODER__DCN20_H__
#define __DC_LINK_ENCODER__DCN20_H__

#include "dcn10/dcn10_link_encoder.h"

#define DCN2_AUX_REG_LIST(id)\
	AUX_REG_LIST(id), \
	SRI(AUX_DPHY_TX_CONTROL, DP_AUX, id)

#define UNIPHY_MASK_SH_LIST(mask_sh)\
	LE_SF(SYMCLKA_CLOCK_ENABLE, SYMCLKA_CLOCK_ENABLE, mask_sh),\
	LE_SF(UNIPHYA_CHANNEL_XBAR_CNTL, UNIPHY_LINK_ENABLE, mask_sh),\
	LE_SF(UNIPHYA_CHANNEL_XBAR_CNTL, UNIPHY_CHANNEL0_XBAR_SOURCE, mask_sh),\
	LE_SF(UNIPHYA_CHANNEL_XBAR_CNTL, UNIPHY_CHANNEL1_XBAR_SOURCE, mask_sh),\
	LE_SF(UNIPHYA_CHANNEL_XBAR_CNTL, UNIPHY_CHANNEL2_XBAR_SOURCE, mask_sh),\
	LE_SF(UNIPHYA_CHANNEL_XBAR_CNTL, UNIPHY_CHANNEL3_XBAR_SOURCE, mask_sh)

#define DPCS_MASK_SH_LIST(mask_sh)\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX0_CLK_RDY, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX0_DATA_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX1_CLK_RDY, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX1_DATA_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX2_CLK_RDY, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX2_DATA_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX3_CLK_RDY, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX3_DATA_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL4, RDPCS_PHY_DP_TX0_TERM_CTRL, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL4, RDPCS_PHY_DP_TX1_TERM_CTRL, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL4, RDPCS_PHY_DP_TX2_TERM_CTRL, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL4, RDPCS_PHY_DP_TX3_TERM_CTRL, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL11, RDPCS_PHY_DP_MPLLB_MULTIPLIER, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL5, RDPCS_PHY_DP_TX0_WIDTH, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL5, RDPCS_PHY_DP_TX0_RATE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL5, RDPCS_PHY_DP_TX1_WIDTH, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL5, RDPCS_PHY_DP_TX1_RATE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_TX2_PSTATE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_TX3_PSTATE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_TX2_MPLL_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_TX3_MPLL_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL7, RDPCS_PHY_DP_MPLLB_FRACN_QUOT, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL7, RDPCS_PHY_DP_MPLLB_FRACN_DEN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL8, RDPCS_PHY_DP_MPLLB_SSC_PEAK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL9, RDPCS_PHY_DP_MPLLB_SSC_UP_SPREAD, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL9, RDPCS_PHY_DP_MPLLB_SSC_STEPSIZE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL10, RDPCS_PHY_DP_MPLLB_FRACN_REM, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL11, RDPCS_PHY_DP_REF_CLK_MPLLB_DIV, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL11, RDPCS_PHY_HDMI_MPLLB_HDMI_DIV, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL12, RDPCS_PHY_DP_MPLLB_SSC_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL12, RDPCS_PHY_DP_MPLLB_DIV5_CLK_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL12, RDPCS_PHY_DP_MPLLB_TX_CLK_DIV, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL12, RDPCS_PHY_DP_MPLLB_WORD_DIV2_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL12, RDPCS_PHY_DP_MPLLB_STATE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL13, RDPCS_PHY_DP_MPLLB_DIV_CLK_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL13, RDPCS_PHY_DP_MPLLB_DIV_MULTIPLIER, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL14, RDPCS_PHY_DP_MPLLB_FRACN_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL14, RDPCS_PHY_DP_MPLLB_PMIX_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CNTL, RDPCS_TX_FIFO_LANE0_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CNTL, RDPCS_TX_FIFO_LANE1_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CNTL, RDPCS_TX_FIFO_LANE2_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CNTL, RDPCS_TX_FIFO_LANE3_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CNTL, RDPCS_TX_FIFO_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CNTL, RDPCS_TX_FIFO_RD_START_DELAY, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CLOCK_CNTL, RDPCS_EXT_REFCLK_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CLOCK_CNTL, RDPCS_SRAMCLK_BYPASS, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CLOCK_CNTL, RDPCS_SRAMCLK_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CLOCK_CNTL, RDPCS_SRAMCLK_CLOCK_ON, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CLOCK_CNTL, RDPCS_SYMCLK_DIV2_CLOCK_ON, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CLOCK_CNTL, RDPCS_SYMCLK_DIV2_GATE_DIS, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_CLOCK_CNTL, RDPCS_SYMCLK_DIV2_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX0_DISABLE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX1_DISABLE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX2_DISABLE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX3_DISABLE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX0_REQ, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX1_REQ, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX2_REQ, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX3_REQ, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX0_ACK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX1_ACK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX2_ACK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX3_ACK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX0_RESET, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX1_RESET, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX2_RESET, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL3, RDPCS_PHY_DP_TX3_RESET, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL0, RDPCS_PHY_RESET, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL0, RDPCS_PHY_CR_MUX_SEL, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL0, RDPCS_PHY_REF_RANGE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL0, RDPCS_SRAM_BYPASS, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL0, RDPCS_SRAM_EXT_LD_DONE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL0, RDPCS_PHY_HDMIMODE_ENABLE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL0, RDPCS_SRAM_INIT_DONE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL2, RDPCS_PHY_DP4_POR, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PLL_UPDATE_DATA, RDPCS_PLL_UPDATE_DATA, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_INTERRUPT_CONTROL, RDPCS_REG_FIFO_ERROR_MASK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_INTERRUPT_CONTROL, RDPCS_TX_FIFO_ERROR_MASK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_INTERRUPT_CONTROL, RDPCS_DPALT_DISABLE_TOGGLE_MASK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_INTERRUPT_CONTROL, RDPCS_DPALT_4LANE_TOGGLE_MASK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCS_TX_CR_ADDR, RDPCS_TX_CR_ADDR, mask_sh),\
	LE_SF(RDPCSTX0_RDPCS_TX_CR_DATA, RDPCS_TX_CR_DATA, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE0, RDPCS_PHY_DP_MPLLB_V2I, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE0, RDPCS_PHY_DP_TX0_EQ_MAIN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE0, RDPCS_PHY_DP_TX0_EQ_PRE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE0, RDPCS_PHY_DP_TX0_EQ_POST, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE0, RDPCS_PHY_DP_MPLLB_FREQ_VCO, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE1, RDPCS_PHY_DP_MPLLB_CP_INT, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE1, RDPCS_PHY_DP_MPLLB_CP_PROP, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE1, RDPCS_PHY_DP_TX1_EQ_MAIN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE1, RDPCS_PHY_DP_TX1_EQ_PRE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE1, RDPCS_PHY_DP_TX1_EQ_POST, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE2, RDPCS_PHY_DP_TX2_EQ_MAIN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE2, RDPCS_PHY_DP_TX2_EQ_PRE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE2, RDPCS_PHY_DP_TX2_EQ_POST, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE3, RDPCS_PHY_DP_TX3_EQ_MAIN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE3, RDPCS_PHY_DCO_FINETUNE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE3, RDPCS_PHY_DCO_RANGE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE3, RDPCS_PHY_DP_TX3_EQ_PRE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_FUSE3, RDPCS_PHY_DP_TX3_EQ_POST, mask_sh),\
	LE_SF(DPCSTX0_DPCSTX_TX_CLOCK_CNTL, DPCS_SYMCLK_CLOCK_ON, mask_sh),\
	LE_SF(DPCSTX0_DPCSTX_TX_CLOCK_CNTL, DPCS_SYMCLK_GATE_DIS, mask_sh),\
	LE_SF(DPCSTX0_DPCSTX_TX_CLOCK_CNTL, DPCS_SYMCLK_EN, mask_sh),\
	LE_SF(DPCSTX0_DPCSTX_TX_CNTL, DPCS_TX_DATA_SWAP, mask_sh),\
	LE_SF(DPCSTX0_DPCSTX_TX_CNTL, DPCS_TX_DATA_ORDER_INVERT, mask_sh),\
	LE_SF(DPCSTX0_DPCSTX_TX_CNTL, DPCS_TX_FIFO_EN, mask_sh),\
	LE_SF(DPCSTX0_DPCSTX_TX_CNTL, DPCS_TX_FIFO_RD_START_DELAY, mask_sh),\
	LE_SF(DPCSTX0_DPCSTX_DEBUG_CONFIG, DPCS_DBG_CBUS_DIS, mask_sh)

#define DPCS_DCN2_MASK_SH_LIST(mask_sh)\
	DPCS_MASK_SH_LIST(mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_RX_LD_VAL, RDPCS_PHY_RX_REF_LD_VAL, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_RX_LD_VAL, RDPCS_PHY_RX_VCO_LD_VAL, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DPALT_DISABLE_ACK, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_TX0_PSTATE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_TX1_PSTATE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_TX0_MPLL_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_TX1_MPLL_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DP_REF_CLK_EN, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DPALT_DP4, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL6, RDPCS_PHY_DPALT_DISABLE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL5, RDPCS_PHY_DP_TX2_WIDTH, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL5, RDPCS_PHY_DP_TX2_RATE, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL5, RDPCS_PHY_DP_TX3_WIDTH, mask_sh),\
	LE_SF(RDPCSTX0_RDPCSTX_PHY_CNTL5, RDPCS_PHY_DP_TX3_RATE, mask_sh),\
	LE_SF(DCIO_SOFT_RESET, UNIPHYA_SOFT_RESET, mask_sh),\
	LE_SF(DCIO_SOFT_RESET, UNIPHYB_SOFT_RESET, mask_sh),\
	LE_SF(DCIO_SOFT_RESET, UNIPHYC_SOFT_RESET, mask_sh),\
	LE_SF(DCIO_SOFT_RESET, UNIPHYD_SOFT_RESET, mask_sh),\
	LE_SF(DCIO_SOFT_RESET, UNIPHYE_SOFT_RESET, mask_sh)

#define LINK_ENCODER_MASK_SH_LIST_DCN20(mask_sh)\
	LINK_ENCODER_MASK_SH_LIST_DCN10(mask_sh),\
	LE_SF(DP0_DP_DPHY_CNTL, DPHY_FEC_EN, mask_sh),\
	LE_SF(DP0_DP_DPHY_CNTL, DPHY_FEC_READY_SHADOW, mask_sh),\
	LE_SF(DP0_DP_DPHY_CNTL, DPHY_FEC_ACTIVE_STATUS, mask_sh),\
	LE_SF(DIG0_DIG_LANE_ENABLE, DIG_LANE0EN, mask_sh),\
	LE_SF(DIG0_DIG_LANE_ENABLE, DIG_LANE1EN, mask_sh),\
	LE_SF(DIG0_DIG_LANE_ENABLE, DIG_LANE2EN, mask_sh),\
	LE_SF(DIG0_DIG_LANE_ENABLE, DIG_LANE3EN, mask_sh),\
	LE_SF(DIG0_DIG_LANE_ENABLE, DIG_CLK_EN, mask_sh),\
	LE_SF(DIG0_TMDS_CTL_BITS, TMDS_CTL0, mask_sh), \
	UNIPHY_MASK_SH_LIST(mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_RX_CONTROL0, AUX_RX_START_WINDOW, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_RX_CONTROL0, AUX_RX_HALF_SYM_DETECT_LEN, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_RX_CONTROL0, AUX_RX_TRANSITION_FILTER_EN, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_RX_CONTROL0, AUX_RX_ALLOW_BELOW_THRESHOLD_PHASE_DETECT, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_RX_CONTROL0, AUX_RX_ALLOW_BELOW_THRESHOLD_START, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_RX_CONTROL0, AUX_RX_ALLOW_BELOW_THRESHOLD_STOP, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_RX_CONTROL0, AUX_RX_PHASE_DETECT_LEN, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_RX_CONTROL0, AUX_RX_DETECTION_THRESHOLD, mask_sh), \
	LE_SF(DP_AUX0_AUX_DPHY_TX_CONTROL, AUX_TX_PRECHARGE_LEN, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_TX_CONTROL, AUX_TX_PRECHARGE_SYMBOLS, mask_sh),\
	LE_SF(DP_AUX0_AUX_DPHY_TX_CONTROL, AUX_MODE_DET_CHECK_DELAY, mask_sh)

#define UNIPHY_DCN2_REG_LIST(id) \
	SRI(CLOCK_ENABLE, SYMCLK, id), \
	SRI(CHANNEL_XBAR_CNTL, UNIPHY, id)

#define DPCS_DCN2_CMN_REG_LIST(id) \
	SRI(DIG_LANE_ENABLE, DIG, id), \
	SRI(TMDS_CTL_BITS, DIG, id), \
	SRI(RDPCSTX_PHY_CNTL3, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL4, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL5, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL6, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL7, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL8, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL9, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL10, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL11, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL12, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL13, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL14, RDPCSTX, id), \
	SRI(RDPCSTX_CNTL, RDPCSTX, id), \
	SRI(RDPCSTX_CLOCK_CNTL, RDPCSTX, id), \
	SRI(RDPCSTX_INTERRUPT_CONTROL, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL0, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_CNTL2, RDPCSTX, id), \
	SRI(RDPCSTX_PLL_UPDATE_DATA, RDPCSTX, id), \
	SRI(RDPCS_TX_CR_ADDR, RDPCSTX, id), \
	SRI(RDPCS_TX_CR_DATA, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_FUSE0, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_FUSE1, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_FUSE2, RDPCSTX, id), \
	SRI(RDPCSTX_PHY_FUSE3, RDPCSTX, id), \
	SRI(DPCSTX_TX_CLOCK_CNTL, DPCSTX, id), \
	SRI(DPCSTX_TX_CNTL, DPCSTX, id), \
	SRI(DPCSTX_DEBUG_CONFIG, DPCSTX, id), \
	SRI(RDPCSTX_DEBUG_CONFIG, RDPCSTX, id), \
	SR(RDPCSTX0_RDPCSTX_SCRATCH)


#define DPCS_DCN2_REG_LIST(id) \
	DPCS_DCN2_CMN_REG_LIST(id), \
	SRI(RDPCSTX_PHY_RX_LD_VAL, RDPCSTX, id),\
	SRI(RDPCSTX_DMCU_DPALT_DIS_BLOCK_REG, RDPCSTX, id)

#define LE_DCN2_REG_LIST(id) \
		LE_DCN10_REG_LIST(id), \
		SR(DCIO_SOFT_RESET)

struct mpll_cfg {
	uint32_t mpllb_ana_v2i;
	uint32_t mpllb_ana_freq_vco;
	uint32_t mpllb_ana_cp_int;
	uint32_t mpllb_ana_cp_prop;
	uint32_t mpllb_multiplier;
	uint32_t ref_clk_mpllb_div;
	bool mpllb_word_div2_en;
	bool mpllb_ssc_en;
	bool mpllb_div5_clk_en;
	bool mpllb_div_clk_en;
	bool mpllb_fracn_en;
	bool mpllb_pmix_en;
	uint32_t mpllb_div_multiplier;
	uint32_t mpllb_tx_clk_div;
	uint32_t mpllb_fracn_quot;
	uint32_t mpllb_fracn_den;
	uint32_t mpllb_ssc_peak;
	uint32_t mpllb_ssc_stepsize;
	uint32_t mpllb_ssc_up_spread;
	uint32_t mpllb_fracn_rem;
	uint32_t mpllb_hdmi_div;
	// TODO: May not mpll params, need to figure out.
	uint32_t tx_vboost_lvl;
	uint32_t hdmi_pixel_clk_div;
	uint32_t ref_range;
	uint32_t ref_clk;
	bool hdmimode_enable;
	bool sup_pre_hp;
	bool dp_tx0_vergdrv_byp;
	bool dp_tx1_vergdrv_byp;
	bool dp_tx2_vergdrv_byp;
	bool dp_tx3_vergdrv_byp;
#if defined(CONFIG_DRM_AMD_DC_DCN3_0)
	uint32_t tx_peaking_lvl;
	uint32_t ctr_reqs_pll;
#endif

};

struct dpcssys_phy_seq_cfg {
	bool program_fuse;
	bool bypass_sram;
	bool lane_en[4];
	bool use_calibration_setting;
	struct mpll_cfg mpll_cfg;
	bool load_sram_fw;
#if 0

	bool hdmimode_enable;
	bool silver2;
	bool ext_refclk_en;
	uint32_t dp_tx0_term_ctrl;
	uint32_t dp_tx1_term_ctrl;
	uint32_t dp_tx2_term_ctrl;
	uint32_t dp_tx3_term_ctrl;
	uint32_t fw_data[0x1000];
	uint32_t dp_tx0_width;
	uint32_t dp_tx1_width;
	uint32_t dp_tx2_width;
	uint32_t dp_tx3_width;
	uint32_t dp_tx0_rate;
	uint32_t dp_tx1_rate;
	uint32_t dp_tx2_rate;
	uint32_t dp_tx3_rate;
	uint32_t dp_tx0_eq_main;
	uint32_t dp_tx0_eq_pre;
	uint32_t dp_tx0_eq_post;
	uint32_t dp_tx1_eq_main;
	uint32_t dp_tx1_eq_pre;
	uint32_t dp_tx1_eq_post;
	uint32_t dp_tx2_eq_main;
	uint32_t dp_tx2_eq_pre;
	uint32_t dp_tx2_eq_post;
	uint32_t dp_tx3_eq_main;
	uint32_t dp_tx3_eq_pre;
	uint32_t dp_tx3_eq_post;
	bool data_swap_en;
	bool data_order_invert_en;
	uint32_t ldpcs_fifo_start_delay;
	uint32_t rdpcs_fifo_start_delay;
	bool rdpcs_reg_fifo_error_mask;
	bool rdpcs_tx_fifo_error_mask;
	bool rdpcs_dpalt_disable_mask;
	bool rdpcs_dpalt_4lane_mask;
#endif
};

struct dcn20_link_encoder {
	struct dcn10_link_encoder enc10;
	struct dpcssys_phy_seq_cfg phy_seq_cfg;
};

void enc2_fec_set_enable(struct link_encoder *enc, bool enable);
void enc2_fec_set_ready(struct link_encoder *enc, bool ready);
bool enc2_fec_is_active(struct link_encoder *enc);
void enc2_hw_init(struct link_encoder *enc);

void link_enc2_read_state(struct link_encoder *enc, struct link_enc_state *s);

void dcn20_link_encoder_enable_dp_output(
	struct link_encoder *enc,
	const struct dc_link_settings *link_settings,
	enum clock_source_id clock_source);

bool dcn20_link_encoder_is_in_alt_mode(struct link_encoder *enc);
void dcn20_link_encoder_get_max_link_cap(struct link_encoder *enc,
	struct dc_link_settings *link_settings);

uint32_t dcn20_link_encoder_get_phy_dp_tx_status(struct link_encoder *enc);

void dcn20_link_encoder_shutdown(struct link_encoder *enc,
	const struct dc_link_settings *link_settings);
void dcn20_link_encoder_wakeup(struct link_encoder *enc,
	const struct dc_link_settings *link_settings);

void dcn20_link_encoder_construct(
	struct dcn20_link_encoder *enc20,
	const struct encoder_init_data *init_data,
	const struct encoder_feature_support *enc_features,
	const struct dcn10_link_enc_registers *link_regs,
	const struct dcn10_link_enc_aux_registers *aux_regs,
	const struct dcn10_link_enc_hpd_registers *hpd_regs,
	const struct dcn10_link_enc_shift *link_shift,
	const struct dcn10_link_enc_mask *link_mask);

#endif /* __DC_LINK_ENCODER__DCN20_H__ */

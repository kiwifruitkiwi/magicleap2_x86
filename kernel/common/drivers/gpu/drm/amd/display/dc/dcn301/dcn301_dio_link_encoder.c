/*
 * Copyright (C) 2018-2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
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

#include "reg_helper.h"

#include "core_types.h"
#include "link_encoder.h"
#include "dcn301_dio_link_encoder.h"
#include "stream_encoder.h"
#include "i2caux_interface.h"
#include "dc_bios_types.h"

#include "gpio_service_interface.h"

#define CTX \
	enc10->base.ctx
#define DC_LOGGER \
	enc10->base.ctx->logger

#define REG(reg)\
	(enc10->link_regs->reg)

#undef FN
#define FN(reg_name, field_name) \
	enc10->link_shift->field_name, enc10->link_mask->field_name

#define IND_REG(index) \
	(enc10->link_regs->index)

static void enc301_hw_init(struct link_encoder *enc)
{
	struct dcn10_link_encoder *enc10 = TO_DCN10_LINK_ENC(enc);
	struct dc_context *ctx = enc10->base.ctx;

/*
 *	This reg is 1st init in above dcn10_aux_init,
 *	and here we'd overwrite the bit-field of AUX_RX_RECEIVE_WINDOW to 1
 *	AUX_DPHY_RX_CONTROL0::
 *	AUX_RX_START_WINDOW                            = 1 [6:4]
 *	AUX_RX_RECEIVE_WINDOW                          = 1 [10:8] default is 2
 *	AUX_RX_HALF_SYM_DETECT_LEN                     = 1 [13:12] default is 1
 *	AUX_RX_TRANSITION_FILTER_EN                    = 1 [16] default is 1
 *	AUX_RX_ALLOW_BELOW_THRESHOLD_PHASE_DETECT      = 0 [17] default is 0
 *	AUX_RX_ALLOW_BELOW_THRESHOLD_START             = 1 [18] default is 1
 *	AUX_RX_ALLOW_BELOW_THRESHOLD_STOP              = 1 [19] default is 1
 *	AUX_RX_PHASE_DETECT_LEN                        = 3 [21,20] default is 3
 *	AUX_RX_DETECTION_THRESHOLD                     = 1 [30:28] default is 2
 */
	dm_write_reg(ctx, enc10->aux_regs->AUX_DPHY_RX_CONTROL0, 0x103D1110);

/*
 *	Note: to reduce the AUX pre-charge time, we'd program the reg
 *	AUX_DPHY_TX_CONTROL.AUX_TX_PRECHARGE_LEN to 0xD and
 *	AUX_DPHY_TX_CONTROL.AUX_TX_PRECHARGE_LEN_MUL to 0x0
 *	But this is to be fixed in DMUB and not necessary to do overwrite here.
 *	AUX_DPHY_TX_CONTROL::
 *	AUX_TX_PRECHARGE_LEN		= 0xD [3:0]
 *	AUX_TX_PRECHARGE_LEN_MUL	= 0x0 [5:4]
 *	AUX_TX_OE_ASSERT_TIME		= 0x1 [6]
 *	AUX_TX_PRECHARGE_SYMBOLS	= 0x1C[13:8]
 *	AUX_MODE_DET_CHECK_DELAY	= 0x2 [18:16]
*/

	// Set TMDS_CTL0 to 1.  This is a legacy setting.
	REG_UPDATE(TMDS_CTL_BITS, TMDS_CTL0, 1);

	dcn10_aux_initialize(enc10);
}

/* this function reads additionally dp mst content protection status related register
 * fields to be logged later in dcn10_log_hw_state.
 */
static void link_enc301_read_cp_state(struct link_encoder *enc, struct link_enc_state *s)
{
	struct dcn10_link_encoder *enc10 = TO_DCN10_LINK_ENC(enc);
	uint32_t mse_sat[3];

	/* dp mst link content encryption status */
	s->dp_mse_sat_update = REG_READ(DP_MSE_SAT_UPDATE);
	mse_sat[0] = REG_READ(DP_MSE_SAT0);
	mse_sat[1] = REG_READ(DP_MSE_SAT1);
	mse_sat[2] = REG_READ(DP_MSE_SAT2);

	s->dp_mse_sat0 = mse_sat[0];
	s->dp_mse_sat1 = mse_sat[1];
	s->dp_mse_sat2 = mse_sat[2];

	/* 0th and 1th payload */
	s->dp_encrypt_payload_0 = (mse_sat[0] & (1 << 4)) >> 4;
	s->dp_encrypt_type_payload_0 = (mse_sat[0] & (1 << 5)) >> 5;
	s->dp_encrypt_payload_1 = (mse_sat[0] & (1 << 20)) >> 20;
	s->dp_encrypt_type_payload_1 = (mse_sat[0] & (1 << 20)) >> 20;
	/* 2th and 3th payload */
	s->dp_encrypt_payload_2 = (mse_sat[1] & (1 << 4)) >> 4;
	s->dp_encrypt_type_payload_2 = (mse_sat[1] & (1 << 5)) >> 5;
	s->dp_encrypt_payload_3 = (mse_sat[1] & (1 << 20)) >> 20;
	s->dp_encrypt_type_payload_3 = (mse_sat[1] & (1 << 20)) >> 20;
	/* 4th and 5th payload */
	s->dp_encrypt_payload_4 = (mse_sat[2] & (1 << 4)) >> 4;
	s->dp_encrypt_type_payload_4 = (mse_sat[2] & (1 << 5)) >> 5;
	s->dp_encrypt_payload_5 = (mse_sat[2] & (1 << 20)) >> 20;
	s->dp_encrypt_type_payload_5 = (mse_sat[2] & (1 << 20)) >> 20;
}

/* Send TPS1 and TPS2 for the specified amount of time in us */
#define DP_FAST_TRAINING_TP1_US 500
#define DP_FAST_TRAINING_TP2_US 500
/* Calculate the polling interval and count for fast LT completion */
#define DP_FAST_TRAINING_COMPLETION_RETRY_INT 10
#define DP_FAST_TRAINING_COMPLETION_RETRY_CNT \
	(DP_FAST_TRAINING_TP1_US + DP_FAST_TRAINING_TP2_US) / \
	DP_FAST_TRAINING_COMPLETION_RETRY_INT

void dcn301_setup_fast_link_training(struct link_encoder *enc)
{
	struct dcn10_link_encoder *enc10 = TO_DCN10_LINK_ENC(enc);
	uint32_t reg;

	REG_UPDATE(DP_DPHY_FAST_TRAINING, DPHY_RX_FAST_TRAINING_CAPABLE, 1);
	REG_UPDATE(DP_DPHY_FAST_TRAINING, DPHY_STREAM_RESET_DURING_FAST_TRAINING, 1);
	REG_UPDATE_2(DP_DPHY_FAST_TRAINING,
		     DPHY_FAST_TRAINING_TP1_TIME, DP_FAST_TRAINING_TP1_US,
		     DPHY_FAST_TRAINING_TP2_TIME, DP_FAST_TRAINING_TP2_US);

	// DP_DPHY_FAST_TRAINING.PHY_FAST_TRAINING_vline_EDGE_DETECT_EN = 0
	REG_UPDATE(DP_DPHY_FAST_TRAINING, DPHY_FAST_TRAINING_VBLANK_EDGE_DETECT_EN, 0x0);

	// Clear FAST_TRAINING_COMPLETE_OCCURRED bit if set
	REG_GET(DP_DPHY_FAST_TRAINING_STATUS, DPHY_FAST_TRAINING_COMPLETE_OCCURRED, &reg);

	if (reg) {
		// If fast training occurred, ack
		REG_UPDATE(DP_DPHY_FAST_TRAINING_STATUS, DPHY_FAST_TRAINING_COMPLETE_ACK, 0x1);
	}
}


bool dcn301_perform_fast_link_training_start(struct link_encoder *enc)
{
	struct dcn10_link_encoder *enc10 = TO_DCN10_LINK_ENC(enc);
	uint32_t reg1;
	uint32_t reg2;

	REG_UPDATE(DP_DPHY_FAST_TRAINING, DPHY_SW_FAST_TRAINING_START, 0x1);

	REG_GET(DP_DPHY_FAST_TRAINING_STATUS, DPHY_FAST_TRAINING_COMPLETE_OCCURRED, &reg1);
	REG_GET(DP_DPHY_FAST_TRAINING_STATUS, DPHY_FAST_TRAINING_STATE, &reg2);

	if ((reg1 == 0) && (reg2 == 0)) {
		// In this case,
		//  1. Occurred bit is not set, so we have not completed fast link training
		//  2. The FAST_TRAINING_STATE bits are not set.
		//     A value of 0 indicates there is no link training occurring at all
		// In this scenario, it seems programming the FAST_TRAINING_START bit
		// did not trigger the link training.
		// Let's retry sending the start signal.
		return false;
	}

	return true;
}

bool dcn301_perform_fast_link_training_confirm(struct link_encoder *enc)
{
	struct dcn10_link_encoder *enc10 = TO_DCN10_LINK_ENC(enc);
	uint32_t reg = 0;

	REG_UPDATE(DP_LINK_CNTL, DP_LINK_TRAINING_COMPLETE, 0x1);

	REG_WAIT(DP_DPHY_FAST_TRAINING_STATUS,
		 DPHY_FAST_TRAINING_COMPLETE_OCCURRED, 1,
		 DP_FAST_TRAINING_COMPLETION_RETRY_INT,
		 DP_FAST_TRAINING_COMPLETION_RETRY_CNT);

	REG_GET(DP_DPHY_FAST_TRAINING_STATUS, DPHY_FAST_TRAINING_COMPLETE_OCCURRED, &reg);

	return reg;
}

void dcn301_perform_fast_link_training_finish(struct link_encoder *enc)
{
	struct dcn10_link_encoder *enc10 = TO_DCN10_LINK_ENC(enc);

	// If fast training occurred, ack
	REG_UPDATE(DP_DPHY_FAST_TRAINING_STATUS, DPHY_FAST_TRAINING_COMPLETE_ACK, 0x1);
}

static const struct link_encoder_funcs dcn301_link_enc_funcs = {
	.read_state = link_enc2_read_state,
	.read_cp_state = link_enc301_read_cp_state,
	.validate_output_with_stream = dcn10_link_encoder_validate_output_with_stream,
	.hw_init = enc301_hw_init,
	.setup = dcn10_link_encoder_setup,
	.enable_tmds_output = dcn10_link_encoder_enable_tmds_output,
	.enable_dp_output = dcn20_link_encoder_enable_dp_output,
	.enable_dp_mst_output = dcn10_link_encoder_enable_dp_mst_output,
	.disable_output = dcn10_link_encoder_disable_output,
	.dp_set_lane_settings = dcn10_link_encoder_dp_set_lane_settings,
	.dp_get_link_active_status = dcn20_link_encoder_get_phy_dp_tx_status,
	.dp_set_phy_pattern = dcn10_link_encoder_dp_set_phy_pattern,
	.update_mst_stream_allocation_table = dcn10_link_encoder_update_mst_stream_allocation_table,
	.psr_program_dp_dphy_fast_training = dcn10_psr_program_dp_dphy_fast_training,
	.psr_program_secondary_packet = dcn10_psr_program_secondary_packet,
	.perform_dp_no_link_training = dcn10_perform_dp_no_link_training,
	.setup_fast_link_training = dcn301_setup_fast_link_training,
	.perform_fast_link_training_start = dcn301_perform_fast_link_training_start,
	.perform_fast_link_training_confirm = dcn301_perform_fast_link_training_confirm,
	.perform_fast_link_training_finish = dcn301_perform_fast_link_training_finish,
	.connect_dig_be_to_fe = dcn10_link_encoder_connect_dig_be_to_fe,
	.enable_hpd = dcn10_link_encoder_enable_hpd,
	.disable_hpd = dcn10_link_encoder_disable_hpd,
	.is_dig_enabled = dcn10_is_dig_enabled,
	.destroy = dcn10_link_encoder_destroy,
	.fec_set_enable = enc2_fec_set_enable,
	.fec_set_ready = enc2_fec_set_ready,
	.fec_is_active = enc2_fec_is_active,
	.get_dig_frontend = dcn10_get_dig_frontend,
	.get_dig_mode = dcn10_get_dig_mode,
	.is_in_alt_mode = dcn20_link_encoder_is_in_alt_mode,
	.get_max_link_cap = dcn20_link_encoder_get_max_link_cap,
	.link_shutdown = dcn20_link_encoder_shutdown,
	.link_wakeup = dcn20_link_encoder_wakeup,
};

void dcn301_link_encoder_construct(
	struct dcn20_link_encoder *enc20,
	const struct encoder_init_data *init_data,
	const struct encoder_feature_support *enc_features,
	const struct dcn10_link_enc_registers *link_regs,
	const struct dcn10_link_enc_aux_registers *aux_regs,
	const struct dcn10_link_enc_hpd_registers *hpd_regs,
	const struct dcn10_link_enc_shift *link_shift,
	const struct dcn10_link_enc_mask *link_mask)
{
	struct bp_encoder_cap_info bp_cap_info = {0};
	const struct dc_vbios_funcs *bp_funcs = init_data->ctx->dc_bios->funcs;
	enum bp_result result = BP_RESULT_OK;
	struct dcn10_link_encoder *enc10 = &enc20->enc10;

	enc10->base.funcs = &dcn301_link_enc_funcs;
	enc10->base.ctx = init_data->ctx;
	enc10->base.id = init_data->encoder;

	enc10->base.hpd_source = init_data->hpd_source;
	enc10->base.connector = init_data->connector;

	enc10->base.preferred_engine = ENGINE_ID_UNKNOWN;

	enc10->base.features = *enc_features;

	enc10->base.transmitter = init_data->transmitter;

	/* set the flag to indicate whether driver poll the I2C data pin
	 * while doing the DP sink detect
	 */

/*	if (dal_adapter_service_is_feature_supported(as,
		FEATURE_DP_SINK_DETECT_POLL_DATA_PIN))
		enc10->base.features.flags.bits.
			DP_SINK_DETECT_POLL_DATA_PIN = true;*/

	enc10->base.output_signals =
		SIGNAL_TYPE_DVI_SINGLE_LINK |
		SIGNAL_TYPE_DVI_DUAL_LINK |
		SIGNAL_TYPE_LVDS |
		SIGNAL_TYPE_DISPLAY_PORT |
		SIGNAL_TYPE_DISPLAY_PORT_MST |
		SIGNAL_TYPE_EDP |
		SIGNAL_TYPE_HDMI_TYPE_A;

	/* For DCE 8.0 and 8.1, by design, UNIPHY is hardwired to DIG_BE.
	 * SW always assign DIG_FE 1:1 mapped to DIG_FE for non-MST UNIPHY.
	 * SW assign DIG_FE to non-MST UNIPHY first and MST last. So prefer
	 * DIG is per UNIPHY and used by SST DP, eDP, HDMI, DVI and LVDS.
	 * Prefer DIG assignment is decided by board design.
	 * For DCE 8.0, there are only max 6 UNIPHYs, we assume board design
	 * and VBIOS will filter out 7 UNIPHY for DCE 8.0.
	 * By this, adding DIGG should not hurt DCE 8.0.
	 * This will let DCE 8.1 share DCE 8.0 as much as possible
	 */

	enc10->link_regs = link_regs;
	enc10->aux_regs = aux_regs;
	enc10->hpd_regs = hpd_regs;
	enc10->link_shift = link_shift;
	enc10->link_mask = link_mask;

	switch (enc10->base.transmitter) {
	case TRANSMITTER_UNIPHY_A:
		enc10->base.preferred_engine = ENGINE_ID_DIGA;
	break;
	case TRANSMITTER_UNIPHY_B:
		enc10->base.preferred_engine = ENGINE_ID_DIGB;
	break;
	case TRANSMITTER_UNIPHY_C:
		enc10->base.preferred_engine = ENGINE_ID_DIGC;
	break;
	case TRANSMITTER_UNIPHY_D:
		enc10->base.preferred_engine = ENGINE_ID_DIGD;
	break;
	case TRANSMITTER_UNIPHY_E:
		enc10->base.preferred_engine = ENGINE_ID_DIGE;
	break;
	case TRANSMITTER_UNIPHY_F:
		enc10->base.preferred_engine = ENGINE_ID_DIGF;
	break;
	case TRANSMITTER_UNIPHY_G:
		enc10->base.preferred_engine = ENGINE_ID_DIGG;
	break;
	default:
		ASSERT_CRITICAL(false);
		enc10->base.preferred_engine = ENGINE_ID_UNKNOWN;
	}

	/* default to one to mirror Windows behavior */
	enc10->base.features.flags.bits.HDMI_6GB_EN = 1;

	result = bp_funcs->get_encoder_cap_info(enc10->base.ctx->dc_bios,
						enc10->base.id, &bp_cap_info);

	/* Override features with DCE-specific values */
	if (result == BP_RESULT_OK) {
		enc10->base.features.flags.bits.IS_HBR2_CAPABLE =
				bp_cap_info.DP_HBR2_EN;
		enc10->base.features.flags.bits.IS_HBR3_CAPABLE =
				bp_cap_info.DP_HBR3_EN;
		enc10->base.features.flags.bits.HDMI_6GB_EN = bp_cap_info.HDMI_6GB_EN;
		enc10->base.features.flags.bits.DP_IS_USB_C =
				bp_cap_info.DP_IS_USB_C;
	} else {
		DC_LOG_WARNING("%s: Failed to get encoder_cap_info from VBIOS with error code %d!\n",
				__func__,
				result);
	}
	if (enc10->base.ctx->dc->debug.hdmi20_disable) {
		enc10->base.features.flags.bits.HDMI_6GB_EN = 0;
	}
}

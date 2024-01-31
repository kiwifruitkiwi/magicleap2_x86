/*
 * Copyright (C) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include "dmub_dkmd.h"
#include "dc.h"
#include "dc_dmub_srv.h"
#include "dmub/dmub_srv.h"
#include "core_types.h"
#include "hw_shared.h"

#define DKMD_OPTIMIZE_DISABLE				0	// no optimization
#define DKMD_OPTIMIZE_LINK_SHUTDOWN_ONLY		1	// only optimize link shutdown
#define DKMD_OPTIMIZE_LINK_WAKEUP_ONLY			2	// only optimize link wakeup, 200 vlines offset
#define DKMD_OPTIMIZE_LINK_WAKEUP_ONLY_LVL2		3	// only optimize link wakeup, 220 vlines offset
#define DKMD_OPTIMIZE_LINK_SHUTDOWN_WAKEUP		4	// optimize link shutdown, wakeup 200 vlines offset
#define DKMD_OPTIMIZE_LINK_SHUTDOWN_WAKEUP_LVL2		5	// optimize link shutdown, wakeup 220 vlines offset
#define DKMD_OPTIMIZE_LVL			DKMD_OPTIMIZE_LINK_SHUTDOWN_WAKEUP_LVL2

/*
 * to optimize dmub dark mode, we'd expect to trigger the VI of link shutdown
 * a bit earlier, considering the AUX Pre-charge that needs 22 vlines, we could
 * thus take this offset for dmub vline interrupt of link shutdown
 */
#define AUX_PRECHARGE_TIME_TO_VLINES	22

#if DKMD_OPTIMIZE_LVL == DKMD_OPTIMIZE_DISABLE
#define VLINE_OFFSET_FOR_LINK_SHUTDOWN	0
#define VLINE_OFFSET_FOR_LINK_WAKEUP	0

#elif DKMD_OPTIMIZE_LVL == DKMD_OPTIMIZE_LINK_SHUTDOWN_ONLY
#define VLINE_OFFSET_FOR_LINK_SHUTDOWN	AUX_PRECHARGE_TIME_TO_VLINES
#define VLINE_OFFSET_FOR_LINK_WAKEUP	0

#elif DKMD_OPTIMIZE_LVL == DKMD_OPTIMIZE_LINK_WAKEUP_ONLY
#define VLINE_OFFSET_FOR_LINK_SHUTDOWN	0
#define VLINE_OFFSET_FOR_LINK_WAKEUP	200

#elif DKMD_OPTIMIZE_LVL == DKMD_OPTIMIZE_LINK_WAKEUP_ONLY_LVL2
#define VLINE_OFFSET_FOR_LINK_SHUTDOWN	0
#define VLINE_OFFSET_FOR_LINK_WAKEUP	220

#elif DKMD_OPTIMIZE_LVL == DKMD_OPTIMIZE_LINK_SHUTDOWN_WAKEUP
#define VLINE_OFFSET_FOR_LINK_SHUTDOWN	AUX_PRECHARGE_TIME_TO_VLINES
#define VLINE_OFFSET_FOR_LINK_WAKEUP	200

#elif DKMD_OPTIMIZE_LVL == DKMD_OPTIMIZE_LINK_SHUTDOWN_WAKEUP_LVL2
#define VLINE_OFFSET_FOR_LINK_SHUTDOWN	AUX_PRECHARGE_TIME_TO_VLINES
#define VLINE_OFFSET_FOR_LINK_WAKEUP	220
#endif

/* the vertical line position to power up the PHY/LINK no matter HBR2/3 */
#define VLINE_FOR_LINK_WAKEUP_BASELINE		2800

/* there's a delta of latency for no-pattern link-training between HBR2 and HBR3
 * link rate. So if stream is running in HBR3 rate, there's an additional budget
 * for link wakeup optimization by pushing the vline position to trigger DMUB
 * link wakeup vertical interrupt later.
 * The latency of NP LT for HBR2 and HBR3 are as below for reference:
 * - HBR2 - ~970 us
 * - HBR3 - ~649 us
 * Let's take 320 us ~= 68 vlines
 */
#define VLINE_OFFSET_FOR_LINK_WAKEUP_IF_HBR3	68

/* the baseline VTOTAL is 3552 vlines, but the timings of final production would
 * be 3576 instead. However, the default vactive is a fixed 1760 vlines. For any
 * timings change, we'd take this into account for link shutdown optimization.
 */
#define MERO_VTOTAL_BASELINE		3552

static void dmub_dkmd_init(struct dmub_dkmd *dmub,  struct dkmd_init_params *init_params)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_init.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_init.header.sub_type = DMUB_CMD__DKMD_INIT;
	cmd.dkmd_init.header.payload_bytes = sizeof(struct dmub_cmd_dkmd_init_data);

	/* needed parameters bundle */
	cmd.dkmd_init.dkmd_init_data.dpphy_inst = init_params->link_res.dpphy_inst;
	cmd.dkmd_init.dkmd_init_data.aux_inst = init_params->link_res.aux_inst;
	cmd.dkmd_init.dkmd_init_data.otg_inst = init_params->stream_res.otg_inst;
	cmd.dkmd_init.dkmd_init_data.digbe_inst = init_params->stream_res.digbe_inst;
	/* VI params */
	cmd.dkmd_init.dkmd_init_data.shutdown_vline = init_params->vlines.shutdown_vline;
	cmd.dkmd_init.dkmd_init_data.wakeup_vline = init_params->vlines.wakeup_vline;
	cmd.dkmd_init.dkmd_init_data.pdt_vi_vline = init_params->vlines.pdt_vi_vline;
	cmd.dkmd_init.dkmd_init_data.shutdown_vline_offset = init_params->vlines.shutdown_vline_offset;
	/* link params */
	cmd.dkmd_init.dkmd_init_data.link_rate = init_params->link_params.link_rate;
	cmd.dkmd_init.dkmd_init_data.lane_count = init_params->link_params.lane_count;
	cmd.dkmd_init.dkmd_init_data.fec_caps = init_params->link_params.fec_caps;
	cmd.dkmd_init.dkmd_init_data.no_aux_lt = init_params->link_params.no_aux_lt;

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	/* use 2 pr_debug to avoid "exceeding 120 characters" or "quoted string split across lines"
	 * WARNING on jenkins build failure
	 */
	pr_debug("[DMUB_DKMD] init, dpphy %d, aux %d, otg %d, digbe %d, vi shutdown %d, vi wakeup %d, vi pdt %d\n",
		 init_params->link_res.dpphy_inst, init_params->link_res.aux_inst,
		 init_params->stream_res.otg_inst, init_params->stream_res.digbe_inst,
		 init_params->vlines.shutdown_vline, init_params->vlines.wakeup_vline,
		 init_params->vlines.pdt_vi_vline);
	pr_debug("[DMUB_DKMD] vi shutdown offset %d, link rate 0x%x, lane count %d, fec cap %d, no aux LT %d\n",
		 init_params->vlines.shutdown_vline_offset,
		 init_params->link_params.link_rate, init_params->link_params.lane_count,
		 init_params->link_params.fec_caps, init_params->link_params.no_aux_lt);
}

static void dmub_dkmd_reset(struct dmub_dkmd *dmub)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_reset.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_reset.header.sub_type = DMUB_CMD__DKMD_RESET;
	cmd.dkmd_reset.header.payload_bytes = 0;

	/* no parameters needed */

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	pr_debug("[DMUB_DKMD] reset\n");
}

static void dmub_dkmd_request(struct dmub_dkmd *dmub, enum dmub_dkmd_mode dkmd_req_mode)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_request.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_request.header.sub_type = DMUB_CMD__DKMD_REQUEST;
	cmd.dkmd_request.header.payload_bytes = sizeof(struct dmub_cmd_dkmd_request_data);

	/* needed parameters bundle */
	cmd.dkmd_request.dkmd_request_data.request_mode = dkmd_req_mode;

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	pr_debug("[DMUB_DKMD] request dkmd mode %d\n", dkmd_req_mode);
}

/**
 * Get DMUB FW dark mode status
 * Send command from x86 to DMUB FW and DMUB will process to dump the current
 * dark mode status and write into a specific SCRATCH register (SCRATCH9), which
 * is only used to save the dark mode related status.
 */
static void dmub_dkmd_get_status(struct dmub_dkmd *dmub)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_get_status.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_get_status.header.sub_type = DMUB_CMD__DKMD_GET_STATUS;
	cmd.dkmd_get_status.header.payload_bytes = 0;

	/* no parameters needed */

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	pr_debug("[DMUB_DKMD] call DMUB to dump dkmd status to SCRATCH9\n");
}

/* send cmd from x86 to DMUB FW to start counting the PHY (link) wakeup/shutdown occurrence */
static void dmub_dkmd_start_phy_toggle_ct(struct dmub_dkmd *dmub)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_start_phy_toggle_ct.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_start_phy_toggle_ct.header.sub_type = DMUB_CMD__DKMD_START_PHY_TOGGLE_CT;
	cmd.dkmd_start_phy_toggle_ct.header.payload_bytes = 0;

	/* no parameters needed */

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	pr_debug("[DMUB_DKMD] start counting PHY Wakeup/Shutdown\n");
}

/* send cmd from x86 to DMUB FW to stop counting the PHY (link) wakeup/shutdown occurrence */
static void dmub_dkmd_stop_phy_toggle_ct(struct dmub_dkmd *dmub)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_stop_phy_toggle_ct.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_stop_phy_toggle_ct.header.sub_type = DMUB_CMD__DKMD_STOP_PHY_TOGGLE_CT;
	cmd.dkmd_stop_phy_toggle_ct.header.payload_bytes = 0;

	/* no parameters needed */

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	pr_debug("[DMUB_DKMD] stop counting PHY Wakeup/Shutdown\n");
}

/* send cmd from x86 to DMUB FW to reset the count of PHY (link) wakeup/shutdown occurrence */
static void dmub_dkmd_reset_phy_toggle_ct(struct dmub_dkmd *dmub)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_reset_phy_toggle_ct.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_reset_phy_toggle_ct.header.sub_type = DMUB_CMD__DKMD_RESET_PHY_TOGGLE_CT;
	cmd.dkmd_reset_phy_toggle_ct.header.payload_bytes = 0;

	/* no parameters needed */

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	pr_debug("[DMUB_DKMD] reset PHY Wakeup/Shutdown count\n");
}

/**
 * Get DMUB FW PHY (link) status toggling (wakeup/shutdown) count
 * Send command from x86 to DMUB FW and DMUB will process to dump the occurrence count
 * of PHY status toggling and write into a specific SCRATCH register (SCRATCH11), which
 * is only used to save the phy status toggle count.
 * - MSB 16 bits [16:31] - count of PHY wakeup
 * - LSB 16 bits [0:15]  - count of PHY shutdown
 */
static void dmub_dkmd_get_phy_toggle_ct(struct dmub_dkmd *dmub)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_get_phy_toggle_ct.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_get_phy_toggle_ct.header.sub_type = DMUB_CMD__DKMD_GET_PHY_TOGGLE_CT;
	cmd.dkmd_get_phy_toggle_ct.header.payload_bytes = 0;

	/* no parameters needed */

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	pr_debug("[DMUB_DKMD] dump dkmd phy status toggle count to SCRATCH11\n");
}

/* call the interface and DMUB FW log the x86 driver event as reference for debugging */
static void dmub_dkmd_debug(struct dmub_dkmd *dmub, enum dmub_dkmd_debug_evt evt)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc_ctx = dmub->ctx;

	cmd.dkmd_debug.header.type = DMUB_CMD__DKMD;
	cmd.dkmd_debug.header.sub_type = DMUB_CMD__DKMD_DEBUG;
	cmd.dkmd_debug.header.payload_bytes = sizeof(struct dmub_cmd_dkmd_debug_data);

	/* needed parameters bundle */
	cmd.dkmd_debug.dkmd_debug_data.event = evt;

	dc_dmub_srv_cmd_queue(dc_ctx->dmub_srv, &cmd);
	dc_dmub_srv_cmd_execute(dc_ctx->dmub_srv);
	dc_dmub_srv_wait_idle(dc_ctx->dmub_srv);

	pr_debug("[DMUB_DKMD] evt %d\n", evt);
}

static const struct dmub_dkmd_funcs dkmd_funcs = {
	.dkmd_init		= dmub_dkmd_init,
	.dkmd_reset		= dmub_dkmd_reset,
	.dkmd_request		= dmub_dkmd_request,
	.dkmd_get_status	= dmub_dkmd_get_status,
	.dkmd_start_phy_toggle_ct = dmub_dkmd_start_phy_toggle_ct,
	.dkmd_stop_phy_toggle_ct  = dmub_dkmd_stop_phy_toggle_ct,
	.dkmd_reset_phy_toggle_ct = dmub_dkmd_reset_phy_toggle_ct,
	.dkmd_get_phy_toggle_ct   = dmub_dkmd_get_phy_toggle_ct,
	.dkmd_debug		= dmub_dkmd_debug,
};

/**
 * Construct DKMD object.
 */
static void dmub_dkmd_construct(struct dmub_dkmd *dkmd, struct dc_context *ctx)
{
	dkmd->ctx = ctx;
	dkmd->funcs = &dkmd_funcs;
}

/**
 * Allocate and initialize DKMD object.
 */
struct dmub_dkmd *dmub_dkmd_create(struct dc_context *ctx)
{
	struct dmub_dkmd *dkmd = kzalloc(sizeof(struct dmub_dkmd), GFP_KERNEL);

	if (dkmd == NULL) {
		BREAK_TO_DEBUGGER();
		return NULL;
	}

	dmub_dkmd_construct(dkmd, ctx);

	return dkmd;
}

/**
 * Deallocate DKMD object.
 */
void dmub_dkmd_destroy(struct dmub_dkmd **dmub)
{
	kfree(*dmub);
	*dmub = NULL;
}

static bool dkmd_has_enough_vblank(uint32_t h_total, uint32_t v_blank, uint32_t pix_clk_100hz)
{
	uint32_t vblank_us = ((uint64_t) h_total * (uint64_t) v_blank) * 10000 / pix_clk_100hz;

	pr_debug("[DMUB_DKMD] has vblank (in us) %u\n", vblank_us);

	return (vblank_us > MERO_DARK_MODE_MINIMUM_VBLANK_INTERVAL_IN_US);
}

/**
 * Get required dkmd init & context params from given link
 */
bool dmub_dkmd_get_params_from_link(struct dmub_dkmd *dmub, struct dc_link *link, struct dkmd_init_params *init_params)
{
	struct pipe_ctx *pipe_ctx = NULL;
	struct resource_context *res_ctx;
	int i = 0;

	if (link && link->ctx && link->ctx->dc && link->ctx->dc->current_state)
		res_ctx = &link->ctx->dc->current_state->res_ctx;
	else
		return FALSE;

	/* find pipe context from given link */
	for (i = 0; i < MAX_PIPES; ++i) {
		if (res_ctx &&
		    res_ctx->pipe_ctx[i].stream &&
		    res_ctx->pipe_ctx[i].stream->link &&
		    res_ctx->pipe_ctx[i].stream->link == link &&
		    dc_is_dp_signal(res_ctx->pipe_ctx[i].stream->link->connector_signal)) {
			pipe_ctx = &res_ctx->pipe_ctx[i];
			break;
		}
	}

	/* did not find pipe context */
	if (!pipe_ctx)
		return FALSE;

	/* ddc channel id for aux inst */
	dmub->dkmd_ctx.channel = link->ddc->ddc_pin->hw_info.ddc_channel;
	init_params->link_res.aux_inst = link->ddc->ddc_pin->hw_info.ddc_channel;

	/* transmitter id for dpphy inst */
	dmub->dkmd_ctx.transmitterId = link->link_enc->transmitter;
	init_params->link_res.dpphy_inst = link->link_enc->transmitter;

	/* engine id for dig fe inst */
	dmub->dkmd_ctx.engineId = link->link_enc->preferred_engine;
	init_params->stream_res.digfe_inst = link->link_enc->preferred_engine;

	/* currently set controller ID to UNDEFINED */
	dmub->dkmd_ctx.controllerId = CONTROLLER_ID_UNDEFINED;
	init_params->stream_res.digbe_inst = link->link_enc->transmitter;

	/* tg inst for otg inst */
	if (pipe_ctx->stream_res.tg)
		init_params->stream_res.otg_inst = pipe_ctx->stream_res.tg->inst;

	/* timings params */
	dmub->dkmd_ctx.timing.v_total = pipe_ctx->stream->timing.v_total;
	dmub->dkmd_ctx.timing.v_active = pipe_ctx->stream->timing.v_addressable;
	dmub->dkmd_ctx.timing.v_blank = dmub->dkmd_ctx.timing.v_total - dmub->dkmd_ctx.timing.v_active;
	/* otg vblank start = frame end - front porch */
	dmub->dkmd_ctx.timing.otg_v_blank_start = pipe_ctx->stream->timing.v_total -
						  pipe_ctx->stream->timing.v_front_porch;
	/* otg vblank end = vblank start - vactive */
	dmub->dkmd_ctx.timing.otg_v_blank_end = dmub->dkmd_ctx.timing.otg_v_blank_start -
						pipe_ctx->stream->timing.v_border_bottom -
						pipe_ctx->stream->timing.v_border_top -
						pipe_ctx->stream->timing.v_addressable;
	/* otg vline0 --> vsync start */
	dmub->dkmd_ctx.timing.otg_v_sync_end = pipe_ctx->stream->timing.v_sync_width;

	/* vlines for shutdown/wakeup */
	init_params->vlines.shutdown_vline = dmub->dkmd_ctx.timing.otg_v_blank_start;
	init_params->vlines.shutdown_vline_offset = VLINE_OFFSET_FOR_LINK_SHUTDOWN;

	if (!dkmd_has_enough_vblank(pipe_ctx->stream->timing.h_total,
				    dmub->dkmd_ctx.timing.v_blank,
				    pipe_ctx->stream->timing.pix_clk_100hz))
		return FALSE;

	/* currently hardcode to 2800 lines, later we need to measure and optimize this value to get as much as
	 * link shutdown duration as possible.
	 * Notice that the NP LT duration under HBR3 and HBR2 are different, we'd also consider this delta for
	 * additional optimization.
	 * If timings->vtotal is larger than basline 3552, we'd push additional offset for link wakeup optimize.
	 */
	init_params->vlines.wakeup_vline = VLINE_FOR_LINK_WAKEUP_BASELINE + VLINE_OFFSET_FOR_LINK_WAKEUP;

	if (link->cur_link_settings.link_rate == LINK_RATE_HIGH3)
		init_params->vlines.wakeup_vline += VLINE_OFFSET_FOR_LINK_WAKEUP_IF_HBR3;

	if (pipe_ctx->stream->timing.v_total > MERO_VTOTAL_BASELINE)
		init_params->vlines.wakeup_vline += (pipe_ctx->stream->timing.v_total - MERO_VTOTAL_BASELINE);

	init_params->vlines.pdt_vi_vline = dmub->dkmd_ctx.timing.otg_v_blank_end + 1;

	/* link params */
	dmub->dkmd_ctx.link_params.link_rate = link->cur_link_settings.link_rate;
	dmub->dkmd_ctx.link_params.lane_count = link->cur_link_settings.lane_count;
	dmub->dkmd_ctx.link_params.fec_caps = link->dpcd_caps.fec_cap.bits.FEC_CAPABLE;
	dmub->dkmd_ctx.link_params.no_aux_lt = link->dpcd_caps.max_down_spread.bits.NO_AUX_HANDSHAKE_LINK_TRAINING;
	init_params->link_params.link_rate = link->cur_link_settings.link_rate;
	init_params->link_params.lane_count = link->cur_link_settings.lane_count;
	init_params->link_params.fec_caps = link->dpcd_caps.fec_cap.bits.FEC_CAPABLE;
	init_params->link_params.no_aux_lt = link->dpcd_caps.max_down_spread.bits.NO_AUX_HANDSHAKE_LINK_TRAINING;

	/* use 3 pr_debug to avoid "exceeding 120 characters" WARNING on jenkins build failure */
	pr_info("[DMUB_DKMD] get link params: dpphy %d, aux %d, otg %d, digbe %d, vi std %d, vi wkp %d\n",
		 init_params->link_res.dpphy_inst, init_params->link_res.aux_inst,
		 init_params->stream_res.otg_inst, init_params->stream_res.digbe_inst,
		 init_params->vlines.shutdown_vline, init_params->vlines.wakeup_vline);
	pr_info("[DMUB_DKMD] get link params: vtotal %d, vblank %d, vactive %d, otg vbs %d, otg vbe %d, otg vse %d\n",
		 dmub->dkmd_ctx.timing.v_total, dmub->dkmd_ctx.timing.v_blank,
		 dmub->dkmd_ctx.timing.v_active, dmub->dkmd_ctx.timing.otg_v_blank_start,
		 dmub->dkmd_ctx.timing.otg_v_blank_end, dmub->dkmd_ctx.timing.otg_v_sync_end);
	pr_info("[DMUB_DKMD] get link ct: %d, rate: %x, fec cap: %d, no aux lt: %d\n",
		link->cur_link_settings.lane_count,
		link->cur_link_settings.link_rate,
		link->dpcd_caps.fec_cap.bits.FEC_CAPABLE,
		link->dpcd_caps.max_down_spread.bits.NO_AUX_HANDSHAKE_LINK_TRAINING);

	return TRUE;
}

#define DMUB_SCRATCH9_DKMD_STATUS_MASK		0xF0
#define DMUB_SCRATCH9_DKMD_STATUS_SHIFT		4
#define DMUB_SCRATCH9_DKMD_CURR_MODE_MASK	0x3
#define DMUB_SCRATCH9_DKMD_CURR_MODE_SHIFT	0
#define DMUB_SCRATCH9_DKMD_REQ_MODE_MASK	0xC
#define DMUB_SCRATCH9_DKMD_REQ_MODE_SHIFT	2
#define DMUB_SCRATCH9_DKMD_STATUS_RD_RETRIES	1000
#define DMUB_SCRATCH11_DKMD_PHY_TOGGLE_UP_CT_MASK	0xFFFF0000
#define DMUB_SCRATCH11_DKMD_PHY_TOGGLE_UP_CT_SHIFT	16
#define DMUB_SCRATCH11_DKMD_PHY_TOGGLE_DOWN_CT_MASK	0xFFFF
#define DMUB_SCRATCH11_DKMD_PHY_TOGGLE_DOWN_CT_SHIFT	0

bool dmub_dkmd_polling_status(struct dmub_dkmd *dkmd, enum dmub_dkmd_status expected_st)
{
	uint32_t val;
	int i;
	enum dmub_dkmd_status actual_st;
	struct dmub_srv *dmub;

	if (!dkmd) {
		DRM_WARN_ONCE("Invalid (null) dmub_dkmd ptr\n");
		return false;
	}

	dmub = dkmd->ctx->dmub_srv->dmub;
	if (!dmub) {
		DRM_WARN_ONCE("Invalid (null) dmub_srv ptr\n");
		return false;
	}

	for (i = 0; i < DMUB_SCRATCH9_DKMD_STATUS_RD_RETRIES; ++i) {
		dkmd->funcs->dkmd_get_status(dkmd);
		udelay(10);
		val = dmub->hw_funcs.get_scratch9(dmub);

		/* scratch9 bitfields:
		 * bit 0-1: dark mode current mode
		 * bit 2-3: dark mode request mode
		 * bit 4-7: dark mode status
		 */
		actual_st = (val & DMUB_SCRATCH9_DKMD_STATUS_MASK) >> DMUB_SCRATCH9_DKMD_STATUS_SHIFT;

		if (actual_st == expected_st)
			return true;
	}

	if (i == DMUB_SCRATCH9_DKMD_STATUS_RD_RETRIES)
		DRM_WARN_ONCE("Max retries hit when polling DMUB dark mode status for %d\n", expected_st);

	return false;
}

/*
 * dmub_dkmd_read_status: read dark mode status and mode information from DMUB FW
 */
bool dmub_dkmd_read_status(struct dmub_dkmd *dkmd,
			   enum dmub_dkmd_mode *op_mode, enum dmub_dkmd_mode *req_mode, enum dmub_dkmd_status *status)
{
	uint32_t val;
	struct dmub_srv *dmub;

	if (!dkmd) {
		DRM_WARN_ONCE("Invalid (null) dmub_dkmd ptr\n");
		return false;
	}

	dmub = dkmd->ctx->dmub_srv->dmub;
	if (!dmub) {
		DRM_WARN_ONCE("Invalid (null) dmub_srv ptr\n");
		return false;
	}

	dkmd->funcs->dkmd_get_status(dkmd);
	udelay(10);
	val = dmub->hw_funcs.get_scratch9(dmub);

	/* scratch9 bitfields:
	 * bit 0-1: dark mode current mode
	 * bit 2-3: dark mode request mode
	 * bit 4-7: dark mode status
	 */
	*status   = (val & DMUB_SCRATCH9_DKMD_STATUS_MASK) >> DMUB_SCRATCH9_DKMD_STATUS_SHIFT;
	*op_mode  = (val & DMUB_SCRATCH9_DKMD_CURR_MODE_MASK) >> DMUB_SCRATCH9_DKMD_CURR_MODE_SHIFT;
	*req_mode = (val & DMUB_SCRATCH9_DKMD_REQ_MODE_MASK) >> DMUB_SCRATCH9_DKMD_REQ_MODE_SHIFT;
	DRM_DEBUG_DRIVER("[DMUB_DKMD] curr md %d, req md %d, st %d\n", *op_mode, *req_mode, *status);

	return true;
}

bool dmub_dkmd_read_phy_toggle_count(struct dmub_dkmd *dkmd, uint16_t *phy_toggle_up_ct, uint16_t *phy_toggle_down_ct)
{
	uint32_t val;
	struct dmub_srv *dmub;

	if (!dkmd) {
		DRM_WARN_ONCE("Invalid (null) dmub_dkmd ptr\n");
		return false;
	}

	dmub = dkmd->ctx->dmub_srv->dmub;
	if (!dmub) {
		DRM_WARN_ONCE("Invalid (null) dmub_srv ptr\n");
		return false;
	}

	dkmd->funcs->dkmd_get_phy_toggle_ct(dkmd);
	udelay(10);
	val = dmub->hw_funcs.get_scratch11(dmub);

	/* scratch11 bitfields:
	 * bits 16-31: PHY status toggle UP (link wakeup) count
	 * bits 0-15:  PHY status toggle DOWN (link shutdown) count
	 */
	*phy_toggle_up_ct =
		(val & DMUB_SCRATCH11_DKMD_PHY_TOGGLE_UP_CT_MASK) >> DMUB_SCRATCH11_DKMD_PHY_TOGGLE_UP_CT_SHIFT;
	*phy_toggle_down_ct =
		(val & DMUB_SCRATCH11_DKMD_PHY_TOGGLE_DOWN_CT_MASK) >> DMUB_SCRATCH11_DKMD_PHY_TOGGLE_DOWN_CT_SHIFT;

	pr_debug("[DMUB_DKMD] link wkp ct %d, link stdn ct %d\n", *phy_toggle_up_ct, *phy_toggle_down_ct);

	return true;
}

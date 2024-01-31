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
#ifndef _DMUB_DKMD_H_
#define _DMUB_DKMD_H_

#include "os_types.h"
#include "dc_types.h"

/* for dark mode, we need a minimum vblank period duration, otherwise it would be
 * impossible to apply a link shutdown + link wakeup sequence.
 * For example, running @ 60Hz with MERO timing, i.e. 1760 vactive & 3552 vtotal,
 * we have enough vblank space to do vblank shutdown dark mode.
 * However, if running @ 120Hz, the vactive in normal mode is still 1760 while
 * the vtotal would be shrinked to 1776, i.e. only 16 vblank lines (~75 us),
 * the dark mode does NOT make sense.
 */
#define MERO_DARK_MODE_MINIMUM_VBLANK_INTERVAL_IN_US	2500

enum dmub_dkmd_status {
	DMUB_DKMD_DISABLED = 0,
	DMUB_DKMD_INITIALIZED,
	DMUB_DKMD_SHUTDOWN_START,	/* shutdown sequence start */
	DMUB_DKMD_FEC_DISABLE,		/* fec disable */
	DMUB_DKMD_DPRX_POWER_OFF,	/* set DPCD600 -> 2 and get ACK from DPRX */
	DMUB_DKMD_PHY_OFF,		/* physical layer main link off */
	DMUB_DKMD_SHUTDOWN_DONE,	/* shutdown sequence end, darkmode entered */
	DMUB_DKMD_WAIT_FOR_WAKEUP_TRIGGER,	/* waiting for a wakeup trigger request when in full dkmd */
	DMUB_DKMD_WAKEUP_START,		/* wakeup sequence start */
	DMUB_DKMD_PHY_ON,		/* physical layer main link on */
	DMUB_DKMD_DPRX_POWER_ON,	/* set DPCD600 -> 1 and get ACK from DPRX */
	DMUB_DKMD_NP_LT,		/* no-pattern link training */
	DMUB_DKMD_FEC_ENABLE,		/* fec enable */
	DMUB_DKMD_WAKEUP_DONE,		/* wakeup sequence end, darkmode exited */
	/* don't add state element below */
	DMUB_DKMD_STATE_UNDEFINED,
};

enum dmub_dkmd_mode {
	DMUB_DKMD_MODE_DISABLE = 0,
	DMUB_DKMD_MODE_FULL,
	DMUB_DKMD_MODE_VBLANK_ONLY,
	/* do not define below this line */
	DMUB_DKMD_MODE_UNDEFINED,
};

enum dmub_dkmd_debug_evt {
	X86_DM_ATOMIC_COMMIT_TAIL_START = 0,
	X86_DM_ATOMIC_COMMIT_TAIL_END,
	X86_DM_ATOMIC_COMMIT_CHECK_START,
	X86_DM_ATOMIC_COMMIT_CHECK_END,
	/* do not define below this line */
	UNKNOWN_DKMD_DEBUG_EVT,
};

struct dkmd_vlines {
	uint16_t shutdown_vline;	/* OTG.vline # to schedule link shutdown */
	uint16_t wakeup_vline;		/* OTG.vline # to schedule link wakeup */
	uint16_t pdt_vi_vline;
	uint16_t shutdown_vline_offset;	/* offset applied for link shutdown optimize */
};

struct dkmd_link_params {
	uint8_t link_rate;
	uint8_t lane_count;
	uint8_t fec_caps;
	uint8_t no_aux_lt;
};

struct dkmd_timings {
	uint16_t v_total;	/* # of vertical total lines */
	uint16_t v_blank;	/* # of vertical blank lines */
	uint16_t v_active;	/* # of vertical addressable lines */
	uint16_t otg_v_blank_start;	/* line # of vblank start of OTG */
	uint16_t otg_v_blank_end;	/* line # of vblank end of OTG */
	uint16_t otg_v_sync_end;	/* line # of vsync end of OTG */
};

/* Link resources for DKMD-enabled display. */
struct dkmd_link_res {
	/**
	 * @dpphy_inst: PHY instance.
	 */
	uint8_t dpphy_inst;

	/**
	 * @aux_inst: AUX instance.
	 */
	uint8_t aux_inst;

	uint8_t pad[2];
};

/* Stream resources for DKMD-enabled display. */
struct dkmd_stream_res {
	/**
	 * @opp_mask: indicating multiple opps combined for ODM.
	 *            eg, if bit0 is 1, opp_inst = 0. if both bit0
	 *            and bit1 are 1, opp instance 0 and 3 are combined
	 */
	uint8_t opp_mask;

	/**
	 * @otg_inst: OTG instance.
	 */
	uint8_t otg_inst;

	/**
	 * @digfe_inst: DIG frontend instance.
	 */
	uint8_t digfe_inst;

	/**
	 * @digbe_inst: DIG backend instance.
	 */
	uint8_t digbe_inst;

	/**
	 * @digfe_inst_2: the second DIG frontend instance.
	 */
	uint8_t digfe_inst_2;

	// Add tailing pad[] here to ensure 4-bytes alignment
	uint8_t pad[3];
};

/* parameter bundle for dmub dkmd init interface */
struct dkmd_init_params {
	struct dkmd_link_res link_res;
	struct dkmd_stream_res stream_res;
	struct dkmd_vlines vlines;
	struct dkmd_link_params link_params;
};

/* define dark-mode context for dmub internal usage (TBD) */
struct dkmd_context {
	/* ddc line */
	enum channel_id channel;

	/* Transmitter id */
	enum transmitter transmitterId;

	enum engine_id engineId;

	enum controller_id controllerId;

	/* link/phy to disable/enable */
	uint8_t link_idx;

	enum dmub_dkmd_status dkmd_status;

	/* 0, 1, 2, dkmd mode requested to use in next frame */
	enum dmub_dkmd_mode request_mode;

	/* 0, 1, 2, dkmd current running mode */
	enum dmub_dkmd_mode current_mode;

	/* shutdown/wakeup vline */
	struct dkmd_vlines vlines;

	/* mode timings, vtotal = vblank + vactive */
	struct dkmd_timings timing;

	/* link resource */
	struct dkmd_link_res link_res;

	/* stream resource */
	struct dkmd_stream_res stream_res;

	/* link parameters */
	struct dkmd_link_params link_params;

	/* 0: disabled, de-register shutdown/wakeup
	 * 1: enabled
	 */
	uint8_t enabled;

	uint8_t wakeup_request;

	uint8_t initialized;
};

struct dmub_dkmd {
	struct dc_context *ctx;
	const struct dmub_dkmd_funcs *funcs;
	struct dkmd_context dkmd_ctx;
};

struct dmub_dkmd_funcs {
	void (*dkmd_init)(struct dmub_dkmd *dmub, struct dkmd_init_params *init_params);
	void (*dkmd_reset)(struct dmub_dkmd *dmub);
	void (*dkmd_request)(struct dmub_dkmd *dmub, enum dmub_dkmd_mode dkmd_req_mode);
	void (*dkmd_get_status)(struct dmub_dkmd *dmub);
	void (*dkmd_start_phy_toggle_ct)(struct dmub_dkmd *dmub);
	void (*dkmd_stop_phy_toggle_ct)(struct dmub_dkmd *dmub);
	void (*dkmd_reset_phy_toggle_ct)(struct dmub_dkmd *dmub);
	void (*dkmd_get_phy_toggle_ct)(struct dmub_dkmd *dmub);
	void (*dkmd_debug)(struct dmub_dkmd *dmub, enum dmub_dkmd_debug_evt evt);
};

struct dmub_dkmd *dmub_dkmd_create(struct dc_context *ctx);
void dmub_dkmd_destroy(struct dmub_dkmd **dmub);

bool dmub_dkmd_get_params_from_link(struct dmub_dkmd *dmub, struct dc_link *link, struct dkmd_init_params *init_params);
bool dmub_dkmd_polling_status(struct dmub_dkmd *dkmd, enum dmub_dkmd_status expected_st);
bool dmub_dkmd_read_status(struct dmub_dkmd *dkmd,
			   enum dmub_dkmd_mode *op_mode, enum dmub_dkmd_mode *req_mode, enum dmub_dkmd_status *status);
bool dmub_dkmd_read_phy_toggle_count(struct dmub_dkmd *dkmd, uint16_t *phy_toggle_up_ct, uint16_t *phy_toggle_down_ct);

#endif /* _DMUB_DKMD_H_ */

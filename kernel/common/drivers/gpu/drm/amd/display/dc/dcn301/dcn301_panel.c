/*
 * Copyright 2012-15 Advanced Micro Devices, Inc.
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
#include "dc_dmub_srv.h"
#include "dcn301_panel.h"

#define TO_DCN301_PANEL(panel)\
	container_of(panel, struct dcn301_panel, base)

#define CTX \
	dcn301_panel->base.ctx

#define DC_LOGGER \
	dcn301_panel->base.ctx->logger

#define REG(reg)\
	dcn301_panel->regs->reg

#undef FN
#define FN(reg_name, field_name) \
	dcn301_panel->shift->field_name, dcn301_panel->mask->field_name

void dcn301_panel_hw_init(struct panel *panel)
{

}

void dcn301_panel_destroy(struct panel **panel)
{
	struct dcn301_panel *dcn301_panel = TO_DCN301_PANEL(*panel);

	kfree(dcn301_panel);
	*panel = NULL;
}

bool dcn301_is_panel_backlight_on(struct panel *panel)
{
	struct dcn301_panel *dcn301_panel = TO_DCN301_PANEL(panel);
	uint32_t value;

	REG_GET(PWRSEQ_CNTL, PANEL_BLON, &value);

	return value;
}

bool dcn301_is_panel_powered_on(struct panel *panel)
{
	struct dcn301_panel *dcn301_panel = TO_DCN301_PANEL(panel);
	uint32_t pwr_seq_state, dig_on, dig_on_ovrd;

	REG_GET(PWRSEQ_STATE, PANEL_PWRSEQ_TARGET_STATE_R, &pwr_seq_state);

	REG_GET_2(PWRSEQ_CNTL, PANEL_DIGON, &dig_on, PANEL_DIGON_OVRD, &dig_on_ovrd);

	return (pwr_seq_state == 1) || (dig_on == 1 && dig_on_ovrd == 1);
}

static const struct panel_funcs dcn301_link_panel_funcs = {
	.destroy = dcn301_panel_destroy,
	.hw_init = dcn301_panel_hw_init,
	.is_panel_backlight_on = dcn301_is_panel_backlight_on,
	.is_panel_powered_on = dcn301_is_panel_powered_on,
};

void dcn301_panel_construct(
	struct dcn301_panel *dcn301_panel,
	const struct panel_init_data *init_data,
	const struct dce_panel_registers *regs,
	const struct dcn301_panel_shift *shift,
	const struct dcn301_panel_mask *mask)
{
	dcn301_panel->regs = regs;
	dcn301_panel->shift = shift;
	dcn301_panel->mask = mask;

	dcn301_panel->base.funcs = &dcn301_link_panel_funcs;
	dcn301_panel->base.ctx = init_data->ctx;
	dcn301_panel->base.inst = init_data->inst;
}

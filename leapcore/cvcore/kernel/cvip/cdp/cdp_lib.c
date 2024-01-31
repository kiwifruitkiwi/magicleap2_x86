// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

#include <linux/platform_device.h>
#include <linux/io.h>

#include "cdp.h"
#include "cdp_common.h"
#include "cdp_top.h"
#include "cdp_desc.h"
#include "cdp_errors.h"
#include "cdp_lib.h"
#include "cdp_config.h"

#define dbg_print  pr_err

volatile cdp_t *cdp;

/********************
 * STREAM LEVEL API *
 ********************/

int32_t cdp_lib_init(void *base)
{

    int32_t rval;
    cdp_rgf_cdp0_frame_max_reg_t stage0_max_rows_cols_reg;
    cdp_rgf_cdp1_frame_max_reg_t stage1_max_rows_cols_reg;
    cdp_rgf_cdp2_frame_max_reg_t stage2_max_rows_cols_reg;

    cdp = (volatile cdp_t *)base;

    rval = cdp_init_descriptor(&cdp_default_desc);
    if (IS_ERR(rval)) {
        return rval;
    }

    // Reset CDP interrupt vector
    write_reg(cdp->eng.cdp_rgf.cdp_global_interrupt_reg, 0x1fffff);

    // Raise maximum allowed rows/cols in image (default is 600*800)
    stage0_max_rows_cols_reg.bitfield.lines_number = 1024;
    stage0_max_rows_cols_reg.bitfield.pixels_number = 1024;

    stage1_max_rows_cols_reg.bitfield.lines_number = 1024;
    stage1_max_rows_cols_reg.bitfield.pixels_number = 1024;

    stage2_max_rows_cols_reg.bitfield.lines_number = 1024;
    stage2_max_rows_cols_reg.bitfield.pixels_number = 1024;

    write_reg(cdp->eng.cdp_rgf.cdp0_cfg_frame_max, stage0_max_rows_cols_reg.register_value);
    write_reg(cdp->eng.cdp_rgf.cdp1_cfg_frame_max, stage1_max_rows_cols_reg.register_value);
    write_reg(cdp->eng.cdp_rgf.cdp2_cfg_frame_max, stage2_max_rows_cols_reg.register_value);

#if 0
    cdp_rgf_mem_power_cntl0_reg.register_value = 0;
    cdp_rgf_mem_power_cntl0_reg.bitfield.MEM_POWER_CTRL_DS = 1;
    cdp_rgf_mem_power_cntl0_reg.bitfield.MEM_POWER_CTRL_LS = 1;
    cdp_rgf_mem_power_cntl0_reg.bitfield.MEM_POWER_CTRL_SD = 1;
    write_reg(cdp->eng.cdp_rgf.cdp_cntl, cdp_rgf_mem_power_cntl0_reg.register_value);
#endif

#if 0
    pr_info("cdp_cntl          =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_cntl));
    pr_info("cdp_s0_fast_thold0=0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s0_fast_thold0));
    pr_info("cdp_s1_fast_thold0=0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s1_fast_thold0));
    pr_info("cdp_s2_fast_thold0=0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s2_fast_thold0));
    pr_info("CDP0_ALG_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP0_ALG_MEM));
    pr_info("CDP1_ALG_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP1_ALG_MEM));
    pr_info("CDP2_ALG_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP2_ALG_MEM));
    pr_info("CDP0_RSZ_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP0_RSZ_MEM));
    pr_info("CDP1_RSZ_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP1_RSZ_MEM));
    pr_info("CDP2_RSZ_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP2_RSZ_MEM));
    pr_info("CDP0_NMS_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP0_NMS_MEM));
    pr_info("CDP1_NMS_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP1_NMS_MEM));
    pr_info("CDP2_NMS_MEM      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.CDP2_NMS_MEM));
    pr_info("axi_wr0           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr0));
    pr_info("axi_wr1           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr1));
    pr_info("axi_wr2           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr2));
    pr_info("axi_wr3           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr3));
    pr_info("axi_wr4           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr4));
    pr_info("axi_wr5           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr5));
    pr_info("axi_wr6           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr6));
    pr_info("axi_wr7           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr7));
    pr_info("axi_rd0           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_rd0));
    pr_info("axi_rd1           =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_rd1));
    pr_info("axi_rd_sync       =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_rd_sync));
    pr_info("axi_rd_sync       =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_rd_sync));
    pr_info("axi_wr_sync0_mem_cntl =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr_sync0_mem_cntl));
    pr_info("axi_wr_sync1_mem_cntl =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr_sync1_mem_cntl));
    pr_info("axi_wr_sync2_mem_cntl =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr_sync2_mem_cntl));
    pr_info("axi_wr_sync3_mem_cntl =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr_sync3_mem_cntl));
    pr_info("axi_wr_sync4_mem_cntl =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr_sync4_mem_cntl));
    pr_info("axi_wr_sync5_mem_cntl =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr_sync5_mem_cntl));
    pr_info("axi_wr_sync6_mem_cntl =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr_sync6_mem_cntl));
    pr_info("axi_wr_sync7_mem_cntl =0x%08X\n", read_reg(cdp->eng.cdp_rgf.axi_wr_sync7_mem_cntl));
    pr_info("cdp_s0_rsz_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s0_rsz_mem));
    pr_info("cdp_s1_rsz_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s1_rsz_mem));
    pr_info("cdp_s2_rsz_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s2_rsz_mem));
    pr_info("cdp_s0_alg_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s0_alg_mem));
    pr_info("cdp_s1_alg_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s1_alg_mem));
    pr_info("cdp_s2_alg_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s2_alg_mem));
    pr_info("cdp_s0_nms_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s0_nms_mem));
    pr_info("cdp_s1_nms_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s1_nms_mem));
    pr_info("cdp_s2_nms_mem    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s2_nms_mem));
    pr_info("cdp_axi_tx_mem_tsel_reg    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_axi_tx_mem_tsel_reg));
    pr_info("cdp_cfg    =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_cfg));
    pr_info("cdp_s0_fast_thold      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s0_fast_thold));
    pr_info("cdp_s1_fast_thold      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s1_fast_thold));
    pr_info("cdp_s2_fast_thold      =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_s2_fast_thold));
    pr_info("cdp_axi_rx_mem_ptsel   =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_axi_rx_mem_ptsel));
    pr_info("cdp_axi_rx_mem_low_reg =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_axi_rx_mem_low_reg));
    pr_info("cdp_axi_rx_mem_hi_reg  =0x%08X\n", read_reg(cdp->eng.cdp_rgf.cdp_axi_rx_mem_hi_reg));
#endif

    return SUCCESS;
}


int32_t cdp_exec_descriptor(void)
{
    cdp_rgf_cdp_global_interrupt_reg__enable_t irq_en;
    int32_t rval = SUCCESS;

    // Enable completion interrupt(s).
    irq_en.register_value = read_reg(cdp->eng.cdp_rgf.cdp_global_interrupt_reg_int_en);
    irq_en.bitfield.cdp_done_intr = 1;
    write_reg(cdp->eng.cdp_rgf.cdp_global_interrupt_reg_int_en, irq_en.register_value);

    // Enable CDP with descriptor 0
    write_reg(cdp->eng.cdp_rgf.cdp_global_cfg_reg, CDP_DEFAULT_DESCRIPTOR_INDEX);

    return rval;
}

int32_t cdp_config_global_descriptor(uint64_t input_addr, uint64_t summary_addr)
{

    int32_t rval = SUCCESS;

    cdp_default_desc.global_desc.word0.bitfield.next_frame_dba_msb = input_addr >> 32;
    cdp_default_desc.global_desc.word1.bitfield.next_frame_dba_lsb = input_addr & 0xffffffff;

    cdp_default_desc.global_desc.word4.bitfield.cand_cnt_dba_msb = summary_addr >> 32;
    cdp_default_desc.global_desc.word5.bitfield.cand_cnt_dba_lsb = summary_addr & 0xffffffff;

    rval = cdp_write_descriptor(CDP_DEFAULT_DESCRIPTOR_INDEX, CDP_DESC_GLOBAL, &cdp_default_desc.global_desc);
    if (IS_ERR(rval)) {
        return rval;
    }

    return rval;
}

int32_t cdp_config_stage_descriptor(uint8_t stage, uint64_t axi_output_buf, uint64_t fast_output_buf)
{
    int32_t rval = SUCCESS;
    cdp_stage_desc_t *stage_desc;


    switch (stage) {
        case CDP_DESC_STAGE_0:
            stage_desc = &(cdp_default_desc.stage0_desc);
            break;
        case CDP_DESC_STAGE_1:
            stage_desc = &(cdp_default_desc.stage1_desc);
            break;
        case CDP_DESC_STAGE_2:
            stage_desc = &(cdp_default_desc.stage2_desc);
            break;
        default:
            dbg_print("%s: Invalid descriptor stage %d\n", __func__, stage);
            return -EINVPAR;
    }

    stage_desc->word14.bitfield.frame_dba_msb = axi_output_buf >> 32;
    stage_desc->word15.bitfield.frame_dba_lsb = axi_output_buf & 0xffffffff;
    stage_desc->word16.bitfield.cand_dba_msb  = fast_output_buf >> 32;
    stage_desc->word17.bitfield.cand_dba_lsb  = fast_output_buf & 0xffffffff;

    rval = cdp_write_descriptor(CDP_DEFAULT_DESCRIPTOR_INDEX, stage, stage_desc);
    if (IS_ERR(rval)) {
        return rval;
    }

    return rval;
}

int32_t cdp_config_thresholds(uint8_t stage, struct cdp_windows* threshold_buf)
{
    int32_t rval = SUCCESS;
    volatile cdp_rgf_fast_thold_mem_t *threshold_memory;

    switch (stage) {
        case CDP_DESC_STAGE_0:
            threshold_memory = cdp->eng.cdp_rgf.cdp_s0_fast_thold_mem;
            break;
        case CDP_DESC_STAGE_1:
            threshold_memory = cdp->eng.cdp_rgf.cdp_s1_fast_thold_mem;
            break;
        case CDP_DESC_STAGE_2:
            threshold_memory = cdp->eng.cdp_rgf.cdp_s2_fast_thold_mem;
            break;
        default:
            dbg_print("%s: Invalid descriptor stage %d\n", __func__, stage);
            return -EINVPAR;
    }

    rval = cdp_write_thresholds(threshold_buf->thresholds, threshold_memory, threshold_buf->n_thresholds);
    if (IS_ERR(rval)) {
        return rval;
    }

    return rval;
}

int32_t cdp_section_received(uint32_t size)
{
    int32_t rval = SUCCESS;
    write_reg(cdp->eng.cdp_rgf.frm_sec_size_reg, size);
    return rval;
}

int32_t cdp_wait_for_finish(void)
{
    int32_t rval = SUCCESS;

    volatile cdp_rgf_cdp_global_interrupt_reg_t intr_reg;
    intr_reg.register_value = read_reg(cdp->eng.cdp_rgf.cdp_global_interrupt_reg);
    while (intr_reg.bitfield.cdp_done_intr != 1) {
        intr_reg.register_value = read_reg(cdp->eng.cdp_rgf.cdp_global_interrupt_reg);
    }

    pr_info("STATUS=0x%08X\n", intr_reg.register_value);

    return rval;
}
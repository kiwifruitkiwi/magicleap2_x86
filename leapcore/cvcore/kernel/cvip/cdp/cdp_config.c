// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include "cdp_lib.h"
#include "cdp_top.h"
#include "cdp_common.h"
#include "cdp_desc.h"
#include "cdp_errors.h"
#include "cdp_config.h"

cdp_desc_t cdp_default_desc;

#define dbg_print  pr_err
#ifdef __va
#undef __va
#define __va(x)  (x)
#endif


int32_t cdp_write_block(uint32_t *src, uint32_t *dest, uint16_t size_bytes)
{
    int32_t rval = SUCCESS;
    uint8_t i = 0;

    if (src == NULL) {
#ifdef DEBUG
        dbg_print("%s: Invalid parameter! src: %p, dest: %p, size_bytes: %d\n", __func__, src, dest, size_bytes);
#endif
        return -EINVPAR;
    }

    for (i = 0; i < size_bytes / 4; i++) {
        writelX(src[i], __va(&(dest[i])));
    }

    return rval;
}

int32_t cdp_write_descriptor(uint8_t desc_num, uint8_t desc_stage, void *desc)
{
    int32_t rval = SUCCESS;
    uint32_t *desc_base_addr = (uint32_t *)(CDP_DESCRIPTOR_BASE_ADDRESS(desc_num));
    uint8_t desc_size_bytes = 0;

    if (desc_num >= CDP_MAX_DESCRIPTOR_NUM) {
#ifdef DEBUG
        dbg_print("%s: Invalid descriptor number %d! Maximum is %d\n;", __func__, desc_num, CDP_MAX_DESCRIPTOR_NUM);
#endif
        return -EINVPAR;
    }
    if (desc_stage >= CDP_DESC_STAGE_MAX) {
#ifdef DEBUG
        dbg_print("%s: Invalid descriptor stage %d! Maximum is %d\n;", __func__, desc_stage, CDP_DESC_STAGE_MAX);
#endif
        return -EINVPAR;
    }
    if (desc == NULL) {
#ifdef DEBUG
        dbg_print("%s: Invalid parameter! desc: %p\n", __func__, desc);
#endif
        return -EINVPAR;
    }

    switch (desc_stage) {
        case CDP_DESC_GLOBAL:
            desc_size_bytes = sizeof(cdp_global_desc_t);
            break;
        case CDP_DESC_STAGE_0:
            desc_base_addr += sizeof(cdp_global_desc_t);
            desc_size_bytes = sizeof(cdp_stage_desc_t);
            break;
        case CDP_DESC_STAGE_1:
            desc_base_addr += sizeof(cdp_global_desc_t) + sizeof(cdp_stage_desc_t);
            desc_size_bytes = sizeof(cdp_stage_desc_t);
            break;
        case CDP_DESC_STAGE_2:
            desc_base_addr += sizeof(cdp_global_desc_t) + 2 * sizeof(cdp_stage_desc_t);
            desc_size_bytes = sizeof(cdp_stage_desc_t);
            break;
        case CDP_DESC_FULL:
            desc_size_bytes = sizeof(cdp_desc_t);
            break;
        default:
            dbg_print("%s: Invalid descriptor stage %d\n", __func__, desc_stage);
            return -EINVPAR;
    }
    // Write descriptor to memory word by word
    rval = cdp_write_block((uint32_t *)desc, (uint32_t *)desc_base_addr, desc_size_bytes);
    if (IS_ERR(rval)) {
        return rval;
    }

    return rval;
}

int32_t cdp_init_default_global_desc(cdp_global_desc_t *global_desc)
{
#ifdef DEBUG
    if (global_desc == NULL) {
        dbg_print("%s: Invalid parameter! global_desc: %p\n", __func__, global_desc);
        return -EINVPAR;
    }
#endif

    int32_t rval = SUCCESS;

    // Set default global descriptor values
    global_desc->word0.bitfield.next_frame_dba_msb = 0;

    global_desc->word1.bitfield.next_frame_dba_lsb = 0;

    global_desc->word2.bitfield.next_frame_rs = CDP_IMAGE_N_COLS;
    global_desc->word2.bitfield.next_frame_rs = 0;
    global_desc->word2.bitfield.next_frame_dpck = 1;

    global_desc->word3.register_value = 0;

    global_desc->word4.bitfield.cand_cnt_dba_msb = 0;

    global_desc->word5.bitfield.cand_cnt_dba_lsb = 0;

    return rval;
}

int32_t cdp_init_default_stage_desc(cdp_stage_desc_t *stage_desc)
{
#ifdef DEBUG
    if (stage_desc == NULL) {
        dbg_print("%s: Invalid parameter! stage_desc: %p\n", __func__, stage_desc);
        return -EINVPAR;
    }
#endif

    int32_t rval = SUCCESS;

    memset(stage_desc, 0, sizeof(*stage_desc));

    stage_desc->word0.bitfield.isp_bypass_en = 1;
    stage_desc->word0.bitfield.isp_bypass = 1;
    stage_desc->word0.bitfield.alg_bypass_en = 1;
    stage_desc->word0.bitfield.alg_bypass = 1;
    stage_desc->word0.bitfield.nxt_bypass_en = 1;
    stage_desc->word0.bitfield.nxt_bypass = 0;
    stage_desc->word0.bitfield.nms_vld_en = 1;
    stage_desc->word0.bitfield.fast = 1;
    stage_desc->word0.bitfield.fast_no_mem = 1;
    stage_desc->word0.bitfield.gauss_en = 0;
    stage_desc->word0.bitfield.frame_obpp = CDP_WCAM_BPP_8;
    stage_desc->word0.bitfield.frame_dpck = 1;
    stage_desc->word0.bitfield.frame_chmod = 1;

    stage_desc->word6.bitfield.fast_seqln = 9;
    stage_desc->word6.bitfield.fast_thold_h_win_size = 128;
    stage_desc->word6.bitfield.fast_thold_v_win_size = 128;

    stage_desc->word8.bitfield.cand_max_per_win = 16384 - 1;
    stage_desc->word8.bitfield.roi_remove_pix_width = 5;
    stage_desc->word8.bitfield.roi_remove_pix_width = 0;
    stage_desc->word8.bitfield.roi_en = 1;
    stage_desc->word8.bitfield.roi_en = 0;

    stage_desc->word13.bitfield.fast_thold_no_mem = 1;

    // Enable FAST pipe w/ no blur or resize, and with NMS.
    stage_desc->word0.bitfield.alg_bypass_en = 1;
    stage_desc->word0.bitfield.alg_bypass    = 1;
    stage_desc->word0.bitfield.nms_vld_en    = 1;  // Must be equal to alg_bypass_en
    stage_desc->word0.bitfield.fast = 1;

    stage_desc->word2.bitfield.resize_sel = CDP_PYRAMID_RESIZE_4_5;
    stage_desc->word0.bitfield.nxt_pyr_en = 1;

    return rval;
}

/*
 * For further documententation of the enable/disable values chosen for each stage, see:
 * https://docs.google.com/document/d/16592o3viB72DCvRNjfoCftfOy4icxo7Q5QPi61MoNmM/edit?ts=5e1c7742
 */

int32_t cdp_init_default_stage0_desc(cdp_stage_desc_t *stage_desc)
{
    int32_t rval = SUCCESS;

    rval = cdp_init_default_stage_desc(stage_desc);
    if (IS_ERR(rval)) {
        return rval;
    }

    //
    // Stage 0 Configuration
    //

    // Disable the AXI output since it just gives us a copy of the original.
    stage_desc->word0.bitfield.isp_bypass_en = 0;
    stage_desc->word0.bitfield.isp_bypass    = 0;

    return rval;
}

int32_t cdp_init_default_stage1_desc(cdp_stage_desc_t *stage_desc)
{
    int32_t rval = SUCCESS;

    rval = cdp_init_default_stage_desc(stage_desc);
    if (IS_ERR(rval)) {
        return rval;
    }

    //
    // Stage 1 Configuration
    //
    // Disable the AXI output to save power, since it's no longer used.
    stage_desc->word0.bitfield.isp_bypass_en = 0;
    stage_desc->word0.bitfield.isp_bypass    = 0;

    return rval;

}


int32_t cdp_init_default_stage2_desc(cdp_stage_desc_t *stage_desc)
{
    int32_t rval = SUCCESS;

    rval = cdp_init_default_stage_desc(stage_desc);
    if (IS_ERR(rval)) {
        return rval;
    }

    //
    // Stage 2 Configuration
    //
    // Disable the AXI output to save power, since it's no longer used.
    stage_desc->word0.bitfield.isp_bypass_en = 0;
    stage_desc->word0.bitfield.isp_bypass    = 0;

    // Disable the NXT pipe.
    stage_desc->word0.bitfield.nxt_bypass_en = 0;
    stage_desc->word0.bitfield.nxt_pyr_en    = 0;

    return rval;
}

int32_t cdp_init_descriptor(cdp_desc_t *desc)
{
    int32_t rval = SUCCESS;

    if (desc == NULL) {
#ifdef DEBUG
        dbg_print("%s: Invalid parameter! desc: %p\n", __func__, desc);
#endif
        return -EINVPAR;
    }

    memset(desc, 0, sizeof(*desc));

    rval = cdp_init_default_global_desc(&(desc->global_desc));
    if (IS_ERR(rval)) {
        return rval;
    }

    rval = cdp_init_default_stage0_desc(&(desc->stage0_desc));
    if (IS_ERR(rval)) {
        return rval;
    }

    rval = cdp_init_default_stage1_desc(&(desc->stage1_desc));
    if (IS_ERR(rval)) {
        return rval;
    }

    rval = cdp_init_default_stage2_desc(&(desc->stage2_desc));
    if (IS_ERR(rval)) {
        return rval;
    }

    rval = cdp_write_descriptor(CDP_DEFAULT_DESCRIPTOR_INDEX, CDP_DESC_FULL, &cdp_default_desc);
    if (IS_ERR(rval)) {
        return rval;
    }

    return rval;
}


int32_t cdp_write_thresholds(cdp_rgf_fast_thold_mem_t* thresholds,
                             volatile cdp_rgf_fast_thold_mem_t *threshold_memory,
                             uint16_t n_windows)
{
    int32_t rval = SUCCESS;

    if ((thresholds == NULL) || (threshold_memory == NULL) || (n_windows > CDP_MAX_N_WINDOWS)) {
#ifdef DEBUG
        dbg_print("%s: Invalid parameter! n_windows: %d, thresholds: %p\n", __func__, n_windows, thresholds);
#endif
        return -EINVPAR;
    }

    rval = cdp_write_block((uint32_t *)thresholds,
                           (uint32_t *)threshold_memory,
                           n_windows * CDP_BYTES_PER_THRESHOLD_VALUE);

    return rval;
}
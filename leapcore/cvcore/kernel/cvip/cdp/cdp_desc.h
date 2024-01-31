/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

//////////////////////////////////////////////////////////////////////////////////
// File : cdp_desc.h
// Created on : Sunday Mar 1, 2020 [14:55:22 pm]
//////////////////////////////////////////////////////////////////////////////////

// Creation date and time
#define cdp_desc_CREATE_TIME_STR                         "Sunday Mar 1, 2020 [14:55:22 pm]"

#define cdp_desc_BASE_ADDRESS                            (0x00000000)


typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int next_frame_dba_msb : 32;
    } bitfield;
    unsigned int register_value;
} cdp_global_desc_word0_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int next_frame_dba_lsb : 32;
    } bitfield;
    unsigned int register_value;
} cdp_global_desc_word1_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int next_frame_rs : 16;
        unsigned int next_frame_shift_r : 4;
        unsigned int next_frame_shift_l : 4;
        unsigned int next_frame_ibpp : 2;
        unsigned int rsrvd0: 4;
        unsigned int next_frame_en : 1;
        unsigned int next_frame_dpck : 1;
    } bitfield;
    unsigned int register_value;
} cdp_global_desc_word2_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int next_frame_rgap : 16;
    } bitfield;
    unsigned int register_value;
} cdp_global_desc_word3_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int cand_cnt_dba_msb : 32;
    } bitfield;
    unsigned int register_value;
} cdp_global_desc_word4_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int cand_cnt_dba_lsb : 32;
    } bitfield;
    unsigned int register_value;
} cdp_global_desc_word5_t;

typedef struct {
    //
    //
    //
    // Offset - 0x0
    cdp_global_desc_word0_t  word0;

    //
    //
    //
    // Offset - 0x4
    cdp_global_desc_word1_t  word1;

    //
    //
    //
    // Offset - 0x8
    cdp_global_desc_word2_t  word2;

    //
    //
    //
    // Offset - 0xc
    cdp_global_desc_word3_t  word3;

    //
    //
    //
    // Offset - 0x10
    cdp_global_desc_word4_t  word4;

    //
    //
    //
    // Offset - 0x14
    cdp_global_desc_word5_t  word5;

} cdp_global_desc_t;


typedef union {
    //
    //
    struct {
        unsigned int isp_bypass_en : 1;
        unsigned int isp_bypass : 1;
        unsigned int isp_pyr_en : 1;
        unsigned int alg_bypass_en : 1;
        unsigned int alg_bypass : 1;
        unsigned int alg_pyr_en : 1;
        unsigned int nxt_bypass_en : 1;
        unsigned int nxt_bypass : 1;
        unsigned int nxt_pyr_en : 1;
        unsigned int nms_vld_en : 1;
        unsigned int fast : 1;
        unsigned int camera_rotation : 1;
        unsigned int gauss_en : 1;
        unsigned int rsrvd0: 1;
        unsigned int fast_th_mem_sel : 1;
        unsigned int fast_no_mem : 1;
        unsigned int frame_dpck : 1;
        unsigned int rsrvd1: 1;
        unsigned int frame_frot : 1;
        unsigned int frame_chmod : 1;
        unsigned int frame_obpp : 2;
        unsigned int rsrvd2: 1;
        unsigned int rsrvd3: 1;
        unsigned int data_shift_r : 4;
        unsigned int data_shift_l : 4;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word0_t;

typedef union {
    //
    //
    struct {
        unsigned int in_image_num_of_lines : 12;
        unsigned int in_image_num_of_pixels : 12;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word1_t;

typedef union {
    //
    //
    struct {
        unsigned int out_image_num_of_lines : 12;
        unsigned int out_image_num_of_pixels : 12;
        unsigned int resize_sel : 3;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word2_t;

typedef union {
    //
    //
    struct {
        unsigned int gauss_coeff_bbb : 12;
        unsigned int gauss_coeff_center : 12;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word3_t;

typedef union {
    //
    //
    struct {
        unsigned int gauss_coeff_cross_in : 12;
        unsigned int gauss_coeff_cross_out : 12;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word4_t;

typedef union {
    //
    //
    struct {
        unsigned int gauss_coeff_corner_in : 12;
        unsigned int gauss_coeff_corner_out : 12;
        unsigned int gauss_shiftback : 5;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word5_t;

typedef union {
    //
    //
    struct {
        unsigned int fast_thold_h_win_size : 12;
        unsigned int rsrvd0 : 4;
        unsigned int fast_thold_v_win_size : 12;
        unsigned int fast_seqln : 4;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word6_t;

typedef union {
// The summary at the top of cdp_top.h doesn't include the 3x3 bit, but the RTL code does!
#if 0
`define CANDIDATE_CFG_WIN_HSIZE_OFFSET    (`CDP_CFG_WORD7 + 0)
`define CANDIDATE_CFG_WIN_VSIZE_OFFSET     (`CDP_CFG_WORD7 + 16)
`define CANDIDATE_CFG_USE_3x3_WIN_OFFSET  (`CDP_CFG_WORD7 + 31)
#endif
    //
    //
    struct {
        unsigned int cand_h_win_size : 12;
        unsigned int rsrvd0 : 4;
        unsigned int cand_v_win_size : 12;
        unsigned int rsrvd1 : 3;
        unsigned int use_3x3_win : 1;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word7_t;

typedef union {
    //
    //
    struct {
        unsigned int cand_max_per_win : 14;
        unsigned int rsrvd0 : 2;
        unsigned int roi_remove_pix_width : 10;
        unsigned int rsrvd1 : 5;
        unsigned int roi_en : 1;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word8_t;

typedef union {
    //
    //
    struct {
        unsigned int roi_cir_r : 24;
        unsigned int rsrvd0 : 7;
        unsigned int roi_cir_en : 1;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word9_t;

typedef union {
    //
    //
    struct {
        unsigned int roi_cir_cxcx : 24;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word10_t;

typedef union {
    //
    //
    struct {
        unsigned int roi_cir_cycy : 24;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word11_t;

typedef union {
    //
    //
    struct {
        unsigned int roi_cir_cx : 12;
        unsigned int roi_cir_pad :  4; // For fixing circular ROI bug. --imagyar
        unsigned int roi_cir_cy : 12;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word12_t;

typedef union {
    //
    //
    struct {
        unsigned int fast_thold_no_mem : 16;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word13_t;

typedef union {
    //
    //
    struct {
        unsigned int frame_dba_msb : 32;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word14_t;

typedef union {
    //
    //
    struct {
        unsigned int frame_dba_lsb : 32;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word15_t;

typedef union {
    //
    //
    struct {
        unsigned int cand_dba_msb : 32;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word16_t;

typedef union {
    //
    //
    struct {
        unsigned int cand_dba_lsb : 32;
    } bitfield;
    unsigned int register_value;
} cdp_stage_desc_word17_t;

typedef struct {
    //
    //
    //
    // Offset - 0x0
    cdp_stage_desc_word0_t  word0;

    //
    //
    //
    // Offset - 0x4
    cdp_stage_desc_word1_t  word1;

    //
    //
    //
    // Offset - 0x8
    cdp_stage_desc_word2_t  word2;

    //
    //
    //
    // Offset - 0xc
    cdp_stage_desc_word3_t  word3;

    //
    //
    //
    // Offset - 0x10
    cdp_stage_desc_word4_t  word4;

    //
    //
    //
    // Offset - 0x14
    cdp_stage_desc_word5_t  word5;

    //
    //
    //
    // Offset - 0x18
    cdp_stage_desc_word6_t  word6;

    //
    //
    //
    // Offset - 0x1c
    cdp_stage_desc_word7_t  word7;

    //
    //
    //
    // Offset - 0x20
    cdp_stage_desc_word8_t  word8;

    //
    //
    //
    // Offset - 0x24
    cdp_stage_desc_word9_t  word9;

    //
    //
    //
    // Offset - 0x28
    cdp_stage_desc_word10_t word10;

    //
    //
    //
    // Offset - 0x2c
    cdp_stage_desc_word11_t word11;

    //
    //
    //
    // Offset - 0x30
    cdp_stage_desc_word12_t word12;

    //
    //
    //
    // Offset - 0x34
    cdp_stage_desc_word13_t word13;

    //
    //
    //
    // Offset - 0x38
    cdp_stage_desc_word14_t word14;

    //
    //
    //
    // Offset - 0x3c
    cdp_stage_desc_word15_t word15;

    //
    //
    //
    // Offset - 0x40
    cdp_stage_desc_word16_t word16;

    //
    //
    //
    // Offset - 0x44
    cdp_stage_desc_word17_t word17;

} cdp_stage_desc_t;


typedef struct {
    //
    //
    cdp_global_desc_t  global_desc;

    //
    // Offset - 0x4
    cdp_stage_desc_t  stage0_desc;

    //
    // Offset - 0x4
    cdp_stage_desc_t  stage1_desc;

    //
    // Offset - 0x4
    cdp_stage_desc_t  stage2_desc;

} cdp_desc_t;

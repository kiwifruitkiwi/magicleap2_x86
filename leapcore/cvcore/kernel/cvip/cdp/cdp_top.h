/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

//////////////////////////////////////////////////////////////////////////////////
// File : cdp.h
// Created on : Monday Sep 02, 2019 [16:06:16 pm]
//////////////////////////////////////////////////////////////////////////////////

// Creation date and time
#define CDP_CREATE_TIME_STR                                "Monday Sep 02, 2019 [16:06:16 pm]"

#define ENG_CDP_RGF_BASE_ADDRESS                           (0x00000000)
#define ENG_BASE_ADDRESS                                   (0x00000000)
#define AXI_BASE_ADDRESS                                   (0x00004000)
#define CDP_BASE_ADDRESS                                   (0xd3000000)

typedef union {
    // CDP Descriptor Configuration Memory
    // This memory is used to configure the CDP stages.
    //   <br>CDP Descriptor Configuration Memory Brakedown:
    //   <br>Name                Bits    Offset  Word      Description
    //   <br>------------------------------------------------------------------------------------------------------------------------
    //   <br>CFG_ISP_BYPASS_EN       1   0   0   Enable the ISP output.
    //   <br>CFG_ISP_BYPASS                  1   1   0   Bypass the downsize and Gaussian for ISP output.
    //   <br>CFG_ISP_PYR_EN                  1   2   0   Enable downsize for ISP output.
    //   <br>CFG_ALG_BYPASS_EN       1   3   0   Enable the FAST output.
    //   <br>CFG_ALG_BYPASS                  1   4   0   Bypass the downsize and Gaussian for FAST output.
    //   <br>CFG_ALG_PYR_EN                  1   5   0   Enable downsize for FAST output.
    //   <br>CFG_NXT_BYPASS_EN       1   6   0   Enable the next CDP stage output.
    //   <br>CFG_NXT_BYPASS          1   7   0   Bypass the downsize and Gaussian for next CDP stage output.
    //   <br>CFG_NXT_PYR_EN                  1   8   0   Enable downsize for next CDP stage output.
    //   <br>CFG_NMS_VLD_EN                  1   9   0   Enable the LNMS corner candidate output.
    //   <br>CFG_CAMERA_ROTATE       1   11  0   Enable camera rotate for this engine.
    //   <br>CFG_GAUSSIAN_EN                 1   12  0   Enable Gaussian.
    //   <br>CFG_NEXT_CDP_VLD        1   14  0   Enable next CDP stage, when '1' the user should make sure that the next CDP stage is configured.
    //   <br>FAST_NO_MEM_MODE        1   15  0   FAST No memory mode, when '1' the FAST threshold is fixed for all image, the threshold value is configured in 'FAST_TH' field in WORD16.
    //   <br>                                                                             when '0' the FAST threshold is grid based, the grid thresholds are written the FAST Threshold memory.
    //   <br>FRAME_DPCKI         1   16  0   Output Image Pack Enable
    //   <br>FRAME_FROTI         2   17  0   Output Image Rotate
    //   <br>FRAME_OBPPI         2   20  0   Output Image BPP
    //   <br>CFG_BPCSFRI         3   24  0   Input Image Shift Right with Round.
    //   <br>CFG_BPCSFLI         3   28  0   Input Image Shift Left.
    //   <br>DOWNSIZE_CFG_INPUT_IMG_LINES    10  0   1   Downsize input image lines number.
    //   <br>DOWNSIZE_CFG_INPUT_IMG_PIXELS   10  12  1   Downsize input image pixels number.
    //   <br>DOWNSIZE_CFG_OUTPUT_IMG_LINES   10  0   2   Downsize output image lines number.
    //   <br>DOWNSIZE_CFG_OUTPUT_IMG_PIXELS  10  12  2   Downsize output image pixels number.
    //   <br>DOWNSIZE_CFG_COEFF_SEL          3   24  2   Downsize ratio select:
    //   <br>                            3'd0: 1
    //   <br>                            3'd1: 1.25
    //   <br>                            3'd2: 1.4
    //   <br>                            3'd3: 1.5
    //   <br>                            3'd4: 1.6
    //   <br>                            3'd5: 1.75
    //   <br>                            3'd6: 2
    //   <br>                            3'd7: 1.2
    //   <br>GASSUIAN_CFG_COEFF_BBB_OUT  12  0   3   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CENTER   12  12  3   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CROSS_IN 12  0   4   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CROSS_OUT    12  12  4   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CORNER_IN    12  0   5   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CORNER_OUT   12  12  5   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_SHIFBACK_CFG       5   24  5   Gaussian Coefficients.
    //   <br>FAST_CFG_THOLD_HWINTSIZE    10  0   6   FAST Threshold horizontal grid size.
    //   <br>                                                          When the engine supports only one camera and less than 5 stages the grid window number can reach 400.
    //   <br>                                                          When the engine supports only one camera, total grid window number is 400, maximum of 64 can be used for the stages 5-8.
    //   <br>                                                          When the engine supports two cameras, the total number per camera is 200.
    //   <br>FAST_CFG_THOLD_VWINTSIZE    10  16  6   FAST Threshold vertical grid size.
    //   <br>                                                          When the engine supports only one camera and less than 5 stages the grid window number can reach 400.
    //   <br>                                                          When the engine supports only one camera, total grid window number is 400, maximum of 64 can be used for the stages 5-8.
    //   <br>                                                          When the engine supports two cameras, the total number per camera is 200.
    //   <br>FAST_CFG_SEQLEN                 4   28  6   FAST K length [9-16]
    //   <br>CANDIDATE_CFG_WIN_HSIZE         10  0   7   LNMS Horizontal Grid Size, the size defines the window grid for maximum corner candidates number.
    //   <br>CANDIDATE_CFG_WIN_VSIZE         10  16  7   LNMS Veritcal Grid Size, the size defines the window grid for maximum corner candidates number.
    //   <br>CANDIDATE_CFG_USE_3x3_WIN   1   31  7   USE 3x3 LNMS Window.
    //   <br>CANDIDATE_CFG_WIN_MAX           14  0   8   Maximum corner candidate for LNMS grid window.
    //   <br>CANDIDATE_CFG_ROI_WIDTH         8   16  8   Rectangular ROI size, the width value defines the number of pixels from the image boundary.
    //   <br>CANDIDATE_CFG_ROI_EN        1   31  8   Enable Rectangular ROI
    //   <br>CANDIDATE_CFG_CIR_ROI_EN    1   31  9   Enable Circular ROI
    //   <br>CANDIDATE_CFG_CIR_ROI_RR    20  0   9   Circular ROI radios.
    //   <br>CANDIDATE_CFG_CIR_ROI_CXCX  20  0   10  Circular ROI Center X coordinate ^2.
    //   <br>CANDIDATE_CFG_CIR_ROI_CX    10  20  10  Circular ROI Center X coordinates.
    //   <br>CANDIDATE_CFG_CIR_ROI_CYCY  20  0   11  Circular ROI Center Y coordinate ^2.
    //   <br>CANDIDATE_CFG_CIR_ROI_CY    10  20  11  Circular ROI Center Y coordinates.
    //   <br>FRAME_MEM_OFFSET        32  0   12  Image Memory Offset, this 32b address is used by the ISP for forwarding the CDP output image to the system memory.
    //   <br>CANDIDATE_MEM_OFFSET            32  0   13  Candidates Memory Offset, this 32b address is used by the ISP for forwarding the CDP output candidates to the system memory.
    //   <br>NEXT_FRAME_MEM                  32  0   14  Next Image Offset, this 32b address is used by the ISP for reading an image from the system memory.
    //   <br>NEXT_FRAME_SIZE                 16  0   15  Next Image Size, in Bytes.
    //   <br>NEXT_FRAME_BPCSFR       4   16  15  Next Image Pixel Shift Right with Round
    //   <br>NEXT_FRAME_BPCSFL       4   20  15  Next Image Pixel Shift Left
    //   <br>NEXT_FRAME_IBPP                 2   24  15  Next Image Input Bit Per Pixel
    //   <br>NEXT_FRAME_EN                   1   30  15  Next Image Enable
    //   <br>NEXT_FRAME_DPCK         1   31  15  Next Image Unpack
    //   <br>NEXT_FRAME_RGAP                 0   16  16  Next Image Row Gap
    //   <br>FAST_TH                         16  16  16  Fast Threshold, when working in 'FAST_NO_MEM_MODE' the threshold value should be defined here.
    struct {
        //
        //
        unsigned int cfg : 32;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cfg_mem_t;

typedef union {
    // CDP Global Configuration Register
    // This register configures the CDP block.
    struct {
        //
        // CDP Accelerator Enable. When set the CDP is enabled. The hardware clears the bit.
        //           <br>Note: The descriptor memory accelerator range should be pre-configured prior to enabling the accelerator.
        //           <br>Note: The block finishes the processing when 'cdp_done_intr' assertted.
        unsigned int accelerator_en : 5;
        // Just for padding
        unsigned int : 27;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_global_cfg_reg_t;

typedef union {
    // CDP Row Spacer Configuration Register
    // This register configures the CDP block.
    struct {
        //
        // Spacer FIFO Threshold configuration.
        unsigned int fifo_af_th : 4;
        // Just for padding
        unsigned int : 28;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_row_spacer_cfg_reg_t;

typedef union {
    // CDP Window Clear Configuration Register
    // This register used to clear the window memory before starting the block.
    struct {
        //
        // ALG Stage 0 Memory Clear.
        unsigned int alg_mem_s0 : 1;
        //
        // ALG Stage 1 Memory Clear.
        unsigned int alg_mem_s1 : 1;
        //
        // ALG Stage 2 Memory Clear.
        unsigned int alg_mem_s2 : 1;
        //
        // LNMS Stage 0 Memory Clear.
        unsigned int nms_mem_s0 : 1;
        //
        // LNMS Stage 1 Memory Clear.
        unsigned int nms_mem_s1 : 1;
        //
        // LNMS Stage 2 Memory Clear.
        unsigned int nms_mem_s2 : 1;
        //
        // RSZ Stage 0 Memory Clear.
        unsigned int rsz_mem_s0 : 1;
        //
        // RSZ Stage 1 Memory Clear.
        unsigned int rsz_mem_s1 : 1;
        //
        // RSZ Stage 2 Memory Clear.
        unsigned int rsz_mem_s2 : 1;
        // Just for padding
        unsigned int : 23;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_window_clr_reg_t;

typedef union {
    // CDP Clock Gate Enable Configuration Register
    // This register used to enable clock gating.
    struct {
        //
        // .
        unsigned int cdp0_rsz_win_cgate_en   : 1;
        //
        // .
        unsigned int cdp0_rsz_gauss_cgate_en : 1;
        //
        // .
        unsigned int cdp0_rsz_img_cgate_en   : 1;
        //
        // .
        unsigned int cdp0_alg_cgate_en       : 1;
        //
        // .
        unsigned int cdp0_lnms_cgate_en      : 1;
        // Just for padding
        unsigned int : 3;
        //
        // .
        unsigned int cdp1_rsz_win_cgate_en   : 1;
        //
        // .
        unsigned int cdp1_rsz_gauss_cgate_en : 1;
        //
        // .
        unsigned int cdp1_rsz_img_cgate_en   : 1;
        //
        // .
        unsigned int cdp1_alg_cgate_en       : 1;
        //
        // .
        unsigned int cdp1_lnms_cgate_en      : 1;
        // Just for padding
        unsigned int : 3;
        //
        // .
        unsigned int cdp2_rsz_win_cgate_en   : 1;
        //
        // .
        unsigned int cdp2_rsz_gauss_cgate_en : 1;
        //
        // .
        unsigned int cdp2_rsz_img_cgate_en   : 1;
        //
        // .
        unsigned int cdp2_alg_cgate_en       : 1;
        //
        // .
        unsigned int cdp2_lnms_cgate_en      : 1;
        // Just for padding
        unsigned int : 11;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_clk_gate_en_reg_t;

typedef union {
    // Fast Thold Configuration Register
    // This register selects the Fast thold memory.
    struct {
        //
        // When '1' select High Memory
        unsigned int S0_sel : 1;
        // Just for padding
        unsigned int : 3;
        //
        // When '1' select High Memory
        unsigned int S1_sel : 1;
        // Just for padding
        unsigned int : 3;
        //
        // When '1' select High Memory
        unsigned int S2_sel : 1;
        // Just for padding
        unsigned int : 23;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_fast_thold_mem_cfg_reg_t;

typedef union {
    // Stage0 Fast Threshold Memory
    // This memory configures the fast thresholds which can be used when working in threshold memory mode.
    //   <br>The memory can support the follwoing camera/stage options:
    //   <br>One Camera + Stages<5 -> 400 Threshold per image.
    //   <br>One Camera + Stages>4 -> 272 for stages 1-4 and 128 for stages 5-8.
    //   <br>Two Camera + Stages<5 -> 200 Threshold per image.
    //   <br>Two Camera + Stages>4 -> 136 for stages 1-4 and 64 for stages 5-8.
    //   <br>Note:The threshold for stages 5-8 can be any power of 2(2,4,16,32,64,128).
    struct {
        //
        // Fast threshold0.
        unsigned int thold0 : 16;
        //
        // Fast threshold1.
        unsigned int thold1 : 16;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_fast_thold_mem_t;

typedef union {
    // Camera0 Fast Configuration Register
    // This register configures the number os thresholds for stages 5-8.
    struct {
        //
        // Stage 5-8 Threshold Size. The number defines the number of thresholds for stages 5-8.
        unsigned int size : 6;
        // Just for padding
        unsigned int : 26;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cfg_fast_thold_mem_t;

typedef union {
    // Features Global Configuration Register
    // This register configures the candidate.
    //   <br>Note:This register is not functional.
    struct {
        //
        // Clear the featurs counters for CDP0.
        unsigned int clr0 : 1;
        //
        // Clear the featurs counters for CDP1.
        unsigned int clr1 : 1;
        //
        // Clear the featurs counters for CDP1.
        unsigned int clr2 : 1;
        //
        // Clear the featurs counters for CDP1.
        unsigned int clr3 : 1;
        // Just for padding
        unsigned int : 28;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_candidate_glbl_cfg_reg_t;

typedef union {
    // Stage0 Features Maximum Threshold Status Register
    // This register holds the X/Y cordinates of the pixel crossed the maximun threshold candidate.
    struct {
        //
        // X Cordinate.
        unsigned int x : 12;
        // Just for padding
        unsigned int : 4;
        //
        // Y Cordinate.
        unsigned int y : 12;
        // Just for padding
        unsigned int : 4;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_th_reached_status_reg_t;

typedef union {
    // CDP AXI Read Frame Section Size
    // Number of bytes added to memory for this channel.
    struct {
        //
        // Bytes added.
        unsigned int val : 32;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_frm_sec_size_reg_t;

typedef union {
    // CDP AXI Read Frame Section Enable
    // Enable Frame Section mode.
    struct {
        //
        // 0: Disable. 1: Enable.
        unsigned int en : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_frm_sec_enb_reg_t;

typedef union {
    // Stage0 CDP Interrupt Register
    // This register holds the CDP interrupts
    struct {
        //
        // Frame length eror - smaller than configured.
        unsigned int frame_length_err_too_small_intr   : 1;
        //
        // Frame length eror - bigger than configured.
        unsigned int frame_length_err_too_big_intr     : 1;
        //
        // Frame depth eror - smaller than configured.
        unsigned int frame_depth_err_too_small_intr    : 1;
        //
        // Frame depth eror - bigger than configured.
        unsigned int frame_depth_err_too_big_intr      : 1;
        //
        // Protocol error - sof without valid.
        unsigned int protocol_err_sof_wo_vld_intr      : 1;
        //
        // Protocol error - eof without valid.
        unsigned int protocol_err_eof_wo_vld_intr      : 1;
        //
        // Protocol error - sol without valid.
        unsigned int protocol_err_sol_wo_vld_intr      : 1;
        //
        // Protocol error - eol without valid.
        unsigned int protocol_err_eol_wo_vld_intr      : 1;
        //
        // Protocol error - double sof.
        unsigned int protocol_err_dbl_sof_intr         : 1;
        //
        // Protocol error - double sol.
        unsigned int protocol_err_dbl_sol_intr         : 1;
        //
        // Protocol error - double eof.
        unsigned int protocol_err_dbl_eof_intr         : 1;
        //
        // Protocol error - double eol.
        unsigned int protocol_err_dbl_eol_intr         : 1;
        //
        // Number of candidates exceed configuration.
        unsigned int too_many_candidates               : 1;
        //
        // RSZ memory parity error.
        unsigned int rsz_mem_parity_err_intr           : 1;
        //
        // ALG memory parity error.
        unsigned int alg_mem_parity_err_intr           : 1;
        //
        // NMS memory parity error.
        unsigned int nms_mem_parity_err_intr           : 1;
        //
        // SW Fast Thrshold memory SBE error.
        unsigned int fast_th_mem_sbe_err_intr_user     : 1;
        //
        // SW Fast Thrshold memory DBE error.
        unsigned int fast_th_mem_dbe_err_intr_user     : 1;
        //
        // HW Fast Thrshold memory SBE error.
        unsigned int fast_th_mem_sbe_err_intr          : 1;
        //
        // HW Fast Thrshold memory DBE error.
        unsigned int fast_th_mem_dbe_err_intr          : 1;
        //
        // Frame bad size max error.
        unsigned int frame_bad_size_max_intr           : 1;
        //
        // Frame bad size min error.
        unsigned int frame_bad_size_min_intr           : 1;
        //
        // Frame bad size odd pixel error.
        unsigned int frame_bad_size_odd_pixel_num_intr : 1;
        //
        // Cand buffer backpressure.
        unsigned int cand_buff_bp_intr                 : 1;
        //
        // Framebuffer backpressure.
        unsigned int frame_buff_bp_intr                : 1;
        //
        // Frame Water Mark
        unsigned int frame_wmrk_intr                   : 1;
        //
        // Cand Water Mark
        unsigned int cand_wmrk_intr                    : 1;
        // Just for padding
        unsigned int : 5;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_interrupt_reg_t;

typedef union {
    //
    // This register determines if an interrupt indication is created when an event happens
    struct {
        //
        // When value is 1, occurance of event 'frame_length_err_too_small_intr' will trigger an interrupt.
        unsigned int frame_length_err_too_small_intr   : 1;
        //
        // When value is 1, occurance of event 'frame_length_err_too_big_intr' will trigger an interrupt.
        unsigned int frame_length_err_too_big_intr     : 1;
        //
        // When value is 1, occurance of event 'frame_depth_err_too_small_intr' will trigger an interrupt.
        unsigned int frame_depth_err_too_small_intr    : 1;
        //
        // When value is 1, occurance of event 'frame_depth_err_too_big_intr' will trigger an interrupt.
        unsigned int frame_depth_err_too_big_intr      : 1;
        //
        // When value is 1, occurance of event 'protocol_err_sof_wo_vld_intr' will trigger an interrupt.
        unsigned int protocol_err_sof_wo_vld_intr      : 1;
        //
        // When value is 1, occurance of event 'protocol_err_eof_wo_vld_intr' will trigger an interrupt.
        unsigned int protocol_err_eof_wo_vld_intr      : 1;
        //
        // When value is 1, occurance of event 'protocol_err_sol_wo_vld_intr' will trigger an interrupt.
        unsigned int protocol_err_sol_wo_vld_intr      : 1;
        //
        // When value is 1, occurance of event 'protocol_err_eol_wo_vld_intr' will trigger an interrupt.
        unsigned int protocol_err_eol_wo_vld_intr      : 1;
        //
        // When value is 1, occurance of event 'protocol_err_dbl_sof_intr' will trigger an interrupt.
        unsigned int protocol_err_dbl_sof_intr         : 1;
        //
        // When value is 1, occurance of event 'protocol_err_dbl_sol_intr' will trigger an interrupt.
        unsigned int protocol_err_dbl_sol_intr         : 1;
        //
        // When value is 1, occurance of event 'protocol_err_dbl_eof_intr' will trigger an interrupt.
        unsigned int protocol_err_dbl_eof_intr         : 1;
        //
        // When value is 1, occurance of event 'protocol_err_dbl_eol_intr' will trigger an interrupt.
        unsigned int protocol_err_dbl_eol_intr         : 1;
        //
        // When value is 1, occurance of event 'too_many_candidates' will trigger an interrupt.
        unsigned int too_many_candidates               : 1;
        //
        // When value is 1, occurance of event 'rsz_mem_parity_err_intr' will trigger an interrupt.
        unsigned int rsz_mem_parity_err_intr           : 1;
        //
        // When value is 1, occurance of event 'alg_mem_parity_err_intr' will trigger an interrupt.
        unsigned int alg_mem_parity_err_intr           : 1;
        //
        // When value is 1, occurance of event 'nms_mem_parity_err_intr' will trigger an interrupt.
        unsigned int nms_mem_parity_err_intr           : 1;
        //
        // When value is 1, occurance of event 'fast_th_mem_sbe_err_intr_user' will trigger an interrupt.
        unsigned int fast_th_mem_sbe_err_intr_user     : 1;
        //
        // When value is 1, occurance of event 'fast_th_mem_dbe_err_intr_user' will trigger an interrupt.
        unsigned int fast_th_mem_dbe_err_intr_user     : 1;
        //
        // When value is 1, occurance of event 'fast_th_mem_sbe_err_intr' will trigger an interrupt.
        unsigned int fast_th_mem_sbe_err_intr          : 1;
        //
        // When value is 1, occurance of event 'fast_th_mem_dbe_err_intr' will trigger an interrupt.
        unsigned int fast_th_mem_dbe_err_intr          : 1;
        //
        // When value is 1, occurance of event 'frame_bad_size_max_intr' will trigger an interrupt.
        unsigned int frame_bad_size_max_intr           : 1;
        //
        // When value is 1, occurance of event 'frame_bad_size_min_intr' will trigger an interrupt.
        unsigned int frame_bad_size_min_intr           : 1;
        //
        // When value is 1, occurance of event 'frame_bad_size_odd_pixel_num_intr' will trigger an interrupt.
        unsigned int frame_bad_size_odd_pixel_num_intr : 1;
        //
        // When value is 1, occurance of event 'cand_buff_bp_intr' will trigger an interrupt.
        unsigned int cand_buff_bp_intr                 : 1;
        //
        // When value is 1, occurance of event 'frame_buff_bp_intr' will trigger an interrupt.
        unsigned int frame_buff_bp_intr                : 1;
        //
        // When value is 1, occurance of event 'frame_wmrk_intr' will trigger an interrupt.
        unsigned int frame_wmrk_intr                   : 1;
        //
        // When value is 1, occurance of event 'cand_wmrk_intr' will trigger an interrupt.
        unsigned int cand_wmrk_intr                    : 1;
        // Just for padding
        unsigned int : 5;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_interrupt_reg__enable_t;

typedef union {
    //
    //
    struct {
        //
        // Holds the current value (not sticky) of 'frame_length_err_too_small_intr'.
        unsigned int frame_length_err_too_small_intr   : 1;
        //
        // Holds the current value (not sticky) of 'frame_length_err_too_big_intr'.
        unsigned int frame_length_err_too_big_intr     : 1;
        //
        // Holds the current value (not sticky) of 'frame_depth_err_too_small_intr'.
        unsigned int frame_depth_err_too_small_intr    : 1;
        //
        // Holds the current value (not sticky) of 'frame_depth_err_too_big_intr'.
        unsigned int frame_depth_err_too_big_intr      : 1;
        //
        // Holds the current value (not sticky) of 'protocol_err_sof_wo_vld_intr'.
        unsigned int protocol_err_sof_wo_vld_intr      : 1;
        //
        // Holds the current value (not sticky) of 'protocol_err_eof_wo_vld_intr'.
        unsigned int protocol_err_eof_wo_vld_intr      : 1;
        //
        // Holds the current value (not sticky) of 'protocol_err_sol_wo_vld_intr'.
        unsigned int protocol_err_sol_wo_vld_intr      : 1;
        //
        // Holds the current value (not sticky) of 'protocol_err_eol_wo_vld_intr'.
        unsigned int protocol_err_eol_wo_vld_intr      : 1;
        //
        // Holds the current value (not sticky) of 'protocol_err_dbl_sof_intr'.
        unsigned int protocol_err_dbl_sof_intr         : 1;
        //
        // Holds the current value (not sticky) of 'protocol_err_dbl_sol_intr'.
        unsigned int protocol_err_dbl_sol_intr         : 1;
        //
        // Holds the current value (not sticky) of 'protocol_err_dbl_eof_intr'.
        unsigned int protocol_err_dbl_eof_intr         : 1;
        //
        // Holds the current value (not sticky) of 'protocol_err_dbl_eol_intr'.
        unsigned int protocol_err_dbl_eol_intr         : 1;
        //
        // Holds the current value (not sticky) of 'too_many_candidates'.
        unsigned int too_many_candidates               : 1;
        //
        // Holds the current value (not sticky) of 'rsz_mem_parity_err_intr'.
        unsigned int rsz_mem_parity_err_intr           : 1;
        //
        // Holds the current value (not sticky) of 'alg_mem_parity_err_intr'.
        unsigned int alg_mem_parity_err_intr           : 1;
        //
        // Holds the current value (not sticky) of 'nms_mem_parity_err_intr'.
        unsigned int nms_mem_parity_err_intr           : 1;
        //
        // Holds the current value (not sticky) of 'fast_th_mem_sbe_err_intr_user'.
        unsigned int fast_th_mem_sbe_err_intr_user     : 1;
        //
        // Holds the current value (not sticky) of 'fast_th_mem_dbe_err_intr_user'.
        unsigned int fast_th_mem_dbe_err_intr_user     : 1;
        //
        // Holds the current value (not sticky) of 'fast_th_mem_sbe_err_intr'.
        unsigned int fast_th_mem_sbe_err_intr          : 1;
        //
        // Holds the current value (not sticky) of 'fast_th_mem_dbe_err_intr'.
        unsigned int fast_th_mem_dbe_err_intr          : 1;
        //
        // Holds the current value (not sticky) of 'frame_bad_size_max_intr'.
        unsigned int frame_bad_size_max_intr           : 1;
        //
        // Holds the current value (not sticky) of 'frame_bad_size_min_intr'.
        unsigned int frame_bad_size_min_intr           : 1;
        //
        // Holds the current value (not sticky) of 'frame_bad_size_odd_pixel_num_intr'.
        unsigned int frame_bad_size_odd_pixel_num_intr : 1;
        //
        // Holds the current value (not sticky) of 'cand_buff_bp_intr'.
        unsigned int cand_buff_bp_intr                 : 1;
        //
        // Holds the current value (not sticky) of 'frame_buff_bp_intr'.
        unsigned int frame_buff_bp_intr                : 1;
        //
        // Holds the current value (not sticky) of 'frame_wmrk_intr'.
        unsigned int frame_wmrk_intr                   : 1;
        //
        // Holds the current value (not sticky) of 'cand_wmrk_intr'.
        unsigned int cand_wmrk_intr                    : 1;
        // Just for padding
        unsigned int : 5;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_interrupt_reg__status_t;

typedef union {
    // CDP Global Interrupt Register
    // This register holds the CDP Global interrupts
    struct {
        //
        // CDP Configuration Memory SBE Interrupt.
        unsigned int cfg_mem_sbe_intr              : 1;
        //
        // CDP Configuration Memory DBE Interrupt.
        unsigned int cfg_mem_dbe_intr              : 1;
        //
        // CDP Done Interrupt.
        unsigned int cdp_done_intr                 : 1;
        //
        // CDP Start Interrupt.
        unsigned int cdp_start_intr                : 1;
        //
        // CDP TX-ISPP Trigger Interrupt.
        unsigned int cdp_txispp_trig_intr          : 1;
        //
        // CDP Start Memory Read Interrupt.
        unsigned int cdp_sysmem_read_intr          : 1;
        //
        // CDP Stage0 Done Interrupt.
        unsigned int cdp_stage0_done_intr          : 1;
        //
        // CDP Stage1 Done Interrupt.
        unsigned int cdp_stage1_done_intr          : 1;
        //
        // CDP Stage2 Done Interrupt.
        unsigned int cdp_stage2_done_intr          : 1;
        //
        // .
        unsigned int axi_rx_fifo_ovf_intr          : 1;
        //
        // .
        unsigned int axi_rx_err_mem_0_par_err_intr : 1;
        //
        // .
        unsigned int axi_rx_err_mem_1_par_err_intr : 1;
        //
        // .
        unsigned int axi_rx_err_mem_2_par_err_intr : 1;
        //
        // .
        unsigned int axi_rx_err_mem_3_par_err_intr : 1;
        //
        // .
        unsigned int axi_rx_err_mem_4_par_err_intr : 1;
        //
        // .
        unsigned int axi_rx_err_mem_5_par_err_intr : 1;
        //
        // .
        unsigned int axi_rx_err_mem_6_par_err_intr : 1;
        //
        // .
        unsigned int axi_rx_err_mem_7_par_err_intr : 1;
        //
        // .
        unsigned int lbm_fifo_rd_par_err_intr      : 1;
        //
        // .
        unsigned int lbm_rd_fifo_overflow_intr     : 1;
        //
        // Spacer FIFO overflow error interrupt.
        unsigned int spacer_ovf_intr               : 1;
        // Just for padding
        unsigned int : 11;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_global_interrupt_reg_t;

typedef union {
    //
    // This register determines if an interrupt indication is created when an event happens
    struct {
        //
        // When value is 1, occurance of event 'cfg_mem_sbe_intr' will trigger an interrupt.
        unsigned int cfg_mem_sbe_intr              : 1;
        //
        // When value is 1, occurance of event 'cfg_mem_dbe_intr' will trigger an interrupt.
        unsigned int cfg_mem_dbe_intr              : 1;
        //
        // When value is 1, occurance of event 'cdp_done_intr' will trigger an interrupt.
        unsigned int cdp_done_intr                 : 1;
        //
        // When value is 1, occurance of event 'cdp_start_intr' will trigger an interrupt.
        unsigned int cdp_start_intr                : 1;
        //
        // When value is 1, occurance of event 'cdp_txispp_trig_intr' will trigger an interrupt.
        unsigned int cdp_txispp_trig_intr          : 1;
        //
        // When value is 1, occurance of event 'cdp_sysmem_read_intr' will trigger an interrupt.
        unsigned int cdp_sysmem_read_intr          : 1;
        //
        // When value is 1, occurance of event 'cdp_stage0_done_intr' will trigger an interrupt.
        unsigned int cdp_stage0_done_intr          : 1;
        //
        // When value is 1, occurance of event 'cdp_stage1_done_intr' will trigger an interrupt.
        unsigned int cdp_stage1_done_intr          : 1;
        //
        // When value is 1, occurance of event 'cdp_stage2_done_intr' will trigger an interrupt.
        unsigned int cdp_stage2_done_intr          : 1;
        //
        // When value is 1, occurance of event 'axi_rx_fifo_ovf_intr' will trigger an interrupt.
        unsigned int axi_rx_fifo_ovf_intr          : 1;
        //
        // When value is 1, occurance of event 'axi_rx_err_mem_0_par_err_intr' will trigger an interrupt.
        unsigned int axi_rx_err_mem_0_par_err_intr : 1;
        //
        // When value is 1, occurance of event 'axi_rx_err_mem_1_par_err_intr' will trigger an interrupt.
        unsigned int axi_rx_err_mem_1_par_err_intr : 1;
        //
        // When value is 1, occurance of event 'axi_rx_err_mem_2_par_err_intr' will trigger an interrupt.
        unsigned int axi_rx_err_mem_2_par_err_intr : 1;
        //
        // When value is 1, occurance of event 'axi_rx_err_mem_3_par_err_intr' will trigger an interrupt.
        unsigned int axi_rx_err_mem_3_par_err_intr : 1;
        //
        // When value is 1, occurance of event 'axi_rx_err_mem_4_par_err_intr' will trigger an interrupt.
        unsigned int axi_rx_err_mem_4_par_err_intr : 1;
        //
        // When value is 1, occurance of event 'axi_rx_err_mem_5_par_err_intr' will trigger an interrupt.
        unsigned int axi_rx_err_mem_5_par_err_intr : 1;
        //
        // When value is 1, occurance of event 'axi_rx_err_mem_6_par_err_intr' will trigger an interrupt.
        unsigned int axi_rx_err_mem_6_par_err_intr : 1;
        //
        // When value is 1, occurance of event 'axi_rx_err_mem_7_par_err_intr' will trigger an interrupt.
        unsigned int axi_rx_err_mem_7_par_err_intr : 1;
        //
        // When value is 1, occurance of event 'lbm_fifo_rd_par_err_intr' will trigger an interrupt.
        unsigned int lbm_fifo_rd_par_err_intr      : 1;
        //
        // When value is 1, occurance of event 'lbm_rd_fifo_overflow_intr' will trigger an interrupt.
        unsigned int lbm_rd_fifo_overflow_intr     : 1;
        //
        // When value is 1, occurance of event 'spacer_ovf_intr' will trigger an interrupt.
        unsigned int spacer_ovf_intr               : 1;
        // Just for padding
        unsigned int : 11;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_global_interrupt_reg__enable_t;

typedef union {
    //
    //
    struct {
        //
        // Holds the current value (not sticky) of 'cfg_mem_sbe_intr'.
        unsigned int cfg_mem_sbe_intr              : 1;
        //
        // Holds the current value (not sticky) of 'cfg_mem_dbe_intr'.
        unsigned int cfg_mem_dbe_intr              : 1;
        //
        // Holds the current value (not sticky) of 'cdp_done_intr'.
        unsigned int cdp_done_intr                 : 1;
        //
        // Holds the current value (not sticky) of 'cdp_start_intr'.
        unsigned int cdp_start_intr                : 1;
        //
        // Holds the current value (not sticky) of 'cdp_txispp_trig_intr'.
        unsigned int cdp_txispp_trig_intr          : 1;
        //
        // Holds the current value (not sticky) of 'cdp_sysmem_read_intr'.
        unsigned int cdp_sysmem_read_intr          : 1;
        //
        // Holds the current value (not sticky) of 'cdp_stage0_done_intr'.
        unsigned int cdp_stage0_done_intr          : 1;
        //
        // Holds the current value (not sticky) of 'cdp_stage1_done_intr'.
        unsigned int cdp_stage1_done_intr          : 1;
        //
        // Holds the current value (not sticky) of 'cdp_stage2_done_intr'.
        unsigned int cdp_stage2_done_intr          : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_fifo_ovf_intr'.
        unsigned int axi_rx_fifo_ovf_intr          : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_err_mem_0_par_err_intr'.
        unsigned int axi_rx_err_mem_0_par_err_intr : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_err_mem_1_par_err_intr'.
        unsigned int axi_rx_err_mem_1_par_err_intr : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_err_mem_2_par_err_intr'.
        unsigned int axi_rx_err_mem_2_par_err_intr : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_err_mem_3_par_err_intr'.
        unsigned int axi_rx_err_mem_3_par_err_intr : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_err_mem_4_par_err_intr'.
        unsigned int axi_rx_err_mem_4_par_err_intr : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_err_mem_5_par_err_intr'.
        unsigned int axi_rx_err_mem_5_par_err_intr : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_err_mem_6_par_err_intr'.
        unsigned int axi_rx_err_mem_6_par_err_intr : 1;
        //
        // Holds the current value (not sticky) of 'axi_rx_err_mem_7_par_err_intr'.
        unsigned int axi_rx_err_mem_7_par_err_intr : 1;
        //
        // Holds the current value (not sticky) of 'lbm_fifo_rd_par_err_intr'.
        unsigned int lbm_fifo_rd_par_err_intr      : 1;
        //
        // Holds the current value (not sticky) of 'lbm_rd_fifo_overflow_intr'.
        unsigned int lbm_rd_fifo_overflow_intr     : 1;
        //
        // Holds the current value (not sticky) of 'spacer_ovf_intr'.
        unsigned int spacer_ovf_intr               : 1;
        // Just for padding
        unsigned int : 11;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_global_interrupt_reg__status_t;

typedef union {
    // CDP Interrupt Register
    // This register holds the CDP interrupts
    struct {
        //
        // CDP0 Interrupt Register.
        unsigned int cdp_s0_interupt     : 1;
        //
        // CDP1 Interrupt Register.
        unsigned int cdp_s1_interupt     : 1;
        //
        // CDP2 Interrupt Register.
        unsigned int cdp_s2_interupt     : 1;
        //
        // CDP Global Interrupt Register.
        unsigned int cdp_global_interupt : 1;
        // Just for padding
        unsigned int : 28;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_master_interrupt_reg_t;

typedef union {
    // CDP Freeze Master Register
    // This register holds the CDP interrupts
    struct {
        //
        // CDP0 Freeze Register.
        unsigned int cdp_s0_interupt     : 1;
        //
        // CDP1 Freeze Register.
        unsigned int cdp_s1_interupt     : 1;
        //
        // CDP2 Freeze Register.
        unsigned int cdp_s2_interupt     : 1;
        //
        // CDP Global Interrupt Register.
        unsigned int cdp_global_interupt : 1;
        // Just for padding
        unsigned int : 28;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_master_freeze_reg_t;

typedef union {
    // CDP Halt Master Register
    // This register holds the CDP interrupts
    struct {
        //
        // CDP0 Halt Register.
        unsigned int cdp_s0_interupt     : 1;
        //
        // CDP1 Halt Register.
        unsigned int cdp_s1_interupt     : 1;
        //
        // CDP2 Halt Register.
        unsigned int cdp_s2_interupt     : 1;
        //
        // CDP Global Interrupt Register.
        unsigned int cdp_global_interupt : 1;
        // Just for padding
        unsigned int : 28;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_master_halt_reg_t;

typedef union {
    // Stage0 Frame Maximum Configuration Register
    // This register used to configure the maximum valid input frame.
    struct {
        //
        // Max Lines.
        unsigned int lines_number  : 12;
        // Just for padding
        unsigned int : 4;
        //
        // Max Pixels.
        unsigned int pixels_number : 12;
        // Just for padding
        unsigned int : 4;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp0_frame_max_reg_t;

typedef union {
    // Stage1 Frame Maximum Configuration Register
    // This register used to configure the maximum valid input frame.
    struct {
        //
        // Max Lines.
        unsigned int lines_number  : 12;
        // Just for padding
        unsigned int : 4;
        //
        // Max Pixels.
        unsigned int pixels_number : 12;
        // Just for padding
        unsigned int : 4;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp1_frame_max_reg_t;

typedef union {
    // Stage2 Frame Maximum Configuration Register
    // This register used to configure the maximum valid input frame.
    struct {
        //
        // Max Lines.
        unsigned int lines_number  : 12;
        // Just for padding
        unsigned int : 4;
        //
        // Max Pixels.
        unsigned int pixels_number : 12;
        // Just for padding
        unsigned int : 4;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp2_frame_max_reg_t;

typedef union {
    // Stage0 Frame Minimum Configuration Register
    // This register used to configure the minimum valid input frame.
    struct {
        //
        // Min Lines.
        unsigned int lines_number  : 12;
        // Just for padding
        unsigned int : 4;
        //
        // Min Pixels.
        unsigned int pixels_number : 12;
        // Just for padding
        unsigned int : 4;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp0_frame_min_reg_t;

typedef union {
    // Stage1 Frame Minimum Configuration Register
    // This register used to configure the minimum valid input frame.
    struct {
        //
        // Min Lines.
        unsigned int lines_number  : 12;
        // Just for padding
        unsigned int : 4;
        //
        // Min Pixels.
        unsigned int pixels_number : 12;
        // Just for padding
        unsigned int : 4;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp1_frame_min_reg_t;

typedef union {
    // Stage2 Frame Minimum Configuration Register
    // This register used to configure the minimum valid input frame.
    struct {
        //
        // Min Lines.
        unsigned int lines_number  : 12;
        // Just for padding
        unsigned int : 4;
        //
        // Min Pixels.
        unsigned int pixels_number : 12;
        // Just for padding
        unsigned int : 4;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp2_frame_min_reg_t;

typedef union {
    // CDP Descriptor Configuration Memory Syndrome Register.
    // This register used for holding the memory ECC syndrome.
    struct {
        //
        // Address.
        unsigned int address  : 12;
        // Just for padding
        unsigned int : 4;
        //
        // Syndrome.
        unsigned int syndrome : 6;
        // Just for padding
        unsigned int : 2;
        //
        // Latch Error Address.
        unsigned int hold     : 1;
        // Just for padding
        unsigned int : 7;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_mem_err_reg_t;

typedef union {
    // CDP Debug Register.
    // This register used for CDP debug.This is only for HW use.
    struct {
        //
        // Value.
        unsigned int val : 32;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_debug_reg_t;

typedef union {
    // CDP Debug Register.
    // This register used for CDP debug.This is only for HW use.
    struct {
        //
        // When '1' - Shutdown.
        unsigned int cfg_mem_SD           : 1;
        //
        // When '1' - Shutdown.
        unsigned int s0_fast_thold_mem_SD : 1;
        //
        // When '1' - Shutdown.
        unsigned int s1_fast_thold_mem_SD : 1;
        //
        // When '1' - Shutdown.
        unsigned int s2_fast_thold_mem_SD : 1;
        //
        // When '1' - Shutdown.
        unsigned int s0_alg_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s1_alg_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s2_alg_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s0_rsz_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s1_rsz_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s2_rsz_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s0_nms_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s1_nms_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s2_nms_mem_SD        : 1;
        //
        // When '1' - Shutdown.
        unsigned int axi_tx_SD            : 1;
        // Just for padding
        unsigned int : 18;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_power_shutdown_reg_t;

typedef union {
    // CDP Debug Register.
    // This register used for CDP debug.This is only for HW use.
    struct {
        //
        // When '1' - Shutdown.
        unsigned int cfg_mem_SLP           : 1;
        //
        // When '1' - Shutdown.
        unsigned int s0_fast_thold_mem_SLP : 1;
        //
        // When '1' - Shutdown.
        unsigned int s1_fast_thold_mem_SLP : 1;
        //
        // When '1' - Shutdown.
        unsigned int s2_fast_thold_mem_SLP : 1;
        //
        // When '1' - Shutdown.
        unsigned int s0_alg_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s1_alg_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s2_alg_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s0_rsz_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s1_rsz_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s2_rsz_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s0_nms_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s1_nms_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int s2_nms_mem_SLP        : 1;
        //
        // When '1' - Shutdown.
        unsigned int axi_tx_SLP            : 1;
        // Just for padding
        unsigned int : 18;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_power_sleep_reg_t;

typedef union {
    // Memory Power Control Register.
    // This register used for memory power control.
    struct {
        //
        // Value.
        unsigned int MEM_POWER_CTRL_SD : 1;
        //
        // Value.
        unsigned int MEM_POWER_CTRL_DS : 1;
        //
        // Value.
        unsigned int MEM_POWER_CTRL_LS : 1;
        // Just for padding
        unsigned int : 29;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_mem_power_cntl0_reg_t;

typedef union {
    // CDP Memory Adjust Register.
    // This register used for CDP.
    struct {
        //
        // Value.
        unsigned int WTSEL : 6;
        //
        // Value.
        unsigned int RTSEL : 6;
        // Just for padding
        unsigned int : 20;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_mem_adj0_reg_t;

typedef union {
    // CDP Memory Adjust Register.
    // This register used for CDP.
    struct {
        //
        // Value.
        unsigned int mem_WTSEL : 2;
        //
        // Value.
        unsigned int mem_RTSEL : 2;
        // Just for padding
        unsigned int : 28;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_mem_adj1_reg_t;

typedef union {
    // CDP Memory Adjust Register.
    // This register used for CDP.
    struct {
        //
        // Value.
        unsigned int low_PTSEL : 16;
        //
        // Value.
        unsigned int hi_PTSEL  : 16;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_mem_adj2_reg_t;

typedef union {
    // CDP Memory Adjust Register.
    // This register used for CDP.
    struct {
        //
        // Value.
        unsigned int WTSEL : 16;
        //
        // Value.
        unsigned int RTSEL : 16;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_cdp_mem_adj3_reg_t;

typedef union {
    // AXI Buffer Threshold Register.
    // This register is used to configure the AXI buffer backpressure.
    struct {
        //
        // Value.
        unsigned int cand_af_th  : 13;
        // Just for padding
        unsigned int : 3;
        //
        // Value.
        unsigned int frame_af_th : 13;
        // Just for padding
        unsigned int : 3;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_axi_th_reg_t;

typedef union {
    // AXI Buffer Size Register.
    // This register is used to configure FAX-AXI burst size in 16B Quanta
    struct {
        //
        // AXI Read Burst size x16B
        unsigned int rd       : 9;
        //
        // AXI Write Frame - Burst size x16B
        unsigned int wr_frame : 9;
        //
        // AXI Write Candidate - Burst size x16B
        unsigned int wr_cand  : 9;
        // Just for padding
        unsigned int : 5;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_axi_burst_size_reg_t;

typedef union {
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    struct {
        //
        // Counter
        unsigned int cnt : 20;
        // Just for padding
        unsigned int : 12;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_watermark_cur_cntr_reg_t;

typedef union {
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    struct {
        //
        // Counter Value
        unsigned int val : 20;
        // Just for padding
        unsigned int : 12;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_watermark_last_cntr_reg_t;

typedef union {
    // Watermark Threshold Register
    // This register controls the Watermark Threshold.
    struct {
        //
        // AXI write Watermark Threshold
        unsigned int wmrk_th : 20;
        // Just for padding
        unsigned int : 12;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_watermark_threshold_reg_t;

typedef union {
    // AXI Write Status Register
    // This register holds the AXI Write status
    struct {
        //
        // Engine 0 - Channel Empty
        unsigned int frame0_empty : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int cand0_empty  : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int frame1_empty : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int cand1_empty  : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int frame2_empty : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int cand2_empty  : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int frame3_empty : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int cand3_empty  : 1;
        // Just for padding
        unsigned int : 24;
    } bitfield;
    unsigned int register_value;
} cdp_rgf_axi_wr_status_reg_t;

typedef struct {
    //
    // CDP Descriptor Configuration Memory
    // This memory is used to configure the CDP stages.
    //   <br>CDP Descriptor Configuration Memory Brakedown:
    //   <br>Name                Bits    Offset  Word      Description
    //   <br>------------------------------------------------------------------------------------------------------------------------
    //   <br>CFG_ISP_BYPASS_EN       1   0   0   Enable the ISP output.
    //   <br>CFG_ISP_BYPASS                  1   1   0   Bypass the downsize and Gaussian for ISP output.
    //   <br>CFG_ISP_PYR_EN                  1   2   0   Enable downsize for ISP output.
    //   <br>CFG_ALG_BYPASS_EN       1   3   0   Enable the FAST output.
    //   <br>CFG_ALG_BYPASS                  1   4   0   Bypass the downsize and Gaussian for FAST output.
    //   <br>CFG_ALG_PYR_EN                  1   5   0   Enable downsize for FAST output.
    //   <br>CFG_NXT_BYPASS_EN       1   6   0   Enable the next CDP stage output.
    //   <br>CFG_NXT_BYPASS          1   7   0   Bypass the downsize and Gaussian for next CDP stage output.
    //   <br>CFG_NXT_PYR_EN                  1   8   0   Enable downsize for next CDP stage output.
    //   <br>CFG_NMS_VLD_EN                  1   9   0   Enable the LNMS corner candidate output.
    //   <br>CFG_CAMERA_ROTATE       1   11  0   Enable camera rotate for this engine.
    //   <br>CFG_GAUSSIAN_EN                 1   12  0   Enable Gaussian.
    //   <br>CFG_NEXT_CDP_VLD        1   14  0   Enable next CDP stage, when '1' the user should make sure that the next CDP stage is configured.
    //   <br>FAST_NO_MEM_MODE        1   15  0   FAST No memory mode, when '1' the FAST threshold is fixed for all image, the threshold value is configured in 'FAST_TH' field in WORD16.
    //   <br>                                                                             when '0' the FAST threshold is grid based, the grid thresholds are written the FAST Threshold memory.
    //   <br>FRAME_DPCKI         1   16  0   Output Image Pack Enable
    //   <br>FRAME_FROTI         2   17  0   Output Image Rotate
    //   <br>FRAME_OBPPI         2   20  0   Output Image BPP
    //   <br>CFG_BPCSFRI         3   24  0   Input Image Shift Right with Round.
    //   <br>CFG_BPCSFLI         3   28  0   Input Image Shift Left.
    //   <br>DOWNSIZE_CFG_INPUT_IMG_LINES    10  0   1   Downsize input image lines number.
    //   <br>DOWNSIZE_CFG_INPUT_IMG_PIXELS   10  12  1   Downsize input image pixels number.
    //   <br>DOWNSIZE_CFG_OUTPUT_IMG_LINES   10  0   2   Downsize output image lines number.
    //   <br>DOWNSIZE_CFG_OUTPUT_IMG_PIXELS  10  12  2   Downsize output image pixels number.
    //   <br>DOWNSIZE_CFG_COEFF_SEL          3   24  2   Downsize ratio select:
    //   <br>                            3'd0: 1
    //   <br>                            3'd1: 1.25
    //   <br>                            3'd2: 1.4
    //   <br>                            3'd3: 1.5
    //   <br>                            3'd4: 1.6
    //   <br>                            3'd5: 1.75
    //   <br>                            3'd6: 2
    //   <br>                            3'd7: 1.2
    //   <br>GASSUIAN_CFG_COEFF_BBB_OUT  12  0   3   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CENTER   12  12  3   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CROSS_IN 12  0   4   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CROSS_OUT    12  12  4   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CORNER_IN    12  0   5   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_COEFF_CORNER_OUT   12  12  5   Gaussian Coefficients.
    //   <br>GASSUIAN_CFG_SHIFBACK_CFG       5   24  5   Gaussian Coefficients.
    //   <br>FAST_CFG_THOLD_HWINTSIZE    10  0   6   FAST Threshold horizontal grid size.
    //   <br>                                                          When the engine supports only one camera and less than 5 stages the grid window number can reach 400.
    //   <br>                                                          When the engine supports only one camera, total grid window number is 400, maximum of 64 can be used for the stages 5-8.
    //   <br>                                                          When the engine supports two cameras, the total number per camera is 200.
    //   <br>FAST_CFG_THOLD_VWINTSIZE    10  16  6   FAST Threshold vertical grid size.
    //   <br>                                                          When the engine supports only one camera and less than 5 stages the grid window number can reach 400.
    //   <br>                                                          When the engine supports only one camera, total grid window number is 400, maximum of 64 can be used for the stages 5-8.
    //   <br>                                                          When the engine supports two cameras, the total number per camera is 200.
    //   <br>FAST_CFG_SEQLEN                 4   28  6   FAST K length [9-16]
    //   <br>CANDIDATE_CFG_WIN_HSIZE         10  0   7   LNMS Horizontal Grid Size, the size defines the window grid for maximum corner candidates number.
    //   <br>CANDIDATE_CFG_WIN_VSIZE         10  16  7   LNMS Veritcal Grid Size, the size defines the window grid for maximum corner candidates number.
    //   <br>CANDIDATE_CFG_USE_3x3_WIN   1   31  7   USE 3x3 LNMS Window.
    //   <br>CANDIDATE_CFG_WIN_MAX           14  0   8   Maximum corner candidate for LNMS grid window.
    //   <br>CANDIDATE_CFG_ROI_WIDTH         8   16  8   Rectangular ROI size, the width value defines the number of pixels from the image boundary.
    //   <br>CANDIDATE_CFG_ROI_EN        1   31  8   Enable Rectangular ROI
    //   <br>CANDIDATE_CFG_CIR_ROI_EN    1   31  9   Enable Circular ROI
    //   <br>CANDIDATE_CFG_CIR_ROI_RR    20  0   9   Circular ROI radios.
    //   <br>CANDIDATE_CFG_CIR_ROI_CXCX  20  0   10  Circular ROI Center X coordinate ^2.
    //   <br>CANDIDATE_CFG_CIR_ROI_CX    10  20  10  Circular ROI Center X coordinates.
    //   <br>CANDIDATE_CFG_CIR_ROI_CYCY  20  0   11  Circular ROI Center Y coordinate ^2.
    //   <br>CANDIDATE_CFG_CIR_ROI_CY    10  20  11  Circular ROI Center Y coordinates.
    //   <br>FRAME_MEM_OFFSET        32  0   12  Image Memory Offset, this 32b address is used by the ISP for forwarding the CDP output image to the system memory.
    //   <br>CANDIDATE_MEM_OFFSET            32  0   13  Candidates Memory Offset, this 32b address is used by the ISP for forwarding the CDP output candidates to the system memory.
    //   <br>NEXT_FRAME_MEM                  32  0   14  Next Image Offset, this 32b address is used by the ISP for reading an image from the system memory.
    //   <br>NEXT_FRAME_SIZE                 16  0   15  Next Image Size, in Bytes.
    //   <br>NEXT_FRAME_BPCSFR       4   16  15  Next Image Pixel Shift Right with Round
    //   <br>NEXT_FRAME_BPCSFL       4   20  15  Next Image Pixel Shift Left
    //   <br>NEXT_FRAME_IBPP                 2   24  15  Next Image Input Bit Per Pixel
    //   <br>NEXT_FRAME_EN                   1   30  15  Next Image Enable
    //   <br>NEXT_FRAME_DPCK         1   31  15  Next Image Unpack
    //   <br>NEXT_FRAME_RGAP                 0   16  16  Next Image Row Gap
    //   <br>FAST_TH                         16  16  16  Fast Threshold, when working in 'FAST_NO_MEM_MODE' the threshold value should be defined here.
    // Offset - 0x0
    cdp_rgf_cfg_mem_t                          cfg_mem[2496];

    //
    // CDP Global Configuration Register
    // This register configures the CDP block.
    // Offset - 0x2700
    cdp_rgf_cdp_global_cfg_reg_t               cdp_global_cfg_reg;

    //
    // CDP Row Spacer Configuration Register
    // This register configures the CDP block.
    // Offset - 0x2704
    cdp_rgf_cdp_row_spacer_cfg_reg_t           cdp_row_spacer_cfg_reg;

    //
    // CDP Window Clear Configuration Register
    // This register used to clear the window memory before starting the block.
    // Offset - 0x2708
    cdp_rgf_cdp_window_clr_reg_t               cdp_window_clr_reg;

    //
    // CDP Clock Gate Enable Configuration Register
    // This register used to enable clock gating.
    // Offset - 0x270c
    cdp_rgf_cdp_clk_gate_en_reg_t              cdp_clk_gate_en_reg;

    //
    // Fast Thold Configuration Register
    // This register selects the Fast thold memory.
    // Offset - 0x2710
    cdp_rgf_fast_thold_mem_cfg_reg_t           fast_thold_mem_cfg_reg;

    //
    // Stage0 Fast Threshold Memory
    // This memory configures the fast thresholds which can be used when working in threshold memory mode.
    //   <br>The memory can support the follwoing camera/stage options:
    //   <br>One Camera + Stages<5 -> 400 Threshold per image.
    //   <br>One Camera + Stages>4 -> 272 for stages 1-4 and 128 for stages 5-8.
    //   <br>Two Camera + Stages<5 -> 200 Threshold per image.
    //   <br>Two Camera + Stages>4 -> 136 for stages 1-4 and 64 for stages 5-8.
    //   <br>Note:The threshold for stages 5-8 can be any power of 2(2,4,16,32,64,128).
    // Offset - 0x2714
    cdp_rgf_fast_thold_mem_t                   cdp_s0_fast_thold_mem[200];

    //
    // Stage1 Fast Threshold Memory
    // This memory configures the fast thresholds which can be used when working in threshold memory mode.
    //   <br>The memory can support the follwoing camera/stage options:
    //   <br>One Camera + Stages<5 -> 400 Threshold per image.
    //   <br>One Camera + Stages>4 -> 272 for stages 1-4 and 128 for stages 5-8.
    //   <br>Two Camera + Stages<5 -> 200 Threshold per image.
    //   <br>Two Camera + Stages>4 -> 136 for stages 1-4 and 64 for stages 5-8.
    //   <br>Note:The threshold for stages 5-8 can be any power of 2(2,4,16,32,64,128).
    // Offset - 0x2a34
    cdp_rgf_fast_thold_mem_t                   cdp_s1_fast_thold_mem[200];

    //
    // Stage2 Fast Threshold Memory
    // This memory configures the fast thresholds which can be used when working in threshold memory mode.
    //   <br>The memory can support the follwoing camera/stage options:
    //   <br>One Camera + Stages<5 -> 400 Threshold per image.
    //   <br>One Camera + Stages>4 -> 272 for stages 1-4 and 128 for stages 5-8.
    //   <br>Two Camera + Stages<5 -> 200 Threshold per image.
    //   <br>Two Camera + Stages>4 -> 136 for stages 1-4 and 64 for stages 5-8.
    //   <br>Note:The threshold for stages 5-8 can be any power of 2(2,4,16,32,64,128).
    // Offset - 0x2d54
    cdp_rgf_fast_thold_mem_t                   cdp_s2_fast_thold_mem[200];

    //
    // Camera0 Fast Configuration Register
    // This register configures the number os thresholds for stages 5-8.
    // Offset - 0x3074
    cdp_rgf_cfg_fast_thold_mem_t               cfg_fast_thold_mem_cam0_rollback;

    //
    // Camera1 Fast Configuration Register
    // This register configures the number os thresholds for stages 5-8.
    // Offset - 0x3078
    cdp_rgf_cfg_fast_thold_mem_t               cfg_fast_thold_mem_cam1_rollback;

    //
    // Features Global Configuration Register
    // This register configures the candidate.
    //   <br>Note:This register is not functional.
    // Offset - 0x307c
    cdp_rgf_candidate_glbl_cfg_reg_t           candidate_glbl_cfg_reg;

    //
    // Stage0 Features Maximum Threshold Status Register
    // This register holds the X/Y cordinates of the pixel crossed the maximun threshold candidate.
    // Offset - 0x3080
    cdp_rgf_th_reached_status_reg_t            cdp0_th_reached_status_reg;

    //
    // Stage1 Features Maximum Threshold Status Register
    // This register holds the X/Y cordinates of the pixel crossed the maximun threshold candidate.
    // Offset - 0x3084
    cdp_rgf_th_reached_status_reg_t            cdp1_th_reached_status_reg;

    //
    // Stage2 Features Maximum Threshold Status Register
    // This register holds the X/Y cordinates of the pixel crossed the maximun threshold candidate.
    // Offset - 0x3088
    cdp_rgf_th_reached_status_reg_t            cdp2_th_reached_status_reg;

    //
    // CDP AXI Read Frame Section Size
    // Number of bytes added to memory for this channel.
    // Offset - 0x308c
    cdp_rgf_frm_sec_size_reg_t                 frm_sec_size_reg;

    //
    // CDP AXI Read Frame Section Enable
    // Enable Frame Section mode.
    // Offset - 0x3090
    cdp_rgf_frm_sec_enb_reg_t                  frm_sec_enb_reg;

    //
    // Stage0 CDP Interrupt Register
    // This register holds the CDP interrupts
    // Offset - 0x3094
    cdp_rgf_cdp_interrupt_reg_t                cdp_s0_interrupt_reg;

    //
    //
    // This register determines if an interrupt indication is created when an event happens
    // Offset - 0x3098
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s0_interrupt_reg_int_en;

    //
    //
    // This register determines if a freeze indication is created when an event happens
    // Offset - 0x309c
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s0_interrupt_reg_frz_en;

    //
    //
    // This register determines if a halt indication is created when an event happens
    // Offset - 0x30a0
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s0_interrupt_reg_hlt_en;

    //
    //
    //
    // Offset - 0x30a4
    cdp_rgf_cdp_interrupt_reg__status_t        cdp_s0_status_reg;

    //
    // Stage1 CDP Interrupt Register
    // This register holds the CDP interrupts
    // Offset - 0x30a8
    cdp_rgf_cdp_interrupt_reg_t                cdp_s1_interrupt_reg;

    //
    //
    // This register determines if an interrupt indication is created when an event happens
    // Offset - 0x30ac
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s1_interrupt_reg_int_en;

    //
    //
    // This register determines if a freeze indication is created when an event happens
    // Offset - 0x30b0
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s1_interrupt_reg_frz_en;

    //
    //
    // This register determines if a halt indication is created when an event happens
    // Offset - 0x30b4
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s1_interrupt_reg_hlt_en;

    //
    //
    //
    // Offset - 0x30b8
    cdp_rgf_cdp_interrupt_reg__status_t        cdp_s1_status_reg;

    //
    // Stage2 CDP Interrupt Register
    // This register holds the CDP interrupts
    // Offset - 0x30bc
    cdp_rgf_cdp_interrupt_reg_t                cdp_s2_interrupt_reg;

    //
    //
    // This register determines if an interrupt indication is created when an event happens
    // Offset - 0x30c0
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s2_interrupt_reg_int_en;

    //
    //
    // This register determines if a freeze indication is created when an event happens
    // Offset - 0x30c4
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s2_interrupt_reg_frz_en;

    //
    //
    // This register determines if a halt indication is created when an event happens
    // Offset - 0x30c8
    cdp_rgf_cdp_interrupt_reg__enable_t        cdp_s2_interrupt_reg_hlt_en;

    //
    //
    //
    // Offset - 0x30cc
    cdp_rgf_cdp_interrupt_reg__status_t        cdp_s2_status_reg;

    //
    // CDP Global Interrupt Register
    // This register holds the CDP Global interrupts
    // Offset - 0x30d0
    cdp_rgf_cdp_global_interrupt_reg_t         cdp_global_interrupt_reg;

    //
    //
    // This register determines if an interrupt indication is created when an event happens
    // Offset - 0x30d4
    cdp_rgf_cdp_global_interrupt_reg__enable_t cdp_global_interrupt_reg_int_en;

    //
    //
    // This register determines if a freeze indication is created when an event happens
    // Offset - 0x30d8
    cdp_rgf_cdp_global_interrupt_reg__enable_t cdp_global_interrupt_reg_frz_en;

    //
    //
    // This register determines if a halt indication is created when an event happens
    // Offset - 0x30dc
    cdp_rgf_cdp_global_interrupt_reg__enable_t cdp_global_interrupt_reg_hlt_en;

    //
    //
    //
    // Offset - 0x30e0
    cdp_rgf_cdp_global_interrupt_reg__status_t cdp_global_status_reg;

    //
    // CDP Interrupt Register
    // This register holds the CDP interrupts
    // Offset - 0x30e4
    cdp_rgf_cdp_master_interrupt_reg_t         cdp_master_interrupt_reg;

    //
    // CDP Freeze Master Register
    // This register holds the CDP interrupts
    // Offset - 0x30e8
    cdp_rgf_cdp_master_freeze_reg_t            cdp_master_freeze_reg;

    //
    // CDP Halt Master Register
    // This register holds the CDP interrupts
    // Offset - 0x30ec
    cdp_rgf_cdp_master_halt_reg_t              cdp_master_halt_reg;

    //
    // Stage0 Frame Maximum Configuration Register
    // This register used to configure the maximum valid input frame.
    // Offset - 0x30f0
    cdp_rgf_cdp0_frame_max_reg_t               cdp0_cfg_frame_max;

    //
    // Stage1 Frame Maximum Configuration Register
    // This register used to configure the maximum valid input frame.
    // Offset - 0x30f4
    cdp_rgf_cdp1_frame_max_reg_t               cdp1_cfg_frame_max;

    //
    // Stage2 Frame Maximum Configuration Register
    // This register used to configure the maximum valid input frame.
    // Offset - 0x30f8
    cdp_rgf_cdp2_frame_max_reg_t               cdp2_cfg_frame_max;

    //
    // Stage0 Frame Minimum Configuration Register
    // This register used to configure the minimum valid input frame.
    // Offset - 0x30fc
    cdp_rgf_cdp0_frame_min_reg_t               cdp0_cfg_frame_min;

    //
    // Stage1 Frame Minimum Configuration Register
    // This register used to configure the minimum valid input frame.
    // Offset - 0x3100
    cdp_rgf_cdp1_frame_min_reg_t               cdp1_cfg_frame_min;

    //
    // Stage2 Frame Minimum Configuration Register
    // This register used to configure the minimum valid input frame.
    // Offset - 0x3104
    cdp_rgf_cdp2_frame_min_reg_t               cdp2_cfg_frame_min;

    //
    // CDP Descriptor Configuration Memory Syndrome Register.
    // This register used for holding the memory ECC syndrome.
    // Offset - 0x3108
    cdp_rgf_cdp_mem_err_reg_t                  cdp_mem_err_reg;

    //
    // CDP Debug Register.
    // This register used for CDP debug.This is only for HW use.
    // Offset - 0x310c
    cdp_rgf_cdp_debug_reg_t                    cdp_input_cntl_debug_reg;

    //
    // CDP Debug Register.
    // This register used for CDP debug.This is only for HW use.
    // Offset - 0x3110
    cdp_rgf_cdp_power_shutdown_reg_t           cdp_power_shutdown_reg;

    //
    // CDP Debug Register.
    // This register used for CDP debug.This is only for HW use.
    // Offset - 0x3114
    cdp_rgf_cdp_power_sleep_reg_t              cdp_power_sleep_reg;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3118
    cdp_rgf_mem_power_cntl0_reg_t              cdp_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x311c
    cdp_rgf_mem_power_cntl0_reg_t              cdp_s0_fast_thold0;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3120
    cdp_rgf_mem_power_cntl0_reg_t              cdp_s1_fast_thold0;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3124
    cdp_rgf_mem_power_cntl0_reg_t              cdp_s2_fast_thold0;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3128
    cdp_rgf_mem_power_cntl0_reg_t              CDP0_ALG_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x312c
    cdp_rgf_mem_power_cntl0_reg_t              CDP1_ALG_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3130
    cdp_rgf_mem_power_cntl0_reg_t              CDP2_ALG_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3134
    cdp_rgf_mem_power_cntl0_reg_t              CDP0_RSZ_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3138
    cdp_rgf_mem_power_cntl0_reg_t              CDP1_RSZ_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x313c
    cdp_rgf_mem_power_cntl0_reg_t              CDP2_RSZ_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3140
    cdp_rgf_mem_power_cntl0_reg_t              CDP0_NMS_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3144
    cdp_rgf_mem_power_cntl0_reg_t              CDP1_NMS_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3148
    cdp_rgf_mem_power_cntl0_reg_t              CDP2_NMS_MEM;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x314c
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr0;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3150
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr1;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3154
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr2;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3158
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr3;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x315c
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr4;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3160
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr5;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3164
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr6;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3168
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr7;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x316c
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr8;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3170
    cdp_rgf_mem_power_cntl0_reg_t              axi_rd0;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3174
    cdp_rgf_mem_power_cntl0_reg_t              axi_rd1;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3178
    cdp_rgf_mem_power_cntl0_reg_t              axi_rd_sync;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x317c
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr_sync0_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3180
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr_sync1_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3184
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr_sync2_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3188
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr_sync3_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x318c
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr_sync4_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3190
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr_sync5_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3194
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr_sync6_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3198
    cdp_rgf_mem_power_cntl0_reg_t              axi_wr_sync7_mem_cntl;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x319c
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s0_rsz_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31a0
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s1_rsz_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31a4
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s2_rsz_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31a8
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s0_alg_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31ac
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s1_alg_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31b0
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s2_alg_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31b4
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s0_nms_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31b8
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s1_nms_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31bc
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_s2_nms_mem;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31c0
    cdp_rgf_cdp_mem_adj0_reg_t                 cdp_axi_tx_mem_tsel_reg;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31c4
    cdp_rgf_cdp_mem_adj1_reg_t                 cdp_cfg;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31c8
    cdp_rgf_cdp_mem_adj1_reg_t                 cdp_s0_fast_thold;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31cc
    cdp_rgf_cdp_mem_adj1_reg_t                 cdp_s1_fast_thold;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31d0
    cdp_rgf_cdp_mem_adj1_reg_t                 cdp_s2_fast_thold;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31d4
    cdp_rgf_cdp_mem_adj2_reg_t                 cdp_axi_rx_mem_ptsel;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31d8
    cdp_rgf_cdp_mem_adj3_reg_t                 cdp_axi_rx_mem_low_reg;

    //
    // CDP Memory Adjust Register.
    // This register used for CDP.
    // Offset - 0x31dc
    cdp_rgf_cdp_mem_adj3_reg_t                 cdp_axi_rx_mem_hi_reg;

    //
    // AXI Buffer Threshold Register.
    // This register is used to configure the AXI buffer backpressure.
    // Offset - 0x31e0
    cdp_rgf_axi_th_reg_t                       cdp_e0_s0;

    //
    // AXI Buffer Threshold Register.
    // This register is used to configure the AXI buffer backpressure.
    // Offset - 0x31e4
    cdp_rgf_axi_th_reg_t                       cdp_e0_s1;

    //
    // AXI Buffer Threshold Register.
    // This register is used to configure the AXI buffer backpressure.
    // Offset - 0x31e8
    cdp_rgf_axi_th_reg_t                       cdp_e0_s2;

    //
    // AXI Buffer Size Register.
    // This register is used to configure FAX-AXI burst size in 16B Quanta
    // Offset - 0x31ec
    cdp_rgf_axi_burst_size_reg_t               axi_burst_size;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x31f0
    cdp_rgf_watermark_cur_cntr_reg_t           frame0_watermark_cur_cntr_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x31f4
    cdp_rgf_watermark_cur_cntr_reg_t           cand0_watermark_cur_cntr_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x31f8
    cdp_rgf_watermark_cur_cntr_reg_t           frame1_watermark_cur_cntr_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x31fc
    cdp_rgf_watermark_cur_cntr_reg_t           cand1_watermark_cur_cntr_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x3200
    cdp_rgf_watermark_cur_cntr_reg_t           frame2_watermark_cur_cntr_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x3204
    cdp_rgf_watermark_cur_cntr_reg_t           cand2_watermark_cur_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x3208
    cdp_rgf_watermark_last_cntr_reg_t          frame0_watermark_last_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x320c
    cdp_rgf_watermark_last_cntr_reg_t          cand0_watermark_last_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x3210
    cdp_rgf_watermark_last_cntr_reg_t          frame1_watermark_last_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x3214
    cdp_rgf_watermark_last_cntr_reg_t          cand1_watermark_last_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x3218
    cdp_rgf_watermark_last_cntr_reg_t          frame2_watermark_last_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x321c
    cdp_rgf_watermark_last_cntr_reg_t          cand2_watermark_last_cntr_reg;

    //
    // Watermark Threshold Register
    // This register controls the Watermark Threshold.
    // Offset - 0x3220
    cdp_rgf_watermark_threshold_reg_t          frame_watermark_threshold_reg;

    //
    // Watermark Threshold Register
    // This register controls the Watermark Threshold.
    // Offset - 0x3224
    cdp_rgf_watermark_threshold_reg_t          cand_watermark_threshold_reg;

    //
    // AXI Write Status Register
    // This register holds the AXI Write status
    // Offset - 0x3228
    cdp_rgf_axi_wr_status_reg_t                cdp_axi_wr_status_reg;

} cdp_rgf_t;

typedef struct {
    //
    //
    //
    // Offset - 0x0
    cdp_rgf_t cdp_rgf;

} cdp_eng_rmap_t;

typedef union {
    // master128 interface0, amount of bytes to copy
    //
    struct {
        //
        //
        unsigned int nn : 20;
        // Just for padding
        unsigned int : 12;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_rd_bytes_reg_t;

typedef union {
    // master128 interface0, read start address register
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_rd_addr_lcl_reg_t;

typedef union {
    // master128 write interface0 start address register
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_rd_addr_rmt_reg_t;

typedef union {
    // master128, interface0 misc controls
    //
    struct {
        //
        //
        unsigned int lcl_size      : 3;
        //
        //
        unsigned int rmt_size      : 3;
        //
        //
        unsigned int lcl_addr_incr : 1;
        //
        //
        unsigned int rmt_addr_incr : 1;
        //
        //
        unsigned int qos           : 4;
        // Just for padding
        unsigned int : 20;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_rd_control_reg_t;

typedef union {
    // writing to this reg, fires the master
    //
    struct {
        //
        //
        unsigned int nn : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_rd_activate_reg_t;

typedef union {
    // master128 interface0, amount of bytes to copy
    //
    struct {
        //
        //
        unsigned int nn : 20;
        // Just for padding
        unsigned int : 12;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_wr_bytes_reg_t;

typedef union {
    // master128 interface0, read start address register
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_wr_addr_lcl_reg_t;

typedef union {
    // master128 write interface0 start address register
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_wr_addr_rmt_reg_t;

typedef union {
    // master128, interface0 misc controls
    //
    struct {
        //
        //
        unsigned int lcl_size      : 3;
        //
        //
        unsigned int rmt_size      : 3;
        //
        //
        unsigned int lcl_addr_incr : 1;
        //
        //
        unsigned int rmt_addr_incr : 1;
        //
        //
        unsigned int qos           : 4;
        // Just for padding
        unsigned int : 20;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_wr_control_reg_t;

typedef union {
    // writing to this reg, fires the master
    //
    struct {
        //
        //
        unsigned int nn : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_wr_activate_reg_t;

typedef union {
    // master128 interface0, max len of burst
    //
    struct {
        //
        //
        unsigned int nn : 8;
        // Just for padding
        unsigned int : 24;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_max_burst_len_reg_t;

typedef union {
    // master128 status register, readonly, mirrors key variables of master
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_status_reg_t;

typedef union {
    // master128 configuration bits
    //
    struct {
        //
        //
        unsigned int softreset    : 1;
        //
        //
        unsigned int abortonrderr : 1;
        //
        //
        unsigned int abortonwrerr : 1;
        //
        //
        unsigned int fixed_rd_id0 : 4;
        //
        //
        unsigned int fixed_rd_id1 : 4;
        //
        //
        unsigned int fixed_wr_id  : 4;
        // Just for padding
        unsigned int : 17;
    } bitfield;
    unsigned int register_value;
} cdp_master128apb_config_reg_t;

typedef struct {
    //
    // master128 interface0, amount of bytes to copy
    //
    // Offset - 0x0
    cdp_master128apb_rd_bytes_reg_t      rd_bytes;

    //
    // master128 interface0, read start address register
    //
    // Offset - 0x4
    cdp_master128apb_rd_addr_lcl_reg_t   rd_addr_lcl;

    //
    // master128 write interface0 start address register
    //
    // Offset - 0x8
    cdp_master128apb_rd_addr_rmt_reg_t   rd_addr_rmt;

    //
    // master128, interface0 misc controls
    //
    // Offset - 0xc
    cdp_master128apb_rd_control_reg_t    rd_control;

    //
    // writing to this reg, fires the master
    //
    // Offset - 0x10
    cdp_master128apb_rd_activate_reg_t   rd_activate;

    //
    // master128 interface0, amount of bytes to copy
    //
    // Offset - 0x14
    cdp_master128apb_wr_bytes_reg_t      wr_bytes;

    //
    // master128 interface0, read start address register
    //
    // Offset - 0x18
    cdp_master128apb_wr_addr_lcl_reg_t   wr_addr_lcl;

    //
    // master128 write interface0 start address register
    //
    // Offset - 0x1c
    cdp_master128apb_wr_addr_rmt_reg_t   wr_addr_rmt;

    //
    // master128, interface0 misc controls
    //
    // Offset - 0x20
    cdp_master128apb_wr_control_reg_t    wr_control;

    //
    // writing to this reg, fires the master
    //
    // Offset - 0x24
    cdp_master128apb_wr_activate_reg_t   wr_activate;

    //
    // master128 interface0, max len of burst
    //
    // Offset - 0x28
    cdp_master128apb_max_burst_len_reg_t max_burst_len;

    //
    // master128 status register, readonly, mirrors key variables of master
    //
    // Offset - 0x2c
    cdp_master128apb_status_reg_t        status;

    //
    // master128 configuration bits
    //
    // Offset - 0x30
    cdp_master128apb_config_reg_t        configx;

} cdp_master128apb_t;

typedef struct {
    //
    //
    //
    // Offset - 0x0
    cdp_eng_rmap_t eng;

    //
    // Dummy filler
    unsigned char RESERVED0[0x4000 - 0x322c];

    //
    //
    //
    // Offset - 0x4000
    cdp_master128apb_t axi;

} cdp_t;

extern volatile cdp_t *cdp;


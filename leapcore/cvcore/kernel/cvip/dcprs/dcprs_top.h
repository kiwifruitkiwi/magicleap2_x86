/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

//////////////////////////////////////////////////////////////////////////////////
// File : dcprs.h
// Created on : Tuesday Jan 07, 2020 [12:21:49 pm]
//////////////////////////////////////////////////////////////////////////////////

// Creation date and time
#define DCPRS_CREATE_TIME_STR                              "Tuesday Jan 07, 2020 [12:21:49 pm]"

#define DCPRS_BASE_ADDRESS                                 (0xd3800000)
#define DCPRS_MPTOP_DCPRS_MP0_ENG0_BASE_ADDRESS            (DCPRS_BASE_ADDRESS + 0x00000000)
#define DCPRS_MPTOP_DCPRS_MP0_ENG1_BASE_ADDRESS            (DCPRS_BASE_ADDRESS + 0x00002000)
#define DCPRS_MPTOP_DCPRS_MP0_DCPRS_MP_BASE_ADDRESS        (DCPRS_BASE_ADDRESS + 0x00004000)
#define DCPRS_MPTOP_DCPRS_MP0_BASE_ADDRESS                 (DCPRS_BASE_ADDRESS + 0x00000000)
#define DCPRS_MPTOP_DCPRS_MP1_ENG0_BASE_ADDRESS            (DCPRS_BASE_ADDRESS + 0x00008000)
#define DCPRS_MPTOP_DCPRS_MP1_ENG1_BASE_ADDRESS            (DCPRS_BASE_ADDRESS + 0x0000a000)
#define DCPRS_MPTOP_DCPRS_MP1_DCPRS_MP_BASE_ADDRESS        (DCPRS_BASE_ADDRESS + 0x0000c000)
#define DCPRS_MPTOP_DCPRS_MP1_BASE_ADDRESS                 (DCPRS_BASE_ADDRESS + 0x00008000)
#define DCPRS_MPTOP_RGF_BASE_ADDRESS                       (DCPRS_BASE_ADDRESS + 0x00010000)
#define DCPRS_MPTOP_BASE_ADDRESS                           (DCPRS_BASE_ADDRESS + 0x00000000)
#define AXIM_BASE_ADDRESS                                  (DCPRS_BASE_ADDRESS + 0x00011000)

typedef union {
    // JPEG-LS decoder configuration
    // JPEG-LS decoder configuration
    struct {
        //
        //
        unsigned int dat : 32;
    } bitfield;
    unsigned int register_value;
} dcprs_jpeglsdec_rgf_jpeglsdec_cfg_mem_t;

typedef struct {
    //
    // JPEG-LS decoder configuration
    // JPEG-LS decoder configuration
    // Offset - 0x0
    dcprs_jpeglsdec_rgf_jpeglsdec_cfg_mem_t jpeglsdec_cfg_mem[2048];

} dcprs_jpeglsdec_rgf_t;

typedef union {
    // Power Control Register
    // This register controls the CPRS Power options.
    struct {
        //
        // CPRS Memories Auto Power Control Enable
        unsigned int dcprs_auto_pwr_ctrl_en : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_power_control_reg_t;

typedef union {
    // MISC Clock Enable Register
    // This register controls CPRS blocks clock enable.
    //         A set field in this register overrides the descriptor enable configuration
    struct {
        //
        // Engine#0 S0 Clock Enable
        unsigned int eng0 : 1;
        //
        // Engine#1 Clock Enable
        unsigned int eng1 : 1;
        // Just for padding
        unsigned int : 30;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_clk_en_reg_t;

typedef union {
    // Bayer Collor Order
    // There are four options for storing a bayer frame. This register indicates idf the first pixel is green or Blue/Red.
    struct {
        //
        // 1: First pixel of frame is Green. 0: First pixel if frame is Blue or Red
        unsigned int cfg : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_bayer_green_first_reg_t;

typedef union {
    // AXI Write FIFO Back Pressure Threshold
    // Back pressure assertion level.
    struct {
        //
        // Back pressure assertion level.
        unsigned int cfg : 16;
        // Just for padding
        unsigned int : 16;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_axi_wr_fifo_bp_th_t;

typedef union {
    // DCPRS AXI Read Frame Section Size
    // Number of bytes added to memory for this channel.
    struct {
        //
        // Bytes added.
        unsigned int val : 32;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_frm_sec_size_reg_t;

typedef union {
    // DCPRS AXI Read Frame Section Enable
    // Enable Frame Section mode.
    struct {
        //
        // 0: Disable. 1: Enable.
        unsigned int en : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_frm_sec_enb_reg_t;

typedef union {
    // DCPRS AXI Read Frame Section Force EOF
    // Force EOF after last frame section was written.
    struct {
        //
        // 0: Disable. 1: Enable.
        unsigned int en : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_frm_sec_force_eof_reg_t;

typedef union {
    // ISPB input data byte switch Enable
    // Switch input that sources decoder:0: dat_out=dat_in. 1: dat_out = {dat_in[7:0],dat_in[15:8],dat_in[23:16],dat_in[31:24]}
    struct {
        //
        // 0: Disable. 1: Enable.
        unsigned int en : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_byte_switch_reg_t;

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
} dcprs_mp_rgf_mem_power_cntl_reg_t;

typedef union {
    // CPRS Engine#0 - Descriptor Execution Register
    // Insert descriptor into the execution FIFO
    struct {
        //
        // Pointer to the begining address of the descriptor for engine#0
        unsigned int eng0_pointer     : 11;
        //
        // Execution Counter. When both engines are activated applyes to engine#1 as well.
        unsigned int eng0_cntr        : 7;
        //
        // Execution Reuse. Reuse Descriptor. After Descriptor was used it is repushed to Exec FIFO. 0: Not pushed. 1: Pushed.
        unsigned int eng0_reuse       : 1;
        //
        // Pointer to the begining address of the descriptor for engine#1. Validated by eng1_valid field.
        unsigned int eng1_pointer     : 11;
        //
        // engine1 valid, activate in parallel both engines.
        unsigned int eng1_valid       : 1;
        //
        // Enable descriptor execution end interrupt.
        unsigned int desc_end_int_enb : 1;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_e0_desc_exec_reg_t;

typedef union {
    // CPRS Engine#1 - Descriptor Execution Register
    // Insert descriptor into the execution FIFO
    struct {
        //
        // Pointer to the begining address of the descriptor for engine#1
        unsigned int eng1_pointer     : 11;
        //
        // Execution Counter for engine#1.
        unsigned int eng1_cntr        : 7;
        //
        // Execution Reuse. Reuse Descriptor. After Descriptor was used it is repushed to Exec FIFO. 0: Not pushed. 1: Pushed.
        unsigned int eng1_reuse       : 1;
        // Just for padding
        unsigned int : 12;
        //
        // Enable descriptor execution end interrupt.
        unsigned int desc_end_int_enb : 1;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_e1_desc_exec_reg_t;

typedef union {
    // Execution Control Graceful Stop Register
    // This register triggers graceful stop.
    struct {
        //
        // Graceful Stop Enable, At end of current descriptor execution engine enters stop state.
        unsigned int en : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_exec_ctrl_gf_stop_reg_t;

typedef union {
    // Execution Control Resume Register
    // This register triggers ewsume after gracefull stop.
    struct {
        //
        // Resume Execution - Pop the next descriptor from the execution FIFO
        unsigned int en : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_exec_ctrl_resume_reg_t;

typedef union {
    // Execution FIFO Ciontrol Register
    // This register enables Execution FIFO flush operation
    struct {
        //
        // Flush
        unsigned int flush : 1;
        // Just for padding
        unsigned int : 31;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_exec_fifo_flush_reg_t;

typedef union {
    // CPRS Descriptor Execution Status Register
    // Staus of CPRS Engine#0 & #1 descriptor execution logic
    struct {
        //
        // Engine#0 Execution Counter
        unsigned int eng0_exec_cntr : 7;
        //
        // Engine#0 Execution FIFO Fill Level
        unsigned int eng0_fifo_fl   : 6;
        // Just for padding
        unsigned int : 3;
        //
        // Engine#1 Execution Counter
        unsigned int eng1_exec_cntr : 7;
        //
        // Engine#1 Execution FIFO Fill Level
        unsigned int eng1_fifo_fl   : 6;
        // Just for padding
        unsigned int : 3;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_desc_status_reg_t;

typedef union {
    // CPRS Descriptor Memory
    // This memory hold CPRS Descriptors
    struct {
        //
        //
        unsigned int dat : 32;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_desc_mem_t;

typedef union {
    // CPRS Functional Events Register
    // This register holds the CPRS functional events
    struct {
        //
        // In ISPB0 SOF
        unsigned int in0_ispb_sof      : 1;
        //
        // In ISPB0 EOF
        unsigned int in0_ispb_eof      : 1;
        //
        // In ISPB1 SOF
        unsigned int in1_ispb_sof      : 1;
        //
        // In ISPB1 EOF
        unsigned int in1_ispb_eof      : 1;
        //
        // Out ISPB0 SOF
        unsigned int out0_ispb_sof     : 1;
        //
        // Out ISPB0 EOF
        unsigned int out0_ispb_eof     : 1;
        //
        // Out ISPB1 SOF
        unsigned int out1_ispb_sof     : 1;
        //
        // Out ISPB1 EOF
        unsigned int out1_ispb_eof     : 1;
        //
        // Engine#0 loaded new descriptor.
        unsigned int e0_new_desc       : 1;
        //
        // Engine#0 completed descriptor execution.
        unsigned int e0_exec_end       : 1;
        //
        // Engine#1 loaded new descriptor.
        unsigned int e1_new_desc       : 1;
        //
        // Engine#1 completed descriptor execution.
        unsigned int e1_exec_end       : 1;
        //
        // Engine#1 Execution FIFO completed sequence. Enabled by execution FIFO field - sequence_end_int_enb
        unsigned int e0_sequence_end   : 1;
        //
        // Engine#1 Execution FIFO completed sequence. Enabled by execution FIFO field - sequence_end_int_enb
        unsigned int e1_sequence_end   : 1;
        //
        // Engine#0 & Engine#1 both ended frame execution, used when both engines are working on the same frame.
        unsigned int frame_end_both    : 1;
        //
        // Engine#0 Graceful stop active.
        unsigned int e0_gracefull_stop : 1;
        //
        // Engine#1 Graceful stop active.
        unsigned int e1_gracefull_stop : 1;
        //
        // Engine#0 Water Mark
        unsigned int e0_wmrk           : 1;
        //
        // Engine#1 Water Mark
        unsigned int e1_wmrk           : 1;
        // Just for padding
        unsigned int : 13;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_func_event_reg_t;

typedef union {
    //
    // This register determines if an interrupt indication is created when an event happens
    struct {
        //
        // When value is 1, occurance of event 'in0_ispb_sof' will trigger an interrupt.
        unsigned int in0_ispb_sof      : 1;
        //
        // When value is 1, occurance of event 'in0_ispb_eof' will trigger an interrupt.
        unsigned int in0_ispb_eof      : 1;
        //
        // When value is 1, occurance of event 'in1_ispb_sof' will trigger an interrupt.
        unsigned int in1_ispb_sof      : 1;
        //
        // When value is 1, occurance of event 'in1_ispb_eof' will trigger an interrupt.
        unsigned int in1_ispb_eof      : 1;
        //
        // When value is 1, occurance of event 'out0_ispb_sof' will trigger an interrupt.
        unsigned int out0_ispb_sof     : 1;
        //
        // When value is 1, occurance of event 'out0_ispb_eof' will trigger an interrupt.
        unsigned int out0_ispb_eof     : 1;
        //
        // When value is 1, occurance of event 'out1_ispb_sof' will trigger an interrupt.
        unsigned int out1_ispb_sof     : 1;
        //
        // When value is 1, occurance of event 'out1_ispb_eof' will trigger an interrupt.
        unsigned int out1_ispb_eof     : 1;
        //
        // When value is 1, occurance of event 'e0_new_desc' will trigger an interrupt.
        unsigned int e0_new_desc       : 1;
        //
        // When value is 1, occurance of event 'e0_exec_end' will trigger an interrupt.
        unsigned int e0_exec_end       : 1;
        //
        // When value is 1, occurance of event 'e1_new_desc' will trigger an interrupt.
        unsigned int e1_new_desc       : 1;
        //
        // When value is 1, occurance of event 'e1_exec_end' will trigger an interrupt.
        unsigned int e1_exec_end       : 1;
        //
        // When value is 1, occurance of event 'e0_sequence_end' will trigger an interrupt.
        unsigned int e0_sequence_end   : 1;
        //
        // When value is 1, occurance of event 'e1_sequence_end' will trigger an interrupt.
        unsigned int e1_sequence_end   : 1;
        //
        // When value is 1, occurance of event 'frame_end_both' will trigger an interrupt.
        unsigned int frame_end_both    : 1;
        //
        // When value is 1, occurance of event 'e0_gracefull_stop' will trigger an interrupt.
        unsigned int e0_gracefull_stop : 1;
        //
        // When value is 1, occurance of event 'e1_gracefull_stop' will trigger an interrupt.
        unsigned int e1_gracefull_stop : 1;
        //
        // When value is 1, occurance of event 'e0_wmrk' will trigger an interrupt.
        unsigned int e0_wmrk           : 1;
        //
        // When value is 1, occurance of event 'e1_wmrk' will trigger an interrupt.
        unsigned int e1_wmrk           : 1;
        // Just for padding
        unsigned int : 13;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_func_event_reg__enable_t;

typedef union {
    //
    //
    struct {
        //
        // Holds the current value (not sticky) of 'in0_ispb_sof'.
        unsigned int in0_ispb_sof      : 1;
        //
        // Holds the current value (not sticky) of 'in0_ispb_eof'.
        unsigned int in0_ispb_eof      : 1;
        //
        // Holds the current value (not sticky) of 'in1_ispb_sof'.
        unsigned int in1_ispb_sof      : 1;
        //
        // Holds the current value (not sticky) of 'in1_ispb_eof'.
        unsigned int in1_ispb_eof      : 1;
        //
        // Holds the current value (not sticky) of 'out0_ispb_sof'.
        unsigned int out0_ispb_sof     : 1;
        //
        // Holds the current value (not sticky) of 'out0_ispb_eof'.
        unsigned int out0_ispb_eof     : 1;
        //
        // Holds the current value (not sticky) of 'out1_ispb_sof'.
        unsigned int out1_ispb_sof     : 1;
        //
        // Holds the current value (not sticky) of 'out1_ispb_eof'.
        unsigned int out1_ispb_eof     : 1;
        //
        // Holds the current value (not sticky) of 'e0_new_desc'.
        unsigned int e0_new_desc       : 1;
        //
        // Holds the current value (not sticky) of 'e0_exec_end'.
        unsigned int e0_exec_end       : 1;
        //
        // Holds the current value (not sticky) of 'e1_new_desc'.
        unsigned int e1_new_desc       : 1;
        //
        // Holds the current value (not sticky) of 'e1_exec_end'.
        unsigned int e1_exec_end       : 1;
        //
        // Holds the current value (not sticky) of 'e0_sequence_end'.
        unsigned int e0_sequence_end   : 1;
        //
        // Holds the current value (not sticky) of 'e1_sequence_end'.
        unsigned int e1_sequence_end   : 1;
        //
        // Holds the current value (not sticky) of 'frame_end_both'.
        unsigned int frame_end_both    : 1;
        //
        // Holds the current value (not sticky) of 'e0_gracefull_stop'.
        unsigned int e0_gracefull_stop : 1;
        //
        // Holds the current value (not sticky) of 'e1_gracefull_stop'.
        unsigned int e1_gracefull_stop : 1;
        //
        // Holds the current value (not sticky) of 'e0_wmrk'.
        unsigned int e0_wmrk           : 1;
        //
        // Holds the current value (not sticky) of 'e1_wmrk'.
        unsigned int e1_wmrk           : 1;
        // Just for padding
        unsigned int : 13;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_func_event_reg__status_t;

typedef union {
    //
    //
    struct {
        //
        // in0_ispb_sof mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int in0_ispb_sof      : 1;
        //
        // in0_ispb_eof mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int in0_ispb_eof      : 1;
        //
        // in1_ispb_sof mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int in1_ispb_sof      : 1;
        //
        // in1_ispb_eof mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int in1_ispb_eof      : 1;
        //
        // out0_ispb_sof mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int out0_ispb_sof     : 1;
        //
        // out0_ispb_eof mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int out0_ispb_eof     : 1;
        //
        // out1_ispb_sof mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int out1_ispb_sof     : 1;
        //
        // out1_ispb_eof mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int out1_ispb_eof     : 1;
        //
        // e0_new_desc mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e0_new_desc       : 1;
        //
        // e0_exec_end mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e0_exec_end       : 1;
        //
        // e1_new_desc mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e1_new_desc       : 1;
        //
        // e1_exec_end mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e1_exec_end       : 1;
        //
        // e0_sequence_end mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e0_sequence_end   : 1;
        //
        // e1_sequence_end mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e1_sequence_end   : 1;
        //
        // frame_end_both mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int frame_end_both    : 1;
        //
        // e0_gracefull_stop mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e0_gracefull_stop : 1;
        //
        // e1_gracefull_stop mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e1_gracefull_stop : 1;
        //
        // e0_wmrk mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e0_wmrk           : 1;
        //
        // e1_wmrk mux select. 0 - status is selected; 1 - interrupt is selected
        unsigned int e1_wmrk           : 1;
        // Just for padding
        unsigned int : 13;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_func_event_reg__mux_select_t;

typedef union {
    // CPRS Error Interrupt Register
    // This register holds the CPRS error interrupts
    struct {
        //
        // Padding buffer#0 parity error.
        unsigned int padd_buf_par_int0          : 1;
        //
        // Padding buffer#1 parity error.
        unsigned int padd_buf_par_int1          : 1;
        //
        // Engine#0 Descriptor ECC single error
        unsigned int e0_desc_ecc_single_err     : 1;
        //
        // Engine#0 Descriptor ECC double error
        unsigned int e0_desc_ecc_double_err     : 1;
        //
        // Engine#1 Descriptor ECC single error
        unsigned int e1_desc_ecc_single_err     : 1;
        //
        // Engine#1 Descriptor ECC double error
        unsigned int e1_desc_ecc_double_err     : 1;
        //
        // Engine#0 Error
        unsigned int engine_err0                : 1;
        //
        // Engine#1 Error
        unsigned int engine_err1                : 1;
        //
        // Internal CDP contention, Both Engines are configured to source CDP.
        unsigned int internal_cdp_cntention_err : 1;
        //
        // External CDP contention, Both dcprs_mp configured to source CDP.
        unsigned int external_cdp_cntention_err : 1;
        // Just for padding
        unsigned int : 22;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_err_interrupt_reg_t;

typedef union {
    //
    // This register determines if an interrupt indication is created when an event happens
    struct {
        //
        // When value is 1, occurance of event 'padd_buf_par_int0' will trigger an interrupt.
        unsigned int padd_buf_par_int0          : 1;
        //
        // When value is 1, occurance of event 'padd_buf_par_int1' will trigger an interrupt.
        unsigned int padd_buf_par_int1          : 1;
        //
        // When value is 1, occurance of event 'e0_desc_ecc_single_err' will trigger an interrupt.
        unsigned int e0_desc_ecc_single_err     : 1;
        //
        // When value is 1, occurance of event 'e0_desc_ecc_double_err' will trigger an interrupt.
        unsigned int e0_desc_ecc_double_err     : 1;
        //
        // When value is 1, occurance of event 'e1_desc_ecc_single_err' will trigger an interrupt.
        unsigned int e1_desc_ecc_single_err     : 1;
        //
        // When value is 1, occurance of event 'e1_desc_ecc_double_err' will trigger an interrupt.
        unsigned int e1_desc_ecc_double_err     : 1;
        //
        // When value is 1, occurance of event 'engine_err0' will trigger an interrupt.
        unsigned int engine_err0                : 1;
        //
        // When value is 1, occurance of event 'engine_err1' will trigger an interrupt.
        unsigned int engine_err1                : 1;
        //
        // When value is 1, occurance of event 'internal_cdp_cntention_err' will trigger an interrupt.
        unsigned int internal_cdp_cntention_err : 1;
        //
        // When value is 1, occurance of event 'external_cdp_cntention_err' will trigger an interrupt.
        unsigned int external_cdp_cntention_err : 1;
        // Just for padding
        unsigned int : 22;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_err_interrupt_reg__enable_t;

typedef union {
    //
    //
    struct {
        //
        // Holds the current value (not sticky) of 'padd_buf_par_int0'.
        unsigned int padd_buf_par_int0          : 1;
        //
        // Holds the current value (not sticky) of 'padd_buf_par_int1'.
        unsigned int padd_buf_par_int1          : 1;
        //
        // Holds the current value (not sticky) of 'e0_desc_ecc_single_err'.
        unsigned int e0_desc_ecc_single_err     : 1;
        //
        // Holds the current value (not sticky) of 'e0_desc_ecc_double_err'.
        unsigned int e0_desc_ecc_double_err     : 1;
        //
        // Holds the current value (not sticky) of 'e1_desc_ecc_single_err'.
        unsigned int e1_desc_ecc_single_err     : 1;
        //
        // Holds the current value (not sticky) of 'e1_desc_ecc_double_err'.
        unsigned int e1_desc_ecc_double_err     : 1;
        //
        // Holds the current value (not sticky) of 'engine_err0'.
        unsigned int engine_err0                : 1;
        //
        // Holds the current value (not sticky) of 'engine_err1'.
        unsigned int engine_err1                : 1;
        //
        // Holds the current value (not sticky) of 'internal_cdp_cntention_err'.
        unsigned int internal_cdp_cntention_err : 1;
        //
        // Holds the current value (not sticky) of 'external_cdp_cntention_err'.
        unsigned int external_cdp_cntention_err : 1;
        // Just for padding
        unsigned int : 22;
    } bitfield;
    unsigned int register_value;
} dcprs_mp_rgf_dcprs_err_interrupt_reg__status_t;

typedef struct {
    //
    // Power Control Register
    // This register controls the CPRS Power options.
    // Offset - 0x0
    dcprs_mp_rgf_power_control_reg_t                power_control_reg;

    //
    // MISC Clock Enable Register
    // This register controls CPRS blocks clock enable.
    //         A set field in this register overrides the descriptor enable configuration
    // Offset - 0x4
    dcprs_mp_rgf_dcprs_clk_en_reg_t                 dcprs_clk_en_reg;

    //
    // Bayer Collor Order
    // There are four options for storing a bayer frame. This register indicates idf the first pixel is green or Blue/Red.
    // Offset - 0x8
    dcprs_mp_rgf_bayer_green_first_reg_t            bayer_green_first_reg;

    //
    // AXI Write FIFO Back Pressure Threshold
    // Back pressure assertion level.
    // Offset - 0xc
    dcprs_mp_rgf_axi_wr_fifo_bp_th_t                axi_wr0_fifo_bp_th;

    //
    // AXI Write FIFO Back Pressure Threshold
    // Back pressure assertion level.
    // Offset - 0x10
    dcprs_mp_rgf_axi_wr_fifo_bp_th_t                axi_wr1_fifo_bp_th;

    //
    // DCPRS AXI Read Frame Section Size
    // Number of bytes added to memory for this channel.
    // Offset - 0x14
    dcprs_mp_rgf_frm_sec_size_reg_t                 engine0_frm_sec_size_reg;

    //
    // DCPRS AXI Read Frame Section Size
    // Number of bytes added to memory for this channel.
    // Offset - 0x18
    dcprs_mp_rgf_frm_sec_size_reg_t                 engine1_frm_sec_size_reg;

    //
    // DCPRS AXI Read Frame Section Enable
    // Enable Frame Section mode.
    // Offset - 0x1c
    dcprs_mp_rgf_frm_sec_enb_reg_t                  engine0_frm_sec_enb_reg;

    //
    // DCPRS AXI Read Frame Section Enable
    // Enable Frame Section mode.
    // Offset - 0x20
    dcprs_mp_rgf_frm_sec_enb_reg_t                  engine1_frm_sec_enb_reg;

    //
    // DCPRS AXI Read Frame Section Force EOF
    // Force EOF after last frame section was written.
    // Offset - 0x24
    dcprs_mp_rgf_frm_sec_force_eof_reg_t            engine0_frm_sec_force_eof_reg;

    //
    // DCPRS AXI Read Frame Section Force EOF
    // Force EOF after last frame section was written.
    // Offset - 0x28
    dcprs_mp_rgf_frm_sec_force_eof_reg_t            engine1_frm_sec_force_eof_reg;

    //
    // ISPB input data byte switch Enable
    // Switch input that sources decoder:0: dat_out=dat_in. 1: dat_out = {dat_in[7:0],dat_in[15:8],dat_in[23:16],dat_in[31:24]}
    // Offset - 0x2c
    dcprs_mp_rgf_byte_switch_reg_t                  byte_switch_reg;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x30
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine0_desc_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x34
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine1_desc_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x38
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine0_pad_even_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x3c
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine0_pad_odd_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x40
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine1_pad_even_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x44
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine1_pad_odd_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x48
    dcprs_mp_rgf_mem_power_cntl_reg_t               axi_rd_sync0_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x4c
    dcprs_mp_rgf_mem_power_cntl_reg_t               axi_rd_sync1_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x50
    dcprs_mp_rgf_mem_power_cntl_reg_t               axi_wr_sync0_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x54
    dcprs_mp_rgf_mem_power_cntl_reg_t               axi_wr_sync1_mem_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x58
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine0_abcnk;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x5c
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine0_lbf0;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x60
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine0_lbf1;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x64
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine1_abcnk;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x68
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine1_lbf0;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x6c
    dcprs_mp_rgf_mem_power_cntl_reg_t               engine1_lbf1;

    //
    // CPRS Engine#0 - Descriptor Execution Register
    // Insert descriptor into the execution FIFO
    // Offset - 0x70
    dcprs_mp_rgf_dcprs_e0_desc_exec_reg_t           dcprs_e0_desc_exec_reg;

    //
    // CPRS Engine#1 - Descriptor Execution Register
    // Insert descriptor into the execution FIFO
    // Offset - 0x74
    dcprs_mp_rgf_dcprs_e1_desc_exec_reg_t           dcprs_e1_desc_exec_reg;

    //
    // Execution Control Graceful Stop Register
    // This register triggers graceful stop.
    // Offset - 0x78
    dcprs_mp_rgf_exec_ctrl_gf_stop_reg_t            eng0_exec_ctrl_gf_stop_reg;

    //
    // Execution Control Graceful Stop Register
    // This register triggers graceful stop.
    // Offset - 0x7c
    dcprs_mp_rgf_exec_ctrl_gf_stop_reg_t            eng1_exec_ctrl_gf_stop_reg;

    //
    // Execution Control Resume Register
    // This register triggers ewsume after gracefull stop.
    // Offset - 0x80
    dcprs_mp_rgf_exec_ctrl_resume_reg_t             eng0_exec_ctrl_resume_reg;

    //
    // Execution Control Resume Register
    // This register triggers ewsume after gracefull stop.
    // Offset - 0x84
    dcprs_mp_rgf_exec_ctrl_resume_reg_t             eng1_exec_ctrl_resume_reg;

    //
    // Execution FIFO Ciontrol Register
    // This register enables Execution FIFO flush operation
    // Offset - 0x88
    dcprs_mp_rgf_exec_fifo_flush_reg_t              eng0_exec_fifo_flush_reg;

    //
    // Execution FIFO Ciontrol Register
    // This register enables Execution FIFO flush operation
    // Offset - 0x8c
    dcprs_mp_rgf_exec_fifo_flush_reg_t              eng1_exec_fifo_flush_reg;

    //
    // CPRS Descriptor Execution Status Register
    // Staus of CPRS Engine#0 & #1 descriptor execution logic
    // Offset - 0x90
    dcprs_mp_rgf_dcprs_desc_status_reg_t            dcprs_desc_status_reg;

    //
    // CPRS Descriptor Memory
    // This memory hold CPRS Descriptors
    // Offset - 0x94
    dcprs_mp_rgf_dcprs_desc_mem_t                   dcprs_eng0_desc_mem[448];

    //
    // CPRS Descriptor Memory
    // This memory hold CPRS Descriptors
    // Offset - 0x794
    dcprs_mp_rgf_dcprs_desc_mem_t                   dcprs_eng1_desc_mem[448];

    //
    // CPRS Functional Events Register
    // This register holds the CPRS functional events
    // Offset - 0xe94
    dcprs_mp_rgf_dcprs_func_event_reg_t             dcprs_func_event_reg;

    //
    //
    // This register determines if an interrupt indication is created when an event happens
    // Offset - 0xe98
    dcprs_mp_rgf_dcprs_func_event_reg__enable_t     dcprs_func_event_reg_int_en;

    //
    //
    // This register determines if a freeze indication is created when an event happens
    // Offset - 0xe9c
    dcprs_mp_rgf_dcprs_func_event_reg__enable_t     dcprs_func_event_reg_frz_en;

    //
    //
    // This register determines if a halt indication is created when an event happens
    // Offset - 0xea0
    dcprs_mp_rgf_dcprs_func_event_reg__enable_t     dcprs_func_event_reg_hlt_en;

    //
    //
    //
    // Offset - 0xea4
    dcprs_mp_rgf_dcprs_func_event_reg__status_t     dcprs_func_event_reg_status;

    //
    //
    //
    // Offset - 0xea8
    dcprs_mp_rgf_dcprs_func_event_reg__mux_select_t dcprs_func_event_reg_mux_select;

    //
    // CPRS Error Interrupt Register
    // This register holds the CPRS error interrupts
    // Offset - 0xeac
    dcprs_mp_rgf_dcprs_err_interrupt_reg_t          dcprs_err_interrupt_reg;

    //
    //
    // This register determines if an interrupt indication is created when an event happens
    // Offset - 0xeb0
    dcprs_mp_rgf_dcprs_err_interrupt_reg__enable_t  dcprs_err_interrupt_reg_int_en;

    //
    //
    // This register determines if a freeze indication is created when an event happens
    // Offset - 0xeb4
    dcprs_mp_rgf_dcprs_err_interrupt_reg__enable_t  dcprs_err_interrupt_reg_frz_en;

    //
    //
    // This register determines if a halt indication is created when an event happens
    // Offset - 0xeb8
    dcprs_mp_rgf_dcprs_err_interrupt_reg__enable_t  dcprs_err_interrupt_reg_hlt_en;

    //
    //
    //
    // Offset - 0xebc
    dcprs_mp_rgf_dcprs_err_interrupt_reg__status_t  dcprs_err_status_reg;

} dcprs_mp_rgf_t;

typedef struct {
    //
    //
    //
    // Offset - 0x0
    dcprs_jpeglsdec_rgf_t eng0;

    //
    //
    //
    // Offset - 0x2000
    dcprs_jpeglsdec_rgf_t eng1;

    //
    //
    //
    // Offset - 0x4000
    dcprs_mp_rgf_t        dcprs_mp;

} dcprs_mp_rmap_t;

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
} dcprs_rgf_mem_power_cntl0_reg_t;

typedef union {
    // Rx-ISPP - AXI_WR Backpressure Threshold Register
    // This register sets the value of the backpressure from the AXI_WR.
    struct {
        //
        // Backpressure Threshold
        unsigned int th : 13;
        // Just for padding
        unsigned int : 19;
    } bitfield;
    unsigned int register_value;
} dcprs_rgf_dcprs_axiwr_bp_reg_t;

typedef union {
    // AXI_WR Burst Size Register
    // This register controls the AXI_WR burst size. NOTE: AXI Master regsiter needs to be configured as well.
    struct {
        //
        // Burst Size
        unsigned int size : 9;
        // Just for padding
        unsigned int : 23;
    } bitfield;
    unsigned int register_value;
} dcprs_rgf_dcprs_axiwr_burst_reg_t;

typedef union {
    // AXI_RD Burst Size Register
    // This register controls the AXI_WR burst size. NOTE: AXI Master regsiter needs to be configured as well.
    struct {
        //
        // Burst Size
        unsigned int size : 9;
        // Just for padding
        unsigned int : 23;
    } bitfield;
    unsigned int register_value;
} dcprs_rgf_dcprs_axird_burst_reg_t;

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
} dcprs_rgf_watermark_cur_cntr_reg_t;

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
} dcprs_rgf_watermark_last_cntr_reg_t;

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
} dcprs_rgf_watermark_threshold_reg_t;

typedef union {
    // AXI Write Status Register
    // This register holds the AXI Write status
    struct {
        //
        // Engine 0 - Channel Empty
        unsigned int m0e0_empty : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int m0e1_empty : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int m1e0_empty : 1;
        //
        // Engine 0 - Channel Empty
        unsigned int m1e1_empty : 1;
        // Just for padding
        unsigned int : 28;
    } bitfield;
    unsigned int register_value;
} dcprs_rgf_axi_wr_status_reg_t;

typedef union {
    // DCPRS Top Interrupt Register
    // This register holds the DCPRS top interrupts
    struct {
        //
        // DCPRS_MP 0 Engine 0 AXI_RD Channel Parity Error
        unsigned int mp0_eng0_rd_par_err_int : 1;
        //
        // DCPRS_MP 0 Engine 1 AXI_RD Channel Parity Error
        unsigned int mp0_eng1_rd_par_err_int : 1;
        //
        // DCPRS_MP 1 Engine 0 AXI_RD Channel Parity Error
        unsigned int mp1_eng0_rd_par_err_int : 1;
        //
        // DCPRS_MP 1 Engine 1 AXI_RD Channel Parity Error
        unsigned int mp1_eng1_rd_par_err_int : 1;
        //
        // DCPRS_MP 0 Engine 0 AXI_WR Channel Parity Error
        unsigned int mp0_eng0_wr_par_err_int : 1;
        //
        // DCPRS_MP 0 Engine 1 AXI_WR Channel Parity Error
        unsigned int mp0_eng1_wr_par_err_int : 1;
        //
        // DCPRS_MP 1 Engine 0 AXI_WR Channel Parity Error
        unsigned int mp1_eng0_wr_par_err_int : 1;
        //
        // DCPRS_MP 1 Engine 1 AXI_WR Channel Parity Error
        unsigned int mp1_eng1_wr_par_err_int : 1;
        //
        // DCPRS_MP 0 Engine 0 AXI_RD Channel Overflow
        unsigned int mp0_eng0_rd_ovf_int     : 1;
        //
        // DCPRS_MP 0 Engine 1 AXI_RD Channel Overflow
        unsigned int mp0_eng1_rd_ovf_int     : 1;
        //
        // DCPRS_MP 1 Engine 0 AXI_RD Channel Overflow
        unsigned int mp1_eng0_rd_ovf_int     : 1;
        //
        // DCPRS_MP 1 Engine 1 AXI_RD Channel Overflow
        unsigned int mp1_eng1_rd_ovf_int     : 1;
        //
        // DCPRS_MP 0 Engine 0 AXI_WR Channel Overflow
        unsigned int mp0_eng0_wr_ovf_int     : 1;
        //
        // DCPRS_MP 0 Engine 1 AXI_WR Channel Overflow
        unsigned int mp0_eng1_wr_ovf_int     : 1;
        //
        // DCPRS_MP 1 Engine 0 AXI_WR Channel Overflow
        unsigned int mp1_eng0_wr_ovf_int     : 1;
        //
        // DCPRS_MP 1 Engine 1 AXI_WR Channel Overflow
        unsigned int mp1_eng1_wr_ovf_int     : 1;
        //
        // DCPRS_MP 0 Engine 0 Water Mark
        unsigned int m0e0_wmrk               : 1;
        //
        // DCPRS_MP 0 Engine 1 Water Mark
        unsigned int m0e1_wmrk               : 1;
        //
        // DCPRS_MP 1 Engine 0 Water Mark
        unsigned int m1e0_wmrk               : 1;
        //
        // DCPRS_MP 1 Engine 0 Water Mark
        unsigned int m1e1_wmrk               : 1;
        // Just for padding
        unsigned int : 12;
    } bitfield;
    unsigned int register_value;
} dcprs_rgf_dcprs_axi_err_interrupt_reg_t;

typedef union {
    //
    // This register determines if an interrupt indication is created when an event happens
    struct {
        //
        // When value is 1, occurance of event 'mp0_eng0_rd_par_err_int' will trigger an interrupt.
        unsigned int mp0_eng0_rd_par_err_int : 1;
        //
        // When value is 1, occurance of event 'mp0_eng1_rd_par_err_int' will trigger an interrupt.
        unsigned int mp0_eng1_rd_par_err_int : 1;
        //
        // When value is 1, occurance of event 'mp1_eng0_rd_par_err_int' will trigger an interrupt.
        unsigned int mp1_eng0_rd_par_err_int : 1;
        //
        // When value is 1, occurance of event 'mp1_eng1_rd_par_err_int' will trigger an interrupt.
        unsigned int mp1_eng1_rd_par_err_int : 1;
        //
        // When value is 1, occurance of event 'mp0_eng0_wr_par_err_int' will trigger an interrupt.
        unsigned int mp0_eng0_wr_par_err_int : 1;
        //
        // When value is 1, occurance of event 'mp0_eng1_wr_par_err_int' will trigger an interrupt.
        unsigned int mp0_eng1_wr_par_err_int : 1;
        //
        // When value is 1, occurance of event 'mp1_eng0_wr_par_err_int' will trigger an interrupt.
        unsigned int mp1_eng0_wr_par_err_int : 1;
        //
        // When value is 1, occurance of event 'mp1_eng1_wr_par_err_int' will trigger an interrupt.
        unsigned int mp1_eng1_wr_par_err_int : 1;
        //
        // When value is 1, occurance of event 'mp0_eng0_rd_ovf_int' will trigger an interrupt.
        unsigned int mp0_eng0_rd_ovf_int     : 1;
        //
        // When value is 1, occurance of event 'mp0_eng1_rd_ovf_int' will trigger an interrupt.
        unsigned int mp0_eng1_rd_ovf_int     : 1;
        //
        // When value is 1, occurance of event 'mp1_eng0_rd_ovf_int' will trigger an interrupt.
        unsigned int mp1_eng0_rd_ovf_int     : 1;
        //
        // When value is 1, occurance of event 'mp1_eng1_rd_ovf_int' will trigger an interrupt.
        unsigned int mp1_eng1_rd_ovf_int     : 1;
        //
        // When value is 1, occurance of event 'mp0_eng0_wr_ovf_int' will trigger an interrupt.
        unsigned int mp0_eng0_wr_ovf_int     : 1;
        //
        // When value is 1, occurance of event 'mp0_eng1_wr_ovf_int' will trigger an interrupt.
        unsigned int mp0_eng1_wr_ovf_int     : 1;
        //
        // When value is 1, occurance of event 'mp1_eng0_wr_ovf_int' will trigger an interrupt.
        unsigned int mp1_eng0_wr_ovf_int     : 1;
        //
        // When value is 1, occurance of event 'mp1_eng1_wr_ovf_int' will trigger an interrupt.
        unsigned int mp1_eng1_wr_ovf_int     : 1;
        //
        // When value is 1, occurance of event 'm0e0_wmrk' will trigger an interrupt.
        unsigned int m0e0_wmrk               : 1;
        //
        // When value is 1, occurance of event 'm0e1_wmrk' will trigger an interrupt.
        unsigned int m0e1_wmrk               : 1;
        //
        // When value is 1, occurance of event 'm1e0_wmrk' will trigger an interrupt.
        unsigned int m1e0_wmrk               : 1;
        //
        // When value is 1, occurance of event 'm1e1_wmrk' will trigger an interrupt.
        unsigned int m1e1_wmrk               : 1;
        // Just for padding
        unsigned int : 12;
    } bitfield;
    unsigned int register_value;
} dcprs_rgf_dcprs_axi_err_interrupt_reg__enable_t;

typedef union {
    //
    //
    struct {
        //
        // Holds the current value (not sticky) of 'mp0_eng0_rd_par_err_int'.
        unsigned int mp0_eng0_rd_par_err_int : 1;
        //
        // Holds the current value (not sticky) of 'mp0_eng1_rd_par_err_int'.
        unsigned int mp0_eng1_rd_par_err_int : 1;
        //
        // Holds the current value (not sticky) of 'mp1_eng0_rd_par_err_int'.
        unsigned int mp1_eng0_rd_par_err_int : 1;
        //
        // Holds the current value (not sticky) of 'mp1_eng1_rd_par_err_int'.
        unsigned int mp1_eng1_rd_par_err_int : 1;
        //
        // Holds the current value (not sticky) of 'mp0_eng0_wr_par_err_int'.
        unsigned int mp0_eng0_wr_par_err_int : 1;
        //
        // Holds the current value (not sticky) of 'mp0_eng1_wr_par_err_int'.
        unsigned int mp0_eng1_wr_par_err_int : 1;
        //
        // Holds the current value (not sticky) of 'mp1_eng0_wr_par_err_int'.
        unsigned int mp1_eng0_wr_par_err_int : 1;
        //
        // Holds the current value (not sticky) of 'mp1_eng1_wr_par_err_int'.
        unsigned int mp1_eng1_wr_par_err_int : 1;
        //
        // Holds the current value (not sticky) of 'mp0_eng0_rd_ovf_int'.
        unsigned int mp0_eng0_rd_ovf_int     : 1;
        //
        // Holds the current value (not sticky) of 'mp0_eng1_rd_ovf_int'.
        unsigned int mp0_eng1_rd_ovf_int     : 1;
        //
        // Holds the current value (not sticky) of 'mp1_eng0_rd_ovf_int'.
        unsigned int mp1_eng0_rd_ovf_int     : 1;
        //
        // Holds the current value (not sticky) of 'mp1_eng1_rd_ovf_int'.
        unsigned int mp1_eng1_rd_ovf_int     : 1;
        //
        // Holds the current value (not sticky) of 'mp0_eng0_wr_ovf_int'.
        unsigned int mp0_eng0_wr_ovf_int     : 1;
        //
        // Holds the current value (not sticky) of 'mp0_eng1_wr_ovf_int'.
        unsigned int mp0_eng1_wr_ovf_int     : 1;
        //
        // Holds the current value (not sticky) of 'mp1_eng0_wr_ovf_int'.
        unsigned int mp1_eng0_wr_ovf_int     : 1;
        //
        // Holds the current value (not sticky) of 'mp1_eng1_wr_ovf_int'.
        unsigned int mp1_eng1_wr_ovf_int     : 1;
        //
        // Holds the current value (not sticky) of 'm0e0_wmrk'.
        unsigned int m0e0_wmrk               : 1;
        //
        // Holds the current value (not sticky) of 'm0e1_wmrk'.
        unsigned int m0e1_wmrk               : 1;
        //
        // Holds the current value (not sticky) of 'm1e0_wmrk'.
        unsigned int m1e0_wmrk               : 1;
        //
        // Holds the current value (not sticky) of 'm1e1_wmrk'.
        unsigned int m1e1_wmrk               : 1;
        // Just for padding
        unsigned int : 12;
    } bitfield;
    unsigned int register_value;
} dcprs_rgf_dcprs_axi_err_interrupt_reg__status_t;

typedef struct {
    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x0
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_rd_ch0_mem0_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x4
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_rd_ch0_mem1_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x8
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_rd_ch1_mem0_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0xc
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_rd_ch1_mem1_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x10
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_rd_ch2_mem0_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x14
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_rd_ch2_mem1_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x18
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_rd_ch3_mem0_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x1c
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_rd_ch3_mem1_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x20
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_wr_mem0_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x24
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_wr_mem1_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x28
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_wr_mem2_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x2c
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_wr_mem3_cntl;

    //
    // Memory Power Control Register.
    // This register used for memory power control.
    // Offset - 0x30
    dcprs_rgf_mem_power_cntl0_reg_t                 axi_wr_mem4_cntl;

    //
    // Rx-ISPP - AXI_WR Backpressure Threshold Register
    // This register sets the value of the backpressure from the AXI_WR.
    // Offset - 0x34
    dcprs_rgf_dcprs_axiwr_bp_reg_t                  dcprs_axiwr_bp_reg;

    //
    // AXI_WR Burst Size Register
    // This register controls the AXI_WR burst size. NOTE: AXI Master regsiter needs to be configured as well.
    // Offset - 0x38
    dcprs_rgf_dcprs_axiwr_burst_reg_t               dcprs_axiwr_burst_reg;

    //
    // AXI_RD Burst Size Register
    // This register controls the AXI_WR burst size. NOTE: AXI Master regsiter needs to be configured as well.
    // Offset - 0x3c
    dcprs_rgf_dcprs_axird_burst_reg_t               dcprs_axird_burst_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x40
    dcprs_rgf_watermark_cur_cntr_reg_t              m0e0_watermark_cur_cntr_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x44
    dcprs_rgf_watermark_cur_cntr_reg_t              m0e1_watermark_cur_cntr_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x48
    dcprs_rgf_watermark_cur_cntr_reg_t              m1e0_watermark_cur_cntr_reg;

    //
    // Watermark Current Counter Register
    // This register holds the current watermark counter
    // Offset - 0x4c
    dcprs_rgf_watermark_cur_cntr_reg_t              m1e1_watermark_cur_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x50
    dcprs_rgf_watermark_last_cntr_reg_t             m0e0_watermark_last_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x54
    dcprs_rgf_watermark_last_cntr_reg_t             m0e1_watermark_last_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x58
    dcprs_rgf_watermark_last_cntr_reg_t             m1e0_watermark_last_cntr_reg;

    //
    // Watermark Last Counter Register
    // This register holds the last watermark counter value.
    //         It is update on watermark event, when the current watermark counter reach the threshold
    // Offset - 0x5c
    dcprs_rgf_watermark_last_cntr_reg_t             m1e1_watermark_last_cntr_reg;

    //
    // Watermark Threshold Register
    // This register controls the Watermark Threshold.
    // Offset - 0x60
    dcprs_rgf_watermark_threshold_reg_t             m0e0_watermark_threshold_reg;

    //
    // Watermark Threshold Register
    // This register controls the Watermark Threshold.
    // Offset - 0x64
    dcprs_rgf_watermark_threshold_reg_t             m0e1_watermark_threshold_reg;

    //
    // Watermark Threshold Register
    // This register controls the Watermark Threshold.
    // Offset - 0x68
    dcprs_rgf_watermark_threshold_reg_t             m1e0_watermark_threshold_reg;

    //
    // Watermark Threshold Register
    // This register controls the Watermark Threshold.
    // Offset - 0x6c
    dcprs_rgf_watermark_threshold_reg_t             m1e1_watermark_threshold_reg;

    //
    // AXI Write Status Register
    // This register holds the AXI Write status
    // Offset - 0x70
    dcprs_rgf_axi_wr_status_reg_t                   axi_wr_status_reg;

    //
    // DCPRS Top Interrupt Register
    // This register holds the DCPRS top interrupts
    // Offset - 0x74
    dcprs_rgf_dcprs_axi_err_interrupt_reg_t         dcprs_axi_err_interrupt_reg;

    //
    //
    // This register determines if an interrupt indication is created when an event happens
    // Offset - 0x78
    dcprs_rgf_dcprs_axi_err_interrupt_reg__enable_t dcprs_axi_err_interrupt_reg_int_en;

    //
    //
    // This register determines if a freeze indication is created when an event happens
    // Offset - 0x7c
    dcprs_rgf_dcprs_axi_err_interrupt_reg__enable_t dcprs_axi_err_interrupt_reg_frz_en;

    //
    //
    // This register determines if a halt indication is created when an event happens
    // Offset - 0x80
    dcprs_rgf_dcprs_axi_err_interrupt_reg__enable_t dcprs_axi_err_interrupt_reg_hlt_en;

    //
    //
    //
    // Offset - 0x84
    dcprs_rgf_dcprs_axi_err_interrupt_reg__status_t dcprs_axi_err_status_reg;

} dcprs_rgf_t;

typedef struct {
    //
    //
    //
    // Offset - 0x0
    dcprs_mp_rmap_t dcprs_mp0;

    //
    // Dummy filler
    unsigned char RESERVED0[0x8000 - 0x4ec0];

    //
    //
    //
    // Offset - 0x8000
    dcprs_mp_rmap_t dcprs_mp1;

    //
    // Dummy filler
    unsigned char RESERVED1[0x10000 - 0xcec0];

    //
    //
    //
    // Offset - 0x10000
    dcprs_rgf_t     rgf;

} dcprs_slow_rmap_t;

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
} master128apb_rd_bytes_reg_t;

typedef union {
    // master128 interface0, read start address register
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} master128apb_rd_addr_lcl_reg_t;

typedef union {
    // master128 write interface0 start address register
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} master128apb_rd_addr_rmt_reg_t;

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
} master128apb_rd_control_reg_t;

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
} master128apb_rd_activate_reg_t;

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
} master128apb_wr_bytes_reg_t;

typedef union {
    // master128 interface0, read start address register
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} master128apb_wr_addr_lcl_reg_t;

typedef union {
    // master128 write interface0 start address register
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} master128apb_wr_addr_rmt_reg_t;

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
} master128apb_wr_control_reg_t;

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
} master128apb_wr_activate_reg_t;

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
} master128apb_max_burst_len_reg_t;

typedef union {
    // master128 status register, readonly, mirrors key variables of master
    //
    struct {
        //
        //
        unsigned int nn : 32;
    } bitfield;
    unsigned int register_value;
} master128apb_status_reg_t;

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
} master128apb_config_reg_t;

typedef struct {
    //
    // master128 interface0, amount of bytes to copy
    //
    // Offset - 0x0
    master128apb_rd_bytes_reg_t      rd_bytes;

    //
    // master128 interface0, read start address register
    //
    // Offset - 0x4
    master128apb_rd_addr_lcl_reg_t   rd_addr_lcl;

    //
    // master128 write interface0 start address register
    //
    // Offset - 0x8
    master128apb_rd_addr_rmt_reg_t   rd_addr_rmt;

    //
    // master128, interface0 misc controls
    //
    // Offset - 0xc
    master128apb_rd_control_reg_t    rd_control;

    //
    // writing to this reg, fires the master
    //
    // Offset - 0x10
    master128apb_rd_activate_reg_t   rd_activate;

    //
    // master128 interface0, amount of bytes to copy
    //
    // Offset - 0x14
    master128apb_wr_bytes_reg_t      wr_bytes;

    //
    // master128 interface0, read start address register
    //
    // Offset - 0x18
    master128apb_wr_addr_lcl_reg_t   wr_addr_lcl;

    //
    // master128 write interface0 start address register
    //
    // Offset - 0x1c
    master128apb_wr_addr_rmt_reg_t   wr_addr_rmt;

    //
    // master128, interface0 misc controls
    //
    // Offset - 0x20
    master128apb_wr_control_reg_t    wr_control;

    //
    // writing to this reg, fires the master
    //
    // Offset - 0x24
    master128apb_wr_activate_reg_t   wr_activate;

    //
    // master128 interface0, max len of burst
    //
    // Offset - 0x28
    master128apb_max_burst_len_reg_t max_burst_len_rd;

    //
    // master128 interface0, max len of burst
    //
    // Offset - 0x2c
    master128apb_max_burst_len_reg_t max_burst_len_wr;

    //
    // master128 status register, readonly, mirrors key variables of master
    //
    // Offset - 0x30
    master128apb_status_reg_t        status;

    //
    // master128 configuration bits
    //
    // Offset - 0x34
    master128apb_config_reg_t        configx;

} master128apb_t;

typedef struct {
    //
    //
    //
    // Offset - 0x0
    dcprs_slow_rmap_t dcprs_mptop;

    //
    // Dummy filler
    unsigned char RESERVED0[0x11000 - 0x10088];

    //
    //
    //
    // Offset - 0x11000
    master128apb_t    axim;

} dcprs_t;

extern volatile dcprs_t *dcprs;


// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "dcprs.h"
#include "dcprs_lib.h"
#include "dcprs_config.h"
#include "dcprs_top.h"
#include "dcprs_desc.h"
#include "cast.h"

#define DPRINT  if (g_debug) pr_info

#define  DCPRS_DEFAULT_MP_INDEX          (0)
#define  DCPRS_DEFAULT_DESCRIPTOR_INDEX  (0)


volatile dcprs_t *dcprs;

extern uint32_t g_debug;



/********************
 * HELPER FUNCTIONS *
 ********************/
static volatile dcprs_mp_rmap_t *dcprs_get_default_mp(void)
{
    return (DCPRS_DEFAULT_MP_INDEX == DCPRS_MP0) ? &(dcprs->dcprs_mptop.dcprs_mp0) : &(dcprs->dcprs_mptop.dcprs_mp1);
}


/********************
 * STREAM LEVEL API *
 ********************/

dcprs_desc_t dcprs_default_mp0_eng0_desc;
dcprs_desc_t dcprs_default_mp0_eng1_desc;
volatile dcprs_mp_rmap_t *default_mp;


int32_t dcprs_lib_init(void *base, uint32_t bpp)
{
    int32_t rval = SUCCESS;
    uint32_t bppc;

    // Mapping the DCPRS
    dcprs = (volatile dcprs_t *)base;

    default_mp = dcprs_get_default_mp();

    // Enable DCPRS cast core
    write_reg(default_mp->eng0.jpeglsdec_cfg_mem, 1);
    write_reg(default_mp->eng1.jpeglsdec_cfg_mem, 1);

    // Configure frame section mode
    write_reg(default_mp->dcprs_mp.engine0_frm_sec_enb_reg, DCPRS_FRAME_SEC_MODE_DISABLED);
    write_reg(default_mp->dcprs_mp.engine1_frm_sec_enb_reg, DCPRS_FRAME_SEC_MODE_DISABLED);
#if 0
    // TODO(imagyar): Reconsider enabling these interrupts in the future.
    // Enable error interrupts.
    write_reg(default_mp->dcprs_mp.dcprs_err_interrupt_reg_int_en, 0xffffffff);
    write_reg(dcprs->dcprs_mptop.rgf.dcprs_axi_err_interrupt_reg_int_en, 0xffffffff);
#endif
    // Write default descriptors to their place in descriptor memory
    rval = dcprs_init_descriptor(&dcprs_default_mp0_eng0_desc);
#ifdef DEBUG
    if IS_ERR(rval) {
        dbg_print("%s: Failed to initialize default descriptor for mp0, eng0!\n");
        return rval;
    }
#endif

    // Set color depth.
    switch (bpp) {
        case 8:
            bppc = DCPRS_BPP_RAW8;
            break;
        case 10:
            bppc = DCPRS_BPP_RAW10;
            break;
        case 12:
            bppc = DCPRS_BPP_RAW12;
            break;
        case 16:
            bppc = DCPRS_BPP_RAW16;
            break;
        default:
            pr_err("illegal color depth of %d, using 8\n", bpp);
            bppc = DCPRS_BPP_RAW8;
            break;
    }
    DPRINT("CONFIG COLOR DEPTH = %d ==> %d\n", bpp, bppc);
    dcprs_default_mp0_eng0_desc.word3.bitfield.axirdch0obpp = bppc;
    dcprs_default_mp0_eng0_desc.word3.bitfield.axiwrch0dpck = 1;

    /*
        The Original frame that was compressed by Hydra might have pixel padding or not.
        DCPRS receives a compressed frame without any padding.
        DCPRS can be configured to write the decompressed frame to memory with or without padding.

        If you want the frame written without padding for raw10:
            AXIWRCH0DPCK =1.  AXIRDCH0IBPP=01

        If you want the frame written with padding for raw10:
            (10b pixel + 6b ad)
            AXIWRCH0DPCK =0.  AXIRDCH0IBPP=01
    */

    rval = dcprs_write_descriptor(&dcprs_default_mp0_eng0_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE0, DCPRS_DEFAULT_DESCRIPTOR_INDEX);
#ifdef DEBUG
    if IS_ERR(rval) {
        dbg_print("%s: Failed to write descriptor to mp%d, eng%d, index%d!\n", DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE1, DCPRS_DEFAULT_DESCRIPTOR_INDEX);
        return rval;
    }
#endif

    rval = dcprs_init_descriptor(&dcprs_default_mp0_eng1_desc);
#ifdef DEBUG
    if IS_ERR(rval) {
        dbg_print("%s: Failed to initialize default descriptor for mp0, eng1!\n");
        return rval;
    }
#endif

    rval = dcprs_write_descriptor(&dcprs_default_mp0_eng1_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE1, DCPRS_DEFAULT_DESCRIPTOR_INDEX);
#ifdef DEBUG
    if IS_ERR(rval) {
        dbg_print("%s: Failed to write descriptor to mp%d, eng%d, index%d!\n", DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE1, DCPRS_DEFAULT_DESCRIPTOR_INDEX);
        return rval;
    }
#endif

    // Set bayer color order
    write_reg(default_mp->dcprs_mp.bayer_green_first_reg, DCPRS_BAYER_FIRST_PIXEL_GREEN);

    return rval;
}

int32_t dcprs_exec_descriptor(int chain_engines, int enable_int)
{
    int32_t rval = SUCCESS;
    dcprs_mp_rgf_dcprs_e0_desc_exec_reg_t exec_reg_e0;
    //dcprs_mp_rgf_exec_ctrl_gf_stop_reg_t stop;
    //dcprs_mp_rgf_exec_ctrl_resume_reg_t  resume;
    //dcprs_mp_rgf_dcprs_desc_status_reg_t status;

    exec_reg_e0.register_value = 0;

    default_mp->dcprs_mp.dcprs_func_event_reg_int_en.bitfield.e0_exec_end = 1;

    exec_reg_e0.bitfield.eng0_pointer = DCPRS_DEFAULT_DESCRIPTOR_INDEX;
    exec_reg_e0.bitfield.eng0_cntr = 0;
    if (enable_int) {
        exec_reg_e0.bitfield.desc_end_int_enb = 1; // Enable completion interrupt.
    }
    if (chain_engines) {
        exec_reg_e0.bitfield.eng1_pointer = DCPRS_DEFAULT_DESCRIPTOR_INDEX;
        exec_reg_e0.bitfield.eng1_valid = 1;
    }
    //exec_reg_e0.bitfield.eng0_reuse = 1;
    //stop.bitfield.en = 1;
    //write_reg(dcprs->dcprs_mptop.dcprs_mp0.dcprs_mp.eng0_exec_ctrl_gf_stop_reg, stop.register_value);

    // Push desc
    write_reg(default_mp->dcprs_mp.dcprs_e0_desc_exec_reg, exec_reg_e0.register_value);
    //write_reg(default_mp->dcprs_mp.dcprs_e0_desc_exec_reg, exec_reg_e0.register_value);

    //status.register_value = read_reg(dcprs->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_desc_status_reg.register_value);
    //pr_info("ENG0 FL IS: %d, ENG1 FL IS: %d\n", status.bitfield.eng0_fifo_fl, status.bitfield.eng1_fifo_fl);
    //dcprs_engine_flush();
    //status.register_value = read_reg(dcprs->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_desc_status_reg.register_value);
    //pr_info("ENG0 FL IS: %d, ENG1 FL IS: %d\n", status.bitfield.eng0_fifo_fl, status.bitfield.eng1_fifo_fl);

    //resume.bitfield.en = 1;
    //write_reg(dcprs->dcprs_mptop.dcprs_mp0.dcprs_mp.eng0_exec_ctrl_resume_reg, resume.register_value);

    return rval;
}

int32_t dcprs_config_descriptor(uint64_t input_addr,
                                uint64_t output_addr,
                                uint8_t engine)
{

#ifdef DEBUG
    if ((engine != DCPRS_ENGINE0) && (engine != DCPRS_ENGINE1)) {
        dbg_print("%s: Invalid engine %d, expected %d or %d\n", DCPRS_ENGINE0, DCPRS_ENGINE1);
        return -EINVPAR;
    }
#endif

    int32_t rval = SUCCESS;
    dcprs_desc_t *eng_desc = engine == DCPRS_ENGINE0 ? &(dcprs_default_mp0_eng0_desc) : &(dcprs_default_mp0_eng1_desc);
    eng_desc->word6.bitfield.axirdch0ba_l = input_addr & 0xffffffff;
    eng_desc->word7.bitfield.axirdch0ba_h = input_addr >> 32;
    eng_desc->word9.bitfield.axiwrch0ba_l = output_addr & 0xffffffff;
    eng_desc->word10.bitfield.axiwrch0ba_h = output_addr >> 32;

    rval = dcprs_write_descriptor(eng_desc, DCPRS_DEFAULT_MP_INDEX, engine, DCPRS_DEFAULT_DESCRIPTOR_INDEX);

#ifdef DEBUG
    if IS_ERR(rval) {
        dbg_print("%s: Failed to write descriptor!\n");
        return rval;
    }
#endif

    return rval;
}

int32_t dcprs_section_received(uint8_t engine, uint32_t size)
{

    int32_t rval = SUCCESS;

    if (engine == DCPRS_ENGINE0) {
        write_reg(default_mp->dcprs_mp.engine0_frm_sec_size_reg, size);
    } else if (engine == DCPRS_ENGINE1) {
        write_reg(default_mp->dcprs_mp.engine1_frm_sec_size_reg, size);
    } else {
        pr_err("%s: Invalid engine %d\n", __func__, engine);
        return -EINVAL;
    }

    return rval;
}

int32_t dcprs_eof_received(uint8_t engine)
{
    int32_t rval = SUCCESS;

    dcprs_mp_rgf_frm_sec_force_eof_reg_t force_eof_reg = {0};
    force_eof_reg.bitfield.en = 1;
    if (engine == DCPRS_ENGINE0) {
        write_reg(default_mp->dcprs_mp.engine0_frm_sec_force_eof_reg, force_eof_reg.register_value);
        write_reg(default_mp->dcprs_mp.engine0_frm_sec_force_eof_reg, 0);
    } else if (engine == DCPRS_ENGINE1) {
        write_reg(default_mp->dcprs_mp.engine1_frm_sec_force_eof_reg, force_eof_reg.register_value);
        write_reg(default_mp->dcprs_mp.engine1_frm_sec_force_eof_reg, 0);
    } else {
        pr_err("%s: Invalid engine %d\n", __func__, engine);
        return -EINVAL;
    }

    return rval;
}

int32_t dcprs_wait_for_finish(void)
{
    int32_t rval = SUCCESS;
    // TODO(izalic): Make this not busy wait
    volatile dcprs_mp_rgf_dcprs_func_event_reg_t event_reg;
    event_reg.register_value = read_reg(default_mp->dcprs_mp.dcprs_func_event_reg);
    while ((event_reg.bitfield.e0_exec_end != 1) || (event_reg.bitfield.e1_exec_end != 1)) {
        event_reg.register_value = read_reg(default_mp->dcprs_mp.dcprs_func_event_reg);
    }

    // Clear decompression complete event from both engines
    event_reg.bitfield.e0_exec_end = 1;
    event_reg.bitfield.e1_exec_end = 1;
    write_reg(default_mp->dcprs_mp.dcprs_func_event_reg, event_reg.register_value);

    return rval;
}


void dcprs_engine_flush(void)
{
    dcprs_mp_rgf_exec_fifo_flush_reg_t reg;

    reg.bitfield.flush = 1;
    write_reg(default_mp->dcprs_mp.eng0_exec_fifo_flush_reg, reg.register_value);
    write_reg(default_mp->dcprs_mp.eng1_exec_fifo_flush_reg, reg.register_value);
}


void dump_jpegls(void)
{
    uint32_t *wp = (uint32_t *)default_mp->eng0.jpeglsdec_cfg_mem;
    cast_t *cp0 = (cast_t *)default_mp->eng0.jpeglsdec_cfg_mem;
    cast_t *cp1 = (cast_t *)default_mp->eng1.jpeglsdec_cfg_mem;
    int i;

    pr_info("CAST(0x%08x) - ENG 0\n", cp0->version);
    pr_info("CAST ERROR: 0x%08X\n", cp0->error);
    wp = (uint32_t *)default_mp->eng0.jpeglsdec_cfg_mem;
    for (i = 0; i < 12; i++) {
        pr_info("jpegls #%02d: 0x%08X\n", i, wp[i]);
    }

    pr_info("CAST(0x%08x) - ENG 1\n", cp1->version);
    pr_info("CAST ERROR: 0x%08X\n", cp1->error);
    wp = (uint32_t *)default_mp->eng1.jpeglsdec_cfg_mem;
    for (i = 0; i < 12; i++) {
        pr_info("jpegls #%02d: 0x%08X\n", i, wp[i]);
    }
}

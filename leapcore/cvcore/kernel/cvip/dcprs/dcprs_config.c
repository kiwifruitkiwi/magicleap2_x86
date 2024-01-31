
// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

#include <linux/platform_device.h>
#include <linux/io.h>

#include "dcprs_lib.h"
#include "dcprs_config.h"
#include "dcprs_top.h"
#include "dcprs_desc.h"

#define dbg_print  pr_err
#ifdef __va
#undef __va
#define __va(x)  (x)
#endif

int32_t dcprs_write_block(uint32_t *src, uint32_t *dest, uint16_t size_bytes)
{
    int32_t rval = SUCCESS;
    uint8_t i;

    // TODO(imagyar): change to memcpy if it is available
    for (i = 0; i < size_bytes / 4; i++) {
        writel(src[i], __va(&(dest[i])));
    }

    return rval;
}

dcprs_mp_rgf_dcprs_desc_mem_t *dcprs_get_descriptor_base_address(uint8_t mp_num, uint8_t engine_num, uint8_t desc_num)
{
    // TODO(izalic): Consider an array of start addresses indexed by an enum of eng+mp?

#ifdef DEBUG
    if ((mp_num != DCPRS_MP0) && (mp_num != DCPRS_MP1)) {
        dbg_print("%s: Invalid DCPRS MP %d\n", __func__, mp_num);
        return NULL;
    }
    if ((engine_num != DCPRS_ENGINE0) && (engine_num != DCPRS_ENGINE1)) {
        dbg_print("%s: Invalid DCPRS engine %d\n", __func__, engine_num);
        return NULL;
    }
    if (desc_num > DCPRS_MAX_DESCRIPTOR_INDEX) {
        dbg_print("%s: Invalid DCPRS descriptor index %d, max is %d\n", __func__, engine_num, DCPRS_MAX_DESCRIPTOR_INDEX);
        return NULL;
    }
#endif

    uint32_t offset = desc_num * sizeof(dcprs_desc_t);
    volatile dcprs_mp_rmap_t *mp = (mp_num == DCPRS_MP0) ? &(dcprs->dcprs_mptop.dcprs_mp0) : &(dcprs->dcprs_mptop.dcprs_mp1);
    return (engine_num == DCPRS_ENGINE0) ?
           (dcprs_mp_rgf_dcprs_desc_mem_t *)(&(mp->dcprs_mp.dcprs_eng0_desc_mem) + offset) :
           (dcprs_mp_rgf_dcprs_desc_mem_t *)(&(mp->dcprs_mp.dcprs_eng1_desc_mem) + offset);
}

int32_t dcprs_write_descriptor(dcprs_desc_t *desc, uint8_t mp, uint8_t eng, uint8_t desc_num)
{
    int32_t rval = SUCCESS;
    
    dcprs_mp_rgf_dcprs_desc_mem_t *desc_base_addr = dcprs_get_descriptor_base_address(mp, eng, desc_num);

#ifdef DEBUG
    if (desc_base_addr == NULL) {
        dbg_print("%s: Failed to get descriptor base address\n");
        return -EINVPAR;
    }
#endif

    // Write descriptor to memory word by word
    rval = dcprs_write_block((uint32_t *)desc, (uint32_t *)desc_base_addr, sizeof(dcprs_desc_t));
#ifdef DEBUG
    if IS_ERR(rval) {
        dbg_print("%s: Failed to write block\n");
        return rval;
    }
#endif

    return rval;
}

int32_t dcprs_init_descriptor(dcprs_desc_t *desc)
{
    int32_t rval = SUCCESS;

#ifdef DEBUG
    if (desc == NULL) {
        dbg_print("%s: Invalid parameter! desc: %p\n", __func__, desc);
        return -EINVPAR;
    }
#endif

    desc->word0.bitfield.mergemod = DCPRS_MERGEMOD_RGB;
    desc->word0.bitfield.cdpe = DCPRS_CDP_DISABLED;
    desc->word0.bitfield.padonly = 0;
    desc->word0.bitfield.frps = 0;
    desc->word0.bitfield.lcpo = 0;
    desc->word0.bitfield.fcpo = 0;
    desc->word0.bitfield.lrpo = 0;
    desc->word0.bitfield.frpo = 0;

    // Word 1
    desc->word1.bitfield.lcps = 0;
    desc->word1.bitfield.fcps = 0;
    desc->word1.bitfield.lrps = 0;

    // Word 2
    desc->word2.bitfield.ncol = 0; // it has no meaning since it's driven by the design to the fax
    desc->word2.bitfield.nrow = 1; // the compressed frame comes into to block as 1 line.

    // Word 3
    desc->word3.bitfield.axirdch0ibpp = DCPRS_BPP_RAW16;
    desc->word3.bitfield.axirdch0fifoen = 0;
    desc->word3.bitfield.axirdch0mod = 0;
    desc->word3.bitfield.axirdch0aie = 0;
    desc->word3.bitfield.axirdch0bppcsfl = 0;
    desc->word3.bitfield.axirdch0bppcsfr = 0;
    desc->word3.bitfield.axirdch0dpck = 1;
    desc->word3.bitfield.axiwrch0vflip = 0;
    desc->word3.bitfield.axiwrch0fifoen = 0;
    desc->word3.bitfield.axiwrch0aie = 0;
    desc->word3.bitfield.axiwrch0dpck = 1;
    desc->word3.bitfield.axiwrch0mod = 1;
    desc->word3.bitfield.axirdch0obpp = DCPRS_BPP_RAW8;
    desc->word3.bitfield.cdpdescptr = 0;

    // Word 4
    desc->word4.bitfield.axirdch0rs = DCPRS_FRAME_SEC_MODE_DATASIZE;

    // Word 5
    desc->word5.bitfield.axirdch0rgap = 0;
    desc->word5.bitfield.axirdch0ais = 0;

    // Word 6
    desc->word6.bitfield.axirdch0ba_l = 0;

    // Word 7
    desc->word7.bitfield.axirdch0ba_h = 0;

    // Word 8
    desc->word8.bitfield.axiwrch0ais = 0;

    // Word 9
    desc->word9.bitfield.axiwrch0ba_l = 0;

    // Word 10
    desc->word10.bitfield.axiwrch0ba_h = 0;

    return rval;
}


void dcprs_dump_descriptor(dcprs_desc_t *desc)
{
    pr_info("w0:  %08x", desc->word0.register_value);
    pr_info("w1:  %08x", desc->word1.register_value);
    pr_info("w2:  %08x", desc->word2.register_value);
    pr_info("w3:  %08x", desc->word3.register_value);
    pr_info("w4:  %08x", desc->word4.register_value);
    pr_info("w5:  %08x", desc->word5.register_value);
    pr_info("w6:  %08x", desc->word6.register_value);
    pr_info("w7:  %08x", desc->word7.register_value);
    pr_info("w8:  %08x", desc->word8.register_value);
    pr_info("w9:  %08x", desc->word9.register_value);
    pr_info("w10: %08x", desc->word10.register_value);

    pr_info("w0: mergemod = %08x", desc->word0.bitfield.mergemod);
    pr_info("w0: cdpe     = %08x", desc->word0.bitfield.cdpe);
    pr_info("w0: padonly  = %08x", desc->word0.bitfield.padonly);
    pr_info("w0: frps     = %08x", desc->word0.bitfield.frps);
    pr_info("w0: lcpo     = %08x", desc->word0.bitfield.lcpo);
    pr_info("w0: fcpo     = %08x", desc->word0.bitfield.fcpo);
    pr_info("w0: lrpo     = %08x", desc->word0.bitfield.lrpo);
    pr_info("w0: frpo     = %08x", desc->word0.bitfield.frpo);

    // Word 1
    pr_info("w1: lcps = %08x", desc->word1.bitfield.lcps);
    pr_info("w1: fcps = %08x", desc->word1.bitfield.fcps);
    pr_info("w1: lrps = %08x", desc->word1.bitfield.lrps);

    // Word 2
    pr_info("w2: ncol = %08x", desc->word2.bitfield.ncol);
    pr_info("w2: nrow = %08x", desc->word2.bitfield.nrow);

    // Word 3
    pr_info("w3: axirdch0ibpp    = %08x", desc->word3.bitfield.axirdch0ibpp);
    pr_info("w3: axirdch0fifoen  = %08x", desc->word3.bitfield.axirdch0fifoen);
    pr_info("w3: axirdch0mod     = %08x", desc->word3.bitfield.axirdch0mod);
    pr_info("w3: axirdch0aie     = %08x", desc->word3.bitfield.axirdch0aie);
    pr_info("w3: axirdch0bppcsfl = %08x", desc->word3.bitfield.axirdch0bppcsfl);
    pr_info("w3: axirdch0bppcsfr = %08x", desc->word3.bitfield.axirdch0bppcsfr);
    pr_info("w3: axirdch0dpck    = %08x", desc->word3.bitfield.axirdch0dpck);
    pr_info("w3: axiwrch0vflip   = %08x", desc->word3.bitfield.axiwrch0vflip);
    pr_info("w3: axiwrch0fifoen  = %08x", desc->word3.bitfield.axiwrch0fifoen);
    pr_info("w3: axiwrch0aie     = %08x", desc->word3.bitfield.axiwrch0aie);
    pr_info("w3: axiwrch0dpck    = %08x", desc->word3.bitfield.axiwrch0dpck);
    pr_info("w3: axiwrch0mod     = %08x", desc->word3.bitfield.axiwrch0mod);
    pr_info("w3: axirdch0obpp    = %08x", desc->word3.bitfield.axirdch0obpp);
    pr_info("w3: cdpdescptr      = %08x", desc->word3.bitfield.cdpdescptr);

    // Word 4
    pr_info("w4: axirdch0rs = %08x", desc->word4.bitfield.axirdch0rs);

    // Word 5
    pr_info("w5: axirdch0ais  = %08x", desc->word5.bitfield.axirdch0ais);
    pr_info("w5: axirdch0rgap = %08x", desc->word5.bitfield.axirdch0rgap);

    // Word 6
    pr_info("w6: axirdch0ba_l = %08x", desc->word6.bitfield.axirdch0ba_l);

    // Word 7
    pr_info("w7: axirdch0ba_h = %08x", desc->word7.bitfield.axirdch0ba_h);

    // Word 8
    pr_info("w8: axiwrch0ais = %08x", desc->word8.bitfield.axiwrch0ais);

    // Word 9
    pr_info("w9: axiwrch0ba_l = %08x", desc->word9.bitfield.axiwrch0ba_l);

    // Word 10
    pr_info("w10: axiwrch0ba_h = %08x", desc->word10.bitfield.axiwrch0ba_h);
}


// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

#include <linux/types.h>
#include <linux/kernel.h>
#include "nwl.h"

#define DMA_BASE          (0x0000)
#define DMA_CHAN_SIZE     (0x80)
#define TRAN_EGRESS_BASE  (0x8C00)
#define TRAN_EGRESS_SIZE  (0x020)
#define TRAN_INGRESS_BASE (0x8800)
#define TRAN_INGRESS_SIZE (0x020)
#define TRAN_WINDOWS_COUNT (8)

int pci_xlat_cfg(uint8_t *base, uint8_t win_num, xlat_dir_t dir, xlat_win_size_t win_size,
                 uint64_t src, uint64_t dst)
{
    uint32_t mask = (1 << (win_size + 12)) - 1;

    pr_info("pci_xlat_cfg: base=%px, src=0x%llx, dst=0x%0llx", base, src, dst);

    if (win_num > TRAN_WINDOWS_COUNT) {
        pr_err("pci_xlat_cfg: bad window: %d\n", win_num);
        return -1;
    }

    if ((src & mask) || (dst & mask)) {
        // Bad address alignment!
        pr_err("pci_xlat_cfg: bad alignment: %d\n", win_num);
        return -1;
    }

    if (dir == XLAT_EGRESS) {
        egress_xlat_t *egress =
            (egress_xlat_t *)(base + TRAN_EGRESS_BASE + (win_num * TRAN_EGRESS_SIZE));

        egress->control.WORD = 0;

        egress->dst_base_hi = dst >> 32;
        egress->dst_base_lo = dst & 0xffffffff;

        egress->src_base_hi = src >> 32;
        egress->src_base_lo = src & 0xffffffff;

        egress->control.FIELDS.egress_size = win_size;

        egress->control.FIELDS.egress_enable = 1;
    } else {
        ingress_xlat_t *ingress =
            (ingress_xlat_t *)(base + TRAN_INGRESS_BASE + (win_num * TRAN_INGRESS_SIZE));

        ingress->control.WORD = 0;

        ingress->dst_base_hi = dst >> 32;
        ingress->dst_base_lo = dst & 0xffffffff;

        ingress->src_base_hi = src >> 32;
        ingress->src_base_lo = src & 0xffffffff;

        ingress->control.FIELDS.ingress_size = win_size;

        ingress->control.FIELDS.ingress_enable = 1;
    }

    return 0;
}


void pci_xlat_clear(uint8_t *base_egress, uint8_t *base_ingress)
{
    egress_xlat_t *egress = (egress_xlat_t *)(base_egress + TRAN_EGRESS_BASE);
    ingress_xlat_t *ingress = (ingress_xlat_t *)(base_ingress + TRAN_INGRESS_BASE);
    int i;

    for (i = 0; i < TRAN_WINDOWS_COUNT; i++) {
        ingress->control.WORD = 0;
        ingress->status.WORD  = 0;
        ingress->rsvd         = 0;
        ingress->src_base_lo  = 0;
        ingress->src_base_hi  = 0;
        ingress->dst_base_hi  = 0;
        ingress->dst_base_lo  = 0;
        ingress++;

        egress->control.WORD = 0;
        egress->status.WORD  = 0;
        egress->rsvd         = 0;
        egress->src_base_lo  = 0;
        egress->src_base_hi  = 0;
        egress->dst_base_hi  = 0;
        egress->dst_base_lo  = 0;
        egress++;
    }
}


uint32_t pci_xlat_min_region_size(uint32_t buf_len)
{
    int i;

    for (i = 0; i < 31; i++) {
        if ((1 << i) >= buf_len) {
            if (i < 12) {
                return 12;
            } else {
                return i - 12;
            }
        }
    }

    return 0;
}


int pci_xlat_dump(uint8_t *base, uint8_t win_num, xlat_dir_t dir)
{
    if (win_num > TRAN_WINDOWS_COUNT) {
        return -1;
    }

    if (dir == XLAT_EGRESS) {
        egress_xlat_t *egress =
            (egress_xlat_t *)(base + TRAN_EGRESS_BASE + (win_num * TRAN_EGRESS_SIZE));

        pr_info("Egress #%d @%p:\n", win_num, egress);

        pr_info("\t%p    control     = 0x%08X\n", &egress->control.WORD,
                egress->control.WORD);

        pr_info("\t%p    src_base_lo = 0x%08X\n", &egress->src_base_lo,
                egress->src_base_lo);
        pr_info("\t%p    src_base_hi = 0x%08X\n", &egress->src_base_hi,
                egress->src_base_hi);

        pr_info("\t%p    dst_base_lo = 0x%08X\n", &egress->dst_base_lo,
                egress->dst_base_lo);
        pr_info("\t%p    dst_base_hi = 0x%08X\n", &egress->dst_base_hi,
                egress->dst_base_hi);
    } else {
        ingress_xlat_t *ingress =
            (ingress_xlat_t *)(base + TRAN_INGRESS_BASE + (win_num * TRAN_INGRESS_SIZE));

        pr_info("Ingress #%d @%p:\n", win_num, ingress);

        pr_info("\t%p    control     = 0x%08X\n", &ingress->control.WORD,
                ingress->control.WORD);

        pr_info("\t%p    src_base_lo = 0x%08X\n", &ingress->src_base_lo,
                ingress->src_base_lo);
        pr_info("\t%p    src_base_hi = 0x%08X\n", &ingress->src_base_hi,
                ingress->src_base_hi);

        pr_info("\t%p    dst_base_lo = 0x%08X\n", &ingress->dst_base_lo,
                ingress->dst_base_lo);
        pr_info("\t%p    dst_base_hi = 0x%08X\n", &ingress->dst_base_hi,
                ingress->dst_base_hi);
    }

    return 0;
}


void pci_msi_send(uint8_t *base, uint8_t vec)
{
    uint32_t *reg;

    // Enable interrupt.
    reg = (uint32_t *)(base + DMA_BASE + (vec * DMA_CHAN_SIZE) + 0x60);
    *reg = 1;

    // Assert interrupt.
    reg = (uint32_t *)(base + DMA_BASE + (vec * DMA_CHAN_SIZE) + 0x70);
    *reg = 0xffffffff;
}


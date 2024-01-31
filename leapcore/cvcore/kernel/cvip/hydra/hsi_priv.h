/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#define DPRINT(level, ...)  \
    if (g_hydra_dbg_mask >= level) \
    pr_info(__VA_ARGS__)


int32_t hsi_create_class(void);

void hsi_destroy_class(void);

int32_t hsi_init(uint32_t hix, struct pci_dev *pcidev, struct device *sysdev, struct dentry *ddir, struct hsi_root *hsi_root, void *hydradev);

void hsi_uninit(uint32_t hix);

int hsi_probe(uint32_t hix);

void hsi_intr(uint32_t hix, uint32_t irq);

void hsi_tick(void);

uint32_t hydra_get_irq(uint32_t hydra_ix, uint32_t chan);

extern uint32_t g_hydra_dbg_mask;

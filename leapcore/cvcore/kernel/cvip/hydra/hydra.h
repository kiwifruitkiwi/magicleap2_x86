/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include <linux/pci.h>
#include <linux/notifier.h>
#include "shmem_map.h"

#define HIX_HYDRA_PRI   (0)
#define HIX_HYDRA_SEC   (1)

int hydra_devices_attached(struct shmem_map **map);

uint32_t hydra_get_irq(uint32_t hydra_ix, uint32_t chan);

void hydra_irq_post(uint32_t hydra_ix, uint32_t irq);
void hydra_hcp_irq_post(uint32_t hydra_ix);

void hydra_aon_scratch_post(uint32_t hydra_ix, uint32_t val);

uint64_t gac_time_get(void);

uint32_t hydra_req_irq(int hix, int chan_id, uint32_t vector, irq_handler_t handler, irq_handler_t th_handler, int threaded, const char *devname, void *cookie);

const void *hydra_free_irq(int hix, int irq_ix, uint32_t vector, void *cookie);

int register_hydra_notifier(struct notifier_block *nb);
int unregister_hydra_notifier(struct notifier_block *nb);

void hydra_register_hcp_resp_handler(void (*handler)(uint32_t));
void hydra_get_sram_hcp_ptr(int hix, void **ptr);

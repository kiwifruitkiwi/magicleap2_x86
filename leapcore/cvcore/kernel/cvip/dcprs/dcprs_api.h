/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

//
// DCPRS kernel API
//

int dcprs_is_busy(void);
void dcprs_conf(uint32_t slice_length, uint32_t bpp);
int dcprs_decompress_start(void *addr_g, void *addr_rb, void *addr_dst, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb, uint32_t len_g, uint32_t len_rb);
int dcprs_decompress_end(uint32_t *count);
int dcprs_decompress_sect_next(uint32_t eng_ix, uint32_t len);
int dcprs_decompress_sect_start(void *addr_g, void *addr_rb, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb);

/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

struct vcam_buf {
    struct device     *dev;
    dma_addr_t        buf_g_io;
    dma_addr_t        buf_rb_io;
    dma_addr_t        buf_dst_io;
    dma_addr_t        buf_g_phys;
    dma_addr_t        buf_rb_phys;
    void              *buf_g;
    void              *buf_rb;
    void              *buf_dst;
    int               fd_g;
    int               fd_rb;
    uint32_t          size;
    uint32_t          size_g;
    uint32_t          size_rb;
    uint32_t          size_dst;
    uint32_t          off_dst;
    int               in_use;
    int               locked;
    uint64_t          t_decomp_start;
    uint64_t          t_decomp_end;
    uint64_t          sh_name_g;
    uint64_t          sh_name_rb;
    struct dma_buf    *dmabuf_g;
    struct dma_buf    *dmabuf_rb;
    struct list_head  list;
};

void vcam_init(struct device *dev, uint32_t bpp, uint32_t size, uint32_t count, uint32_t slice_length);
void vcam_uninit(void);
int  vcam_is_busy(void);
struct vcam_buf *vcam_buf_alloc(void);
void vcam_buf_free(struct vcam_buf *vp);
void vcam_buf_free_all(void);
int vcam_buf_decompress_start(void *addr_g, void *addr_rb, void *addr_dst, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb, uint32_t len_g, uint32_t len_rb);
int vcam_buf_decompress_end(uint32_t *count);
int vcam_decompress_sect_next(uint32_t eng_ix, uint32_t len);
int vcam_decompress_sect_start(void *addr_g, void *addr_rb, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb);
void vcam_save(struct vcam_buf *vp);
void image_save(char *fname, void *buf, uint32_t len);

// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

// pr_fmt will insert our module name into the pr_info() macro.
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/spinlock.h>
#include <linux/console.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/version.h>

#include "cvcore_name_stub.h"
#include "hsi.h"
#include "hsi_priv.h"
#include "shregion_types.h"
#include "vcam.h"
#ifdef __aarch64__
#include "ion/ion.h"
#include "uapi/mero_ion.h"
#endif

#include "trace.h"
#include "hsi_trace.h"

#if 0
#include "cvcore_name.h"
#else
#define VCAM_NAME_COUNT (8)
static uint64_t g_shname_tab[VCAM_NAME_COUNT] = {
    CVCORE_NAME_STATIC(HSI_VCAM_INBUF0),
    CVCORE_NAME_STATIC(HSI_VCAM_INBUF1),
    CVCORE_NAME_STATIC(HSI_VCAM_INBUF2),
    CVCORE_NAME_STATIC(HSI_VCAM_INBUF3),
    CVCORE_NAME_STATIC(HSI_VCAM_INBUF4),
    CVCORE_NAME_STATIC(HSI_VCAM_INBUF5),
    CVCORE_NAME_STATIC(HSI_VCAM_INBUF6),
    CVCORE_NAME_STATIC(HSI_VCAM_INBUF7),
};
#endif


extern int kalloc_shregion(struct shregion_metadata *meta, void **vaddr);
extern int get_shregion_cdp_dcprs_ioaddr(uint64_t shr_name, ShregionCdpDcprsIOaddr *shr_cdp_dcprs_vaddr);
extern struct dma_buf *get_dma_buf_for_shr(uint64_t shr_name);


static struct list_head g_vbufq_free;
static struct list_head g_vbufq_used;
static spinlock_t g_vbufq_free_lock;

struct {
    struct {
        struct shregion_metadata meta;
        void                    *kmem;
    } tab[VCAM_NAME_COUNT];
    int enabled;
    int count;
    int next;
} g_sh_cache;


#ifndef __aarch64__
//
// Emulate dcprs layer on Raven.
//
void dcprs_conf(uint32_t slice_length, uint32_t bpp)
{
}

int dcprs_decompress_start(void *addr_g, void *addr_rb, void *addr_dst, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb, uint32_t len_g, uint32_t len_rb)
{
    const int meta_size = (1024 * 8);
    const int off_g  = 0;
    const int off_rb = (1024 * 1024 * 3 / 2);

    memcpy(addr_dst + meta_size + off_g,  addr_g,  len_g);
    memcpy(addr_dst + meta_size + off_rb, addr_rb, len_rb);

    return 0;
}

int dcprs_decompress_end(uint32_t *count)
{
    *count = 0;
    return 0;
}

int dcprs_decompress_sect_next(uint32_t eng_ix, uint32_t len)
{
    return -1;
}

int dcprs_decompress_sect_start(void *addr_g, void *addr_rb, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb)
{
    return -1;
}
#else
#include "dcprs_api.h"

#ifdef USE_ION
static int vcam_ion_alloc_buf(uint32_t length, int *fd, dma_addr_t *io_addr, void **kmem)
{
    struct dma_buf *dbuf;
    struct sg_table *sg;


    *fd = ion_alloc(length, 1 << ION_HEAP_CVIP_HYDRA_FMR2_ID, 0);
    if (*fd < 0) {
        pr_err("%s: failed to allocate ion buffer: %d\n", __func__, *fd);
        return -ENOMEM;
    }

    dbuf = dma_buf_get(*fd);
    if (IS_ERR(dbuf)) {
        pr_err("%s: failed to get dma buffer\n", __func__);
        return -EFAULT;
    }

    sg = ion_sg_table(dbuf);
    if (IS_ERR(sg)) {
        pr_err("failed to get sg table\n");
        return -1;
    }
    WARN_ON(sg->nents > 1);

    *io_addr = page_to_phys(sg_page(sg->sgl)) - 0x800000000ULL;

    DPRINT(1, "%s: io_addr=0x%llx, kmem=%px\n", __func__, *io_addr, kmem);

    // The iommu line below is redundant now since all of FMR2 & FMR5 is mapped by DCPRS module.
    // Leave it here though so we remember how to do it though.
    // dcprs_iommu_map(dbuf, *io_addr + 0x800000000ULL);

    if (kmem) {
        *kmem = dma_buf_kmap(dbuf, 0);
        if (IS_ERR(*kmem)) {
            pr_err("failed to vmap\n");
            return -1;
        }
        DPRINT(1, "dma_buf_kmap returns 0x%px\n", *kmem);
    }

    return 0;
}
#endif


static int get_shregion_paddr(struct shregion_metadata *meta, dma_addr_t *paddr)
{
    struct dma_buf *dbuf;
    struct sg_table *sg;

    dbuf = get_dma_buf_for_shr(meta->name);
    sg   = ion_sg_table(dbuf);
    if (IS_ERR(sg)) {
        pr_err("failed to get sg table\n");
        return -1;
    }
    WARN_ON(sg->nents > 1);

    *paddr = page_to_phys(sg_page(sg->sgl)) - 0x800000000ULL;

    return 0;
}


static int vcam_shregion_alloc_buf(uint32_t length, int *fd, dma_addr_t *io_addr, dma_addr_t *phys_addr, void **kmem, uint64_t *sh_name)
{
    struct shregion_metadata meta;
    ShregionCdpDcprsIOaddr ioaddr;
    static int nxt_name = 0;
    int s;

    if (! g_sh_cache.enabled) {
        if (nxt_name == VCAM_NAME_COUNT) {
            pr_err("too many vcam buffers!\n");
            BUG();
        }
        memset(&meta, 0, sizeof(meta));

        meta.name       = g_shname_tab[nxt_name++];
        meta.type       = SHREGION_HYDRA_FMR2;
        meta.size_bytes = length;
        meta.flags      = SHREGION_PERSISTENT | SHREGION_CDP_AND_DCPRS_SHAREABLE | CDP_AND_DCPRS_MMU_MAP_PERSIST ;

        if ((s = kalloc_shregion(&meta, kmem)) != 0) {
            pr_err("kalloc_shregion error: %d\n", s);
            return -ENOMEM;
        }
        g_sh_cache.tab[g_sh_cache.count].meta = meta;
        g_sh_cache.tab[g_sh_cache.count].kmem = *kmem;
        g_sh_cache.count++;
    } else {
        meta  = g_sh_cache.tab[g_sh_cache.next].meta;
        *kmem = g_sh_cache.tab[g_sh_cache.next].kmem;
        g_sh_cache.next++;
    }
    get_shregion_cdp_dcprs_ioaddr(meta.name, &ioaddr);
    *io_addr = ioaddr;
    *sh_name = meta.name;
    *fd = 0;

    get_shregion_paddr(&meta, phys_addr);


    return 0;
}

#endif


static int vcam_alloc_bufs(struct device *dev, uint32_t size, struct vcam_buf **vpp)
{
    struct vcam_buf *vp;

    vp = devm_kzalloc(dev, sizeof(struct vcam_buf), GFP_KERNEL);
    vp->dev  = dev;
    vp->size = size;

    INIT_LIST_HEAD(&vp->list);

#ifdef __aarch64__
    // Get buffer for G data.
    if (vcam_shregion_alloc_buf(size, &vp->fd_g, &vp->buf_g_io, &vp->buf_g_phys, &vp->buf_g, &vp->sh_name_g) < 0) {
        pr_err("%s: Failed to allocate buf_g of size %d", __func__, size);
        return -ENOMEM;
    }

    // Get buffer for RB data.
    if (vcam_shregion_alloc_buf(size, &vp->fd_rb, &vp->buf_rb_io, &vp->buf_rb_phys, &vp->buf_rb, &vp->sh_name_rb) < 0) {
        pr_err("%s: Failed to allocate buf_rb of size %d", __func__, size);
        return -ENOMEM;
    }
    vp->dmabuf_g  = get_dma_buf_for_shr(vp->sh_name_g);
    vp->dmabuf_rb = get_dma_buf_for_shr(vp->sh_name_rb);
#else
    // Get buffer for G data.
    vp->buf_g = dmam_alloc_coherent(dev, size, &vp->buf_g_io, GFP_KERNEL);
    if (! vp->buf_g) {
        pr_err("%s: Failed to allocate buf_g of size %d", __func__, size);
        return -ENOMEM;
    }

    // Get buffer for RB data.
    vp->buf_rb = dmam_alloc_coherent(dev, size, &vp->buf_rb_io, GFP_KERNEL);
    if (! vp->buf_rb) {
        pr_err("%s: Failed to allocate buf_rb of size %d", __func__, size);
        return -ENOMEM;
    }
#endif

    *vpp = vp;

    DPRINT(1, "%s: vp=%px, fd_g=%d, fd_rb=%d, g=%px, rb=%px, g_io=0x%llx, g_phys=0x%llx, rb_io=0x%llx, rb_phys=0x%llx, size=%d\n",
           __func__, vp, vp->fd_g, vp->fd_rb, vp->buf_g, vp->buf_rb, vp->buf_g_io, vp->buf_g_phys, vp->buf_rb_io, vp->buf_rb_phys, size);
    return 0;
}


void image_save(char *fname, void *buf, uint32_t len)
{
    struct file *file;
    loff_t pos = 0;
    ssize_t ret;

    file = filp_open(fname, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (IS_ERR(file)) {
        pr_err("filp_open(%s) for mldma log failed\n", fname);
    }

    ret = kernel_write(file, buf, len, &pos);
    if (ret != len) {
        pr_err("failed to write image to file %s: %ld\n", fname, ret);
    }
    filp_close(file, NULL);
}


int vcam_buf_decompress_start(void *addr_g, void *addr_rb, void *addr_dst, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb, uint32_t len_g, uint32_t len_rb)
{
#if 0
    pr_info("%s: dst=0x%llx, g=0x%llx, rb=0x%llx, g_len=%d, rb_len=%d\n", __func__, ioaddr_dst, ioaddr_g, ioaddr_rb, len_g, len_rb);
#endif

    return dcprs_decompress_start(addr_g, addr_rb, addr_dst, ioaddr_dst, ioaddr_g, ioaddr_rb, len_g, len_rb);
}


int vcam_buf_decompress_end(uint32_t *count)
{
    return dcprs_decompress_end(count);
}


int vcam_decompress_sect_start(void *addr_g, void *addr_rb, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb)
{
    return dcprs_decompress_sect_start(addr_g, addr_rb, ioaddr_dst, ioaddr_g, ioaddr_rb);
}


int vcam_decompress_sect_next(uint32_t eng_ix, uint32_t len)
{
    return dcprs_decompress_sect_next(eng_ix, len);
}


struct vcam_buf *vcam_buf_alloc(void)
{
    struct vcam_buf * vp = 0;
    unsigned long flags;

    spin_lock_irqsave(&g_vbufq_free_lock, flags);
    if (!list_empty(&g_vbufq_free)) {
        vp = list_first_entry(&g_vbufq_free, struct vcam_buf, list);
        while (vp->locked) {
            if ((vp = list_next_entry(vp, list)) == NULL) {
                pr_err("No unlocked vcam bufs!\n");
                spin_unlock_irqrestore(&g_vbufq_free_lock, flags);
                return 0;
            }
        }
        list_move_tail(&vp->list, &g_vbufq_used);
    } else {
        pr_err("no free vcam_bufs");
        spin_unlock_irqrestore(&g_vbufq_free_lock, flags);
        return 0;
    }

    if (vp->in_use) {
        trace_ml_hsi_vcam_buf_free_in_use(vp, vp->buf_g, vp->buf_rb, vp->buf_dst);
        pr_err("%s: vp in use!\n", __func__);
    } else {
        vp->in_use = 1;
    }

    spin_unlock_irqrestore(&g_vbufq_free_lock, flags);

    trace_ml_hsi_vcam_buf_alloc(vp, vp->buf_g, vp->buf_rb, vp->buf_dst);

    return vp;
}


void vcam_buf_free_all(void)
{
    struct vcam_buf *vp = 0;
    struct list_head *csr, *nxt;
    unsigned long flags;

    spin_lock_irqsave(&g_vbufq_free_lock, flags);
    list_for_each_safe(csr, nxt, &g_vbufq_used) {
        vp = list_entry(csr, struct vcam_buf, list);
        trace_ml_hsi_vcam_buf_free_all(vp, vp->buf_g, vp->buf_rb, vp->buf_dst);
        vp->in_use = 0;
        list_move_tail(&vp->list, &g_vbufq_free);
    }
    spin_unlock_irqrestore(&g_vbufq_free_lock, flags);
}


void vcam_buf_free(struct vcam_buf *vp)
{
    unsigned long flags;

    trace_ml_hsi_vcam_buf_free(vp, vp->buf_g, vp->buf_rb, vp->buf_dst);

    spin_lock_irqsave(&g_vbufq_free_lock, flags);

    if (vp->in_use) {
        vp->in_use = 0;
        list_move_tail(&vp->list, &g_vbufq_free);
    } else {
        trace_ml_hsi_bkpt_err_intr(777, 0);
        pr_err("freeing free vcam buf!\n");
    }

    spin_unlock_irqrestore(&g_vbufq_free_lock, flags);
}


void vcam_init(struct device *dev, uint32_t bpp, uint32_t size, uint32_t count, uint32_t slice_length)
{
    struct vcam_buf *vp = 0;
    int i;

    dcprs_conf(slice_length, bpp);

    INIT_LIST_HEAD(&g_vbufq_free);
    INIT_LIST_HEAD(&g_vbufq_used);
    spin_lock_init(&g_vbufq_free_lock);

    for (i = 0; i < count; i++) {
        vcam_alloc_bufs(dev, size, &vp);
        list_add_tail(&vp->list, &g_vbufq_free);
        pr_info("vcam_init: i=%d, count=%d, size=%d, vp=%px\n", i, count, size, vp);
    }
}


int vcam_is_busy(void)
{
    return dcprs_is_busy();
}


void vcam_uninit(void)
{
    pr_info("Vcam Reset\n");
    g_sh_cache.enabled = 1;
    g_sh_cache.next    = 0;
}


void vcam_save(struct vcam_buf *vp)
{
    char name_g[64], name_rb[64], name_dst[64];
    static int count = 0;

    snprintf(name_g,   sizeof(name_g),   "/tmp/g%02d.bin", count);
    snprintf(name_rb,  sizeof(name_rb),  "/tmp/rb%02d.bin", count);
    snprintf(name_dst, sizeof(name_dst), "/tmp/dst%02d.bin", count);
    count++;

    image_save(name_g,   vp->buf_g,    vp->size_g);
    image_save(name_rb,  vp->buf_rb,   vp->size_rb);
    if (vp->buf_dst) {
        image_save(name_dst, (uint8_t *)vp->buf_dst + vp->off_dst, vp->size_dst);
    }
}



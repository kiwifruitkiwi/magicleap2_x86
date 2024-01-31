// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

// pr_fmt will insert our module name into the pr_info() macro.
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/debugfs.h>

#include "cdp.h"
#include "cdp_lib.h"
#include "cdp_desc.h"
#include "cdp_top.h"
#include "cdp_config.h"

extern int dump_stage_desc(struct seq_file *f, cdp_stage_desc_t *stage_desc);

#define BUFLEN  PAGE_SIZE
static char kbuf[PAGE_SIZE];
static struct cdp_hw_err *g_errs;


#define CDP_DECL(n) \
static ssize_t n##_rd(struct file *fp, char __user *user_buffer, size_t count, loff_t *position); \
static ssize_t n##_wr(struct file *fp, const char __user *user_buffer, size_t count, loff_t *position); \
static const struct file_operations fops_##n = { \
    .read = n##_rd,  \
    .write = n##_wr, \
};


// Need this to get around the normal memcpy crashing on device memory.
static void smemcpy(uint8_t *to, uint8_t *from, uint32_t count)
{
    while (count--) {
        *to++ = *from++;
    }
}


static int err_dump_glob(uint32_t reg, uint8_t *buf)
{
    cdp_rgf_cdp_global_interrupt_reg_t r;
    uint8_t *base = buf, *pkbuf = kbuf;

    r.register_value = reg;
    pkbuf += snprintf(pkbuf, BUFLEN, "0x%08x\n", reg);

    if (r.bitfield.cfg_mem_sbe_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cfg_mem_sbe_intr\n");
    if (r.bitfield.cfg_mem_dbe_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cfg_mem_dbe_intr\n");
    if (r.bitfield.cdp_done_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cdp_done_intr\n");
    if (r.bitfield.cdp_start_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cdp_start_intr\n");
    if (r.bitfield.cdp_txispp_trig_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cdp_txispp_trig_intr\n");
    if (r.bitfield.cdp_sysmem_read_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cdp_sysmem_read_intr\n");
    if (r.bitfield.cdp_stage0_done_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cdp_stage0_done_intr\n");
    if (r.bitfield.cdp_stage1_done_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cdp_stage1_done_intr\n");
    if (r.bitfield.cdp_stage2_done_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cdp_stage2_done_intr\n");
    if (r.bitfield.axi_rx_fifo_ovf_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_fifo_ovf_intr\n");
    if (r.bitfield.axi_rx_err_mem_0_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_err_mem_0_par_err_intr\n");
    if (r.bitfield.axi_rx_err_mem_1_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_err_mem_1_par_err_intr\n");
    if (r.bitfield.axi_rx_err_mem_2_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_err_mem_2_par_err_intr\n");
    if (r.bitfield.axi_rx_err_mem_3_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_err_mem_3_par_err_intr\n");
    if (r.bitfield.axi_rx_err_mem_4_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_err_mem_4_par_err_intr\n");
    if (r.bitfield.axi_rx_err_mem_5_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_err_mem_5_par_err_intr\n");
    if (r.bitfield.axi_rx_err_mem_6_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_err_mem_6_par_err_intr\n");
    if (r.bitfield.axi_rx_err_mem_7_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    axi_rx_err_mem_7_par_err_intr\n");
    if (r.bitfield.lbm_fifo_rd_par_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    lbm_fifo_rd_par_err_intr\n");
    if (r.bitfield.lbm_rd_fifo_overflow_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    lbm_rd_fifo_overflow_intr\n");
    if (r.bitfield.spacer_ovf_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    spacer_ovf_intr\n");

    return pkbuf - base;
}


static int err_dump(uint32_t reg, uint8_t *buf)
{
    cdp_rgf_cdp_interrupt_reg_t r;
    uint8_t *base = buf, *pkbuf = kbuf;

    r.register_value = reg;
    pkbuf += snprintf(pkbuf, BUFLEN, "0x%08x\n", reg);

    if (r.bitfield.frame_length_err_too_small_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_length_err_too_small_intr\n");
    if (r.bitfield.frame_length_err_too_big_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_length_err_too_big_intr\n");
    if (r.bitfield.frame_depth_err_too_small_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_depth_err_too_small_intr\n");
    if (r.bitfield.frame_depth_err_too_big_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_depth_err_too_big_intr\n");
    if (r.bitfield.protocol_err_sof_wo_vld_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    protocol_err_sof_wo_vld_intr\n");
    if (r.bitfield.protocol_err_eof_wo_vld_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    protocol_err_eof_wo_vld_intr\n");
    if (r.bitfield.protocol_err_sol_wo_vld_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    protocol_err_sol_wo_vld_intr\n");
    if (r.bitfield.protocol_err_eol_wo_vld_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    protocol_err_eol_wo_vld_intr\n");
    if (r.bitfield.protocol_err_dbl_sof_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    protocol_err_dbl_sof_intr\n");
    if (r.bitfield.protocol_err_dbl_sol_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    protocol_err_dbl_sol_intr\n");
    if (r.bitfield.protocol_err_dbl_eof_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    protocol_err_dbl_eof_intr\n");
    if (r.bitfield.protocol_err_dbl_eol_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    protocol_err_dbl_eol_intr\n");
    if (r.bitfield.too_many_candidates)
        pkbuf += snprintf(pkbuf, BUFLEN, "    too_many_candidates\n");
    if (r.bitfield.rsz_mem_parity_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    rsz_mem_parity_err_intr\n");
    if (r.bitfield.alg_mem_parity_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    alg_mem_parity_err_intr\n");
    if (r.bitfield.nms_mem_parity_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    nms_mem_parity_err_intr\n");
    if (r.bitfield.fast_th_mem_sbe_err_intr_user)
        pkbuf += snprintf(pkbuf, BUFLEN, "    fast_th_mem_sbe_err_intr_user\n");
    if (r.bitfield.fast_th_mem_dbe_err_intr_user)
        pkbuf += snprintf(pkbuf, BUFLEN, "    fast_th_mem_dbe_err_intr_user\n");
    if (r.bitfield.fast_th_mem_sbe_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    fast_th_mem_sbe_err_intr\n");
    if (r.bitfield.fast_th_mem_dbe_err_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    fast_th_mem_dbe_err_intr\n");
    if (r.bitfield.frame_bad_size_max_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_bad_size_max_intr\n");
    if (r.bitfield.frame_bad_size_min_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_bad_size_min_intr\n");
    if (r.bitfield.frame_bad_size_odd_pixel_num_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_bad_size_odd_pixel_num_intr\n");
    if (r.bitfield.cand_buff_bp_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cand_buff_bp_intr\n");
    if (r.bitfield.frame_buff_bp_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_buff_bp_intr\n");
    if (r.bitfield.frame_wmrk_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    frame_wmrk_intr\n");
    if (r.bitfield.cand_wmrk_intr)
        pkbuf += snprintf(pkbuf, BUFLEN, "    cand_wmrk_intr\n");

    return pkbuf - base;
}


static int cdp_stage0_show(struct seq_file *f, void *offset)
{
    return dump_stage_desc(f, &cdp_default_desc.stage0_desc);
}


static int cdp_stage1_show(struct seq_file *f, void *offset)
{
    return dump_stage_desc(f, &cdp_default_desc.stage1_desc);
}


static int cdp_stage2_show(struct seq_file *f, void *offset)
{
    return dump_stage_desc(f, &cdp_default_desc.stage2_desc);
}


static int cdp_stage0_open(struct inode *inode, struct file *file)
{
    return single_open(file, cdp_stage0_show, inode->i_private);
}


static int cdp_stage1_open(struct inode *inode, struct file *file)
{
    return single_open(file, cdp_stage1_show, inode->i_private);
}


static int cdp_stage2_open(struct inode *inode, struct file *file)
{
    return single_open(file, cdp_stage2_show, inode->i_private);
}


static const struct file_operations cdp_stage0_fops = {
    .owner        = THIS_MODULE,
    .open         = cdp_stage0_open,
    .read         = seq_read,
    .llseek       = seq_lseek,
    .release      = single_release,
};


static const struct file_operations cdp_stage1_fops = {
    .owner        = THIS_MODULE,
    .open         = cdp_stage1_open,
    .read         = seq_read,
    .llseek       = seq_lseek,
    .release      = single_release,
};


static const struct file_operations cdp_stage2_fops = {
    .owner        = THIS_MODULE,
    .open         = cdp_stage2_open,
    .read         = seq_read,
    .llseek       = seq_lseek,
    .release      = single_release,
};


static cdp_stage_desc_t *g_stage_descr_tab[3] = { &cdp_default_desc.stage0_desc, &cdp_default_desc.stage1_desc, &cdp_default_desc.stage2_desc };


////////////////////////////////////////////////////////////////////////////
// gauss_enb
////////////////////////////////////////////////////////////////////////////
CDP_DECL(gauss_enb);

static ssize_t gauss_enb_rd(struct file *fp, char __user *user_buffer,
                            size_t count, loff_t *ppos)
{
    int stage = (int)(uintptr_t)fp->f_inode->i_private;

    if (*ppos) {
        return 0;
    }

    snprintf(kbuf, BUFLEN, "%d\n", g_stage_descr_tab[stage]->word0.bitfield.gauss_en);

    return simple_read_from_buffer(user_buffer, count, ppos, kbuf, strlen(kbuf));
}


static ssize_t gauss_enb_wr(struct file *fp, const char __user *user_buffer,
                            size_t count, loff_t *position)
{
    int stage, p;

    stage = (int)(uintptr_t)fp->f_inode->i_private;

    simple_write_to_buffer(kbuf, BUFLEN, position, user_buffer, count);

    p = (kbuf[0] == '0') ? 0 : 1;

    g_stage_descr_tab[stage]->word0.bitfield.gauss_en = p;

    return count;
}


////////////////////////////////////////////////////////////////////////////
// axi_out_enb
////////////////////////////////////////////////////////////////////////////
CDP_DECL(axi_out_enb);

static ssize_t axi_out_enb_rd(struct file *fp, char __user *user_buffer,
                              size_t count, loff_t *ppos)
{
    int stage = (int)(uintptr_t)fp->f_inode->i_private;

    if (*ppos) {
        return 0;
    }

    snprintf(kbuf, BUFLEN, "%d\n", g_stage_descr_tab[stage]->word0.bitfield.isp_bypass_en);

    return simple_read_from_buffer(user_buffer, count, ppos, kbuf, strlen(kbuf));
}


static ssize_t axi_out_enb_wr(struct file *fp, const char __user *user_buffer,
                              size_t count, loff_t *position)
{
    int stage, val, rc;

    stage = (int)(uintptr_t)fp->f_inode->i_private;

    if ((rc = kstrtouint_from_user(user_buffer, count, 16, &val))) {
        return rc;
    }

    g_stage_descr_tab[stage]->word0.bitfield.isp_bypass_en = (val == 0) ? 0 : 1;

    return count;
}


////////////////////////////////////////////////////////////////////////////
// threshold_mem_rd
////////////////////////////////////////////////////////////////////////////
CDP_DECL(threshold_mem);

static ssize_t threshold_mem_rd(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *ppos)
{

    static uint8_t kbuf[PAGE_SIZE];
    uint8_t *tmem = 0, *emem;
    int stage = (int)(uintptr_t)fp->f_inode->i_private;
    int cw, tw;

    switch(stage) {
        case 0:
            tmem = (uint8_t *)cdp->eng.cdp_rgf.cdp_s0_fast_thold_mem;
            break;
        case 1:
            tmem = (uint8_t *)cdp->eng.cdp_rgf.cdp_s1_fast_thold_mem;
            break;
        case 2:
            tmem = (uint8_t *)cdp->eng.cdp_rgf.cdp_s2_fast_thold_mem;
            break;
    }
    emem = tmem + sizeof(cdp->eng.cdp_rgf.cdp_s0_fast_thold_mem); // tmem is the same size for each stage
    cw = emem - (tmem + *ppos);
    tw = (count <= cw) ? count : cw;
    smemcpy(kbuf, tmem + *ppos, tw);

    return simple_read_from_buffer(user_buffer, count, ppos, kbuf, tw);
}


static ssize_t threshold_mem_wr(struct file *fp, const char __user *user_buffer,
                                size_t count, loff_t *position)
{
    return -1;
}


////////////////////////////////////////////////////////////////////////////
// err_stat
////////////////////////////////////////////////////////////////////////////
CDP_DECL(err_stat);
static ssize_t err_stat_rd(struct file *fp, char __user *user_buffer,
                           size_t count, loff_t *ppos)
{
    int stage = (int)(uintptr_t)fp->f_inode->i_private;
    int len;

    if (*ppos) {
        return 0;
    }

    len = err_dump(g_errs->err[stage], kbuf);

    return simple_read_from_buffer(user_buffer, count, ppos, kbuf, len);
}


static ssize_t err_stat_wr(struct file *fp, const char __user *user_buffer,
                           size_t count, loff_t *position)
{
    return -1;
}


////////////////////////////////////////////////////////////////////////////
// err_glob
////////////////////////////////////////////////////////////////////////////
CDP_DECL(err_glob);
static ssize_t err_glob_rd(struct file *fp, char __user *user_buffer,
                           size_t count, loff_t *ppos)
{
    int len;

    if (*ppos) {
        return 0;
    }

    len = err_dump_glob(g_errs->global, kbuf);

    return simple_read_from_buffer(user_buffer, count, ppos, kbuf, len);
}


static ssize_t err_glob_wr(struct file *fp, const char __user *user_buffer,
                           size_t count, loff_t *position)
{
    return -1;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

void cdp_debugfs_init(struct dentry *root, struct cdp_hw_err *errs)
{
    struct dentry *stage[CDP_N_STAGES];

    g_errs = errs;

    stage[0] = debugfs_create_dir("stage0", root);
    stage[1] = debugfs_create_dir("stage1", root);
    stage[2] = debugfs_create_dir("stage2", root);

    debugfs_create_file("desc", 0400, stage[0], 0, &cdp_stage0_fops);
    debugfs_create_file("desc", 0400, stage[1], 0, &cdp_stage1_fops);
    debugfs_create_file("desc", 0400, stage[2], 0, &cdp_stage2_fops);

    debugfs_create_file("gauss_blur_enb", 0644, stage[0], (void *)0, &fops_gauss_enb);
    debugfs_create_file("gauss_blur_enb", 0644, stage[1], (void *)1, &fops_gauss_enb);
    debugfs_create_file("gauss_blur_enb", 0644, stage[2], (void *)2, &fops_gauss_enb);

    debugfs_create_file("axi_out_enb", 0644, stage[0], (void *)0, &fops_axi_out_enb);
    debugfs_create_file("axi_out_enb", 0644, stage[1], (void *)1, &fops_axi_out_enb);
    debugfs_create_file("axi_out_enb", 0644, stage[2], (void *)2, &fops_axi_out_enb);

    debugfs_create_file("threshold_mem", 0644, stage[0], (void *)0, &fops_threshold_mem);
    debugfs_create_file("threshold_mem", 0644, stage[1], (void *)1, &fops_threshold_mem);
    debugfs_create_file("threshold_mem", 0644, stage[2], (void *)2, &fops_threshold_mem);

    debugfs_create_x32("stat_glob",   0400, root, (uint32_t *)&(cdp->eng.cdp_rgf.cdp_global_interrupt_reg.register_value));
    debugfs_create_x32("stat_master", 0400, root, (uint32_t *)&(cdp->eng.cdp_rgf.cdp_master_interrupt_reg.register_value));

    debugfs_create_x32("status", 0400, stage[0], (uint32_t *)&(cdp->eng.cdp_rgf.cdp_s0_interrupt_reg));
    debugfs_create_x32("status", 0400, stage[1], (uint32_t *)&(cdp->eng.cdp_rgf.cdp_s1_interrupt_reg));
    debugfs_create_x32("status", 0400, stage[2], (uint32_t *)&(cdp->eng.cdp_rgf.cdp_s2_interrupt_reg));

    debugfs_create_file("err_status", 0444, stage[0], (void *)0, &fops_err_stat);
    debugfs_create_file("err_status", 0444, stage[1], (void *)1, &fops_err_stat);
    debugfs_create_file("err_status", 0444, stage[2], (void *)2, &fops_err_stat);

    debugfs_create_file("err_global", 0444, root, (void *)0, &fops_err_glob);


}


/*
 * Copyright 2021 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include "amdgpu.h"
#include "amdgpu_vcn.h"
#include "amdgpu_fwlog.h"
#define FWLOG_BUF_ADDR "fwlog_buffer"
const char program_name[] = "fwlog";
#define PROCFS_NAME program_name
static struct proc_dir_entry *entry;
static struct proc_dir_entry *parent;
void *fwlog_cpu_addr = NULL;
uint64_t fwlog_gpu_addr;
struct amdgpu_bo *fwlog_bo;
static ssize_t procfile_read(struct file *file, char __user *buffer,
                 size_t count, loff_t *offset)
{
    uint8_t *p_src = (unsigned char *)fwlog_cpu_addr;
    fw_log *pfwlog = (fw_log *)p_src;
    uint32_t total_read_bytes = 0;
    uint32_t rptr, wptr, hdsz;
    rptr = pfwlog->rptr;
    wptr = pfwlog->wptr;
    hdsz = pfwlog->header_size;
    p_src += rptr;
    if (wptr > rptr) {
        total_read_bytes = wptr - rptr;
        if (total_read_bytes > count)
            total_read_bytes = count;
        if (copy_to_user(buffer, (void *)p_src, total_read_bytes))
            return -EFAULT;
        rptr += total_read_bytes;
    } else if (wptr < rptr) {
        dbgprint("rptr=%5u, wptr=%5u, req=%5ld", rptr, wptr, count);
        total_read_bytes = AMDGPU_VCN_FW_LOG_SIZE - rptr;
        if (count <= total_read_bytes)
            total_read_bytes = count;
        if (copy_to_user(buffer, (void *)p_src, total_read_bytes))
            return -EFAULT;
        rptr += total_read_bytes;
        if (rptr == AMDGPU_VCN_FW_LOG_SIZE)
            rptr = hdsz;
        count -= total_read_bytes;
        if(count > (wptr - rptr)) {
            count = wptr - rptr;
            if(count > 0) {
                p_src = (unsigned char *)fwlog_cpu_addr + hdsz;
                if (copy_to_user(buffer + total_read_bytes, (void *)p_src, count))
                    return -EFAULT;
                rptr += count;
                total_read_bytes += count;
            }
        }
    }
    pfwlog->rptr = rptr;
    *offset += total_read_bytes;
    return total_read_bytes;
}
static int procfile_open(struct inode *inode, struct file *file)
{
    if (fwlog_cpu_addr == NULL)
        return -EFAULT;
    return 0;
}
static loff_t procfile_lseek(struct file *file, loff_t off, int mod)
{
    loff_t newpos;
    switch (mod) {
    case 0: /* SEEK_SET */
        newpos = off;
        break;
    case 1: /* SEEK_CUR */
        newpos = file->f_pos + off;
        break;
    default: /* can't happen */
        return -EINVAL;
    }
    return newpos;
}
static int procfile_release(struct inode *inode, struct file *file)
{
    return 0;
}
/*static struct proc_ops proc_fops = {
    .proc_lseek = procfile_lseek,
    .proc_open = procfile_open,
    .proc_read = procfile_read,
    .proc_release = procfile_release,
};*/
static struct file_operations proc_fops = {
    .llseek = procfile_lseek,
    .open = procfile_open,
    .read = procfile_read,
    .release = procfile_release,
};
/**
 *This function is called from amdgpu_init(..) @amdgpu_drv.c when the module is loaded
 */
int __init vcn3_fwlog_proc_init(void)
{
    parent = proc_mkdir(PROCFS_NAME, NULL);
    entry = proc_create(FWLOG_BUF_ADDR, 0444, parent, &proc_fops);
    if (!entry)
        return -1;
    dbgprint("vcn3_fwlog=%d", amdgpu_vcn3_fwlog);
    return 0;
}
/**
 *This function is called from amdgpu_exit(..) @fwlog_main.c when the module is unloaded
 */
void __exit vcn3_fwlog_proc_cleanup(void)
{
    remove_proc_entry(FWLOG_BUF_ADDR, parent);
    remove_proc_entry(PROCFS_NAME, NULL);
}


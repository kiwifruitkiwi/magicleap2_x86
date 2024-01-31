// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2020-2022
// Magic Leap, Inc. (COMPANY)
//

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <asm/current.h>
#include <linux/version.h>

#ifdef ARM_CVIP
#include "uapi/mero_ion.h"
#endif

#include "shregion_types.h"
#include "shregion_internal.h"

#ifdef CONFIG_DEBUG_FS

#ifdef ARM_CVIP

#define PTE_FLAGS_MASK (0xFFF0000000000FFF)

static inline pteval_t pte_flags(pte_t pte)
{
	return pte_val(pte) & PTE_FLAGS_MASK;
}

#else

#define phys_to_page(phys) pfn_to_page(phys >> PAGE_SHIFT)

#endif

typedef enum {
	REQ_UNKNOWN = 0,
	REQ_ADDR_VIRT,
	REQ_ADDR_PHYS,
	REQ_MEMINFO
} req_type_t;

struct app_check {
	pid_t pid;
	unsigned long addr;
	req_type_t req_type;
	int read_done;
};

static pte_t *find_pte(struct mm_struct *mm, unsigned long addr)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = pgd_offset(mm, addr);
	if (!pgd_present(*pgd)) {
		return NULL;
	}

	p4d = p4d_offset(pgd, addr);
	if (!p4d_present(*p4d)) {
		return NULL;
	}

	pud = pud_offset(p4d, addr);
	if (!pud_present(*pud)) {
		return NULL;
	}

	pmd = pmd_offset(pud, addr);
	if (!pmd_present(*pmd)) {
		return NULL;
	}

	pte = pte_offset_kernel(pmd, addr);
	if (!pte_present(*pte)) {
		return NULL;
	}

	return pte;
}

static ssize_t copy_flags(char *buf, size_t count, unsigned long addr, unsigned long pgprot, unsigned long ptefl)
{
	unsigned long ret;
	char tmp_buf[64];
	int tmp_len = 0;

	tmp_len = snprintf(tmp_buf, 64, "0x%lx 0x%lx 0x%lx\n", addr, pgprot, ptefl);

	if (tmp_len > count)
		return 0;

	ret = copy_to_user(buf, tmp_buf, tmp_len);

	return !ret ?  tmp_len : 0;
}

#ifdef ARM_CVIP
static ssize_t copy_meminfo(char *buf, size_t count)
{
	unsigned long ret;
	char tmp_buf[32];
	int tmp_len = 0;
	uint64_t mair_val, isar_val, pfr_val;

	asm volatile("mrs %0, mair_el1\t\n"
				"mrs %1, id_aa64isar1_el1\n\t"
				"mrs %2, id_aa64pfr1_el1"
				: "=r"(mair_val), "=r"(isar_val), "=r"(pfr_val));

	/* Pass FEAT_XS and FEAT_MTE2 flags along with MAIR value.
	 * Some of MAIR values depend on FEAT_XS or FEAT_MTE2 being present,
	 * otherwise memory is described as 'UNPREDICTABLE'.
	 * More on that:
	 * https://developer.arm.com/documentation/ddi0595/2021-03/AArch64-Registers/MAIR-EL1--Memory-Attribute-Indirection-Register--EL1-
	 */
	tmp_len = snprintf(tmp_buf, 32, "0x%llx %u %u\n", mair_val,
			(unsigned)((isar_val >> 56) & 1), (unsigned)(((pfr_val >> 8) & 0xf) == 2));

	if (tmp_len > count)
		return 0;

	ret = copy_to_user(buf, tmp_buf, tmp_len);

	return !ret ? tmp_len : 0;
}
#endif

static ssize_t app_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct page *page;
	struct vm_area_struct *vma = NULL;
	pte_t *pte = NULL;
	unsigned long addr = 0;
	ssize_t ret = 0, bytes;
	struct task_struct *task;
	struct mm_struct *mm;

	struct app_check *appc = (struct app_check *)filp->private_data;

	if (appc->read_done) {
		appc->read_done = 0;
		return 0;
	}

#ifdef ARM_CVIP
	if (appc->req_type == REQ_MEMINFO) {
		ret = copy_meminfo(buf, count);
		appc->read_done = 1;
		return ret;
	}
#endif

	task = !appc->pid ? current : get_pid_task(find_get_pid(appc->pid), PIDTYPE_PID);

	if (!task)
		return -EINVAL;

	mm = task->mm;


	if (!appc->addr)
		return -EFAULT;

	switch (appc->req_type) {
	case REQ_ADDR_VIRT:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,15,106)
		down_read(&mm->mmap_sem);
		vma = find_vma(mm, appc->addr);
		up_read(&mm->mmap_sem);
#else
		down_read(&mm->mmap_lock);
		vma = find_vma(mm, appc->addr);
		up_read(&mm->mmap_lock);
#endif
		pte = find_pte(mm, appc->addr);
		if (!vma || !pte) {
			return -EFAULT;
		}

		ret = copy_flags(buf, count, appc->addr, vma->vm_page_prot.pgprot, pte_flags(*pte));
		if (!ret)
			ret = -EINVAL;
		break;

	case REQ_ADDR_PHYS:

		page = phys_to_page(appc->addr);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,15,106)
		down_read(&mm->mmap_sem);
#else
                down_read(&mm->mmap_lock);
#endif
		vma = mm->mmap;
		while (vma) {
			addr = vma->vm_start;

			while (addr < vma->vm_end) {
				pte = find_pte(mm, addr);
				if (pte && page_to_pfn(page) == pte_pfn(*pte)) {

					if (ret > count)
						break;

					bytes = copy_flags(buf + ret, count - ret, addr, vma->vm_page_prot.pgprot, pte_flags(*pte));
					if (!bytes) {
						ret = -EINVAL;
						goto out;
					}

					ret += bytes;
					break;
				}
				addr += PAGE_SIZE;
			}
			vma = vma->vm_next;
		}
out:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,105)
		up_read(&mm->mmap_sem);
#else
		up_read(&mm->mmap_lock);
#endif
		if (!ret)
			ret = -EFAULT;

		break;

	case REQ_UNKNOWN:
	default:
		return -EINVAL;
	}

	appc->read_done = 1;
	return ret;
}

static ssize_t app_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int pid;
	unsigned long ret;
	unsigned long addr;
	char req_type;
	struct app_check *appc = (struct app_check*)filp->private_data;
	char tmp_buf[64] = { 0 };

	if (count >= 64) {
		return -EINVAL;
	}

	ret = copy_from_user(tmp_buf, buf, count);
	if (ret) {
		return -EINVAL;
	}

	if (sscanf(tmp_buf, "%c %d %lx", &req_type, &pid, &addr) != 3) {
#ifdef ARM_CVIP
		if(!strncmp(tmp_buf, "meminfo", 4)) {
			appc->req_type = REQ_MEMINFO;
			appc->read_done = 0;
			return count;
		}
#endif
		return -EINVAL;
	}

	switch (req_type) {
	case 'p':
		appc->req_type = REQ_ADDR_PHYS;
		break;
	case 'v':
		appc->req_type = REQ_ADDR_VIRT;
		break;
	default:
		appc->req_type = REQ_UNKNOWN;
		return -EINVAL;
	}

	appc->pid = pid;
	appc->addr = addr;
	appc->read_done = 0;
	return count;
}

static int app_open(struct inode *inode, struct file *filp)
{
	filp->private_data = kzalloc(sizeof(struct app_check), GFP_KERNEL);

	if (!filp->private_data)
		return -ENOMEM;

	return 0;
}

static int app_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static const struct file_operations app_ops = {
	.owner = THIS_MODULE,
	.read = app_read,
	.write = app_write,
	.open = app_open,
	.release = app_release
};

static struct dentry *app_dent;

void shregion_app_register(void)
{
	app_dent = debugfs_create_file("app", S_IFREG | S_IWUGO | S_IRUGO, NULL, NULL, &app_ops);
}

void shregion_app_deregister(void)
{
	debugfs_remove(app_dent);
}

#endif /* CONFIG_DEBUG_FS */

#ifdef ARM_CVIP
uint8_t ion_core_get_heap_type(uint8_t shr_id)
{
    switch (shr_id) {
        case SHREGION_DDR_SHARED:
            return ION_HEAP_X86_CVIP_SHARED_ID;

        case SHREGION_DDR_PRIVATE:
            return ION_HEAP_CVIP_DDR_PRIVATE_ID;

        case SHREGION_Q6_SCM_0:
            return ION_HEAP_CVIP_SRAM_Q6_S0_ID;

        case SHREGION_C5_NCM_0:
            return ION_HEAP_CVIP_SRAM_C5_S0_ID;

        case SHREGION_C5_NCM_1:
            return ION_HEAP_CVIP_SRAM_C5_S1_ID;

        case SHREGION_HYDRA_FMR2:
            return ION_HEAP_CVIP_HYDRA_FMR2_ID;

        case SHREGION_X86_SPCIE_FMR2_10:
            return ION_HEAP_CVIP_X86_SPCIE_FMR2_10_ID;

        case SHREGION_ISPIN_FMR5:
            return ION_HEAP_CVIP_ISPIN_FMR5_ID;

        case SHREGION_ISPOUT_FMR8:
            return ION_HEAP_CVIP_ISPOUT_FMR8_ID;

        case SHREGION_X86_RO_FMR10:
            return ION_HEAP_CVIP_X86_READONLY_FMR10_ID;

        case SHREGION_DDR_PRIVATE_CONTIG:
            return ION_HEAP_CVIP_DDR_PRIVATE_CONTIG_ID;

        case SHREGION_GSM:
            return ION_HEAP_CVIP_SRAM_GSM_ID;

        default:
            return INVALID_HEAP_ID;
    }
}

uint32_t ion_core_get_ion_flags(uint32_t shr_flags)
{
    uint32_t flag_bit;
    uint32_t flags = 0;
    int i = 0;

    while (shr_flags) {

        flag_bit = shr_flags & (1UL << i);
        shr_flags = shr_flags & ~flag_bit;

        ++i;

        switch (flag_bit) {
            case SHREGION_DEVICE_TYPE:
                flags |= ION_FLAG_CVIP_DEVICE_TYPE;
                continue;

            case SHREGION_DRAM_ALT_0:
                flags |= ION_FLAG_CVIP_DRAM_ALT0;
                continue;

            case SHREGION_DRAM_ALT_1:
                flags |= ION_FLAG_CVIP_DRAM_ALT1;
                continue;
        }
    }

    return flags;
}

uint32_t ion_core_get_cache_mask(uint32_t cache_mask) {
    uint32_t cache_bit;
    uint32_t ion_cache_mask = 0;
    int i = 0;

    while (cache_mask) {

        cache_bit = cache_mask & (1UL << i);
        cache_mask = cache_mask & ~cache_bit;

        ++i;

        switch (cache_bit) {
            case SHREGION_ARM_CACHED:
                ion_cache_mask |= ION_FLAG_CVIP_ARM_CACHED;
                continue;

            case SHREGION_CVIP_SLC_0_CACHED:
                ion_cache_mask |= ION_FLAG_CVIP_SLC_CACHED_0;
                continue;

            case SHREGION_CVIP_SLC_1_CACHED:
                ion_cache_mask |= ION_FLAG_CVIP_SLC_CACHED_1;
                continue;

            case SHREGION_CVIP_SLC_2_CACHED:
                ion_cache_mask |= ION_FLAG_CVIP_SLC_CACHED_2;
                continue;

            case SHREGION_CVIP_SLC_3_CACHED:
                ion_cache_mask |= ION_FLAG_CVIP_SLC_CACHED_3;
                continue;
        }
    }

    return ion_cache_mask;
}

int is_shregion_slc_cached(uint32_t cache_mask)
{
    if ((cache_mask & SHREGION_CVIP_SLC_0_CACHED) ||
        (cache_mask & SHREGION_CVIP_SLC_1_CACHED) ||
        (cache_mask & SHREGION_CVIP_SLC_2_CACHED) ||
        (cache_mask & SHREGION_CVIP_SLC_3_CACHED))
        return 1;
    return 0;
}

int is_x86_sharing_supported(uint8_t mem_type)
{
    if (mem_type == SHREGION_DDR_SHARED ||
            mem_type == SHREGION_X86_RO_FMR10 ||
            mem_type == SHREGION_X86_SPCIE_FMR2_10 ||
            mem_type == SHREGION_Q6_SCM_0)
        return 1;
    return 0;
}
#endif

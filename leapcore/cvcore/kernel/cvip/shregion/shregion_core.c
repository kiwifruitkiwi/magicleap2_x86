// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2020-2023
// Magic Leap, Inc. (COMPANY)
//

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/iommu.h>
#include <linux/profile.h>
#include <linux/iova.h>
#include <linux/dma-mapping.h>
#include <asm/page.h>
#include <linux/workqueue.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif

#include <linux/mero/cvip/biommu.h>
#include <linux/mero/cvip/cvip_dsp.h>
#include <linux/syscache.h>
#include <asm/cacheflush.h>
#include <linux/cvip/cvip_plt/cvip_plt.h>

#include "ion/ion.h"
#include "mero_xpc_types.h"
#include "cvcore_xchannel_map.h"
#include "cvcore_name_stub.h"
#include "mero_notifier_private.h"
#include "mero_smmu_private.h"

#include "shregion_common.h"
#include "shregion_status.h"
#include "shregion_types.h"
#include "shregion_internal.h"


#define CREATE_TRACE_POINTS
#include "shregion_trace.h"

static unsigned int mydebug;
module_param(mydebug, uint, 0644);
MODULE_PARM_DESC(mydebug, "Activates local debug info");

static uint gb_enabled = 0;
module_param(gb_enabled, uint, 0444);
MODULE_PARM_DESC(gb_enabled, "If set to 1 shregion guard bands are enabled");

#define MODULE_NAME "shregion"

#define MAX_BIOMMU_STR_LEN_BYTES		(64)

#define CVIP_SCM_0_BASE_ADDRESS  		(0xC4000000)
#define CVIP_NCM_0_BASE_ADDRESS			(0xC6000000)
#define CVIP_NCM_1_BASE_ADDRESS			(0xC6800000)

#define CDP_DCPRS_STREAM_ID				(0x40000)
#define CDP_DCPRS_POOL_BASE_IOADDR		(0xE0000000)
#define CDP_DCPRS_TOTAL_POOL_SIZE		(0x20000000) // 512M

#define S_INTF_SPCIE_BASE_IOADDR		(0x200000)
#define S_INTF_X86_BASE_IOADDR			(0x0)

#define DMAC_FIXED_STREAM_ID			(0x50000)

#define DSP_FIXED_DATA_SID				(0x800C)
#define DSP_FIXED_INST_SID				(0x800D)
#define DSP_FIXED_IDMA_SID				(0x800E)

#define SLC_CACHE_LINE_SIZE 32

#ifndef CONFIG_CVIP_BUILD_TYPE_USER
#define IS_DEBUGBUILD (true)
#else
#define IS_DEBUGBUILD (false)
#endif

#define dprintk(level, fmt, arg...)             \
  do {                                          \
    if (level <= mydebug)                       \
      pr_info(MODULE_NAME ": "fmt, ## arg);     \
  } while (0)

enum iommu_dma_cookie_type {
	IOMMU_DMA_IOVA_COOKIE,
	IOMMU_DMA_MSI_COOKIE,
};

static ssize_t dsp_maps_read_handler(struct file *filp, struct kobject *kobj,
				struct bin_attribute *attr, char *buf, loff_t offset,
				size_t max_size);

static struct bin_attribute bin_attr_dsp_maps = {
	.attr = {.name = "dsp_maps", .mode = (0444)},
	.size = 0,
	.read = dsp_maps_read_handler
};

struct iommu_dma_cookie {
	enum iommu_dma_cookie_type	type;
	union {
		/* Full allocator for IOMMU_DMA_IOVA_COOKIE */
		struct iova_domain	iovad;
		/* Trivial linear page allocator for IOMMU_DMA_MSI_COOKIE */
		dma_addr_t		msi_iova;
	};
	struct list_head		msi_page_list;

	/* Domain for flush queue callback; NULL if flush queue not in use */
	struct iommu_domain* fq_domain;
};

struct cdp_dcprs_meta_s {
	uint32_t shr_cdp_dcprs_ioaddr;
	struct iommu_domain *sid_domain;
	uint32_t mapped_size;
	pid_t creator_pid;
};

struct s_intf_spcie_meta_s {
	uint32_t shr_s_intf_spcie_ioaddr;
	struct iommu_domain *sid_domain;
	uint32_t mapped_size;
	pid_t creator_pid;
};

struct s_intf_sram_x86_meta_s {
	uint32_t shr_s_intf_sram_x86_ioaddr;
	struct iommu_domain *sid_domain;
	uint32_t mapped_size;
	pid_t creator_pid;
};

struct dmac_meta_s {
	uint32_t dmac_ioaddr;
	struct iova_domain* dmac_iovad;
	struct iommu_domain* dmac_sid_domain;
	struct iommu_dma_cookie* dmac_iommu_cookie;
	uint32_t dmac_mapped_size;
	uint32_t pid_ref_count;
	pid_t pid[SHREGION_MAX_REF_PIDS];
};

// structure private to this module.
struct shr_meta_priv {
	struct list_head list;
	struct shregion_metadata_drv meta_drv;
	void *k_virt_addr;
	uint32_t page_multiple_shr_size;
	dma_addr_t dma_handle;
	uint32_t num_of_ref_pids;
	// Used only for reference counting ion buffer.
	struct dma_buf *dmabuf;
	struct sg_table *sg_table;
	struct dma_buf_attachment *dma_attach;

	uint8_t guard_bands_config;
	bool violation_already_logged;

	// Stores PID and it's corresponding buffer fd.
	struct {
		pid_t pid;
		sid_t sid;
		int fd;
		uint64_t shr_uvaddr;
		uint8_t shr_uvaddr_valid;
		uint32_t shr_dsp_vaddr;
		struct iommu_domain *sid_domain;
		uint32_t mapped_size;
		int shared_meta_sid_idx;
	} pid_fd_info[SHREGION_MAX_REF_PIDS];

	struct cdp_dcprs_meta_s cdp_dcprs_meta;
	struct s_intf_spcie_meta_s s_intf_spcie_meta;
	struct s_intf_sram_x86_meta_s s_intf_sram_x86_meta;

	struct dmac_meta_s dmac_meta;
};

struct pid_meta_s {
	pid_t pid;
	uintptr_t cur_avail_dcache_wb_addr;
	uintptr_t cur_avail_noncache_addr;
	uint32_t cur_free_dcache_wb_pool_sz_bytes;
	uint32_t cur_free_noncache_pool_sz_bytes;
};

struct cdp_dcprs_pool_info_s {
	uintptr_t cur_avail_cdp_dcprs_addr;
	uint32_t cur_free_cdp_dcprs_pool_sz_bytes;
} cdp_dcprs_pool_info;

// structure holding tgid of all the
// processes interacting with shregion
// driver.
struct registered_processes_s {
	struct pid_meta_s pid_meta[SHREGION_MAX_REF_PIDS];
	int nr_of_valid_pids;
} registered_processes;

// Declare and init the head node of the linked list
LIST_HEAD(shregion_nodes);

static DEFINE_MUTEX(shr_meta_mutex);
static struct device dma0_device;
static struct dsp_shregion_metadata *dsp_shregion_meta = NULL;

static struct sid_meta_s {
	struct {
		uint32_t meta_virt_addr;
		pid_t pid;
		uint16_t sid;
		struct iommu_domain *sid_domain;
		uint32_t mapped_size;
		uint8_t entry_valid;
	} sid_virt_addr_map[MAX_DSP_INTERACTING_PIDS];
	int nr_of_entries;
} sid_meta;

static struct shared_dsp_meta_buf_info {
	struct dma_buf *dmabuf;
	struct sg_table *sg_table;
	struct dma_buf_attachment *dma_attach;
	int fd;
} dsp_buf_info = { NULL, NULL, NULL, -1 };

static struct x86_shared_shregion_meta *x86_shared_meta = NULL;
static phys_addr_t x86_shared_meta_phys_addr;

static int dsp_notifier_meta_pg_sz_multiple;
static int dsp_shregion_meta_pg_sz_multiple;

// forward declaration
static int register_tgid(void);
int kalloc_shregion(struct shregion_metadata *meta, void **vaddr);
static int ion_buffer_get_phys_addr(struct shr_meta_priv *meta_node, uint32_t offset);

static int violation_chk_period_s = 0; // disabled
static void check_shr_guard_bands_violation(struct work_struct *dummy);
static DECLARE_DELAYED_WORK(check_shr_work, check_shr_guard_bands_violation);

/* Assume nova is alive by default */
static uint8_t nova_present = 1;
static uint8_t cvip_only = 0;

static int shregion_guard_band_size(int flags, uint8_t mem_type);

#ifdef CONFIG_DEBUG_FS
static int nodes_show(struct seq_file *s, void *data)
{
	struct shr_meta_priv *meta_node;
	int i;

	seq_puts(s, "name\t\tsize\ttype\tcache_mask\tflags\trefs\tpid\tfd\n");
	seq_puts(s, "-------------------------------------------------------------\n");

	mutex_lock(&shr_meta_mutex);
	list_for_each_entry(meta_node, &shregion_nodes, list) {
		seq_printf(s, "%llx\t%u\t%d\t0x%x\t\t0x%x\t%d\n",
			   meta_node->meta_drv.meta.name,
			   meta_node->meta_drv.meta.size_bytes,
			   meta_node->meta_drv.meta.type,
			   meta_node->meta_drv.meta.cache_mask,
			   meta_node->meta_drv.meta.flags,
			   meta_node->num_of_ref_pids);

		for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {
			if (meta_node->pid_fd_info[i].pid <= 0)
				continue;
			seq_printf(s, "\t\t\t\t\t\t\t\t%d\t%d\n",
				   meta_node->pid_fd_info[i].pid,
				   meta_node->pid_fd_info[i].fd);
		}
	}
	mutex_unlock(&shr_meta_mutex);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(nodes);

static uint32_t dsp_maps_sid = -1;

static int dsp_maps_read(struct seq_file *s, void *data)
{
	int i, j;

	mutex_lock(&shr_meta_mutex);
	seq_printf(s, "%11s%19s%11s%11s\n", "SID", "NAME", "VADDR", "SIZE");
	seq_puts(s, "--------------------------------------------------\n");
	for (i = 0; i < dsp_shregion_meta->nr_of_sid_entries; i++) {
		for (j = 0; j < dsp_shregion_meta->sids_meta[i].nr_of_shr_entries; j++) {
			if (dsp_maps_sid > 0xffff || dsp_maps_sid == dsp_shregion_meta->sids_meta[i].sid)
				seq_printf(s, "0x%08x,0x%016llx,0x%08x,0x%08x\n",
					dsp_shregion_meta->sids_meta[i].sid,
					dsp_shregion_meta->sids_meta[i].shregion_entry[j].name,
					dsp_shregion_meta->sids_meta[i].shregion_entry[j].virt_addr,
					dsp_shregion_meta->sids_meta[i].shregion_entry[j].size_bytes);
		}
	}

	mutex_unlock(&shr_meta_mutex);
	return 0;
}

ssize_t dsp_maps_write(struct file *filp, const char __user *buf, size_t sz, loff_t *off)
{
	char sid_buf[10];
	uint32_t sid;

	if (sz > 10)
		return -EINVAL;

	if (copy_from_user(sid_buf, buf, sz))
		return -EFAULT;

	if (sscanf(sid_buf, "%x", &sid) != 1)
		return -EINVAL;

	mutex_lock(&shr_meta_mutex);
	dsp_maps_sid = sid;
	mutex_unlock(&shr_meta_mutex);

	return sz;
}

static int dsp_maps_open(struct inode *inode, struct file *file)
{
	return single_open(file, dsp_maps_read, inode->i_private);
}

static const struct file_operations dsp_maps_fops = {
	.owner		= THIS_MODULE,
	.open		= dsp_maps_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= dsp_maps_write,
};
#endif

#define BYTES_PER_MAP_LINE (53)
static ssize_t dsp_maps_read_handler(struct file *filp, struct kobject *kobj,
				struct bin_attribute *attr, char *buf, loff_t offset,
				size_t max_size)
{
	int i, j;
	int estimated_lines;
	ssize_t len;
	static char *dsp_maps_buf;
	int dsp_maps_buf_len;
	char *pos;
	char *header_sep = "----------------------------------------------------\n";

	mutex_lock(&shr_meta_mutex);
	if (offset == 0 && dsp_maps_buf != NULL) {    // A new read starting over
		kfree(dsp_maps_buf);
		dsp_maps_buf = NULL;
	}

	if (dsp_maps_buf == NULL) {
		estimated_lines = 0;
		for (i = 0; i < dsp_shregion_meta->nr_of_sid_entries; i++)
			for (j = 0; j < dsp_shregion_meta->sids_meta[i].nr_of_shr_entries; j++)
				estimated_lines++;
		dsp_maps_buf_len = estimated_lines * BYTES_PER_MAP_LINE;
		// Add space for headers plus some headroom
		dsp_maps_buf_len += 8 + (2 * strlen(header_sep)) + (dsp_maps_buf_len / 10);
		dsp_maps_buf = kmalloc(dsp_maps_buf_len, GFP_KERNEL);
		if (!dsp_maps_buf) {
			return -ENOMEM;
			mutex_unlock(&shr_meta_mutex);
		}

		pos = dsp_maps_buf;
		len = sprintf(pos, "%11s%19s%11s%11s\n", "SID", "NAME", "VADDR", "SIZE");
		pos += len;
		len = sprintf(pos, header_sep);
		pos += len;
		for (i = 0; i < dsp_shregion_meta->nr_of_sid_entries; i++) {
			for (j = 0; j < dsp_shregion_meta->sids_meta[i].nr_of_shr_entries; j++) {
				if (pos + BYTES_PER_MAP_LINE >= dsp_maps_buf + dsp_maps_buf_len)
					break;
				len = sprintf(pos, "0x%08x,0x%016llx,0x%08x,0x%08x\n",
					dsp_shregion_meta->sids_meta[i].sid,
					dsp_shregion_meta->sids_meta[i].shregion_entry[j].name,
					dsp_shregion_meta->sids_meta[i].shregion_entry[j].virt_addr,
					dsp_shregion_meta->sids_meta[i].shregion_entry[j].size_bytes);
				pos += len;
			}
		}
	}

	if (offset > strlen(dsp_maps_buf) || offset < 0)
		len = 0;
	else
		len = strlen(dsp_maps_buf) - offset;

	if (len > max_size)
		len = max_size;

	memcpy(buf, dsp_maps_buf + offset, len);

	if (len == 0) {
		kfree(dsp_maps_buf);
		dsp_maps_buf = NULL;
	}
	mutex_unlock(&shr_meta_mutex);

	return len;
}

// The `type` parameter to access_ok was removed in Linux 5.0.
// Previous releases included this parameter but ignored it.
#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
#define shregion_access_ok(type, addr, size) access_ok(addr, size)
#else
#define shregion_access_ok(type, addr, size) access_ok(type, addr, size)
#endif

static int shregion_open(struct inode *inode, struct file *file)
{
	return 0;
}

// Must be called while holding shr_meta_mutex lock.
// It returns true if the calling PID exists in pid_fd_info table
// of this shregion node.
// Returns the first free/vacant index (*idx) in the table if the pid wasn't
// found or else returns the idx of the pid found.
static bool this_pid_exists(struct shr_meta_priv *node, uint32_t *idx)
{
	int i;

	*idx = -1;

	// check if this PID entry already exists.
	for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {
		if (node->pid_fd_info[i].pid == current->tgid) {
			*idx = i;
			return true;
		}

		if ((*idx == -1) && (node->pid_fd_info[i].pid == -1))
			*idx = i;
	}

	return false;
}

// Must be called while holding shr_meta_mutex lock.
static void get_buf_fd(struct shregion_metadata_drv *meta_drv,
		       struct shr_meta_priv *meta_node)
{
	bool pid_entry_found = false;
	int idx;

	pid_entry_found = this_pid_exists(meta_node, &idx);

	// Does it not exist and did shr ref reached
	// the max already?
	if (!pid_entry_found && (idx == -1)) {
		dprintk(0, "Exceeding max shr refs(%d) for 0x%llx.\n",
				SHREGION_MAX_REF_PIDS, meta_node->meta_drv.meta.name);
		meta_drv->status =
			SHREGION_MEM_MAX_REF_ERROR;
		return;
	}

	// Was this PID already allocated a fd?
	if (pid_entry_found) {
		// Return the already existing fd.
		meta_drv->buf_fd = meta_node->pid_fd_info[idx].fd;
	} else {
		// If it wasn't allocated one,
		// allocate a new one.
		meta_drv->buf_fd = dma_buf_fd(meta_node->dmabuf, O_CLOEXEC);
		if (meta_drv->buf_fd < 0) {
			dprintk(0, "shr(name=%llx) dma_buf_fd(fd=%d) failed.\n",
				meta_drv->meta.name, meta_drv->buf_fd);
			meta_drv->status = SHREGION_MEM_GET_BUFFER_FD_FAILED;
			return;
		}
		// dma_buf_fd() doesn't increment the ref count to this
		// buffer. So, we hack to increment the buf count.
		dma_buf_get(meta_drv->buf_fd);

		// Register the PID and corresponding fd.
		meta_node->pid_fd_info[idx].pid = current->tgid;
		meta_node->pid_fd_info[idx].fd = meta_drv->buf_fd;
		meta_node->num_of_ref_pids++;
	}

	meta_drv->status = SHREGION_MEM_SHAREABLE;
}

static void proc_shr_uvaddr_req(struct shr_meta_priv *meta_node,
				struct shr_prexist_meta *spm)
{
	int i;

	for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {

		if (meta_node->pid_fd_info[i].pid == current->tgid) {

			if (meta_node->pid_fd_info[i].shr_uvaddr_valid) {
				spm->shr_uvaddr = meta_node->pid_fd_info[i].shr_uvaddr;
				spm->shr_uvaddr_valid = meta_node->pid_fd_info[i].shr_uvaddr_valid;
			} else {
				if (spm->cmd == SET_SHR_UVADDR) {
					meta_node->pid_fd_info[i].shr_uvaddr_valid = 1;
					meta_node->pid_fd_info[i].shr_uvaddr = spm->shr_uvaddr;
				}
			}
			return;
		}
	}
}

static int shregion_slc_sync(struct shr_slc_sync_meta *slc_meta)
{
	int ret = 0;
	phys_addr_t phys_addr;
	uint32_t offset;
	uint32_t page_offset;
	uint32_t sync_size = 0;
	uint32_t bytes = slc_meta->size_bytes;
	struct shr_meta_priv *meta_node = NULL;
	struct shr_prexist_meta spm = { 0 };
	int guard_band_size;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == slc_meta->name) {
			break;
		}
	}

	if (meta_node->meta_drv.meta.name != slc_meta->name) {
		dprintk(1, "Unable to find shregion %llx\n", slc_meta->name);
		mutex_unlock(&shr_meta_mutex);
		return -EINVAL;
	}

	guard_band_size = shregion_guard_band_size(meta_node->meta_drv.meta.flags,
			meta_node->meta_drv.meta.type);

	proc_shr_uvaddr_req(meta_node, &spm);
	if (!spm.shr_uvaddr_valid) {
		dprintk(1, "Shregion %llx is not mapped by %d\n", slc_meta->name, current->tgid);
		mutex_unlock(&shr_meta_mutex);
		return -EINVAL;
	}

	if (slc_meta->start_addr >= spm.shr_uvaddr + guard_band_size &&
		slc_meta->start_addr < spm.shr_uvaddr + meta_node->meta_drv.meta.size_bytes -
		guard_band_size) {
		offset = slc_meta->start_addr - spm.shr_uvaddr;
	} else {
		dprintk(1, "Invalid start addr for %llx\n", slc_meta->name);
		mutex_unlock(&shr_meta_mutex);
		return -EINVAL;
	}

	if (offset + slc_meta->size_bytes > meta_node->meta_drv.meta.size_bytes - guard_band_size) {
		dprintk(1, "Invalid range for %llx\n", slc_meta->name);
		mutex_unlock(&shr_meta_mutex);
		return -EINVAL;
	}

	while (bytes) {
		ret = ion_buffer_get_phys_addr(meta_node, offset);
		if (ret) {
			dprintk(0, "Unable to get phys addr for %llx at offset %d\n",
					slc_meta->name, (int)offset);
			mutex_unlock(&shr_meta_mutex);
			return -EFAULT;
		}

		phys_addr = meta_node->meta_drv.phys_addr & ~(SLC_CACHE_LINE_SIZE - 1);
		page_offset = meta_node->meta_drv.phys_addr & (PAGE_SIZE - 1);

		if (bytes >= PAGE_SIZE - page_offset) {
			sync_size = PAGE_SIZE - page_offset;
			bytes -= PAGE_SIZE - page_offset;
		} else {
			sync_size = bytes;
			bytes = 0;
		}

		offset += sync_size;
		sync_size += (SLC_CACHE_LINE_SIZE - 1);
		sync_size &= ~(SLC_CACHE_LINE_SIZE - 1);

		if (slc_meta->flags & SHREGION_SLC_SYNC_CLEAN) {
			ret = syscache_clean_range(phys_addr, phys_addr + sync_size);

		} else if (slc_meta->flags & SHREGION_SLC_SYNC_FLUSH) {
			ret = syscache_flush_range(phys_addr, phys_addr + sync_size);

		} else if (slc_meta->flags & SHREGION_SLC_SYNC_INVAL) {
			ret = syscache_inv_range(phys_addr, phys_addr + sync_size);
		} else {
			dprintk(1, "Invalid slc operation for %llx\n", slc_meta->name);
			mutex_unlock(&shr_meta_mutex);
			return -EINVAL;
		}

		if (ret) {
			dprintk(0, "Failed to sync slc for %llx\n", slc_meta->name);
			break;
		}
	}

	mutex_unlock(&shr_meta_mutex);
	return ret;
}

extern int xpc_register_command_handler(XpcChannel channel,
	XpcCommandHandler handler,
	XpcHandlerArgs args);

extern int pl330_share_dma_device(struct device *sh_dev);

static struct device * get_dma_device(void)
{
	static uint8_t init_once = 0;
	int retval;

	if (!init_once) {
		retval = pl330_share_dma_device(&dma0_device);
		if (retval) {
			dprintk(0, "Failed to get dma device\n");
			return NULL;
		}
		init_once = 1;
	}

	return &dma0_device;
}

int shregion_arm_cache_invalidate(uint64_t shr_name, struct dma_buf *dmabuf)
{
	int i;
	struct ion_buffer *ion_buf;
	struct sg_table *table;
	struct page *page;
	struct scatterlist *sg;
	struct shr_meta_priv *meta_node;

	if (!dmabuf) {
		list_for_each_entry(meta_node, &shregion_nodes, list) {
			if (shr_name == meta_node->meta_drv.meta.name) {
				dmabuf = meta_node->dmabuf;
				break;
			}
		}
	}

	if (!dmabuf) {
		dprintk(0, "%s: dma_buf not found for name=%llx\n", __func__, shr_name);
		return -EINVAL;
	}

	ion_buf = dmabuf->priv;
	table = ion_buf->sg_table;

	for_each_sg(table->sgl, sg, table->nents, i) {
		page = sg_page(sg);
		invalidate_dcache_range(page_address(page), PAGE_ALIGN(sg->length));
	}

	return 0;
}
EXPORT_SYMBOL(shregion_arm_cache_invalidate);

// Invalidate a range in the shregion from:
//   [shregion_base_address + offset, shregion_base_address + offset + size)
// The invalidate operation works on cache line granularity, so any cache lines
// within the range will be invalidated.
// If the start or end address are not cache line aligned(e.g the address falls
// in the middle of a cache line), that line will also be cleaned before being
// invalidated to prevent data loss.
int shregion_arm_cache_invalidate_range(uint64_t shr_name, struct dma_buf *dmabuf,
		unsigned long offset, size_t size)
{
	struct ion_buffer *ion_buf;
	struct sg_table *table;
	struct scatterlist *sg;
	struct shr_meta_priv *meta_node;

	if (!dmabuf) {
		list_for_each_entry(meta_node, &shregion_nodes, list) {
			if (shr_name == meta_node->meta_drv.meta.name) {
				dmabuf = meta_node->dmabuf;
				break;
			}
		}
	}

	if (!dmabuf) {
		dprintk(0, "%s: dma_buf not found for name=%llx\n", __func__, shr_name);
		return -EINVAL;
	}

	ion_buf = dmabuf->priv;
	table = ion_buf->sg_table;

	// The shregion can potentially have multiple sg entries if the physical
	// addresses are not contiguous, but the virtual address of the shregion
	// will be contiguous. So we are going to use the first sg entry to get
	// the virtual address and apply the offset and size to that.
	sg = table->sgl;
	invalidate_dcache_range(sg_virt(sg) + offset, size);

	return 0;
}
EXPORT_SYMBOL(shregion_arm_cache_invalidate_range);

/* shregion_smmu_map() needs to call this while already holding a lock */
static int shregion_register_sid_meta_unlocked(unsigned long arg)
{
	int retval, i;
	int nr_of_entries;
	bool entry_present_but_invalid = false;
	uint16_t user_sid;

	char biommu_sid_str[MAX_BIOMMU_STR_LEN_BYTES];
	struct iommu_domain *sid_domain = NULL;
	uint32_t mapped_size = 0;
	int iommu_prot;

	user_sid = (uint32_t)arg;

	nr_of_entries = sid_meta.nr_of_entries;

	for (i = 0; i < nr_of_entries; i++) {
		if (sid_meta.sid_virt_addr_map[i].sid == user_sid) {
			if(sid_meta.sid_virt_addr_map[i].entry_valid == ENTRY_IS_VALID) {
				dprintk(2, "%s called for sid=0x%x, EXISTS", __func__, user_sid);
				return 0;
			}
			entry_present_but_invalid = true;
			break;
		}
	}

	// this registration here happens only once per userspace process.
	if (nr_of_entries >= MAX_DSP_INTERACTING_PIDS && !entry_present_but_invalid) {
		dprintk(0, "Exceeding max dsp interacting pids limit.\n");
		return -EDQUOT;
	}

	// Attach this buffer to desired streamID device
	snprintf(biommu_sid_str, MAX_BIOMMU_STR_LEN_BYTES, "biommu!vdsp@%x", user_sid);

	if (entry_present_but_invalid) {

		nr_of_entries = i;

	} else {

		nr_of_entries = dsp_shregion_meta->nr_of_sid_entries;

		// check here again
		if (nr_of_entries >= MAX_DSP_INTERACTING_PIDS) {
			dprintk(0, "mmap: Exceeding nr of sids limit.\n");
			return -EDQUOT;
		}

		wmb();

		++(sid_meta.nr_of_entries);
		++(dsp_shregion_meta->nr_of_sid_entries);
	}

	dsp_shregion_meta->sids_meta[nr_of_entries].sid = user_sid;
	dsp_shregion_meta->sids_meta[nr_of_entries].nr_of_shr_entries = 0;
	dsp_shregion_meta->sids_meta[nr_of_entries].meta_init_done = META_INIT_MAGIC_NUM;

	sid_meta.sid_virt_addr_map[nr_of_entries].sid = user_sid;
	sid_meta.sid_virt_addr_map[nr_of_entries].pid = current->tgid;
	sid_meta.sid_virt_addr_map[nr_of_entries].meta_virt_addr =
		DSP_SHREGION_META_FIXED_VADDR;

	wmb();

	sid_meta.sid_virt_addr_map[nr_of_entries].entry_valid = ENTRY_IS_VALID;

	iommu_prot = IOMMU_READ;

	sid_domain = biommu_get_domain_byname(biommu_sid_str);
	if (IS_ERR(sid_domain)) {
		dprintk(0, "Failed to get device for sid=0x%0x\n", user_sid);
		retval = PTR_ERR(sid_domain);
		return retval;
	}

	mapped_size = iommu_map_sg(sid_domain,
				(unsigned long)DSP_SHREGION_META_FIXED_VADDR,
				dsp_buf_info.sg_table->sgl, dsp_buf_info.sg_table->nents,
				iommu_prot);
	if (mapped_size <= 0) {
		dprintk(0, "Failed to create iommu map for sid=0x%x,vaddr=%x,sz=%d bytes\n",
			user_sid, DSP_SHREGION_META_FIXED_VADDR,
			(int32_t)sizeof(struct dsp_shregion_metadata));
		return -EFAULT;
	}

	dprintk(1, "sid(0x%x): created shregion meta mapping at %x,sz=%d bytes\n",
		user_sid, DSP_SHREGION_META_FIXED_VADDR,
		(int32_t)sizeof(struct dsp_shregion_metadata));

	sid_meta.sid_virt_addr_map[nr_of_entries].sid_domain = sid_domain;
	sid_meta.sid_virt_addr_map[nr_of_entries].mapped_size = mapped_size;

	return 0;
}

static int shregion_register_sid_meta(unsigned long arg)
{
	int ret;

	mutex_lock(&shr_meta_mutex);
	ret = shregion_register_sid_meta_unlocked(arg);
	mutex_unlock(&shr_meta_mutex);
	return ret;
}

static int get_address_from_virtual_addr_pool(
	struct dsp_shregion_metadata_drv *dsp_meta_drv,
	uint32_t *addr)
{
	uint32_t size_bytes_pg_sz_multiple;
	int i;

	for (i = 0; i < registered_processes.nr_of_valid_pids; ++i) {
		if (registered_processes.pid_meta[i].pid == current->tgid)
			break;
	}

	if (i >= registered_processes.nr_of_valid_pids) {
		dprintk(0, "Failed to find pid(%d) registration\n", current->tgid);
		return -1;
	}

	if (dsp_meta_drv->shr_size_bytes % PAGE_SIZE)
		size_bytes_pg_sz_multiple =
		(((dsp_meta_drv->shr_size_bytes / PAGE_SIZE) + 1) * PAGE_SIZE);
	else
		size_bytes_pg_sz_multiple = dsp_meta_drv->shr_size_bytes;

	if (dsp_meta_drv->shr_cache_mask & SHREGION_DSP_DATA_WB_CACHED) {
		if (size_bytes_pg_sz_multiple >
			registered_processes.pid_meta[i].cur_free_dcache_wb_pool_sz_bytes) {
			dprintk(0, "shregion dsp dcache addr pool(sz=%d bytes) exhausted\n",
                    CVCORE_DSP_DATA_REGION0_SIZE);
			dprintk(0, "Map total shregion size in your proc(%d) to not exceed this pool limit\n",
				current->tgid);
			return -1;
		}

		*addr = (uint32_t)registered_processes.pid_meta[i].cur_avail_dcache_wb_addr;
		registered_processes.pid_meta[i].cur_avail_dcache_wb_addr +=
		size_bytes_pg_sz_multiple;
		registered_processes.pid_meta[i].cur_free_dcache_wb_pool_sz_bytes -=
		size_bytes_pg_sz_multiple;

	} else {

		if (size_bytes_pg_sz_multiple >
			registered_processes.pid_meta[i].cur_free_noncache_pool_sz_bytes) {
			dprintk(0, "shregion dsp non-cached addr pool(sz=%d bytes) exhausted\n",
                    CVCORE_DSP_DATA_REGION1_SIZE - dsp_notifier_meta_pg_sz_multiple -
                    dsp_shregion_meta_pg_sz_multiple);
			dprintk(0, "Map total shregion size in your proc(%d) to not exceed this pool limit\n",
				current->tgid);
			return -1;
		}

		*addr = (uint32_t)registered_processes.pid_meta[i].cur_avail_noncache_addr;
		registered_processes.pid_meta[i].cur_avail_noncache_addr +=
		size_bytes_pg_sz_multiple;
		registered_processes.pid_meta[i].cur_free_noncache_pool_sz_bytes -=
		size_bytes_pg_sz_multiple;
	}

	return 0;
}

static uint8_t get_shregion_guard_bands_status(void)
{
	static int shregion_guard_bands_status = -1;
	char shr_gb_dbg_val[32];
	char shr_gb_chk_interval_s[32];
	long chk_interval_s;

	if (shregion_guard_bands_status == -1) {
		cvip_plt_property_get_safe("persist.shrdbg.gb", shr_gb_dbg_val,
						sizeof(shr_gb_dbg_val), "0");
		if (shr_gb_dbg_val[0] == '1') {
			gb_enabled = 1;
			shregion_guard_bands_status = GUARD_BANDS_ENABLED;
			cvip_plt_property_get_safe("persist.shrdbg.chk_interval_s", shr_gb_chk_interval_s,
						sizeof(shr_gb_dbg_val), "0");

			if (!kstrtol(shr_gb_chk_interval_s, 0, &chk_interval_s) && chk_interval_s > 0) {
				violation_chk_period_s = (int)chk_interval_s;
			}

			if (violation_chk_period_s)
				dprintk(0, "shregion guard band violation check period set to %ds\n",
					violation_chk_period_s);
			else
				dprintk(0, "shregion guard band violation periodic check disabled\n");
		} else {
			shregion_guard_bands_status = GUARD_BANDS_DISABLED;
		}
	}
	return shregion_guard_bands_status;
}

static int shregion_guard_band_size(int flags, uint8_t mem_type)
{
	if (get_shregion_guard_bands_status() == GUARD_BANDS_DISABLED)
		return 0;

	if (flags & SHREGION_FORCE_DISABLE_GUARD_BANDS)
		return 0;

	if ((flags & SHREGION_CVIP_SRAM_SPCIE_SHAREABLE) ||
	    (flags & SHREGION_DSP_CODE) ||
	    (flags & SHREGION_DSP_MAP_ALL_STREAM_IDS) ||
	    (flags & SHREGION_FIXED_DMAC_IO_VADDR) ||
	    (flags & SHREGION_DMAC_SHAREABLE) ||
	    (flags & SHREGION_CDP_AND_DCPRS_SHAREABLE) ||
	    (mem_type == SHREGION_ISPOUT_FMR8) ||
	    (mem_type == SHREGION_ISPIN_FMR5) ||
	    (mem_type == SHREGION_GSM)) {
		return 0;
	}

	return SHREGION_GUARD_BAND_SIZE_BYTES;
}

/* shregion_smmu_map() needs to call this while already holding a lock */
static int dsp_shregion_metadata_update_unlocked(struct dsp_shregion_metadata_drv *dsp_shr_meta_drv)
{
	int nr_of_entries;
	int i, retval = 0;
	int sm_idx = -1;
	struct shr_meta_priv *meta_node = NULL;

	char biommu_sid_str[MAX_BIOMMU_STR_LEN_BYTES];
	struct iommu_domain *sid_domain = NULL;
	uint32_t mapped_size = 0;
	int iommu_prot;
	uint32_t shr_dsp_vaddr = 0;

	nr_of_entries = sid_meta.nr_of_entries;

	for (i = 0; i < nr_of_entries; i++) {
		if (sid_meta.sid_virt_addr_map[i].sid == dsp_shr_meta_drv->sid) {
			sm_idx = i;
			break;
		}
	}

	if (sm_idx == -1) {
		dprintk(0, "stream id(0x%x) not registered.\n", dsp_shr_meta_drv->sid);
		return -ENOENT;
	}

	if (!dsp_shregion_meta) {
		dprintk(0, "dsp_shregion_meta isn't initialized yet");
		return -EPERM;
	}

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == dsp_shr_meta_drv->shr_name)
			break;
	}

	if (!meta_node) {
		dprintk(0, "%s: shr(%llx) not registered\n", __func__,
			dsp_shr_meta_drv->shr_name);
		return -ENOENT;
	}

	for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {
		if (meta_node->pid_fd_info[i].pid == current->tgid) {
			if (meta_node->pid_fd_info[i].shr_dsp_vaddr) {
				if (dsp_shr_meta_drv->sid == DSP_FIXED_INST_SID ||
						dsp_shr_meta_drv->sid == DSP_FIXED_IDMA_SID) {
					shr_dsp_vaddr = meta_node->pid_fd_info[i].shr_dsp_vaddr;
					break;
				}
				return 0;
			}
			break;
		}
	}

	if (!shr_dsp_vaddr)
		retval = get_address_from_virtual_addr_pool(dsp_shr_meta_drv, &shr_dsp_vaddr);

	if (retval != 0) {
		dprintk(0, "Failed to get dsp shareable vaddr for shr(name=%llx)",
			dsp_shr_meta_drv->shr_name);
		return -1;
	}

	nr_of_entries = dsp_shregion_meta->sids_meta[sm_idx].nr_of_shr_entries;

	if (nr_of_entries < MAX_DSP_SHARED_SHR_ENTRIES) {

		dsp_shregion_meta->sids_meta[sm_idx].shregion_entry[nr_of_entries].name =
			dsp_shr_meta_drv->shr_name;
		dsp_shregion_meta->sids_meta[sm_idx].shregion_entry[nr_of_entries].virt_addr =
			shr_dsp_vaddr + shregion_guard_band_size(meta_node->meta_drv.meta.flags,
								 meta_node->meta_drv.meta.type);
		dsp_shregion_meta->sids_meta[sm_idx].shregion_entry[nr_of_entries].size_bytes =
			dsp_shr_meta_drv->shr_size_bytes -
			(NR_OF_GUARD_BANDS_PER_SHREGION *
                         shregion_guard_band_size(meta_node->meta_drv.meta.flags,
						  meta_node->meta_drv.meta.type));
		dsp_shregion_meta->sids_meta[sm_idx].shregion_entry[nr_of_entries].dmac_iova =
			meta_node->dmac_meta.dmac_ioaddr;

		wmb();

		++(dsp_shregion_meta->sids_meta[sm_idx].nr_of_shr_entries);

	} else {
		dprintk(0, "Exceeding max dsp shregions limit.\n");
		return -EDQUOT;
	}

	snprintf(biommu_sid_str, MAX_BIOMMU_STR_LEN_BYTES, "biommu!vdsp@%x",
		dsp_shr_meta_drv->sid);

	sid_domain = biommu_get_domain_byname(biommu_sid_str);
	if (IS_ERR(sid_domain)) {
		dprintk(0, "Failed to get device for sid=0x%x\n", dsp_shr_meta_drv->sid);
		return PTR_ERR(sid_domain);
	}

	if (!meta_node->sg_table) {
		dprintk(0, "sg_table for shr=%llx (sid=0x%x) can't be null",
			dsp_shr_meta_drv->shr_name,
			dsp_shr_meta_drv->sid);
		return -EINVAL;
	}

	iommu_prot = IOMMU_READ | IOMMU_WRITE;

	mapped_size = iommu_map_sg(sid_domain,
				(unsigned long)shr_dsp_vaddr,
				meta_node->sg_table->sgl, meta_node->sg_table->nents,
				iommu_prot);
	if (mapped_size <= 0) {
		dprintk(0, "Failed to create mmu map for(%llx) sid=0x%x,v=%x,sz=%d bytes\n",
			dsp_shr_meta_drv->shr_name, dsp_shr_meta_drv->sid,
			shr_dsp_vaddr, meta_node->meta_drv.meta.size_bytes);
		return -EFAULT;
	}

	/*
	 * Successfully mapped dsp shregion so get dma buf reference
	 * to keep it around until shregion_smmu_unmap() puts it back.
	 */
	dma_buf_get(dsp_shr_meta_drv->buf_fd);

	dprintk(3, "%s:(%d,%d): iommu map(n=%llx,sid=0x%x) p=%llx,v=%x,sz=%d bytes\n", __func__,
		current->pid, current->tgid, dsp_shr_meta_drv->shr_name, dsp_shr_meta_drv->sid,
		sg_phys(meta_node->sg_table->sgl), shr_dsp_vaddr,
		meta_node->meta_drv.meta.size_bytes);

	if (!meta_node->num_of_ref_pids) {
		dprintk(0, "num_of_ref_pids for shr=%llx can't be 0",
			dsp_shr_meta_drv->shr_name);
		return -EINVAL;
	}

	// register domain, mapped size, virtual addr for unmap during
	// cleanup on pid's death.
	for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {

		if (meta_node->pid_fd_info[i].pid == current->tgid) {
			if (meta_node->pid_fd_info[i].shr_dsp_vaddr == shr_dsp_vaddr) {
				meta_node->pid_fd_info[meta_node->num_of_ref_pids].sid = dsp_shr_meta_drv->sid;
				meta_node->pid_fd_info[meta_node->num_of_ref_pids].pid = current->tgid;
				meta_node->pid_fd_info[meta_node->num_of_ref_pids].fd = meta_node->pid_fd_info[i].fd;
				meta_node->pid_fd_info[meta_node->num_of_ref_pids].shr_dsp_vaddr = shr_dsp_vaddr;
				meta_node->pid_fd_info[meta_node->num_of_ref_pids].sid_domain = sid_domain;
				meta_node->pid_fd_info[meta_node->num_of_ref_pids].mapped_size = mapped_size;
				meta_node->pid_fd_info[meta_node->num_of_ref_pids].shared_meta_sid_idx = sm_idx;
				meta_node->num_of_ref_pids++;
				break;
			}

			meta_node->pid_fd_info[i].sid = dsp_shr_meta_drv->sid;
			meta_node->pid_fd_info[i].shr_dsp_vaddr = shr_dsp_vaddr;
			meta_node->pid_fd_info[i].sid_domain = sid_domain;
			meta_node->pid_fd_info[i].mapped_size = mapped_size;
			meta_node->pid_fd_info[i].shared_meta_sid_idx = sm_idx;
			break;
		}
	}

	if (i == SHREGION_MAX_REF_PIDS) {
		dprintk(0, "update failed for shr=%llx, pid=%d not found",
			dsp_shr_meta_drv->shr_name,
			current->tgid);
		return -ESRCH;
	}

	return 0;
}

static int dsp_shregion_metadata_update(struct dsp_shregion_metadata_drv *dsp_shr_meta_drv)
{
	int ret;

	mutex_lock(&shr_meta_mutex);
	ret = dsp_shregion_metadata_update_unlocked(dsp_shr_meta_drv);
	mutex_unlock(&shr_meta_mutex);
	return ret;
}

static int alloc_dsp_dma_buf_resources(void)
{
	int retval = 0;
	void *vaddr = NULL;
	struct device *iommudev_ret = NULL;
	char biommu_sid_str[MAX_BIOMMU_STR_LEN_BYTES];

	// Need to attach this dma_buf to some random stream id. Let that be
	// 0.
	snprintf(biommu_sid_str, MAX_BIOMMU_STR_LEN_BYTES, "biommu!vdsp@%x", 0);

	dsp_buf_info.fd = ion_alloc(sizeof(struct dsp_shregion_metadata),
		1 << ION_HEAP_CVIP_DDR_PRIVATE_ID, 0);
	if (dsp_buf_info.fd < 0) {
		dprintk(0, "Failed to allocated ION private ddr memory.\n");
		return -ENOMEM;
	}

	dsp_buf_info.dmabuf = dma_buf_get(dsp_buf_info.fd);
	if (IS_ERR(dsp_buf_info.dmabuf)) {
		dprintk(0, "Invalid fd unable to get dma buffer.\n");
		ksys_close(dsp_buf_info.fd);
		return -EINVAL;
	}

	// Invalidate before use
	retval = shregion_arm_cache_invalidate(0, dsp_buf_info.dmabuf);
	if (retval) {
		dprintk(0, "arm_cache_invalidate failed for private ddr, ret=%d\n",
			retval);
		dma_buf_put(dsp_buf_info.dmabuf);
		ksys_close(dsp_buf_info.fd);
		return retval;
	}

	vaddr = dma_buf_kmap(dsp_buf_info.dmabuf, 0);
	if (!vaddr) {
		dprintk(0, "Failed kmap for private ddr dmabuf\n");
		dma_buf_put(dsp_buf_info.dmabuf);
		ksys_close(dsp_buf_info.fd);
		return PTR_ERR(vaddr);
	}

	dsp_shregion_meta = (struct dsp_shregion_metadata *)vaddr;

	iommudev_ret = biommu_get_domain_dev_byname(biommu_sid_str);
	if (IS_ERR(iommudev_ret)) {
		dprintk(0, "Failed to get device for DSP.\n");
		retval = PTR_ERR(iommudev_ret);
		return retval;
	}

	dsp_buf_info.dma_attach = dma_buf_attach(dsp_buf_info.dmabuf, iommudev_ret);
	if (IS_ERR(dsp_buf_info.dma_attach)) {
		dprintk(0, "Failed to attach dsp to dmabuf.\n");
		retval = PTR_ERR(dsp_buf_info.dma_attach);
		return retval;
	}

	// presume all 64 bits are dma capable.
	iommudev_ret->coherent_dma_mask = DMA_BIT_MASK(64);
	iommudev_ret->dma_mask = &iommudev_ret->coherent_dma_mask;

	dsp_buf_info.sg_table = dma_buf_map_attachment(dsp_buf_info.dma_attach,
		DMA_BIDIRECTIONAL);
	if (IS_ERR(dsp_buf_info.sg_table)) {
		dprintk(0, "Failed to get sg for shr meta dmabuf.\n");
		retval = PTR_ERR(dsp_buf_info.sg_table);
		dma_buf_detach(dsp_buf_info.dmabuf, dsp_buf_info.dma_attach);
		return retval;
	}

	memset((void *)dsp_shregion_meta, 0, sizeof(struct dsp_shregion_metadata));

	return retval;
}

static void init_cdp_dcprs_pool(void)
{
	cdp_dcprs_pool_info.cur_avail_cdp_dcprs_addr =
		CDP_DCPRS_POOL_BASE_IOADDR;
	cdp_dcprs_pool_info.cur_free_cdp_dcprs_pool_sz_bytes =
		CDP_DCPRS_TOTAL_POOL_SIZE;
}

static int get_cdp_dcprs_addr_from_ioaddr_pool(struct shregion_metadata_drv *meta_drv,
	uint32_t *addr)
{
	uint32_t size_bytes_pg_sz_multiple;

	if (meta_drv->meta.size_bytes % PAGE_SIZE)
		size_bytes_pg_sz_multiple =
		(((meta_drv->meta.size_bytes / PAGE_SIZE) + 1) * PAGE_SIZE);
	else
		size_bytes_pg_sz_multiple = meta_drv->meta.size_bytes;

	if (size_bytes_pg_sz_multiple >
		cdp_dcprs_pool_info.cur_free_cdp_dcprs_pool_sz_bytes) {
		dprintk(0, "shregion cdp dcprs addr pool(sz=%d bytes) exhausted\n",
                CDP_DCPRS_TOTAL_POOL_SIZE);
		return -1;
	}

	*addr = (uint32_t)cdp_dcprs_pool_info.cur_avail_cdp_dcprs_addr;
	cdp_dcprs_pool_info.cur_avail_cdp_dcprs_addr += size_bytes_pg_sz_multiple;
	cdp_dcprs_pool_info.cur_free_cdp_dcprs_pool_sz_bytes -= size_bytes_pg_sz_multiple;

	return 0;
}

static int get_shr_dmac_ioaddr(struct dmac_shregion_ioaddr_meta* dmac_ioaddr_meta)
{
	struct shr_meta_priv* meta_node = NULL;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == dmac_ioaddr_meta->shr_name)
			break;
	}

	if (!meta_node) {
		dprintk(0, "%s: shr(%llx) not found\n", __func__, dmac_ioaddr_meta->shr_name);
		mutex_unlock(&shr_meta_mutex);
		return -ENOENT;
	}


	if (meta_node->dmac_meta.dmac_ioaddr) {
		dmac_ioaddr_meta->shr_dmac_ioaddr = meta_node->dmac_meta.dmac_ioaddr;
		mutex_unlock(&shr_meta_mutex);
		return 0;
	}

	dprintk(0, "dmac_ioaddr for (%llx) is null.\n", dmac_ioaddr_meta->shr_name);

	mutex_unlock(&shr_meta_mutex);

	return -EINVAL;
}

static int get_s_intf_spcie_ioaddr(struct shr_meta_priv* meta_node,
	struct shregion_metadata_drv *meta_drv, uint32_t *addr)
{
	int ret;

	ret = ion_buffer_get_phys_addr(meta_node, 0);
	if (ret)
		return -1;

	*addr =  S_INTF_SPCIE_BASE_IOADDR + (meta_node->meta_drv.phys_addr - CVIP_SCM_0_BASE_ADDRESS);

	return 0;
}

int get_shregion_dmac_ioaddr(uint64_t shr_name, ShregionDmacIOaddr* shr_dmac_ioaddr)
{
	int ret;
	struct dmac_shregion_ioaddr_meta ioaddr_meta;

	ioaddr_meta.shr_name = shr_name;

	ret = get_shr_dmac_ioaddr(&ioaddr_meta);
	if (ret != 0) {
		dprintk(0, "%s failed with err=%d\n", __func__, ret);
		return ret;
	}

	*shr_dmac_ioaddr = ioaddr_meta.shr_dmac_ioaddr;
	return ret;
}
EXPORT_SYMBOL(get_shregion_dmac_ioaddr);

static int shregion_register_cdp_dcprs(struct shregion_metadata_drv *meta_drv)
{
	int retval;
	uint32_t shr_cdp_dcprs_ioaddr;
	struct shr_meta_priv *meta_node = NULL;
	struct ion_buffer *ion_buf;

	char biommu_sid_str[MAX_BIOMMU_STR_LEN_BYTES];
	struct iommu_domain *sid_domain = NULL;
	uint32_t mapped_size = 0;
	int iommu_prot;
	struct sg_table *sgt;

	if (!(meta_drv->meta.flags & SHREGION_CDP_AND_DCPRS_SHAREABLE)) {
		dprintk(0, "SHREGION_CDP_AND_DCPRS_SHAREABLE is not set\n");
		return -EINVAL;
	}

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta_drv->meta.name)
			break;
	}

	if (!meta_node) {
		dprintk(0, "%s: shr(%llx) not registered\n", __func__, meta_drv->meta.name);
		mutex_unlock(&shr_meta_mutex);
		return -ENOENT;
	}

	if (meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr) {
		mutex_unlock(&shr_meta_mutex);
		return 0;
	}

	retval = get_cdp_dcprs_addr_from_ioaddr_pool(meta_drv, &shr_cdp_dcprs_ioaddr);
	if (retval != 0) {
		dprintk(0, "Failed to get cdp shareable ioaddr for shr(name=%llx)",
			meta_drv->meta.name);
		mutex_unlock(&shr_meta_mutex);
		return -1;
	}

	snprintf(biommu_sid_str, MAX_BIOMMU_STR_LEN_BYTES, "biommu!cdp_dcprs@%x",
		CDP_DCPRS_STREAM_ID);

	sid_domain = biommu_get_domain_byname(biommu_sid_str);
	if (IS_ERR(sid_domain)) {
		dprintk(0, "Failed to get device for sid=0x%x\n",
			CDP_DCPRS_STREAM_ID);
		mutex_unlock(&shr_meta_mutex);
		return PTR_ERR(sid_domain);
	}

	iommu_prot = IOMMU_READ | IOMMU_WRITE;

	ion_buf = meta_node->dmabuf->priv;

	// Use sg_table of ion_buf if a shregion is slc cached.
	// CDP/DCPRS must not go through SLC physical address.
	// sg_extbl represents alternative paths ALT_0/ALT_1
	// if a shregion access needs to go through such paths.
	if (meta_drv->meta.flags & SHREGION_CVIP_SLC_CACHED)
		sgt = ion_buf->sg_table;
	else
		sgt = ion_buf->sg_extbl ? ion_buf->sg_extbl : ion_buf->sg_table;

	mapped_size = iommu_map_sg(sid_domain, (unsigned long)shr_cdp_dcprs_ioaddr,
				sgt->sgl, sgt->nents, iommu_prot);
	if (mapped_size <= 0) {
		dprintk(0, "Failed to create cdp/dcprs mmu map for(%llx) sid=0x%x,v=%x,sz=%d bytes\n",
			meta_drv->meta.name, CDP_DCPRS_STREAM_ID,
			shr_cdp_dcprs_ioaddr, meta_node->meta_drv.meta.size_bytes);
		mutex_unlock(&shr_meta_mutex);
		return -EFAULT;
	}

	dprintk(3, "%s:(%d,%d): cdp/dcprs iommu map(n=%llx,sid=0x%x) p=%llx,v=%x,sz=%d bytes\n",
		__func__, current->pid, current->tgid, meta_drv->meta.name, CDP_DCPRS_STREAM_ID,
		sg_phys(sgt->sgl), shr_cdp_dcprs_ioaddr,
		meta_node->meta_drv.meta.size_bytes);

	meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr = shr_cdp_dcprs_ioaddr;
	meta_node->cdp_dcprs_meta.sid_domain = sid_domain;
	meta_node->cdp_dcprs_meta.mapped_size = mapped_size;
	meta_node->cdp_dcprs_meta.creator_pid = current->tgid;

	mutex_unlock(&shr_meta_mutex);

	return 0;
}

static int shregion_register_s_intf_spcie(struct shregion_metadata_drv *meta_drv)
{
	int retval;
	uint32_t shr_s_intf_spcie_ioaddr;
	struct shr_meta_priv *meta_node = NULL;
	struct ion_buffer *ion_buf;

	char biommu_sid_str[MAX_BIOMMU_STR_LEN_BYTES];
	struct iommu_domain *sid_domain = NULL;
	uint32_t mapped_size = 0;
	int iommu_prot;
	struct sg_table *sgt;

	if (!(meta_drv->meta.flags & SHREGION_CVIP_SRAM_SPCIE_SHAREABLE)) {
		dprintk(0, "SHREGION_CVIP_SRAM_SPCIE_SHAREABLE is not set\n");
		return -EINVAL;
	}

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta_drv->meta.name)
			break;
	}

	if (!meta_node) {
		dprintk(0, "%s: shr(%llx) not registered\n", __func__, meta_drv->meta.name);
		mutex_unlock(&shr_meta_mutex);
		return -ENOENT;
	}

	if (meta_node->s_intf_spcie_meta.shr_s_intf_spcie_ioaddr) {
		mutex_unlock(&shr_meta_mutex);
		return 0;
	}

	retval = get_s_intf_spcie_ioaddr(meta_node, meta_drv, &shr_s_intf_spcie_ioaddr);
	if (retval) {
		dprintk(0, "Failed to get s_intf_spcie shareable ioaddr for shr(name=%llx)",
			meta_drv->meta.name);
		mutex_unlock(&shr_meta_mutex);
		return -1;
	}

	strncpy(biommu_sid_str, "biommu!2200000000.pcie-apu_s_intf_spcie",
		MAX_BIOMMU_STR_LEN_BYTES);

	sid_domain = biommu_get_domain_byname(biommu_sid_str);
	if (IS_ERR(sid_domain)) {
		dprintk(0, "Failed to get device for s_intf_spcie sid\n");
		mutex_unlock(&shr_meta_mutex);
		return PTR_ERR(sid_domain);
	}

	iommu_prot = IOMMU_READ | IOMMU_WRITE;

	ion_buf = meta_node->dmabuf->priv;

	sgt = ion_buf->sg_table;

	mapped_size = iommu_map_sg(sid_domain, (unsigned long)shr_s_intf_spcie_ioaddr,
				sgt->sgl, sgt->nents, iommu_prot);
	if (mapped_size <= 0) {
		dprintk(0, "Failed to create mmu map for(%llx) s_intf_spcie,v=%x,sz=%d bytes\n",
			meta_drv->meta.name, shr_s_intf_spcie_ioaddr,
			meta_node->meta_drv.meta.size_bytes);
		mutex_unlock(&shr_meta_mutex);
		return -EFAULT;
	}

	dprintk(3, "%s:(%d,%d): s_intf_spcie iommu map(n=%llx) p=%llx,v=%x,sz=%d bytes\n",
		__func__, current->pid, current->tgid, meta_drv->meta.name,
		sg_phys(sgt->sgl), shr_s_intf_spcie_ioaddr,
		meta_node->meta_drv.meta.size_bytes);

	meta_node->s_intf_spcie_meta.shr_s_intf_spcie_ioaddr = shr_s_intf_spcie_ioaddr;
	meta_node->s_intf_spcie_meta.sid_domain = sid_domain;
	meta_node->s_intf_spcie_meta.mapped_size = mapped_size;
	meta_node->s_intf_spcie_meta.creator_pid = current->tgid;

	mutex_unlock(&shr_meta_mutex);

	return 0;
}

static int shregion_dmac_register_user_buf(struct dmac_reg_user_buf_meta *dmac_user_buf_meta) {
	int ret;
	int iommu_prot;
	struct iommu_domain* dmac_iommu_domain = NULL;
	struct iova_domain* dmac_iova_domain = NULL;
	struct iommu_dma_cookie* dmac_iommu_cookie;
	uint32_t dmac_ioaddr;
	uint32_t npages = 0;
	//used for fixed dmac iovas.
	uint32_t rsv_req_pfn_lo = 0;
	uint32_t rsv_req_pfn_hi = 0;
	struct iova* reserved_iova;
	struct iova* allocated_iova;

	ret = pl330_share_dma_device(&dma0_device);
	if (ret) {
		dprintk(0, "failed to get dmac device, err=%d\n", ret);
		return ret;
	}

	dmac_iommu_domain = iommu_get_domain_for_dev(&dma0_device);
	if (IS_ERR(dmac_iommu_domain)) {
		dprintk(0, "failed to get a valid iommu domain for dma0 dev\n");
		return PTR_ERR(dmac_iommu_domain);
	}

	if (dmac_iommu_domain->type != IOMMU_DOMAIN_DMA) {
		dprintk(0, "dma0 dev's iommu domain is not of type IOMMU_DOMAIN_DMA\n");
		return -1;
	}

	//let's get manual access to iova_domain
	//only exposed through direct access of iommu_dma_cookie of our iommu domain
	dmac_iommu_cookie = dmac_iommu_domain->iova_cookie; //pull out iommu_dma_cookie of iommu_domain
	dmac_iova_domain  = &(dmac_iommu_cookie->iovad);

	if (dmac_iova_domain == NULL) {
		dprintk(0, "failed to get iova domain from dma0 dev iommu domain\n");
		return -ENOENT;
	}
	if (!dmac_iova_domain->start_pfn) {
		dprintk(0, "dmac iova_domain is not initialized\n");
		return -1;
	}

	npages = PAGE_ALIGN(dmac_user_buf_meta->user_buf_size) >> PAGE_SHIFT;

	if (dmac_user_buf_meta->fixed_dmac_iova != 0) {
		if (!dmac_user_buf_meta->user_req_dmac_ioaddr) {
			dprintk(0, "fixed dmac iova shouldn't be null\n");
			return -EINVAL;
		}

		if (dmac_user_buf_meta->user_req_dmac_ioaddr % PAGE_SIZE) {
			dprintk(0, "fixed dmac iova should be page_size aligned\n");
			return -EINVAL;
		}

		rsv_req_pfn_lo = dmac_user_buf_meta->user_req_dmac_ioaddr >> PAGE_SHIFT;
		rsv_req_pfn_hi = rsv_req_pfn_lo + npages;

		reserved_iova = reserve_iova(dmac_iova_domain, rsv_req_pfn_lo, rsv_req_pfn_hi);

		if (!reserved_iova) {
			__free_iova(dmac_iova_domain, reserved_iova);
			dprintk(0, "fixed dmac iova unable to be reserved\n");
			return -ENOMEM;
		}

		dmac_ioaddr = dmac_user_buf_meta->user_req_dmac_ioaddr;
	}
	else {

		allocated_iova = alloc_iova(dmac_iova_domain, npages, PHYS_PFN(DMA_BIT_MASK(32)), false);
		dmac_ioaddr = (allocated_iova->pfn_lo) << PAGE_SHIFT;
	}

	iommu_prot = IOMMU_READ | IOMMU_WRITE;

	ret = iommu_map(dmac_iommu_domain, (unsigned long)dmac_ioaddr, (unsigned long)dmac_user_buf_meta->user_buf_paddr,
		dmac_user_buf_meta->user_buf_size, iommu_prot);
	if (ret) {
		dprintk(0, "Failed to create pl330_dmac iommu map from user passed phys buf\n");
		return -EFAULT;
	}

	dmac_user_buf_meta->user_buf_dmac_ioaddr = dmac_ioaddr;

	return 0;
}

static int shregion_register_dmac(struct shregion_metadata_drv* meta_drv)
{
	int retval;
	struct shr_meta_priv* meta_node = NULL;

	uint32_t mapped_size = 0;
	int iommu_prot;

	uint32_t dmac_pid_cnt;
	uint32_t dmac_ioaddr;
	struct iommu_domain* dmac_iommu_domain = NULL;
	struct iova_domain* dmac_iova_domain = NULL;
	struct iommu_dma_cookie* dmac_iommu_cookie;
	uint32_t shr_npages = 0;

	//used for fixed dmac iovas.
	uint32_t rsv_req_pfn_lo = 0;
	uint32_t rsv_req_pfn_hi = 0;
	struct iova* reserved_iova;
	struct iova* allocated_iova;

	//handle ncm mappings
	static const int ncm_map_size = 0x200000;
	static bool ncm_iova_map_alloc = false;
	static uint32_t ncm_iova_base = 0;
	static uint32_t ncm_iova_map_idx = 0;

	if (!(meta_drv->meta.flags & SHREGION_DMAC_SHAREABLE)) {
		dprintk(0, "SHREGION_DMAC_SHAREABLE is not set\n");
		return -EINVAL;
	}

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta_drv->meta.name)
			break;
	}

	if (!meta_node) {
		dprintk(0, "%s: shr(%llx) not registered\n", __func__, meta_drv->meta.name);
		mutex_unlock(&shr_meta_mutex);
		return -ENOENT;
	}

	if (meta_node->dmac_meta.dmac_ioaddr) {
		mutex_unlock(&shr_meta_mutex);
		return 0;
	}

	//grab a handle to both DMAC/DMA330 engines
	retval = pl330_share_dma_device(&dma0_device);
	if (retval) {
		dprintk(0, "failed to get dmac device\n");
		mutex_unlock(&shr_meta_mutex);
		return retval;
	}

	//grab the iommu domain for our DMAC device
	//let's validate this domain has been properly registered for DMA
	dmac_iommu_domain = iommu_get_domain_for_dev(&dma0_device);
	if (IS_ERR(dmac_iommu_domain)) {
		dprintk(0, "failed to get a valid iommu domain for dma0 dev\n");
		mutex_unlock(&shr_meta_mutex);
		return PTR_ERR(dmac_iommu_domain);
	}

	if (dmac_iommu_domain->type != IOMMU_DOMAIN_DMA) {
		dprintk(0, "dma0 dev's iommu domain is not of type IOMMU_DOMAIN_DMA\n");
		mutex_unlock(&shr_meta_mutex);
		return -1;
	}

	//let's get manual access to iova_domain
	//only exposed through direct access of iommu_dma_cookie of our iommu domain
	dmac_iommu_cookie = dmac_iommu_domain->iova_cookie; //pull out iommu_dma_cookie of iommu_domain
	dmac_iova_domain = &(dmac_iommu_cookie->iovad);

	//lets validate this iova domain has been fetched successfully and initialized (pfn > 0)
	if (dmac_iova_domain == NULL) {
		dprintk(0, "failed to get iova domain from dma0 dev iommu domain\n");
		mutex_unlock(&shr_meta_mutex);
		return -ENOENT;
	}
	if (!dmac_iova_domain->start_pfn) {
		dprintk(0, "dmac iova_domain is not initialized\n");
		mutex_unlock(&shr_meta_mutex);
		return -1;
	}

	shr_npages = PAGE_ALIGN(meta_drv->meta.size_bytes) >> PAGE_SHIFT;

	//handle ncm mappings
	if (meta_drv->meta.type == SHREGION_C5_NCM_0 || meta_drv->meta.type == SHREGION_C5_NCM_1) {
		//allocate iova map on first ncm shregion mapping
		if (!ncm_iova_map_alloc) {
			shr_npages = PAGE_ALIGN(ncm_map_size) >> PAGE_SHIFT;
			allocated_iova = alloc_iova(dmac_iova_domain, shr_npages, PHYS_PFN(DMA_BIT_MASK(32)), false);
			ncm_iova_base = (allocated_iova->pfn_lo) << PAGE_SHIFT;
			ncm_iova_map_alloc = true;
		}
		//set dmac ioaddr from offset in ncm iova map
		dmac_ioaddr = ncm_iova_base + ncm_iova_map_idx;
		ncm_iova_map_idx += meta_drv->meta.size_bytes;
	}
	else {
		if (meta_drv->meta.flags & SHREGION_FIXED_DMAC_IO_VADDR) {
			if (!meta_drv->meta.fixed_dmac_iova) {
				dprintk(0, "fixed dmac iova shouldn't be null\n");
				mutex_unlock(&shr_meta_mutex);
				return -EINVAL;
			}

			if (meta_drv->meta.fixed_dmac_iova % PAGE_SIZE) {
				dprintk(0, "fixed dmac iova should be page_size aligned\n");
				mutex_unlock(&shr_meta_mutex);
				return -EINVAL;
			}

			rsv_req_pfn_lo = meta_drv->meta.fixed_dmac_iova >> PAGE_SHIFT;
			rsv_req_pfn_hi = rsv_req_pfn_lo + shr_npages;

			reserved_iova = reserve_iova(dmac_iova_domain, rsv_req_pfn_lo, rsv_req_pfn_hi);

			if (!reserved_iova) {
				__free_iova(dmac_iova_domain, reserved_iova);
				dprintk(0, "fixed dmac iova unable to be reserved\n");
				mutex_unlock(&shr_meta_mutex);
				return -ENOMEM;
			}

			dmac_ioaddr = meta_drv->meta.fixed_dmac_iova;
		}
		else {
			allocated_iova = alloc_iova(dmac_iova_domain, shr_npages, PHYS_PFN(DMA_BIT_MASK(32)), false);
			dmac_ioaddr = (allocated_iova->pfn_lo) << PAGE_SHIFT;
		}
	}

	iommu_prot = IOMMU_READ | IOMMU_WRITE;

	mapped_size = iommu_map_sg(dmac_iommu_domain, (unsigned long)dmac_ioaddr, meta_node->sg_table->sgl,
		meta_node->sg_table->nents, iommu_prot);
	if (mapped_size <= 0) {
		dprintk(0, "Failed to create pl330_dmac iommu map for(%llx) sid=0x%x,v=%x,sz=%d bytes\n",
			meta_node->meta_drv.meta.name, DMAC_FIXED_STREAM_ID,
			dmac_ioaddr, meta_node->meta_drv.meta.size_bytes);
		mutex_unlock(&shr_meta_mutex);
		return -EFAULT;
	}

	meta_node->dmac_meta.dmac_ioaddr = dmac_ioaddr;
	meta_node->dmac_meta.dmac_sid_domain = dmac_iommu_domain;
	meta_node->dmac_meta.dmac_iovad = dmac_iova_domain;
	meta_node->dmac_meta.dmac_iommu_cookie = dmac_iommu_cookie;
	meta_node->dmac_meta.dmac_mapped_size = mapped_size;

	dmac_pid_cnt = meta_node->dmac_meta.pid_ref_count++;
	meta_node->dmac_meta.pid[dmac_pid_cnt] = current->tgid;

	mutex_unlock(&shr_meta_mutex);

	return 0;
}

static int get_shr_cdp_dcprs_ioaddr(struct cdp_dcprs_shr_ioaddr_meta *cdp_dcprs_ioaddr_meta)
{
	struct shr_meta_priv *meta_node = NULL;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == cdp_dcprs_ioaddr_meta->shr_name)
			break;
	}

	if (!meta_node) {
		dprintk(0, "%s: shr(%llx) not found\n", __func__, cdp_dcprs_ioaddr_meta->shr_name);
		mutex_unlock(&shr_meta_mutex);
		return -ENOENT;
	}


	if (meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr) {
		cdp_dcprs_ioaddr_meta->shr_cdp_dcprs_ioaddr =
		meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr;
		mutex_unlock(&shr_meta_mutex);
		return 0;
	}

	dprintk(0, "shr_cdp_dcprs_ioaddr for (%llx) is null.\n", cdp_dcprs_ioaddr_meta->shr_name);

	mutex_unlock(&shr_meta_mutex);

	return -EINVAL;
}

int get_shregion_cdp_dcprs_ioaddr(uint64_t shr_name, ShregionCdpDcprsIOaddr *shr_cdp_dcprs_ioaddr)
{
	int ret;
	struct cdp_dcprs_shr_ioaddr_meta ioaddr_meta;

	ioaddr_meta.shr_name = shr_name;

	ret = get_shr_cdp_dcprs_ioaddr(&ioaddr_meta);
	if (ret != 0) {
		dprintk(0, "%s failed with err=%d\n", __func__, ret);
		return ret;
	}

	*shr_cdp_dcprs_ioaddr = ioaddr_meta.shr_cdp_dcprs_ioaddr;

	return ret;
}
EXPORT_SYMBOL(get_shregion_cdp_dcprs_ioaddr);

__maybe_unused static int test_cdp_alloc(void) {
	void *ioaddr;
	int ret;
	ShregionCdpDcprsIOaddr shr_cdp_dcprs_ioaddr;
	struct shregion_metadata meta;

	meta.name = CVCORE_NAME_STATIC(CDP_DCPRS_KERNEL_TEST);
	meta.flags = SHREGION_CDP_AND_DCPRS_SHAREABLE | SHREGION_PERSISTENT;
	meta.cache_mask = 0;
	meta.size_bytes = PAGE_SIZE;
	meta.type = SHREGION_HYDRA_FMR2;

	ret = kalloc_shregion(&meta, &ioaddr);
	if (ret != 0) {
		dprintk(0, "kalloc shregion failed for cdp with err=%d\n", ret);
		return -1;
	}

	ret = get_shregion_cdp_dcprs_ioaddr(meta.name, &shr_cdp_dcprs_ioaddr);
	if (ret != 0) {
		dprintk(0, "get cdp ioaddr failed with err=%d\n", ret);
		return -1;
	}

	dprintk(0, "Got cdp ioaddr = 0x%x\n", shr_cdp_dcprs_ioaddr);

	return 0;
}

static int get_shregion_dsp_vaddr(struct dsp_shregion_vaddr_meta *dsp_vaddr_meta)
{
	int i;
	struct shr_meta_priv *meta_node = NULL;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == dsp_vaddr_meta->shr_name)
			break;
	}

	if (!meta_node) {
		dprintk(0, "%s: shr(%llx) not found\n", __func__, dsp_vaddr_meta->shr_name);
		mutex_unlock(&shr_meta_mutex);
		return -ENOENT;
	}

	/* dsp code and all stream id shregions always use fixed dsp virtual address */
	if ((meta_node->meta_drv.meta.flags & SHREGION_DSP_MAP_ALL_STREAM_IDS) ||
			(meta_node->meta_drv.meta.flags & SHREGION_DSP_CODE)) {
		dsp_vaddr_meta->shr_dsp_vaddr = meta_node->meta_drv.meta.fixed_dsp_v_addr;
		dsp_vaddr_meta->guard_bands_config = meta_node->guard_bands_config;
		mutex_unlock(&shr_meta_mutex);
		return 0;
	}

	for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {
		if (meta_node->pid_fd_info[i].pid == current->tgid) {
			if (meta_node->pid_fd_info[i].shr_dsp_vaddr) {
				dsp_vaddr_meta->shr_dsp_vaddr = meta_node->pid_fd_info[i].shr_dsp_vaddr;
				dsp_vaddr_meta->guard_bands_config = meta_node->guard_bands_config;
				dprintk(1, "shr_dsp_vaddr is 0x%x for pid(%d) and shr(%llx)\n",
					dsp_vaddr_meta->shr_dsp_vaddr, current->tgid, dsp_vaddr_meta->shr_name);
				mutex_unlock(&shr_meta_mutex);
				return 0;
			} else {
				dprintk(0, "shr_dsp_vaddr is null. pid(%d) hasn't mapped this shr(%llx)\n",
					current->tgid, dsp_vaddr_meta->shr_name);
				mutex_unlock(&shr_meta_mutex);
				return -EINVAL;
			}
		}
	}

	dprintk(0, "pid(%d) not registered for shr(name=%llx)\n", current->tgid,
		dsp_vaddr_meta->shr_name);

	mutex_unlock(&shr_meta_mutex);

	return -ENOENT;
}

int shregion_command_handler(XpcChannel channel, XpcHandlerArgs args, uint8_t *command_buffer,
        size_t command_buffer_length, uint8_t *response_buffer, size_t resize_buffer_length,
        size_t *response_length)
{
	int i;
	uint16_t sid;
	int nr_of_entries;
	uintptr_t virt_addr;
	const uint8_t valid_entry = ENTRY_IS_VALID;
	const uint8_t invalid_entry = ENTRY_IS_INVALID;
	const uint8_t no_entry = ENTRY_NOT_FOUND;
	struct shr_xpc_msg_resp msg_resp;
	struct shr_xpc_msg_req msg_req;

	dprintk(2 ,"shregion: received xpc interrupt\n");

	if (channel != CVCORE_XCHANNEL_SHREGION) {
		dprintk(0, "Not the vchannel(got=%d,interested=%d) shregion is interested in.\n",
			channel, CVCORE_XCHANNEL_SHREGION);
		return 0;
	}

	memcpy((void *)&msg_req, (void *)command_buffer, sizeof(struct shr_xpc_msg_req));

	if (msg_req.src_id == DSP_SRC_CORE_ID) {

		nr_of_entries = sid_meta.nr_of_entries;
		sid = msg_req.stream_id;

		dprintk(2, "got sid=0x%x, len=%ld\n\n", sid, command_buffer_length);
		dprintk(2, "cmd_0=%d, cmd_1=%d\n\n", command_buffer[0], command_buffer[1]);

		*response_length = 0;

		for (i = 0; i < nr_of_entries; i++) {

			if (sid_meta.sid_virt_addr_map[i].sid == sid) {

				if (sid_meta.sid_virt_addr_map[i].entry_valid != ENTRY_IS_VALID) {
					dprintk(0, "sid(%d) is not valid.\n", sid);
					msg_resp.entry_status = invalid_entry;
					memcpy((void *)response_buffer, (void *)&msg_resp, sizeof(struct shr_xpc_msg_resp));
					*response_length = sizeof(struct shr_xpc_msg_resp);
					return 0;
				}

				// Note that there's chance this virtual address may become invalid while
				// we're sending it to the dsps. This can potentially happen when a process
				// is dying and the cleanup() in this driver file is getting called at the
				// same time when we are here in process of sending the address to the dsp.
				// A lock can't solve this since the process death can occur immediately
				// after we release the lock at the end of this function.
				virt_addr = (uintptr_t)sid_meta.sid_virt_addr_map[i].meta_virt_addr;
				dprintk(3, "Found virt addr(sid=%d)=0x%lx, i=%d\n", sid, virt_addr, i);
				msg_resp.entry_status = valid_entry;
				msg_resp.addr = virt_addr;
				memcpy((void *)response_buffer, (void *)&msg_resp, sizeof(struct shr_xpc_msg_resp));
				*response_length = sizeof(struct shr_xpc_msg_resp);
				return 0;
			}
		}

		dprintk(0, "sid(%d) not registered\n", sid);
		msg_resp.entry_status = no_entry;
		memcpy((void *)response_buffer, (void *)&msg_resp, sizeof(struct shr_xpc_msg_resp));
		*response_length = sizeof(struct shr_xpc_msg_resp);

	} else if (msg_req.src_id == X86_SRC_CORE_ID) {

		msg_resp.entry_status = valid_entry;
		msg_resp.addr = x86_shared_meta_phys_addr;
		memcpy((void *)response_buffer, (void *)&msg_resp, sizeof(struct shr_xpc_msg_resp));
		*response_length = sizeof(struct shr_xpc_msg_resp);

	} else {

		dprintk(0, "Invalid src id (%d)\n", msg_req.src_id);
		msg_resp.entry_status = invalid_entry;
		memcpy((void *)response_buffer, (void *)&msg_resp, sizeof(struct shr_xpc_msg_resp));
		*response_length = sizeof(struct shr_xpc_msg_resp);
	}

	return 0;
}

static void free_dsp_dma_buf_resources(void)
{
	if (dsp_buf_info.sg_table)
		dma_buf_unmap_attachment(dsp_buf_info.dma_attach,
			dsp_buf_info.sg_table, DMA_BIDIRECTIONAL);
	if (dsp_buf_info.dma_attach)
		dma_buf_detach(dsp_buf_info.dmabuf, dsp_buf_info.dma_attach);

	dma_buf_put(dsp_buf_info.dmabuf);
	ksys_close(dsp_buf_info.fd);
}

static void deregister_xpc_shr_command_handler(void)
{
	int retval;
	retval = xpc_register_command_handler(CVCORE_XCHANNEL_SHREGION, NULL, NULL);
	if (retval != 0)
		dprintk(0, "failed to deregister xpc command handler.\n");
}

static int register_xpc_shr_command_handler(void)
{
	int retval;
	// Register shregion cmd handler with xpc
	retval = xpc_register_command_handler(CVCORE_XCHANNEL_SHREGION,
						&shregion_command_handler, NULL);
	if (retval != 0) {
		dprintk(0, "failed to register command handler,status=%d\n", retval);
		return -1;
	}
	return 0;
}

static int init_x86_shared_meta(void)
{
	int fd;
	struct dma_buf *dmabuf;
	struct ion_buffer *ion_buf;
	void *vaddr;
	phys_addr_t phys_addr;

	fd = ion_alloc(sizeof(struct x86_shared_shregion_meta),
		1 << ION_HEAP_CVIP_X86_READONLY_FMR10_ID, 0);
	if (fd < 0) {
		dprintk(0, "Failed to allocated fmr_10 memory.\n");
		return -ENOMEM;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		dprintk(0, "Invalid fd, unable to get dma buffer for fmr_10\n");
		ksys_close(fd);
		return -EINVAL;
	}

	vaddr = dma_buf_kmap(dmabuf, 0);
	if (!vaddr) {
		dprintk(0, "Failed kmap for fmr_10 dmabuf\n");
		dma_buf_put(dmabuf);
		ksys_close(fd);
		return PTR_ERR(vaddr);
	}

	x86_shared_meta = (struct x86_shared_shregion_meta *)vaddr;

	x86_shared_meta->nr_of_entries = 0;

	x86_shared_meta->meta_init_done = META_INIT_MAGIC_NUM;

	ion_buf = dmabuf->priv;

	phys_addr = page_to_phys(sg_page(ion_buf->sg_table->sgl));

	x86_shared_meta_phys_addr = phys_addr - MERO_CVIP_XPORT_APERTURE_SIZE;

	dprintk(0, "Shared metadata at %px initialized, phys %px\n",
		x86_shared_meta, (void *)x86_shared_meta_phys_addr);

	return 0;
}

static int create_s_intf_sram_x86_mapping(struct shr_meta_priv *meta_node)
{
	static uint32_t shr_s_intf_sram_x86_ioaddr = S_INTF_X86_BASE_IOADDR;
	struct ion_buffer *ion_buf;

	char biommu_sid_str[MAX_BIOMMU_STR_LEN_BYTES];
	struct iommu_domain *sid_domain = NULL;
	uint32_t mapped_size = 0;
	int iommu_prot;
	struct sg_table *sgt;

	strncpy(biommu_sid_str, "biommu!apu_s_intf_x86", MAX_BIOMMU_STR_LEN_BYTES);

	sid_domain = biommu_get_domain_byname(biommu_sid_str);
	if (IS_ERR_OR_NULL(sid_domain)) {
		dprintk(0, "Failed to get device for s_intf_sram_x86 sid\n");
		return PTR_ERR(sid_domain);
	}

	iommu_prot = IOMMU_READ;

	ion_buf = meta_node->dmabuf->priv;

	sgt = ion_buf->sg_table;

	mapped_size = iommu_map_sg(sid_domain, (unsigned long)shr_s_intf_sram_x86_ioaddr,
				sgt->sgl, sgt->nents, iommu_prot);
	if (mapped_size <= 0) {
		dprintk(0, "Failed to create mmu map for(%llx) s_intf_sram_x86,v=%x,sz=%d bytes\n",
			meta_node->meta_drv.meta.name, shr_s_intf_sram_x86_ioaddr,
			meta_node->meta_drv.meta.size_bytes);
		return -EFAULT;
	}

	dprintk(3, "%s:(%d,%d): s_intf_sram_x86 iommu map(n=%llx) p=%llx,v=%x,sz=%d bytes\n",
		__func__, current->pid, current->tgid, meta_node->meta_drv.meta.name,
		sg_phys(sgt->sgl), shr_s_intf_sram_x86_ioaddr,
		meta_node->meta_drv.meta.size_bytes);

	meta_node->s_intf_sram_x86_meta.shr_s_intf_sram_x86_ioaddr = shr_s_intf_sram_x86_ioaddr;
	meta_node->s_intf_sram_x86_meta.sid_domain = sid_domain;
	meta_node->s_intf_sram_x86_meta.mapped_size = mapped_size;
	meta_node->s_intf_sram_x86_meta.creator_pid = current->tgid;

	shr_s_intf_sram_x86_ioaddr += mapped_size;

	return 0;
}

// Must be called while holding shr_meta_mutex.
static int update_x86_shared_shregion_meta(struct shr_meta_priv *meta_node)
{
	int ret;
	uint32_t nr_of_entries;
	phys_addr_t phys_addr;
	struct ion_buffer *ion_buf = NULL;
	struct shregion_metadata *meta = &(meta_node->meta_drv.meta);

	if (!x86_shared_meta) {
		pr_err("x86 shared meta not initialized yet.\n");
		return -EINVAL;
	}

	if (meta->flags & X86_SHARED_SHREGION_SRAM) {
		if (!is_x86_sharing_supported(meta->type)) {
			dprintk(0, "x86_shared flag(%d) can't be used with mem type(%d)\n",
				meta->flags, meta->type);
			return -EINVAL;
		}
	}

	nr_of_entries = x86_shared_meta->nr_of_entries;

	if (nr_of_entries >= MAX_x86_SHARED_SHR_ENTRIES) {
		pr_err("shared ddr meta entry quota exceeded\n");
		return -EDQUOT;
	}

	ion_buf = meta_node->dmabuf->priv;

	phys_addr = page_to_phys(sg_page(ion_buf->sg_table->sgl));

	// subtract MERO_CVIP_ADDR_XPORT_BASE/MERO_CVIP_XPORT_APERTURE_SIZE
	// from the address for x86 to see it.
	if (meta->type == SHREGION_DDR_SHARED)
		phys_addr -= MERO_CVIP_ADDR_XPORT_BASE;
	else if (meta->type == SHREGION_X86_RO_FMR10 ||
		meta->type == SHREGION_X86_SPCIE_FMR2_10)
		phys_addr -= MERO_CVIP_XPORT_APERTURE_SIZE;
	else if (meta->type == SHREGION_Q6_SCM_0)
		// Just store the offset from scm base addr
		phys_addr -= CVIP_SCM_0_BASE_ADDRESS;
	else if (meta->type == SHREGION_C5_NCM_0)
		// Just store the offset from ncm_0 base addr
		phys_addr -= CVIP_NCM_0_BASE_ADDRESS;
	else if (meta->type == SHREGION_C5_NCM_1)
		// Just store the offset from ncm_1 base addr
		phys_addr -= CVIP_NCM_1_BASE_ADDRESS;

	if (meta->flags & X86_SHARED_SHREGION_SRAM) {
		ret = create_s_intf_sram_x86_mapping(meta_node);
		if (ret) {
			dprintk(0, "create_s_intf_sram_x86_mapping failed, err=%d\n", ret);
			return ret;
		}
	}

	// add cvcore name entry.
	x86_shared_meta->cvcore_name_entry[nr_of_entries].name = meta->name;
	x86_shared_meta->cvcore_name_entry[nr_of_entries].size_bytes = meta->size_bytes;
	x86_shared_meta->cvcore_name_entry[nr_of_entries].type = meta->type;
	x86_shared_meta->cvcore_name_entry[nr_of_entries].cache_mask = meta->cache_mask;
	x86_shared_meta->cvcore_name_entry[nr_of_entries].guard_bands_config = meta_node->guard_bands_config;
	x86_shared_meta->cvcore_name_entry[nr_of_entries].x86_phys_addr = phys_addr;

	wmb();

	++(x86_shared_meta->nr_of_entries);

	return 0;
}

static int shregion_register(struct shregion_metadata_drv *meta_drv, void **vaddr)
{
	struct shr_meta_priv *meta_node;
	int i, ret;
	uint8_t ion_shr_type;
	uint32_t ion_cache_mask;
	struct iommu_domain *sid_domain;
	int mapped_size;
	int iommu_prot;
	int sid; // stream id
	char biommu_sid_str[MAX_BIOMMU_STR_LEN_BYTES];
	struct device *iommudev_ret = NULL;
	uint8_t *guard_band_vaddr;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta_drv->meta.name) {
			dprintk(1, "shregion(name=%llx) already registered.\n",
				meta_drv->meta.name);

			if (vaddr) {
				if (!(meta_node->meta_drv.meta.flags & SHREGION_PERSISTENT)) {
					dprintk(0, "Can't map a non-persistent shregion\n");
					mutex_unlock(&shr_meta_mutex);
					return -EINVAL;
				}
				*vaddr = meta_node->k_virt_addr;
			}

			mutex_unlock(&shr_meta_mutex);
			meta_drv->status = SHREGION_MEM_PREREGISTERED;
			return 0;
		}
	}

	meta_node = kmalloc(sizeof(struct shr_meta_priv), GFP_KERNEL);
	if (!meta_node) {
		mutex_unlock(&shr_meta_mutex);
		return -ENOMEM;
	}

	if (vaddr) {
		ion_shr_type = ion_core_get_heap_type(meta_drv->meta.type);
		if (ion_shr_type == INVALID_HEAP_ID) {
			dprintk(0, "Invalid shregion type\n");
			kfree(meta_node);
			mutex_unlock(&shr_meta_mutex);
			return -EINVAL;
		}

		// TODO(sjain): add flags for ALT_0 and ALT_1 if they are selected
		// by the user.

		ion_cache_mask = ion_core_get_cache_mask(meta_drv->meta.cache_mask);

		meta_drv->buf_fd = ion_alloc(meta_drv->meta.size_bytes,
					1 << ion_shr_type, ion_cache_mask);
		if (meta_drv->buf_fd < 0) {
			dprintk(0, "Failed to allocated ion memory\n");
			kfree(meta_node);
			mutex_unlock(&shr_meta_mutex);
			return -ENOMEM;
		}
	}

	// Register the PID and corresponding fd.
	meta_node->pid_fd_info[0].pid = current->tgid;
	meta_node->pid_fd_info[0].sid = -1;
	meta_node->pid_fd_info[0].fd = meta_drv->buf_fd;
	meta_node->pid_fd_info[0].mapped_size = 0;
	meta_node->pid_fd_info[0].shr_dsp_vaddr = 0;
	meta_node->pid_fd_info[0].shr_uvaddr_valid = 0;
	meta_node->num_of_ref_pids = 1;

	// Init pid_fd_info values.
	for (i = 1; i < SHREGION_MAX_REF_PIDS; i++) {
		// PIDs.
		meta_node->pid_fd_info[i].pid = -1;
		meta_node->pid_fd_info[i].sid = -1;
		// Does it have fd? No.
		meta_node->pid_fd_info[i].fd = -1;
		meta_node->pid_fd_info[i].mapped_size = 0;
		meta_node->pid_fd_info[i].shr_dsp_vaddr = 0;
		meta_node->pid_fd_info[i].shr_uvaddr_valid = 0;
	}

	meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr = 0;
	meta_node->cdp_dcprs_meta.mapped_size = 0;

	meta_node->s_intf_spcie_meta.shr_s_intf_spcie_ioaddr = 0;
	meta_node->s_intf_spcie_meta.mapped_size = 0;

	meta_node->s_intf_sram_x86_meta.shr_s_intf_sram_x86_ioaddr = 0;
	meta_node->s_intf_sram_x86_meta.mapped_size = 0;

	meta_node->dmac_meta.dmac_ioaddr = 0;
	meta_node->dmac_meta.dmac_mapped_size = 0;
	meta_node->dmac_meta.pid_ref_count = 0;

	meta_node->dmabuf = dma_buf_get(meta_drv->buf_fd);

	meta_node->k_virt_addr = dma_buf_kmap(meta_node->dmabuf, 0);

	if (!(meta_node->k_virt_addr)) {
		pr_err("Failed kmap for dmabuf\n");
		ret = PTR_ERR(meta_node->k_virt_addr);
		goto error;
	}

	if (vaddr) {
		*vaddr = meta_node->k_virt_addr;
	}

	// We just needed a reference to the actual buffer. We've
	// got it and so, we're free to deref count the same buffer
	// for non-persistent shregion. The reason we don't deref count
	// PERSISTENT shregion is because we want such shregions to
	// persist (i.e. not-get-released-back-to-the-ion-pool)
	// irrespective of whether the process lives or dies by holding
	// a ref to it's buffer forever.
	if (!(meta_drv->meta.flags & SHREGION_PERSISTENT))
		dma_buf_put(meta_node->dmabuf);

	memcpy(&(meta_node->meta_drv.meta), &(meta_drv->meta),
		sizeof(struct shregion_metadata));

	// Invalidate this shregion access is provided
	// to the end user. This invalidation resolves data corruption issue
	// that can be seen with non-arm-cached shregions.
	if (meta_drv->meta.type != SHREGION_GSM) {
		ret = shregion_arm_cache_invalidate(meta_drv->meta.name, meta_node->dmabuf);
		if (ret) {
			dprintk(0, "shregion_arm_cache_invalidate failed, ret=%d\n", ret);
			goto error;
		}
	}

	if (shregion_guard_band_size(meta_drv->meta.flags, meta_drv->meta.type) && !vaddr) {
		guard_band_vaddr = (uint8_t *)meta_node->k_virt_addr;

		// Fill pattern in top guard band of this shregion
		for (i = 0; i < SHREGION_GUARD_BAND_SIZE_BYTES; ++i) {
			guard_band_vaddr[i] = SHREGION_GUARD_BAND_PATTERN;
		}

		//dprintk(0, "DEBUGME shr(name=%llx), type=%d\n", meta_drv->meta.name, meta_drv->meta.type);

		// flush this pattern to physical memory
		__flush_dcache_area(guard_band_vaddr, SHREGION_GUARD_BAND_SIZE_BYTES);

		// Here, meta_drv->meta.size_bytes represents the actual shregion
		// size requested by user plus both sizes of both the guard bands.
		guard_band_vaddr = (uint8_t *)((uintptr_t)meta_node->k_virt_addr +
			meta_drv->meta.size_bytes - SHREGION_GUARD_BAND_SIZE_BYTES);

		// Fill pattern in bottom guard band of this shregion
		for (i = 0; i < SHREGION_GUARD_BAND_SIZE_BYTES; ++i) {
			guard_band_vaddr[i] = SHREGION_GUARD_BAND_PATTERN;
		}

		// flush this pattern to physical memory
		__flush_dcache_area(guard_band_vaddr, SHREGION_GUARD_BAND_SIZE_BYTES);

		meta_node->guard_bands_config = GUARD_BANDS_ENABLED;
		meta_node->violation_already_logged = false;
	} else {
		meta_node->guard_bands_config = GUARD_BANDS_DISABLED;
	}

        /* if nova is not present we can't use the shared ddr, if nova is
         * present but we are in cvip_only mode, still can't use the shared DDR
         * */
	if (!cvip_only && (meta_drv->meta.type == SHREGION_DDR_SHARED ||
		meta_drv->meta.type == SHREGION_X86_SPCIE_FMR2_10 ||
		meta_drv->meta.type == SHREGION_X86_RO_FMR10 ||
		(meta_drv->meta.flags & X86_SHARED_SHREGION_SRAM))) {
		ret = update_x86_shared_shregion_meta(meta_node);
		if (ret != 0) {
			pr_err("failed to update shared ddr meta (ret=%d)\n", ret);
			goto error;
		}
	}

	if ((meta_drv->meta.flags & SHREGION_DSP_SHAREABLE)||
		(meta_drv->meta.flags & SHREGION_DSP_CODE) ||
		(meta_drv->meta.flags & SHREGION_DSP_MAP_ALL_STREAM_IDS) ||
		(meta_drv->meta.flags & SHREGION_CDP_AND_DCPRS_SHAREABLE) ||
		(meta_drv->meta.flags & SHREGION_DMAC_SHAREABLE) ||
		(meta_drv->meta.type == SHREGION_DDR_SHARED)) {

		if ((meta_drv->meta.type == SHREGION_DDR_SHARED)) {
			/* for the x86 get the pl330 dma device so we have the
			 * correct dma ops when attaching the dma buf */
			iommudev_ret = get_dma_device();
			if (!iommudev_ret) {
				dprintk(0, "Failed to get dma device\n");
				ret = -ENODEV;
				goto error;
			}
		} else {
			// Attach it to some random device (say test stream id vdsp@0 since
			// we believe vdsp@0 will be available for eternity).
			// Our only goal here is have this dma buf be attached to 'some'
			// device in order to have dma buf sync work.
			snprintf(biommu_sid_str, MAX_BIOMMU_STR_LEN_BYTES, "biommu!vdsp@%x", 0);

			iommudev_ret = biommu_get_domain_dev_byname(biommu_sid_str);
			if (IS_ERR(iommudev_ret)) {
				dprintk(0, "Failed to get device for shr=%llx\n", meta_drv->meta.name);
				ret = PTR_ERR(iommudev_ret);
				goto error;
			}
		}

		meta_node->dma_attach = dma_buf_attach(meta_node->dmabuf, iommudev_ret);
		if (IS_ERR(meta_node->dma_attach)) {
			dprintk(0, "Failed to attach dev to shr(%llx)'s dmabuf\n", meta_drv->meta.name);
			ret = PTR_ERR(meta_node->dma_attach);
			goto error;
		}

		// Presume all 64 bits are dma-capable.
		iommudev_ret->coherent_dma_mask = DMA_BIT_MASK(64);
		iommudev_ret->dma_mask = &iommudev_ret->coherent_dma_mask;

		meta_node->sg_table = dma_buf_map_attachment(meta_node->dma_attach, DMA_BIDIRECTIONAL);
		if (IS_ERR(meta_node->sg_table)) {
			dprintk(0, "Failed to get sg for shr(%llx)'s dmabuf\n", meta_drv->meta.name);
			dma_buf_detach(meta_node->dmabuf, meta_node->dma_attach);
			ret = PTR_ERR(meta_node->sg_table);
			goto error;
		}

	} else {
		meta_node->dma_attach = NULL;
		meta_node->sg_table = NULL;
	}

	if ((meta_drv->meta.flags & SHREGION_DSP_CODE) ||
		(meta_drv->meta.flags & SHREGION_DSP_MAP_ALL_STREAM_IDS)) {

		if (!meta_drv->meta.fixed_dsp_v_addr) {
			dprintk(0, "fixed_dsp_v_addr shouldn't be null\n");
			ret = -EINVAL;
			goto error;
		}

		// This shregion is not going to be shared with the dsps
		// like we share normal shregions using the shregions framework.
		// The "normal" shregions will have different user space virtual
		// address unlike this shregions which will have same virtual to
		// physical address mapping for all the stream ids.
		for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {

			sid = cvcore_stream_ids[i];

			dprintk(2, "creating mappings for sid=0x%x", sid);

			snprintf(biommu_sid_str, MAX_BIOMMU_STR_LEN_BYTES, "biommu!vdsp@%x", sid);

			sid_domain = biommu_get_domain_byname(biommu_sid_str);
			if (IS_ERR(sid_domain)) {
				dprintk(0, "Failed to get device for sid=0x%x\n", sid);
				ret = PTR_ERR(sid_domain);
				goto error;
			}

			// SHREGION_DSP_MAP_ALL_STREAM_IDS should be IOMMU_READ | IOMMU_WRITE, and
			// SHREGION_DSP_CODE should be just IOMMU_READ
			if (IS_DEBUGBUILD) {
				iommu_prot = IOMMU_READ | IOMMU_WRITE;
			} else {
				iommu_prot = IOMMU_READ;
			}

			mapped_size = iommu_map_sg(sid_domain,
				meta_drv->meta.fixed_dsp_v_addr,
				meta_node->sg_table->sgl,
				meta_node->sg_table->nents,
				iommu_prot);

			if (mapped_size <= 0) {
				dprintk(0, "Failed to create iommu map in domain for sid=0x%x\n", sid);
				ret = -EFAULT;
				goto error;
			}

			meta_node->pid_fd_info[i].mapped_size = mapped_size;
			meta_node->pid_fd_info[i].sid_domain = sid_domain;
			meta_node->pid_fd_info[i].shr_dsp_vaddr = meta_drv->meta.fixed_dsp_v_addr;
			meta_node->pid_fd_info[i].shr_uvaddr = meta_drv->meta.fixed_dsp_v_addr;
		}
	}

	list_add(&meta_node->list, &shregion_nodes);

	dprintk(1, "shr(name=%llx) registered\n", meta_drv->meta.name);

	mutex_unlock(&shr_meta_mutex);

	return 0;

error:
	// we dma_buf_put for shregion allocated in kernel space
	if (vaddr) {
		dma_buf_put(meta_node->dmabuf);
		ksys_close(meta_drv->buf_fd);
	}
	kfree(meta_node);
	mutex_unlock(&shr_meta_mutex);
	return ret;
}

// Currently, we only support allocating "persistent" shregions
// in kernel space. If a persistent shregion is already allocated, this
// function returns successfully with the shregion's kernel
// virtual address that was stored duing its creation.
int kalloc_shregion(struct shregion_metadata *meta, void **vaddr)
{
	int ret;
	struct shregion_metadata_drv meta_drv;

	*vaddr = NULL;

	// make sure user is requesting a alloc a persistent shregion
	if (!(meta->flags & SHREGION_PERSISTENT)) {
		dprintk(0, "Can't allocate non-persistent shregion\n");
		return -EINVAL;
	}

	memcpy(&(meta_drv.meta), meta, sizeof(struct shregion_metadata));

	mutex_lock(&shr_meta_mutex);

	ret = register_tgid();
	if (ret != 0) {
		mutex_unlock(&shr_meta_mutex);
		return ret;
	}

	mutex_unlock(&shr_meta_mutex);

	ret = shregion_register(&meta_drv, vaddr);
	if (ret != 0) {
		dprintk(0, "shregion registration failed with err=%d\n", ret);
		return ret;
	}

	if (meta->flags & SHREGION_CDP_AND_DCPRS_SHAREABLE) {
		ret = shregion_register_cdp_dcprs(&meta_drv);
		if (ret != 0) {
			dprintk(0, "cdp dcprs shregion registration failed with err=%d\n",
				ret);
			return ret;
		}
	}

	if (meta->flags & SHREGION_DMAC_SHAREABLE) {
		ret = shregion_register_dmac(&meta_drv);
		if (ret != 0) {
			dprintk(0, "dmac shregion registration failed with err=%d\n",
				ret);
			return ret;
		}
	}
	return ret;
}
EXPORT_SYMBOL(kalloc_shregion);

int get_shregion_vaddr(struct shregion_metadata *meta, void **vaddr)
{
	struct shr_meta_priv *meta_node;

	if (!vaddr || !meta) {
		dprintk(0, "%s: Invalid argument\n", __func__);
		return -EINVAL;
	}

	*vaddr = NULL;
	meta->size_bytes = 0;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta->name) {
			*vaddr = meta_node->k_virt_addr;
			meta->size_bytes = meta_node->meta_drv.meta.size_bytes;
			break;
		}
	}

	mutex_unlock(&shr_meta_mutex);

	if (*vaddr)
		return 0;

	if (meta->size_bytes)
		dprintk(0, "shregion(name=%llx) exists but vaddr is null\n",
			meta->name);

	return -ENOENT;
}
EXPORT_SYMBOL(get_shregion_vaddr);

struct dma_buf *get_dma_buf_for_shr(uint64_t shr_name)
{
	struct shr_meta_priv *meta_node;

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == shr_name) {
			return meta_node->dmabuf;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_dma_buf_for_shr);

static int ion_buffer_get_phys_addr(struct shr_meta_priv *meta_node,
	uint32_t offset)
{
	int i;
	int j;
	int page_count = 0;
	int nr_of_pages;
	int page_offset;
	struct sg_table *table;
	struct page *page;
	struct scatterlist *sg;
	struct ion_buffer *ion_buf = meta_node->dmabuf->priv;

	table = ion_buf->sg_table;

	if (!offset) {
		page = sg_page(table->sgl);
		meta_node->meta_drv.phys_addr = PFN_PHYS(page_to_pfn(page));
		return 0;
	}

	if (offset >= meta_node->meta_drv.meta.size_bytes) {
		dprintk(0, "Invalid offset(%d) for shr(%llx)\n", offset,
			meta_node->meta_drv.meta.name);
		return -1;
	}

	nr_of_pages = offset / PAGE_SIZE;
	page_offset = offset % PAGE_SIZE;

	for_each_sg(table->sgl, sg, table->nents, i) {
		int npages_this_entry = PAGE_ALIGN(sg->length) / PAGE_SIZE;
		page = sg_page(sg);

		for (j = 0; j < npages_this_entry; j++) {
			if (page_count >= nr_of_pages) {
				meta_node->meta_drv.phys_addr =
				PFN_PHYS(page_to_pfn(page)) + page_offset;
				return 0;
			}
			++page;
			++page_count;
		}
	}

	return -1;
}

static int register_tgid(void)
{
	int i;
	static int dsp_notifier_meta_sz = sizeof(struct dsp_notifier_metadata);
	static int dsp_shregion_meta_sz = sizeof(struct dsp_shregion_metadata);

	// Register process which has called query
	for (i = 0; i < registered_processes.nr_of_valid_pids; ++i) {
		if (current->tgid == registered_processes.pid_meta[i].pid)
			return 0;
	}

	if (i >= SHREGION_MAX_REF_PIDS) {
		dprintk(0, "registered_processes quota exceeded(%d)\n", i);
		return -EDQUOT;
	}

	if (!dsp_notifier_meta_pg_sz_multiple) {
		if (dsp_notifier_meta_sz % PAGE_SIZE)
			dsp_notifier_meta_pg_sz_multiple =
			(((dsp_notifier_meta_sz / PAGE_SIZE) + 1) * PAGE_SIZE);
		else
			dsp_notifier_meta_pg_sz_multiple = dsp_notifier_meta_sz;
	}

	if (!dsp_shregion_meta_pg_sz_multiple) {
		if (dsp_shregion_meta_sz % PAGE_SIZE)
			dsp_shregion_meta_pg_sz_multiple =
			(((dsp_shregion_meta_sz / PAGE_SIZE) + 1) * PAGE_SIZE);
		else
			dsp_shregion_meta_pg_sz_multiple = dsp_shregion_meta_sz;
	}

	registered_processes.pid_meta[i].pid = current->tgid;

	registered_processes.pid_meta[i].cur_avail_dcache_wb_addr =
		CVCORE_DSP_DATA_REGION0_VADDR;
	registered_processes.pid_meta[i].cur_free_dcache_wb_pool_sz_bytes =
		CVCORE_DSP_DATA_REGION0_SIZE;

	registered_processes.pid_meta[i].cur_avail_noncache_addr =
		CVCORE_DSP_DATA_REGION1_VADDR + dsp_notifier_meta_pg_sz_multiple +
		dsp_shregion_meta_pg_sz_multiple;
	registered_processes.pid_meta[i].cur_free_noncache_pool_sz_bytes =
		CVCORE_DSP_DATA_REGION1_SIZE - dsp_notifier_meta_pg_sz_multiple -
		dsp_shregion_meta_pg_sz_multiple;

	++registered_processes.nr_of_valid_pids;

	return 0;
}

static int shregion_query_entry(struct shregion_metadata_drv *meta_drv,
				struct file *filp)
{
	int ret;
	struct shr_meta_priv *meta_node;
	uint32_t flags;
	pid_t shr_creator_pid;

	_Static_assert(sizeof(filp->private_data) == sizeof(meta_drv->meta.name),
		"ERR: sizeof(private_data) != sizeof(meta.name)");

	mutex_lock(&shr_meta_mutex);

	ret = register_tgid();
	if (ret != 0) {
		mutex_unlock(&shr_meta_mutex);
		return ret;
	}

	meta_drv->spm.shr_uvaddr_valid = 0;

	list_for_each_entry(meta_node, &shregion_nodes, list) {

		shr_creator_pid = meta_node->pid_fd_info[0].pid;

		flags = meta_node->meta_drv.meta.flags;

		if (meta_node->meta_drv.meta.name == meta_drv->meta.name) {
			// [if it's non-shareable and created by this pid] OR
			// [If shregion is shareable], then return the
			// metadata.
			// If it is not shareable and not created by the
			// current pid, then the caller is not allowed to use
			// it and so don't return the metadata.
			if ((shr_creator_pid == current->tgid &&
				!(flags & SHREGION_SHAREABLE)) ||
				(flags & SHREGION_SHAREABLE)) {
				// We return stored metadata corresponding to
				// this pre-existent shregion because when
				// shregion_map() is called it only knows its
				// name and has no info on it's metadata.
				meta_drv->meta.type =
						meta_node->meta_drv.meta.type;
				meta_drv->meta.size_bytes =
					meta_node->meta_drv.meta.size_bytes;
				meta_drv->meta.cache_mask =
					meta_node->meta_drv.meta.cache_mask;
				meta_drv->meta.flags = flags;

				proc_shr_uvaddr_req(meta_node, &meta_drv->spm);

				if ((flags & SHREGION_DSP_CODE) ||
					(flags & SHREGION_DSP_MAP_ALL_STREAM_IDS)) {
					meta_drv->meta.fixed_dsp_v_addr =
					meta_node->meta_drv.meta.fixed_dsp_v_addr;
				}

				if (flags & SHREGION_FIXED_DMAC_IO_VADDR) {
					meta_drv->meta.fixed_dmac_iova =
						meta_node->meta_drv.meta.fixed_dmac_iova;
				}

				get_buf_fd(meta_drv, meta_node);

				meta_drv->status =
						SHREGION_MEM_SHAREABLE;
				mutex_unlock(&shr_meta_mutex);
				return 0;
			}
			mutex_unlock(&shr_meta_mutex);
			dprintk(0, "shr(name=%llx) is not shareable.\n",
				meta_drv->meta.name);
			meta_drv->status = SHREGION_MEM_NON_SHAREABLE;
			return 0;
		}
	}

	mutex_unlock(&shr_meta_mutex);
	meta_drv->status = SHREGION_MEM_NONEXISTENT;
	return 0;
}

static struct device shregion_dev = {
	.init_name = "shregion",
	// dma_alloc_coherent(): allow any address
	.coherent_dma_mask = ~0,
	// other APIs: use the same mask as coherent
	.dma_mask = &shregion_dev.coherent_dma_mask,
};

static uint64_t shregion_get_uaddr_from_node(struct shr_meta_priv *meta_node)
{
	int i;

	for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {

		if (meta_node->pid_fd_info[i].pid == current->tgid) {

			if (meta_node->pid_fd_info[i].shr_uvaddr_valid) {
				return meta_node->pid_fd_info[i].shr_uvaddr;
			}
		}
	}
	return 0;
}

void *shregion_core_get_phys_addr(uint64_t user_virt_addr)
{
	int ret = 0;
	struct shr_meta_priv *meta_node;
	uint64_t this_node_virt;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.flags & SHREGION_INCLUDE_IN_CORE) {
			this_node_virt = shregion_get_uaddr_from_node(meta_node);
			if (this_node_virt == user_virt_addr) {
				ret = ion_buffer_get_phys_addr(meta_node, 0);
				if (ret) {
					mutex_unlock(&shr_meta_mutex);
					return NULL;
				}

				mutex_unlock(&shr_meta_mutex);
				return (void *)(meta_node->meta_drv.phys_addr);
			}
		}
	}

	mutex_unlock(&shr_meta_mutex);

	return NULL;
}

static void shregion_get_phys_addr(struct shregion_metadata_drv *meta_drv)
{
	int ret = 0;
	struct shr_meta_priv *meta_node;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta_drv->meta.name) {
			// Don't return physical address for non-persistent
			// shregion.
			if (!(meta_node->meta_drv.meta.flags &
				SHREGION_PERSISTENT) && !IS_DEBUGBUILD) {
				dprintk(0, "Physaddr req for non-persist shr(%llx) not supported\n",
                                       meta_node->meta_drv.meta.name);
				meta_drv->status =
					SHREGION_MEM_GET_PHYS_FAILED;
				mutex_unlock(&shr_meta_mutex);
				return;
			}

			ret = ion_buffer_get_phys_addr(meta_node, meta_drv->offset);
			if (ret) {
				meta_drv->status = SHREGION_MEM_GET_PHYS_FAILED;
				mutex_unlock(&shr_meta_mutex);
				return;
			}

			meta_drv->phys_addr = meta_node->meta_drv.phys_addr;
			meta_drv->meta.flags = meta_node->meta_drv.meta.flags;
			meta_drv->meta.type = meta_node->meta_drv.meta.type;
			meta_drv->status = SHREGION_MEM_GET_PHYS_SUCCESS;
			mutex_unlock(&shr_meta_mutex);
			return;
		}
	}

	meta_drv->status = SHREGION_MEM_NONEXISTENT;

	mutex_unlock(&shr_meta_mutex);
}

int get_shregion_phys_addr(uint64_t name, phys_addr_t *phys_addr)
{
	struct shregion_metadata_drv meta_drv;

	if (!phys_addr)
		return -1;

	meta_drv.meta.name = name;
	meta_drv.offset = 0;

	shregion_get_phys_addr(&meta_drv);

	if (meta_drv.status == SHREGION_MEM_GET_PHYS_SUCCESS) {
		*phys_addr = meta_drv.phys_addr;
		return 0;
	}

	return -1;
}
EXPORT_SYMBOL(get_shregion_phys_addr);

static void shregion_free(struct shregion_metadata_drv *meta_drv)
{
	int i, j;
	struct shr_meta_priv *meta_node, *temp_node;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry_safe(meta_node, temp_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta_drv->meta.name) {

			// Remove this PID's entry if it's present.
			for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {
				if (meta_node->pid_fd_info[i].pid ==
						current->tgid) {
					/*
					 * If stream id hasn't been released don't free
					 * pid_fd_info slot just yet. Set pid to some
					 * unlikely value.
					 */
					if (meta_node->pid_fd_info[i].sid == (sid_t)-1) {
						meta_node->pid_fd_info[i].pid = -1;
						meta_node->num_of_ref_pids--;
					} else {
						meta_node->pid_fd_info[i].pid = 0;
					}

					meta_node->pid_fd_info[i].fd = -1;
					meta_node->pid_fd_info[i].shr_uvaddr_valid = 0;

					// Temporarily using SHREGION_PERSISTENT here as a flag to persist
					// IOMMU mappings for DMAC shareable shregions to help prevent exhausting
					// the iova allocator pool, since allocated addresses aren't returned.
					// TODO(jirick): Integrate Linux IOVA allocator 'free' here and ensure
					// addresses are reusable after a shregion is unmapped.
					if ((meta_node->meta_drv.meta.flags &
						SHREGION_DMAC_SHAREABLE) && !(meta_node->meta_drv.meta.flags &
							SHREGION_PERSISTENT)) {
						for (j = 0; j < meta_node->dmac_meta.pid_ref_count; j++) {
							if (meta_node->dmac_meta.pid[j] == current->tgid) {
								meta_node->dmac_meta.pid_ref_count--;
								break;
							}
						}
						if (meta_node->dmac_meta.pid_ref_count == 0) {
							iommu_unmap(
								meta_node->dmac_meta.dmac_sid_domain,
								meta_node->dmac_meta.dmac_ioaddr,
								meta_node->dmac_meta.dmac_mapped_size
							);

							meta_node->dmac_meta.dmac_mapped_size = 0;
							meta_node->dmac_meta.dmac_ioaddr = 0;
						}
					}

					if (!(meta_node->meta_drv.meta.flags &
					CDP_AND_DCPRS_MMU_MAP_PERSIST) &&
					(meta_node->cdp_dcprs_meta.creator_pid == current->tgid) &&
					(meta_node->cdp_dcprs_meta.mapped_size != 0)) {

						iommu_unmap(
						meta_node->cdp_dcprs_meta.sid_domain,
						meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr,
						meta_node->cdp_dcprs_meta.mapped_size
						);

						meta_node->cdp_dcprs_meta.mapped_size = 0;
						meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr = 0;
					}

					break;
				}
			}

			if ((meta_node->num_of_ref_pids == 0) &&
					!(meta_node->meta_drv.meta.flags &
						SHREGION_PERSISTENT)) {
				// Destroy shregion if num_of_ref_pids is 0 and
				// shregion is non-persistent,
				// as no one else is referencing it apart from
				// this process who has called shregion_free().
				meta_node->k_virt_addr = NULL;
				list_del(&meta_node->list);
				kfree(meta_node);
				dprintk(0, "non-prst-shr(name=%llx) destroyed.\n",
					meta_drv->meta.name);
			}
			meta_drv->status = SHREGION_MEM_FREE_SUCCESS;
			mutex_unlock(&shr_meta_mutex);
			return;
		}
	}

	mutex_unlock(&shr_meta_mutex);
	meta_drv->status = SHREGION_MEM_NONEXISTENT;
}

static void deregister_sid(sid_t sid)
{
	int nr_of_entries, i;

	nr_of_entries = sid_meta.nr_of_entries;

	for (i = 0; i < nr_of_entries; i++) {
		if (sid_meta.sid_virt_addr_map[i].sid == sid) {
			if (sid_meta.sid_virt_addr_map[i].mapped_size != 0) {
				iommu_unmap(sid_meta.sid_virt_addr_map[i].sid_domain,
					sid_meta.sid_virt_addr_map[i].meta_virt_addr,
					sid_meta.sid_virt_addr_map[i].mapped_size);
				sid_meta.sid_virt_addr_map[i].mapped_size = 0;
			}

			sid_meta.sid_virt_addr_map[i].entry_valid = ENTRY_IS_INVALID;

			dprintk(2, "deregistering sid = 0x%x\n",
				sid_meta.sid_virt_addr_map[i].sid);

			return;
		}
	}
}

static void shregion_get_metadata(struct shregion_metadata_drv *meta_drv)
{
	struct shr_meta_priv *meta_node;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta_drv->meta.name) {
			memcpy(&(meta_drv->meta), &(meta_node->meta_drv.meta),
				sizeof(struct shregion_metadata));
			meta_drv->status = SHREGION_MEM_META_SUCCESS;
			mutex_unlock(&shr_meta_mutex);
			return;
		}
	}

	mutex_unlock(&shr_meta_mutex);
	meta_drv->status = SHREGION_MEM_META_FAILED;
}

static long shregion_ioctl(struct file *filp, unsigned int cmd,
				    unsigned long arg)
{
	int retval = 0;
	struct shregion_metadata_drv meta_drv;
	struct dsp_shregion_metadata_drv dsp_shr_meta_drv;
	struct dsp_shregion_vaddr_meta dsp_vaddr_meta;
	struct cdp_dcprs_shr_ioaddr_meta cdp_dcprs_ioaddr_meta;
	struct dmac_shregion_ioaddr_meta dmac_ioaddr_meta;
	struct shr_slc_sync_meta slc_sync_meta;
	struct dmac_reg_user_buf_meta dmac_user_buf_meta;

	if (!shregion_access_ok(VERIFY_WRITE, (void *)arg,
		sizeof(struct shregion_metadata_drv))) {
		dprintk(0, "Invalid user-space buffer.\n");
		return -EINVAL;
	}

	if (_IOC_TYPE(cmd) != SHREGION_IOC_MAGIC) {
		dprintk(0, "Invalid ioctl.\n");
		return -EINVAL;
	}

	if (cmd != SHREGION_IOC_DSP_META_UPDATE &&
		cmd != SHREGION_IOC_GET_DSP_VADDR &&
		cmd != SHREGION_IOC_REGISTER_SID_META &&
		cmd != SHREGION_IOC_SLC_SYNC) {
		if (copy_from_user((void *)&meta_drv, (void *)arg,
			sizeof(struct shregion_metadata_drv))) {
			dprintk(0, "copy_from_user failed.\n");
			return -EFAULT;
		}

                if (!(nova_present || (cvip_only == 0)) && (meta_drv.meta.type == SHREGION_DDR_SHARED)) {
			// There's no shared ddr in cvip only mode.
			dprintk(0, "shregions - shared ddr switched to private\n");
			meta_drv.meta.type = SHREGION_DDR_PRIVATE;
		}
	}

	switch (cmd) {

	case SHREGION_IOC_REGISTER:
		retval = shregion_register(&meta_drv, NULL);
		trace_register(&meta_drv, retval);
		break;

	case SHREGION_IOC_QUERY:
		retval = shregion_query_entry(&meta_drv, filp);
		trace_query(&meta_drv, retval);
		if (copy_to_user((void *)arg, &meta_drv,
			sizeof(struct shregion_metadata_drv))) {
			dprintk(0, "query ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}
		break;

	case SHREGION_IOC_GET_PHYS_ADDR:
		shregion_get_phys_addr(&meta_drv);
		trace_get_phys_addr(&meta_drv, 0);
		if (copy_to_user((void *)arg, &meta_drv,
			sizeof(struct shregion_metadata_drv))) {
			dprintk(0, "get phys ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}
		break;

	case SHREGION_IOC_DSP_META_UPDATE:
		if (copy_from_user((void *)&dsp_shr_meta_drv, (void *)arg,
			sizeof(struct dsp_shregion_metadata_drv))) {
			dprintk(0, "dsp meta update ioctl cmd: copy_from_user failed.\n");
			return -EFAULT;
		}

		retval = dsp_shregion_metadata_update(&dsp_shr_meta_drv);
		trace_dsp_metadata_update(&dsp_shr_meta_drv, retval);

		if (!retval && (dsp_shr_meta_drv.sid == DSP_FIXED_DATA_SID)) {

			retval = shregion_register_sid_meta(DSP_FIXED_INST_SID);
			trace_register_sid_meta(DSP_FIXED_INST_SID, retval);

			retval |= shregion_register_sid_meta(DSP_FIXED_IDMA_SID);
			trace_register_sid_meta(DSP_FIXED_IDMA_SID, retval);

			dsp_shr_meta_drv.sid = DSP_FIXED_INST_SID;
			retval |= dsp_shregion_metadata_update(&dsp_shr_meta_drv);
			trace_dsp_metadata_update(&dsp_shr_meta_drv, retval);

			dsp_shr_meta_drv.sid = DSP_FIXED_IDMA_SID;
			retval |= dsp_shregion_metadata_update(&dsp_shr_meta_drv);
			trace_dsp_metadata_update(&dsp_shr_meta_drv, retval);

			if (retval) {
				dprintk(0, "shregion(0x%llx) duplication failed\n",
						dsp_shr_meta_drv.shr_name);
			}
		}

		return retval;

	case SHREGION_IOC_GET_DSP_VADDR:
		if (copy_from_user((void *)&dsp_vaddr_meta, (void *)arg,
			sizeof(struct dsp_shregion_vaddr_meta))) {
			dprintk(0, "get dsp vaddr ioctl cmd: copy_from_user failed.\n");
			return -EFAULT;
		}

		retval = get_shregion_dsp_vaddr(&dsp_vaddr_meta);
		trace_get_dsp_vaddr(&dsp_vaddr_meta, retval);

		if (copy_to_user((void *)arg, &dsp_vaddr_meta,
			sizeof(struct dsp_shregion_vaddr_meta))) {
			dprintk(0, "get dsp vaddr ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}

		break;

	case SHREGION_IOC_GET_CDP_DCPRS_IOADDR:
		if (copy_from_user((void *)&cdp_dcprs_ioaddr_meta, (void *)arg,
			sizeof(struct cdp_dcprs_shr_ioaddr_meta))) {
			dprintk(0, "get cdp dcprs vaddr ioctl cmd: copy_from_user failed.\n");
			return -EFAULT;
		}

		retval = get_shr_cdp_dcprs_ioaddr(&cdp_dcprs_ioaddr_meta);
		trace_get_shr_cdp_dcprs_ioaddr(&cdp_dcprs_ioaddr_meta, retval);

		//(void)test_cdp_alloc();

		if (copy_to_user((void *)arg, &cdp_dcprs_ioaddr_meta,
			sizeof(struct cdp_dcprs_shr_ioaddr_meta))) {
			dprintk(0, "get cdp dcprs ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}

		break;

	case SHREGION_IOC_CDP_DCPRS:
		retval = shregion_register_cdp_dcprs(&meta_drv);
		trace_register_cdp_dcprs(&meta_drv, retval);
		return retval;

	case SHREGION_IOC_S_INTF_SPCIE:
		retval = shregion_register_s_intf_spcie(&meta_drv);
		return retval;

	case SHREGION_IOC_REGISTER_SID_META:
		if ((uint32_t)arg == DSP_FIXED_INST_SID || (uint32_t)arg == DSP_FIXED_DATA_SID
			|| (uint32_t)arg == DSP_FIXED_IDMA_SID)
			arg = DSP_FIXED_DATA_SID;

		retval = shregion_register_sid_meta(arg);
		trace_register_sid_meta(arg, retval);
		return retval;

	case SHREGION_IOC_SLC_SYNC:
		if (copy_from_user((void *)&slc_sync_meta, (void *)arg,
			sizeof(struct shr_slc_sync_meta))) {
			dprintk(0, "slc sync ioctl cmd: copy_from_user failed.\n");
			return -EFAULT;
		}
		retval = shregion_slc_sync(&slc_sync_meta);
		trace_slc_sync(&slc_sync_meta, retval);
		return retval;

	case SHREGION_IOC_FREE:
		shregion_free(&meta_drv);
		trace_free(&meta_drv, 0);
		if (copy_to_user((void *)arg, &meta_drv,
			sizeof(struct shregion_metadata_drv))) {
			dprintk(0, "free ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}
		break;

	case SHREGION_IOC_META:
		shregion_get_metadata(&meta_drv);
		trace_get_metadata(&meta_drv, 0);
		if (copy_to_user((void *)arg, &meta_drv,
			sizeof(struct shregion_metadata_drv))) {
			dprintk(0, "meta ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}
		break;

	case SHREGION_IOC_REGISTER_DMAC_BUF:
		if (copy_from_user((void*)&dmac_user_buf_meta, (void*)arg,
			sizeof(struct dmac_reg_user_buf_meta))) {
			dprintk(0, "register dmac user buf ioctl cmd: copy_from_user failed.\n");
			return -EFAULT;
		}

		retval = shregion_dmac_register_user_buf(&dmac_user_buf_meta);

		if (copy_to_user((void*)arg, &dmac_user_buf_meta,
			sizeof(struct dmac_reg_user_buf_meta))) {
			dprintk(0, "register dmac user buf ioctl cmd: copy_from_user failed.\n");
			return -EFAULT;
		}

		break;

	case SHREGION_IOC_GET_DMAC_IOADDR:
		if (copy_from_user((void*)&dmac_ioaddr_meta, (void*)arg,
			sizeof(struct dmac_shregion_ioaddr_meta))) {
			dprintk(0, "get dmac ioaddr ioctl cmd: copy_from_user failed.\n");
			return -EFAULT;
		}

		retval = get_shr_dmac_ioaddr(&dmac_ioaddr_meta);
		//trace_get_shr_dmac_ioaddr(&dmac_ioaddr_meta, retval); TODO(jirick): implement

		if (copy_to_user((void*)arg, &dmac_ioaddr_meta,
			sizeof(struct dmac_shregion_ioaddr_meta))) {
			dprintk(0, "get dmac ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}

		break;

	case SHREGION_IOC_DMAC:
		retval = shregion_register_dmac(&meta_drv);
		//trace_register_dmac(&meta_drv, retval);
		return retval;

	default:
		dprintk(0, "shregion: got bad cmd 0x%08x\n", cmd);
		return -EINVAL;
	}

	return retval;
}

static void _shregion_smmu_unmap(sid_t stream_id)
{
	int i;
	int sid_idx = -1;
	bool log_start = true;
	bool log_end = false;
	struct shr_meta_priv *meta_node, *temp_node;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry_safe(meta_node, temp_node, &shregion_nodes, list) {

		if (meta_node->meta_drv.meta.flags & SHREGION_DSP_SHAREABLE) {

			for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {

				if (meta_node->pid_fd_info[i].sid == stream_id) {

					if (meta_node->pid_fd_info[i].mapped_size != 0) {

						if (log_start) {
							dprintk(0, "UNMAP_SHREGIONS start for i=%d,sid=0x%x,pid=%d\n",
								i, stream_id, current->tgid);
							log_start = false;
							log_end = true;
						}

						iommu_unmap(
						meta_node->pid_fd_info[i].sid_domain,
						meta_node->pid_fd_info[i].shr_dsp_vaddr,
						meta_node->pid_fd_info[i].mapped_size
						);

						dprintk(3, "%s:(%d,%d): iommu unmap(%llx) v=%x\n",
							__func__, current->pid, current->tgid,
							meta_node->meta_drv.meta.name,
							meta_node->pid_fd_info[i].shr_dsp_vaddr);

						/*
						 * Decrement dmabuf ref count to get it back
						 * to the ION pool.
						 */
						dma_buf_put(meta_node->dmabuf);

						sid_idx = meta_node->pid_fd_info[i].shared_meta_sid_idx;

						--dsp_shregion_meta->sids_meta[sid_idx].nr_of_shr_entries;

						meta_node->pid_fd_info[i].sid = -1;
						meta_node->pid_fd_info[i].mapped_size = 0;
						meta_node->pid_fd_info[i].shr_dsp_vaddr = 0;

						/* If arm process is no longer attached release pid_fd_info slot */
						if (!meta_node->pid_fd_info[i].pid) {
							meta_node->pid_fd_info[i].pid = -1;
							meta_node->num_of_ref_pids--;
						}

						trace_free(&meta_node->meta_drv, 0);

						// Destroy the shregion if it's non-persistent and
						// it's num_of_ref_pids has reached 0.
						if (!(meta_node->meta_drv.meta.flags &
							SHREGION_PERSISTENT) &&
							(meta_node->num_of_ref_pids == 0)) {
							list_del(&meta_node->list);
							meta_node->k_virt_addr = NULL;
							dprintk(0, "%s: non-persistent shr(name=%llx) freed.\n",
								__func__, meta_node->meta_drv.meta.name);
							kfree(meta_node);
						}

						// There can be only one stream attached to a pid
						break;
					}
				}
			}
		}
	}

	if (log_end) {
		dprintk(0, "UNMAP_SHREGIONS end for i=%d,sid=0x%x,pid=%d\n",
				i, stream_id, current->tgid);
		log_end = false;
	}

	if (sid_idx != -1) {
		if (dsp_shregion_meta->sids_meta[sid_idx].nr_of_shr_entries) {
			dprintk(0, "FATAL: nr of entries (%d) must be 0 for sid (%d)",
				dsp_shregion_meta->sids_meta[sid_idx].nr_of_shr_entries,
				stream_id);
		}
	}

	deregister_sid(stream_id);
	mutex_unlock(&shr_meta_mutex);
}

// Must be called while holding the gsm spinlock
// NOTE: No need to hold mutex in this API. This API
// will be the only one to be accessing shared
// (with arm-only tasks) metadata since it gets
// called once the stream id reference count
// drops to 0.
void shregion_smmu_unmap(sid_t stream_id)
{
	_shregion_smmu_unmap(stream_id);

	if (stream_id == DSP_FIXED_DATA_SID) {
		_shregion_smmu_unmap(DSP_FIXED_INST_SID);
		_shregion_smmu_unmap(DSP_FIXED_IDMA_SID);
	}
}
EXPORT_SYMBOL(shregion_smmu_unmap);

static int _shregion_smmu_map(sid_t stream_id)
{
	int i;
	int ret = 0;
	int sid_idx = -1;
	bool log_start = true;
	bool log_end = false;
	struct shr_meta_priv *meta_node;
	struct dsp_shregion_metadata_drv dsp_shr_meta_drv;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {

		if (meta_node->meta_drv.meta.flags & SHREGION_DSP_SHAREABLE) {

			for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {

				if (meta_node->pid_fd_info[i].pid == current->tgid) {

					if (meta_node->pid_fd_info[i].mapped_size == 0) {

						if (log_start) {
							dprintk(0, "MAP_SHREGIONS start for i=%d,sid=0x%x,pid=%d\n",
								i, stream_id, current->tgid);
							log_start = false;
							log_end = true;
						}

						ret = shregion_register_sid_meta_unlocked(stream_id);
						trace_register_sid_meta(stream_id, ret);
						if (ret) {
							dprintk(0, "MAP_SHREGIONS register sid meta failed for i=%d,sid=0x%x,pid=%d\n",
								i, stream_id, current->tgid);
							mutex_unlock(&shr_meta_mutex);
							return ret;
						}

						dsp_shr_meta_drv.shr_name = meta_node->meta_drv.meta.name;
						dsp_shr_meta_drv.shr_size_bytes = meta_node->meta_drv.meta.size_bytes;
						dsp_shr_meta_drv.shr_cache_mask = meta_node->meta_drv.meta.cache_mask;
						dsp_shr_meta_drv.buf_fd = meta_node->pid_fd_info[i].fd;
						dsp_shr_meta_drv.sid = stream_id;
						dsp_shr_meta_drv.flags = meta_node->meta_drv.meta.flags;

						ret = dsp_shregion_metadata_update_unlocked(&dsp_shr_meta_drv);
						trace_dsp_metadata_update(&dsp_shr_meta_drv, ret);

						if (ret) {
							dprintk(0, "MAP_SHREGIONS dsp metadata update failed for i=%d,sid=0x%x,pid=%d\n",
								i, stream_id, current->tgid);
							mutex_unlock(&shr_meta_mutex);
							return ret;
						}

						sid_idx = meta_node->pid_fd_info[i].shared_meta_sid_idx;
						// There can be only one stream attached to a pid
						break;
					}
				}
			}
		}
	}

	if (log_end) {
		dprintk(0, "MAP_SHREGIONS end for i=%d,sid=%x,pid=%d\n",
				i, stream_id, current->tgid);
		log_end = false;
	}

	mutex_unlock(&shr_meta_mutex);
	return ret;
}

int shregion_smmu_map(sid_t stream_id)
{
	int ret;

	ret = _shregion_smmu_map(stream_id);

	if (stream_id == DSP_FIXED_DATA_SID && !ret) {
		ret = _shregion_smmu_map(DSP_FIXED_INST_SID);

		if (ret)
			return ret;

		ret = _shregion_smmu_map(DSP_FIXED_IDMA_SID);
	}

	return ret;
}
EXPORT_SYMBOL(shregion_smmu_map);

// Free all non-persistent shregions whose num_of_ref_pids is 0.
// This func also clears out the fd (for persistent shregion)
// for the terminating process as that fd is going to be invalid.
// Note that dma_buf_put() automatically gets called on process exit.
static void shregion_cleanup(struct task_struct *task)
{
	int i, j;
	int last_valid_idx;
	struct shr_meta_priv *meta_node, *temp_node;

	mutex_lock(&shr_meta_mutex);

	// Check if this process is already registered with shregion
	// driver.
	for (i = 0; i < registered_processes.nr_of_valid_pids; ++i) {
		if (registered_processes.pid_meta[i].pid == task->pid)
			break;
	}

	if (i >= registered_processes.nr_of_valid_pids) {
		mutex_unlock(&shr_meta_mutex);
		return;
	}

	// De-register process id.
	last_valid_idx = registered_processes.nr_of_valid_pids - 1;
	memcpy((void *)&registered_processes.pid_meta[i],
		(void *)&registered_processes.pid_meta[last_valid_idx],
		sizeof(struct pid_meta_s));
	--registered_processes.nr_of_valid_pids;

	list_for_each_entry_safe(meta_node, temp_node, &shregion_nodes, list) {

		if (meta_node->num_of_ref_pids > 0) {
			for (i = 0; i < SHREGION_MAX_REF_PIDS &&
					meta_node->num_of_ref_pids > 0; i++) {
				if (meta_node->pid_fd_info[i].pid == task->pid) {
					/*
					 * If stream id hasn't been released don't free
					 * pid_fd_info slot just yet. Set pid to some
					 * unlikely value.
					 */
					if (meta_node->pid_fd_info[i].sid == (sid_t)-1) {
						meta_node->pid_fd_info[i].pid = -1;
						meta_node->num_of_ref_pids--;
					} else {
						meta_node->pid_fd_info[i].pid = 0;
					}

					meta_node->pid_fd_info[i].fd = -1;
					meta_node->pid_fd_info[i].shr_uvaddr_valid = 0;

					// Temporarily using SHREGION_PERSISTENT here as a flag to persist
					// IOMMU mappings for DMAC shareable shregions to help prevent exhausting
					// the iova allocator pool, since allocated addresses aren't returned.
					// TODO(jirick): Integrate Linux IOVA allocator 'free' here and ensure
					// addresses are reusable after a shregion is unmapped.
					if ((meta_node->meta_drv.meta.flags &
						SHREGION_DMAC_SHAREABLE) && !(meta_node->meta_drv.meta.flags &
							SHREGION_PERSISTENT)) {
						for (j = 0; j < meta_node->dmac_meta.pid_ref_count; j++) {
							if (meta_node->dmac_meta.pid[j] == task->tgid) {
								meta_node->dmac_meta.pid_ref_count--;
								break;
							}
						}
						if (meta_node->dmac_meta.pid_ref_count == 0) {
							iommu_unmap(
								meta_node->dmac_meta.dmac_sid_domain,
								meta_node->dmac_meta.dmac_ioaddr,
								meta_node->dmac_meta.dmac_mapped_size
							);

							meta_node->dmac_meta.dmac_mapped_size = 0;
							meta_node->dmac_meta.dmac_ioaddr = 0;
						}
					}

					if (!(meta_node->meta_drv.meta.flags &
						CDP_AND_DCPRS_MMU_MAP_PERSIST) &&
						(meta_node->cdp_dcprs_meta.creator_pid == current->tgid) &&
						(meta_node->cdp_dcprs_meta.mapped_size != 0)) {

						iommu_unmap(
						meta_node->cdp_dcprs_meta.sid_domain,
						meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr,
						meta_node->cdp_dcprs_meta.mapped_size
						);

						dprintk(3, "%s:(%d,%d): cdp/dcprs iommu unmap(%llx) v=%x\n",
							__func__, task->pid, task->tgid,
							meta_node->meta_drv.meta.name,
							meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr);

						meta_node->cdp_dcprs_meta.mapped_size = 0;
						meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr = 0;
					}
				}
			}
		}

		// Destroy the shregion if it's non-persistent and
		// it's num_of_ref_pids has reached 0.
		if (!(meta_node->meta_drv.meta.flags &
			SHREGION_PERSISTENT) &&
			(meta_node->num_of_ref_pids == 0)) {
			list_del(&meta_node->list);
			meta_node->k_virt_addr = NULL;
			dprintk(0, "%s: non-persistent shr(name=%llx) freed.\n",
				__func__, meta_node->meta_drv.meta.name);
			kfree(meta_node);
		}
	}

	mutex_unlock(&shr_meta_mutex);
}

// If this get called, assume that the process is about to terminate/crash.
// The shregion library doesn't explicitly call close() for /dev/shregion
// interface and hence the only way it can be called is when the process
// terminates.
static int shregion_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static void check_for_shr_guard_bands_violation(void)
{
	int i;
	uint8_t *guard_band_vaddr;
	struct shr_meta_priv *meta_node;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {

		if (meta_node->guard_bands_config && !meta_node->violation_already_logged) {
			guard_band_vaddr = (uint8_t *)meta_node->k_virt_addr;

			for (i = 0; i < SHREGION_GUARD_BAND_SIZE_BYTES; ++i) {
				if (guard_band_vaddr[i] != SHREGION_GUARD_BAND_PATTERN) {
					dprintk(0, "WARNING: shr(name=%llx) upper guard band violated\n",
						meta_node->meta_drv.meta.name);
					meta_node->violation_already_logged = true;
					break;
				}
			}

			guard_band_vaddr = (uint8_t *)((uintptr_t)meta_node->k_virt_addr +
						meta_node->meta_drv.meta.size_bytes - SHREGION_GUARD_BAND_SIZE_BYTES);

			for (i = 0; i < SHREGION_GUARD_BAND_SIZE_BYTES; ++i) {
				if (guard_band_vaddr[i] != SHREGION_GUARD_BAND_PATTERN) {
					dprintk(0, "WARNING: shr(name=%llx) lower guard band violated\n",
						meta_node->meta_drv.meta.name);
					meta_node->violation_already_logged = true;
					break;
				}
			}
		}
	}

	mutex_unlock(&shr_meta_mutex);
}

static void check_shr_guard_bands_violation(struct work_struct *dummy)
{
	check_for_shr_guard_bands_violation();

	schedule_delayed_work(&check_shr_work, round_jiffies_relative(violation_chk_period_s * HZ));
}

static int task_notifier(struct notifier_block *self, unsigned long cmd, void *v)
{
	shregion_cleanup(v);
	return 0;
}

// callback for whenever a task (that includes threads
// and processes) exits. We're using this feature, primarily
// because we want cleanup to be called for userspace processes.
static struct notifier_block task_notifier_block = {
	.notifier_call	= task_notifier,
};

static const struct file_operations shregion_fops = {
	.owner		= THIS_MODULE,
	.open		= shregion_open,
	.unlocked_ioctl	= shregion_ioctl,
	.compat_ioctl	= shregion_ioctl,
	.release	= shregion_close,
};

struct miscdevice shregion_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "shregion",
	.fops = &shregion_fops,
};

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_root;
#endif

#ifdef ARM_CVIP
extern int get_nova_status(void);
static int nova_alive_notify(struct notifier_block *self,
				   unsigned long v, void *p)
{
	nova_present = 1;

	return 0;
}

static struct notifier_block nova_alive_notifier = {
	.notifier_call = nova_alive_notify,
};

static void register_nova_alive_notify(void)
{
	atomic_notifier_chain_register(&nova_event_notifier_list,
					       &nova_alive_notifier);
}
#endif

static int __init shregion_init(void)
{
	int error;
	uint32_t retval;
	char cvip_only_prop[32] = {0};
	struct kobject *dev_kobj;
	int rc;

	error = misc_register(&shregion_device);
	if (error) {
		dprintk(0, "Failed to register cvip shregion misc module\n");
		return error;
	}

	shregion_app_register();
#ifdef ARM_CVIP
	cvip_plt_property_get_safe("persist.boot.mode", cvip_only_prop,
						sizeof(cvip_only_prop), "dual");
	/* looking for "cvip-only" to say that we are in cvip only mode */
        dprintk(0, "shregion : boot mode %s\n", cvip_only_prop);
	error = strncmp("dual", cvip_only_prop, 4);
	if (!error)
		cvip_only = 0;
	else
		cvip_only = 1;

	register_nova_alive_notify();
	retval = get_nova_status();
	if (!retval)
		nova_present = 0;

        // Like Documentation/kobject.txt example, but using sysfs_create_bin_file instead of sysfs_create_file
        // so that we can return more than a page of bytes.
	dev_kobj = kobject_create_and_add("debug", &(shregion_device.this_device->kobj));
	rc = sysfs_create_bin_file(dev_kobj, &bin_attr_dsp_maps);
#endif
	if (cvip_only)
		dprintk(0, "shregion : cvip only mode enabled\n");

	dprintk(0, "shregion : nova_present %d : %d\n", nova_present, cvip_only);

	memset((void *)&sid_meta, 0, sizeof(struct sid_meta_s));

	retval = init_x86_shared_meta();
	if (retval) {
		dprintk(0, "init_x86_shared_meta failed, ret=%d\n", retval);
		return retval;
	}

	retval = register_xpc_shr_command_handler();
	if (retval != 0)
		return retval;

	retval = profile_event_register(PROFILE_TASK_EXIT, &task_notifier_block);
	if (retval) {
		dprintk(0, "Failed to register task exit event\n");
		return retval;
	}

	// Init CDP pool
	init_cdp_dcprs_pool();

	retval = alloc_dsp_dma_buf_resources();
	if (retval) {
		dprintk(0, "alloc_dsp_dma_buf_resources failed, ret=%d\n", retval);
		return retval;
	}

#ifdef CONFIG_DEBUG_FS
	debugfs_root = debugfs_create_dir("shregion", NULL);
	debugfs_create_file("nodes", 0444, debugfs_root, NULL, &nodes_fops);
	debugfs_create_file("dsp_maps", 0644, debugfs_root, NULL, &dsp_maps_fops);
#endif

	// we run the periodic checks when violation_chk_period_s is set.
	if (get_shregion_guard_bands_status() == GUARD_BANDS_ENABLED && violation_chk_period_s)
		schedule_delayed_work(&check_shr_work, round_jiffies_relative(violation_chk_period_s * HZ));

	dprintk(0, "Successfully loaded cvip shregion misc module\n");

	return 0;
}

static void __exit shregion_exit(void)
{
	int i;
	struct shr_meta_priv *meta_node, *temp_node;

	dprintk(0, "Unloading cvip shregion misc module.\n");

#ifdef CONFIG_DEBUG_FS
	debugfs_remove(debugfs_root);
#endif

	profile_event_unregister(PROFILE_TASK_EXIT, &task_notifier_block);

	if (get_shregion_guard_bands_status() == GUARD_BANDS_ENABLED)
		cancel_delayed_work_sync(&check_shr_work);

	mutex_lock(&shr_meta_mutex);

	deregister_xpc_shr_command_handler();

	// Free up the shregions meta.
	list_for_each_entry_safe(meta_node, temp_node, &shregion_nodes,
				list) {
		list_del(&meta_node->list);
		if (meta_node->sg_table)
			dma_buf_unmap_attachment(meta_node->dma_attach,
				meta_node->sg_table, DMA_BIDIRECTIONAL);
		if (meta_node->dma_attach)
			dma_buf_detach(meta_node->dmabuf,
				meta_node->dma_attach);
		if (meta_node->meta_drv.meta.flags &
					SHREGION_PERSISTENT) {
			dma_buf_put(meta_node->dmabuf);
			if (mydebug)
				dprintk(0, "\tprst-shr(name=%llx)",
					meta_node->meta_drv.meta.name);
		}

		if ((meta_node->meta_drv.meta.flags & SHREGION_DSP_CODE) ||
			(meta_node->meta_drv.meta.flags & SHREGION_DSP_MAP_ALL_STREAM_IDS)) {
			for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {
				if (meta_node->pid_fd_info[i].mapped_size
					!= 0) {
					iommu_unmap(
						meta_node->pid_fd_info[i].sid_domain,
						meta_node->pid_fd_info[i].shr_dsp_vaddr,
						meta_node->pid_fd_info[i].mapped_size
						);
				}
			}
		}

		if ((meta_node->meta_drv.meta.flags &
			SHREGION_DMAC_SHAREABLE) &&
			(meta_node->dmac_meta.dmac_mapped_size != 0)) {
				iommu_unmap(
					meta_node->dmac_meta.dmac_sid_domain,
					meta_node->dmac_meta.dmac_ioaddr,
					meta_node->dmac_meta.dmac_mapped_size
					);
		}

		if ((meta_node->meta_drv.meta.flags &
			CDP_AND_DCPRS_MMU_MAP_PERSIST) &&
			(meta_node->cdp_dcprs_meta.mapped_size != 0)) {
			iommu_unmap(
			meta_node->cdp_dcprs_meta.sid_domain,
			meta_node->cdp_dcprs_meta.shr_cdp_dcprs_ioaddr,
			meta_node->cdp_dcprs_meta.mapped_size
			);
		}

		if (meta_node->s_intf_spcie_meta.mapped_size != 0) {
			iommu_unmap(
			meta_node->s_intf_spcie_meta.sid_domain,
			meta_node->s_intf_spcie_meta.shr_s_intf_spcie_ioaddr,
			meta_node->s_intf_spcie_meta.mapped_size
			);
		}

		if (meta_node->s_intf_sram_x86_meta.mapped_size != 0) {
			iommu_unmap(
			meta_node->s_intf_sram_x86_meta.sid_domain,
			meta_node->s_intf_sram_x86_meta.shr_s_intf_sram_x86_ioaddr,
			meta_node->s_intf_sram_x86_meta.mapped_size
			);
		}

		meta_node->k_virt_addr = NULL;
		kfree(meta_node);
	}

	if (dsp_buf_info.dmabuf)
		free_dsp_dma_buf_resources();

	mutex_unlock(&shr_meta_mutex);

	shregion_app_deregister();
	misc_deregister(&shregion_device);
}

#ifdef ARM_CVIP
late_initcall_sync(shregion_init);
#else
module_init(shregion_init);
#endif
module_exit(shregion_exit);

MODULE_AUTHOR("Sumit Jain <sjain@magicleap.com>");
MODULE_DESCRIPTION("cvip shregion misc module");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.4");

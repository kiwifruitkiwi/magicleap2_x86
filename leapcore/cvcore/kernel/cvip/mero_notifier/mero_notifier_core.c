// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2020-2022
// Magic Leap, Inc. (COMPANY)
//

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/profile.h>
#include <linux/rbtree.h>
#include <linux/syscalls.h>
#include <linux/iommu.h>
#include <linux/dma-buf.h>

#define CREATE_TRACE_POINTS
#include "mero_notifier_trace.h"

#include "mero_notifier_meta.h"
#include "mero_notifier_private.h"
#include "cvcore_processor_common.h"

#ifdef ARM_CVIP
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif
#include <asm/cacheflush.h>
#include <linux/mero/cvip/biommu.h>
#include <linux/mero/cvip/cvip_dsp.h>
#include "gsm_spinlock.h"
#include "shregion_internal.h"
#include "mero_smmu_private.h"
#include "ion/ion.h"
#define CVIP_ARM (true)
#else
#include "linux/cvip_status.h"
#define CVIP_ARM (false)
#endif

#ifdef X86_MERO
#define MERO_X86 (true)
#else
#define MERO_X86 (false)
#endif

#include "mero_xpc_kapi.h"
#include "cvcore_xchannel_map.h"

#define MODULE_NAME "mero_notifier"

static unsigned int mydebug;
module_param(mydebug, uint, 0644);
MODULE_PARM_DESC(mydebug, "Activates local debug info");

// Max PIDs processed by a task during the signal call.
#define MAX_PROCESSED_PIDS		(24)

#define MAX_BIOMMU_STR_LEN_BYTES	(32)

#define dprintk(level, fmt, arg...)             \
  do {                                          \
    if (level <= mydebug)                       \
      pr_info(MODULE_NAME ": "fmt, ## arg);     \
  } while (0)

#define dprintkrl(level, fmt, arg...)           \
  do {                                          \
    if (level <= mydebug)                       \
      printk_ratelimited(KERN_INFO MODULE_NAME ": " fmt, ## arg); \
  } while (0)


#define INVALID_TASK_WAIT_ORDER_INDEX	(U64_MAX)

#define TASK_NO_WAKE_UP		(0)
#define TASK_WOKE_UP		(1)
#define NO_NEW_SIG		(-1)
#define NEW_SIG			(0)
#define REGISTER_TASK	(1)
#define DEREGISTER_TASK	(0)

// structure representing a node in rbtree where
// each node is representing a cvcore name.
struct cvcore_name_meta_priv {
	struct rb_node node;
	// cvcore name
	uint64_t name;
	// tasks interested in this cvcore name.
	struct task_meta_s {
		struct task_struct *task;
		// storing the pointer to node here so that we
		// don't have to loop task_meta_nodes linked list.
		struct task_meta_priv *task_node;
		// Used for notify-only-one feature
		// We scan this to find the order of
		// tasks waiting on a cvcore name, so that
		// we can provide FIFO servicing.
		uint64_t task_wait_order_index;
	} task_meta[MAX_TASKS_PER_CVCORE_NAME];
	// payload for each cvcore name.
	uint8_t payload[MAX_PAYLOAD_SIZE_BYTES];
	uint8_t payload_len_bytes;
	// the serial number is incremented on reception of a
	// new signal for a cvcore name.
	uint32_t signal_serial_num;
	// which latest task signalled this name?
	struct task_struct *latest_signaling_task;
	// if this dsp shared cvcore name, then this
	// field contains it's index into shared dsp meta
	int dsp_meta_name_index;
	// number of tasks interested in this cvcore name
	int nr_of_tasks;
	// lock to protect changes to cvcore name node.
	rwlock_t name_rw_lock;

	// All the following elements are sed for
	// notify-only-one feature
	bool notify_only_one_enabled;
	// Denotes if the latest signal to this cvcore
	// name has been consumed by atleast one task
	// that was waiting on it
	bool signal_consumed;
	// How many tasks are waiting on this cvcore
	// name at the moment?
	int nr_of_tasks_waiting;
	// Number of times any task was interested
	// in this cvcore name
	uint64_t tasks_interested_nr_of_times;
};

// structure representing a node in linked list where
// each node is representing individual task information.
// Basically, it's a linked list of task nodes.
// NOTE: linux kernel treats userspace threads and processes
// as individual tasks. We use the same concept here in this
// module.
struct task_meta_priv {
	struct list_head list;
	struct task_struct *task;
	// number of cvcore names the task is interested in getting
	// notified on.
	int nr_of_watch_cvcore_names;
	// this array keeps the index of registered_name_meta
	// that it is currently interested in getting notified for.
	int watch_cvcore_name_index[MAX_CVCORE_NAMES_PER_META_INSTANCE];
	// Is the task doing a wait_all?
	bool wait_all;

	// number of cvcore names this task has registered so far in total.
	int nr_of_registered_cvcore_names;
	struct {
		bool is_entry_valid;
		uint64_t registered_name;
		uint32_t signal_serial_num;
		int32_t name_node_task_index;
		struct cvcore_name_meta_priv *name_node;
	} registered_name_meta[MAX_CVCORE_NAMES_PER_TASK];

	// lock to be used in poll() and signal().
	// Using raw spinlock instead of just/non-raw spinlock for performance
	// reasons.
	raw_spinlock_t lock;
	// Is the task waiting on any cvcore name?
	bool is_task_waiting;
};

__maybe_unused
static struct dsp_notifier_metadata *dsp_notifier_meta = NULL;

__maybe_unused
static struct shared_dsp_meta_buf_info {
	int fd;
	struct dma_buf *dmabuf;
	void *k_vaddr;
	struct sg_table *sg_table;
	struct dma_buf_attachment *dma_attach;
} dsp_buf_info = {-1, NULL, NULL, NULL, NULL};

// Declare and init the head node of the linked list
LIST_HEAD(task_meta_nodes);

// Init rb tree with empty state.
struct rb_root cvcore_name_rbtree_root = RB_ROOT;

#ifdef ARM_CVIP
static struct sid_meta_s {
	struct {
		uint16_t sid;
		struct iommu_domain *sid_domain;
	} sid_virt_addr_map[TOTAL_NR_OF_STREAM_IDS];
	uint32_t mapped_size;
	uint64_t meta_virt_addr;
} sid_meta;
// cross core gsm backed spinlock
struct gsm_spinlock_context gsm_spinlock_ctx;
#endif

struct local_user_meta_s {
	struct mero_notifier_meta meta;
	uint8_t payload[MAX_CVCORE_NAMES_PER_META_INSTANCE][MAX_PAYLOAD_SIZE_BYTES];
};

// Using read-write lock as we don't manipulate the task
// nodes linked list in poll() and signal() functions and using
// just the read lock potentially improves performance by
// letting in mutliple readers.
static rwlock_t meta_rw_lock;

// The `type` parameter to access_ok was removed in Linux 5.0. Previous releases included
// this parameter but ignored it.
// We can strip out this macro once we move to cvip platform.
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#define mero_notifier_access_ok(type, addr, size) access_ok(addr, size)
#else
#define mero_notifier_access_ok(type, addr, size) access_ok(type, addr, size)
#endif

// forward declaration
static int mero_notifier_signal(unsigned long arg, bool user_space_call);
static int find_dsp_shared_name_index(uint64_t name, int *dsp_meta_name_index,
	int *interested_cores);
static void sync_task_with_dsp_meta(struct cvcore_name_meta_priv *name_node,
	uint8_t register_task);

// We don't lock the list access here since it's already being
// called from a locked region at many places. Make sure that
// this API is properly locked wherever you intend to call it
// for debug purposes.
static void debug_print_out_cvcore_name_rbtree(void)
{
	bool no_entries = true;
	struct cvcore_name_meta_priv *name_node;
	struct rb_node *node;

	dprintk(2, "dumping cvcore name rbtree:\n");

	for (node = rb_first(&cvcore_name_rbtree_root); node; node = rb_next(node)) {
		name_node = rb_entry(node, struct cvcore_name_meta_priv, node);
		dprintk(2, "\tname=%llx,nr_of_tasks=%u", name_node->name, name_node->nr_of_tasks);

		if (no_entries)
			no_entries = false;
	}

	if (no_entries)
		dprintk(2, "\tNULL");
}

#if defined(ARM_CVIP) && defined(CONFIG_DEBUG_FS)
static int nodes_show(struct seq_file *s, void *data)
{
	int i;

	if (dsp_notifier_meta) {
		seq_puts(s, "\nCurr dsp names  \t\tInterested core id\t\tNr of tasks\n");
		seq_puts(s,
		"--------------------------------------------------------------------------\n");
		for (i = 0; i < dsp_notifier_meta->nr_of_valid_cvcore_names; ++i) {
			seq_printf(s,
			"0x%016llx\t\t0x%llx\t\t\t%llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
			dsp_notifier_meta->cvcore_name_meta[i].cvcore_name,
			dsp_notifier_meta->cvcore_name_meta[i].interested_cores,
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_ARM],
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_DSP_Q6_5],
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_DSP_Q6_4],
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_DSP_Q6_3],
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_DSP_Q6_2],
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_DSP_Q6_1],
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_DSP_Q6_0],
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_DSP_C5_1],
			dsp_notifier_meta->cvcore_name_meta[i].nr_of_tasks[CVCORE_DSP_C5_0]);
		}
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(nodes);
#endif

static int mero_notifier_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int mero_notifier_close(struct inode *inode, struct file *filp)
{
	return 0;
}

struct cvcore_name_meta_priv *search_cvcore_name_in_rbtree(uint64_t name)
{
	struct rb_node *node = cvcore_name_rbtree_root.rb_node;

	while (node) {
		struct cvcore_name_meta_priv *data = rb_entry(node, struct cvcore_name_meta_priv, node);

		if (name < data->name)
			node = node->rb_left;
		else if (name > data->name)
			node = node->rb_right;
		else
			return data;
	}
	return NULL;
}

static int add_cvcore_name_node_to_rbtree(struct cvcore_name_meta_priv *new)
{
	struct rb_node **link = &cvcore_name_rbtree_root.rb_node;

	struct rb_node *parent = NULL;
	struct cvcore_name_meta_priv *entry;
	int rc = 0;

	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct cvcore_name_meta_priv, node);

		if (new->name == entry->name)
			goto err_out;

		if (new->name < entry->name)
			link = &(*link)->rb_left;
		else
			link = &(*link)->rb_right;
	}
	rb_link_node(&new->node, parent, link);
	rb_insert_color(&new->node, &cvcore_name_rbtree_root);
	goto out;

err_out:
	rc = -EINVAL;
	dprintk(0, "Entry(name=0x%llx) already exists\n", new->name);
out:
	return rc;
}

// This function must be called while holding the meta_rw_lock (write lock).
static void cleanup_task_from_name_node(struct cvcore_name_meta_priv *name_node,
	struct task_struct *task)
{
	int i, j;
	int nr_of_tasks;
	int new_task_idx;
	unsigned long rw_lock_irq_flags;
	struct task_meta_priv *task_node;
	int name_index = -1;
	int interested_cores;

	write_lock_irqsave(&name_node->name_rw_lock, rw_lock_irq_flags);

	nr_of_tasks = name_node->nr_of_tasks;
	for (i = 0; i < nr_of_tasks; i++) {
		if (name_node->task_meta[i].task->pid == task->pid) {
			name_node->task_meta[i].task = name_node->task_meta[nr_of_tasks - 1].task;
			name_node->task_meta[i].task_node = name_node->task_meta[nr_of_tasks - 1].task_node;
			--name_node->nr_of_tasks;

			new_task_idx = i;

			if (name_node->notify_only_one_enabled) {
				if (name_node->task_meta[i].task_wait_order_index !=
					INVALID_TASK_WAIT_ORDER_INDEX) {
					--(name_node->nr_of_tasks_waiting);
				}

				name_node->task_meta[i].task_wait_order_index =
					name_node->task_meta[nr_of_tasks - 1].task_wait_order_index;
				name_node->task_meta[nr_of_tasks - 1].task_wait_order_index =
				INVALID_TASK_WAIT_ORDER_INDEX;

				// update name_node_task_index for the task we just moved
				// to the dying task slot.
				if (name_node->nr_of_tasks) {
					task_node = name_node->task_meta[i].task_node;
					for (j = 0; j < task_node->nr_of_registered_cvcore_names; ++j) {
						if (task_node->registered_name_meta[j].name_node->name ==
						    name_node->name) {
							task_node->registered_name_meta[j].name_node_task_index =
								new_task_idx;
							break;
						}
					}
				}
			}
			break;
		}
	}
	write_unlock_irqrestore(&name_node->name_rw_lock, rw_lock_irq_flags);

	find_dsp_shared_name_index(name_node->name, &name_index, &interested_cores);
	if (i < nr_of_tasks && name_index >= 0 && (interested_cores & (1 << CVCORE_ARM)))
		sync_task_with_dsp_meta(name_node, DEREGISTER_TASK);
}

#ifdef ARM_CVIP

static int check_cvip_state(struct mero_notifier_meta *meta, bool user_space_call)
{
	return 0;
}

static void free_dsp_dma_buf_resources(void)
{
	if (dsp_buf_info.dmabuf) {
		if (dsp_buf_info.sg_table)
			dma_buf_unmap_attachment(dsp_buf_info.dma_attach,
				dsp_buf_info.sg_table, DMA_BIDIRECTIONAL);
		if (dsp_buf_info.dma_attach)
			dma_buf_detach(dsp_buf_info.dmabuf, dsp_buf_info.dma_attach);

		dma_buf_put(dsp_buf_info.dmabuf);
		ksys_close(dsp_buf_info.fd);
	}
}

static void deregister_sids(void)
{
	int i;

	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; i++) {
		iommu_unmap(sid_meta.sid_virt_addr_map[i].sid_domain,
			sid_meta.meta_virt_addr, sid_meta.mapped_size);
		sid_meta.mapped_size = 0;
	}
}

static void ionbuf_arm_cache_invalidate(struct dma_buf *dmabuf)
{
	int i;
	struct ion_buffer *ion_buf;
	struct sg_table *table;
	struct page *page;
	struct scatterlist *sg;

	ion_buf = dmabuf->priv;
	table = ion_buf->sg_table;

	for_each_sg(table->sgl, sg, table->nents, i) {
		page = sg_page(sg);
		invalidate_dcache_range(page_address(page), PAGE_ALIGN(sg->length));
	}
}

static int register_sid_meta(void)
{
	int i;
	int retval = 0;
	void *vaddr = NULL;
	char biommu_sid_str[MAX_BIOMMU_STR_LEN_BYTES];
	struct iommu_domain *sid_domain = NULL;
	struct device *iommudev_ret = NULL;
	uint32_t mapped_size = 0;
	int iommu_prot;
	int sid;

	uint32_t dsp_notifier_region1_vaddr;
	int dsp_shregion_meta_pg_sz_multiple;
	int dsp_shregion_meta_sz = sizeof(struct dsp_shregion_metadata);

	memset((void *)&sid_meta, 0, sizeof(struct sid_meta_s));

	// Attach this buffer to some streamID device, say 0.
	snprintf(biommu_sid_str, MAX_BIOMMU_STR_LEN_BYTES, "biommu!vdsp@%x", 0);

	// Also, ion_alloc gives page aligned buffer which is mandatory for
	// remap_pfn_page to work properly.
	dsp_buf_info.fd = ion_alloc(sizeof(struct dsp_notifier_metadata),
			1 << ION_HEAP_CVIP_DDR_PRIVATE_ID, ION_FLAG_CVIP_SLC_CACHED_0);
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

	vaddr = dma_buf_kmap(dsp_buf_info.dmabuf, 0);
	if (!vaddr) {
		dprintk(0, "Failed kmap for private ddr dmabuf\n");
		dma_buf_put(dsp_buf_info.dmabuf);
		ksys_close(dsp_buf_info.fd);
		return PTR_ERR(vaddr);
	}

	ionbuf_arm_cache_invalidate(dsp_buf_info.dmabuf);

	dsp_notifier_meta = (struct dsp_notifier_metadata *)vaddr;

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

	memset((void *)dsp_notifier_meta, 0, sizeof(struct dsp_notifier_metadata));

	memcpy((void *)&(dsp_notifier_meta->gsm_spinlock_ctx), (void *)&gsm_spinlock_ctx,
		sizeof(struct gsm_spinlock_context));

	if (dsp_shregion_meta_sz % PAGE_SIZE)
		dsp_shregion_meta_pg_sz_multiple =
		(((dsp_shregion_meta_sz / PAGE_SIZE) + 1) * PAGE_SIZE);
	else
		dsp_shregion_meta_pg_sz_multiple = dsp_shregion_meta_sz;

	dsp_notifier_region1_vaddr = DSP_SHREGION_META_FIXED_VADDR +
		dsp_shregion_meta_pg_sz_multiple;

	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {

		sid = cvcore_stream_ids[i];

		dprintk(2, "creating notifier meta mappings for sid=0x%x", sid);

		snprintf(biommu_sid_str, MAX_BIOMMU_STR_LEN_BYTES, "biommu!vdsp@%x", sid);

		sid_domain = biommu_get_domain_byname(biommu_sid_str);
		if (IS_ERR(sid_domain)) {
			dprintk(0, "Failed to get device for sid=0x%x.\n", sid);
			retval = PTR_ERR(sid_domain);
			return retval;
		}

		iommu_prot = IOMMU_READ | IOMMU_WRITE;

		mapped_size = iommu_map_sg(sid_domain,
					(unsigned long)dsp_notifier_region1_vaddr,
					dsp_buf_info.sg_table->sgl, dsp_buf_info.sg_table->nents,
					iommu_prot);
		if (mapped_size <= 0) {
			dprintk(0, "Failed to create iommu map in domain for sid=0x%x\n", sid);
			return -EFAULT;
		}
		sid_meta.sid_virt_addr_map[i].sid_domain = sid_domain;
		sid_meta.sid_virt_addr_map[i].sid = sid;
	}

	sid_meta.mapped_size = mapped_size;
	sid_meta.meta_virt_addr = dsp_notifier_region1_vaddr;

	return retval;
}

// NOTE: the error returns from this API are non-fatal. We just
// log and try to continue normally.
static void sync_task_with_dsp_meta(struct cvcore_name_meta_priv *name_node,
	uint8_t register_task)
{
	int err;
	int dsp_mn_idx;

	dsp_mn_idx = name_node->dsp_meta_name_index;

	if (dsp_mn_idx == -1)
		return;

	err = gsm_spin_lock(&gsm_spinlock_ctx);
	if (err) {
		dprintk(0, "%s: failed to grab gsm spinlock, err=%d\n",
			__func__, err);
		return;
	}

	if (register_task) {
		++(dsp_notifier_meta->cvcore_name_meta[dsp_mn_idx].nr_of_tasks[CVCORE_ARM]);
	} else {
		--(dsp_notifier_meta->cvcore_name_meta[dsp_mn_idx].nr_of_tasks[CVCORE_ARM]);
		if (!dsp_notifier_meta->cvcore_name_meta[dsp_mn_idx].nr_of_tasks[CVCORE_ARM])
			dsp_notifier_meta->cvcore_name_meta[dsp_mn_idx].interested_cores &=
			~(1ULL << CVCORE_ARM);
	}

	err = gsm_spin_unlock(&gsm_spinlock_ctx);
	if (err)
		dprintk(0, "%s: failed to release gsm spinlock, err=%d\n",
			__func__, err);
}

// Must be called while holding the cross core spinlock
static void set_dsp_meta_arm_interested_core(int dsp_meta_name_index)
{
	uint64_t interested_cores;

	interested_cores =
		dsp_notifier_meta->cvcore_name_meta[dsp_meta_name_index].interested_cores;

	if (!(interested_cores & (1ULL << CVCORE_ARM))) {
		// Read the latest value from mem
		interested_cores =
			dsp_notifier_meta->cvcore_name_meta[dsp_meta_name_index].interested_cores;

		interested_cores |= (1ULL << CVCORE_ARM);

		dsp_notifier_meta->cvcore_name_meta[dsp_meta_name_index].interested_cores =
			interested_cores;
	}
}

// Must be called while holding the cross core spinlock
static int register_dsp_shared_names(
	struct cvcore_name_meta_priv *name_node,
	int *interested_cores
	)
{
	int i;
	int nr_of_valid_names;

	nr_of_valid_names = dsp_notifier_meta->nr_of_valid_cvcore_names;

	// check if we reached limit before we allocate a new slot
	if (nr_of_valid_names >= MAX_NR_OF_DSP_SHARED_NAMES) {
		dprintk(0, "num of dsp shared names %d exceeds maximum limit of %d\n",
			nr_of_valid_names, MAX_NR_OF_DSP_SHARED_NAMES);
		return MERO_NOTIFIER_NAME_NR_QUOTA_EXCEEDED;
	}

	// check once again
	for(i = 0; i < nr_of_valid_names; ++i) {
		if (name_node->name == dsp_notifier_meta->cvcore_name_meta[i].cvcore_name)
			break;
	}

	if (i >= nr_of_valid_names)
		dsp_notifier_meta->cvcore_name_meta[i].cvcore_name = name_node->name;

	if (interested_cores) {
		// We are here from the signal() API
		*interested_cores = dsp_notifier_meta->cvcore_name_meta[i].interested_cores;
	} else {
		// We are here from the wait_any API
		dsp_notifier_meta->cvcore_name_meta[i].interested_cores |=
		(1 << CVCORE_ARM);
	}

	name_node->dsp_meta_name_index = i;

	wmb();

	if (i >= nr_of_valid_names)
		++dsp_notifier_meta->nr_of_valid_cvcore_names;

	return 0;
}

static int find_dsp_shared_name_index(
	uint64_t name,
	int *dsp_meta_name_index,
	int *interested_cores
	)
{
	int i;
	int nr_of_valid_names;

	nr_of_valid_names = dsp_notifier_meta->nr_of_valid_cvcore_names;

	for(i = 0; i < nr_of_valid_names; ++i) {
		if (name == dsp_notifier_meta->cvcore_name_meta[i].cvcore_name) {

			*dsp_meta_name_index = i;

			if (interested_cores)
				*interested_cores =
					dsp_notifier_meta->cvcore_name_meta[i].interested_cores;

			break;
		}
	}

	return 0;
}

// If interested_cores is NULL, this function is getting
// called from wait, else it is getting called from signal.
static int mero_notifier_register_dsp_shared_names(
	struct cvcore_name_meta_priv *name_node,
	int *interested_cores
	)
{
	int ret, err;
	int dsp_meta_name_index = -1;

	ret = find_dsp_shared_name_index(
			name_node->name,
			&dsp_meta_name_index,
			interested_cores
			);

	if (ret != 0) {
		dprintk(0, "%s: find_dsp_shared_name_index failed,ret=%d\n",
			__func__, ret);
		return ret;
	}

	if (dsp_meta_name_index != -1) {

		if (name_node->dsp_meta_name_index == -1)
			name_node->dsp_meta_name_index = dsp_meta_name_index;

		if (!interested_cores) {
			// we can only be here from wait_any api.
			// signal() never sets interested_cores.

			err = gsm_spin_lock(&gsm_spinlock_ctx);
			if (err) {
				dprintk(0, "%s: failed to grab gsm spinlock, err=%d\n",
					__func__, err);
				return err;
			}

			set_dsp_meta_arm_interested_core(dsp_meta_name_index);

			err = gsm_spin_unlock(&gsm_spinlock_ctx);
			if (err) {
				dprintk(0, "%s: failed to release gsm spinlock, err=%d\n",
					__func__, err);
				return err;
			}
		}

	} else {

		err = gsm_spin_lock(&gsm_spinlock_ctx);
		if (err) {
			dprintk(0, "%s: failed to grab gsm spinlock, err=%d\n", __func__, err);
			return err;
		}

		ret = register_dsp_shared_names(name_node, interested_cores);

		if (ret != 0)
			dprintk(0, "%s: register_dsp_shared_names failed,ret=%d\n",
				__func__, ret);

		err = gsm_spin_unlock(&gsm_spinlock_ctx);
		if (err) {
			dprintk(0, "%s: failed to release gsm spinlock, err=%d\n", __func__, err);
			return err;
		}
	}

	return ret;
}

static int init_gsm_spinlock(void)
{
	int err;

	err = gsm_spinlock_create(&gsm_spinlock_ctx);
	if (err) {
		dprintk(0, "Failed to create gsm cross core spinlock,err=%d\n", err);
		return err;
	}

	return 0;
}

static void deinit_gsm_spinlock(void)
{
	int err;

	err = gsm_spinlock_destroy(&gsm_spinlock_ctx);
	if (err)
		dprintk(0, "Failed to destroy gsm cross core spinlock, err=%d\n", err);
}

#else

static void sync_task_with_dsp_meta(struct cvcore_name_meta_priv *name_node,
	uint8_t register_task)
{
	return;
}

static int find_dsp_shared_name_index(uint64_t name, int *dsp_meta_name_index,
	int *interested_cores)
{
	return 0;
}

static int check_cvip_state(struct mero_notifier_meta *meta, bool user_space_call)
{
	struct cvip_status stats;

	if (meta->flags & ARM_SHARED && user_space_call) {
		memset(&stats, 0, sizeof(struct cvip_status));
		cvip_status_get_status(&stats);
		if (stats.panic_count)
			return -ENODEV;
	}

	return 0;
}

static int register_sid_meta(void)
{
	return 0;
}

static void free_dsp_dma_buf_resources(void)
{
	return;
}

static void deregister_sids(void)
{
	return;
}

static int mero_notifier_register_dsp_shared_names(
	struct cvcore_name_meta_priv *name_node,
	int *interested_cores
	)
{
	return 0;
}

static int init_gsm_spinlock(void)
{
	return 0;
}

static void deinit_gsm_spinlock(void)
{
	return;
}

#endif

static int mero_notifier_notification_handler(XpcChannel channel,
	XpcHandlerArgs args, uint8_t *notification_buffer,
	size_t notification_buffer_length)
{
	uint8_t notif_type;
	struct mero_notifier_meta meta;
	struct dsp_notifier_xpc_data *notifier_xpc_data =
		(struct dsp_notifier_xpc_data *)notification_buffer;

	dprintk(2 ,"mero notifier: received xpc notification interrupt\n");

	if (channel != CVCORE_XCHANNEL_N_NOTIFIER) {
		dprintk(0, "Not the vchannel(got=%d,interested=%d) notifier is interested in.\n",
			channel, CVCORE_XCHANNEL_N_NOTIFIER);
		return 0;
	}

	notif_type = notifier_xpc_data->notif_meta & NOTIFIER_NOTIF_TYPE_MASK;

	if (notif_type == NOTIFIER_NOTIF_TYPE_PAYLOAD) {

		meta.cvcore_names_ready_list[0] = notifier_xpc_data->cvcore_name;

		meta.payload_len_bytes[0] = notifier_xpc_data->notif_meta &
			NOTIFIER_PAYLOAD_LEN_MASK;

		if (meta.payload_len_bytes[0] != 0)
			meta.payload_buf_addr[0] = (uint64_t)&notifier_xpc_data->payload;

		meta.nr_of_ready_cvcore_names = 1;

		meta.flags = 0;

		mero_notifier_signal((unsigned long)&meta, false);
	}

	return 0;
}

// NOTE: Kept it for future use such as power management event handling etc
static int mero_notifier_command_handler(XpcChannel channel, XpcHandlerArgs args,
	uint8_t *command_buffer, size_t command_buffer_length, uint8_t *response_buffer,
	size_t resize_buffer_length, size_t *response_length)
{
	return 0;
}

static int register_xpc_handlers(void)
{
	int ret;

	dprintk(0, "Registering handler functions with xpc\n");

	// Register notifier cmd handler with xpc
	ret = xpc_register_command_handler(CVCORE_XCHANNEL_C_NOTIFIER,
						&mero_notifier_command_handler, NULL);
	if (ret != 0) {
		dprintk(0, "%s: failed to register xpc cmd handler,ret=%d\n",
			__func__, ret);
		return -1;
	}

	// Register notifier notification handler with xpc
	ret = xpc_register_notification_handler(CVCORE_XCHANNEL_N_NOTIFIER,
						&mero_notifier_notification_handler, NULL);
	if (ret != 0) {
		dprintk(0, "%s: failed to register xpc notif handler,ret=%d\n",
			__func__, ret);
		return -1;
	}

	return 0;
}

static void deregister_xpc_handlers(void)
{
	int ret;
	ret = xpc_register_command_handler(CVCORE_XCHANNEL_C_NOTIFIER,
		NULL, NULL);
	if (ret != 0)
		dprintk(0, "failed to deregister xpc cmd handler,ret=%d\n", ret);

	ret = xpc_register_notification_handler(CVCORE_XCHANNEL_N_NOTIFIER,
		NULL, NULL);
	if (ret != 0)
		dprintk(0, "%s: failed to deregister xpc notif handler,ret=%d\n",
			__func__, ret);
}

static int signal_cross_core(struct cvcore_name_meta_priv *name_node,
	int target)
{
	int status;
	struct dsp_notifier_xpc_data notifier_xpc_data;

	// NOTE: We are delivering cross core signal while holding this write lock
	// as it can potentially help us in maintaining signal send order for the
	// same cvcore name.

	notifier_xpc_data.notif_meta = name_node->payload_len_bytes |
				NOTIFIER_NOTIF_TYPE_PAYLOAD;

	notifier_xpc_data.cvcore_name = name_node->name;

	if (name_node->payload_len_bytes != 0) {
		memcpy((void *)&notifier_xpc_data.payload,
		(void *)name_node->payload,
		name_node->payload_len_bytes);
	}

	dprintk(2, "xpc target=0x%x\n", target);

	status = xpc_notification_send(CVCORE_XCHANNEL_N_NOTIFIER,
			target,
			(uint8_t *)&notifier_xpc_data,
			sizeof(struct dsp_notifier_xpc_data),
			XPC_NOTIFICATION_MODE_POSTED);
	if (status != 0) {
		dprintkrl(0, "PID(%d) %s: xpc_notification_send failed, xpc_target_mask=0x%x, errno=%d",
			  task_pid_nr(current), __func__, target, status);
		if (status == -EAGAIN)
			return -EBUSY;
		return -EIO;
	}

	return 0;
}

static int mero_notifier_register_cvcore_names(struct mero_notifier_meta *meta,
	struct task_meta_priv **task_meta_node, bool wait_all)
{
	struct cvcore_name_meta_priv *name_node;
	struct task_meta_priv *task_node;
	unsigned long rw_lock_irq_flags;

	int i, j;
	int ret;
	int nr_of_registered_names;
	size_t meta_cvcore_names_list_sz;
	size_t cvcore_names_watch_list_sz;

	uint64_t watch_cvcore_name;
	int meta_index;
	bool watch_list_changed = false;

	task_node = *task_meta_node;

	if (!meta->nr_of_registered_cvcore_names) {
		dprintk(0, "Number of cvcore names can't be 0 for task %d\n",
			current->pid);
		return -EINVAL;
	}

	if (meta->nr_of_registered_cvcore_names > MAX_CVCORE_NAMES_PER_META_INSTANCE) {
		dprintk(0, "Number of cvcore names %d exceeding %d per meta instance %d\n",
			meta->nr_of_registered_cvcore_names, MAX_CVCORE_NAMES_PER_META_INSTANCE, current->pid);
		return MERO_NOTIFIER_NAME_NR_QUOTA_EXCEEDED;
	}

	meta_cvcore_names_list_sz = meta->nr_of_registered_cvcore_names * CVCORE_NAME_SIZE_BYTES;

	if (task_node) {
		cvcore_names_watch_list_sz = task_node->nr_of_watch_cvcore_names * CVCORE_NAME_SIZE_BYTES;
		// If the list sizes are not equal, there's no point in doing memcmp. The list
		// has definitely changed in reference to what cvcore names the task was previously
		// interested in.
		if (meta_cvcore_names_list_sz == cvcore_names_watch_list_sz) {
			// the list sizes are the same. Now, just check if it
			// wants to wait on the same cvcore name/s as the previous name/s it was
			// interested in.
			for (i = 0; i < meta->nr_of_registered_cvcore_names; i++) {

				meta_index = task_node->watch_cvcore_name_index[i];
				watch_cvcore_name = task_node->registered_name_meta[meta_index].registered_name;

				if (meta->cvcore_names_register_list[i] != watch_cvcore_name) {
					watch_list_changed = true;
					break;
				}
			}
			// Most of the times, we expect to return from here, if the cvcore name list (and order)
			// for a task doesn't change in reference to what it was interested in the last time
			// it had called wait().
			if (!watch_list_changed) {
				// There's a possibility that a task might want to wait_all instead of
				// wait_any for the same set of cvcore names. So, let's account for that.
				task_node->wait_all = wait_all;
				return 0;
			}
		}
	}

	if (task_node) {
		// Add new names to the task's registered cvcore names list.
		for (i = 0; i < meta->nr_of_registered_cvcore_names; i++) {
			for (j = 0; j < task_node->nr_of_registered_cvcore_names; j++) {
				if (meta->cvcore_names_register_list[i] ==
						task_node->registered_name_meta[j].registered_name)
					break;
			}

			if (j >= task_node->nr_of_registered_cvcore_names) {

				if (task_node->nr_of_registered_cvcore_names >= MAX_CVCORE_NAMES_PER_TASK) {
					dprintk(0, "Number of registered names %d exceeding %d for task %d\n",
						task_node->nr_of_registered_cvcore_names,
                                                MAX_CVCORE_NAMES_PER_TASK, current->pid);
					return MERO_NOTIFIER_NAME_NR_QUOTA_EXCEEDED;
				}

				nr_of_registered_names = task_node->nr_of_registered_cvcore_names;

				task_node->registered_name_meta[nr_of_registered_names].registered_name =
					meta->cvcore_names_register_list[i];
				task_node->registered_name_meta[nr_of_registered_names].name_node = NULL;
				task_node->registered_name_meta[nr_of_registered_names].signal_serial_num = 0;
				task_node->registered_name_meta[nr_of_registered_names].is_entry_valid = true;
				task_node->registered_name_meta[nr_of_registered_names].name_node_task_index = -1;

				task_node->watch_cvcore_name_index[i] = nr_of_registered_names;

				++(task_node->nr_of_registered_cvcore_names);

			} else {
				task_node->watch_cvcore_name_index[i] = j;
			}
		}
		task_node->nr_of_watch_cvcore_names = meta->nr_of_registered_cvcore_names;
	}

	if (!task_node) {
		// we didn't find any such task entry in linked list.
		// hence, create a new node for it.
		task_node = kmalloc(sizeof(struct task_meta_priv), GFP_ATOMIC);
		if (!task_node) {
			dprintk(0, "kmalloc failed to allocate struct task_meta_priv\n");
			return -ENOMEM;
		}

		*task_meta_node = task_node;

		// Initialize node.
		task_node->task = current;
		task_node->nr_of_registered_cvcore_names = meta->nr_of_registered_cvcore_names;
		task_node->nr_of_watch_cvcore_names = meta->nr_of_registered_cvcore_names;

		for (i = 0; i < meta->nr_of_registered_cvcore_names; i++) {
			task_node->registered_name_meta[i].is_entry_valid = true;
			task_node->registered_name_meta[i].registered_name = meta->cvcore_names_register_list[i];
			task_node->registered_name_meta[i].signal_serial_num = 0;
			task_node->registered_name_meta[i].name_node = NULL;
			task_node->registered_name_meta[i].name_node_task_index = -1;

			task_node->watch_cvcore_name_index[i] = i;
		}

		for (; i < MAX_CVCORE_NAMES_PER_TASK; i++)
			task_node->registered_name_meta[i].is_entry_valid = false;

		task_node->is_task_waiting = false;
		raw_spin_lock_init(&task_node->lock);

		write_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);
		// add node to linked list.
		list_add(&task_node->list, &task_meta_nodes);
		write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
	}

	write_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);

	// add/update cvcore name rbtree.
	for (i = 0; i < meta->nr_of_registered_cvcore_names; i++) {
		name_node = search_cvcore_name_in_rbtree(meta->cvcore_names_register_list[i]);
		if (name_node) {
			// The node already exists, just add the task to it's list.
			// But before that, just check if this task already exists.
			// If it does, don't create a dup by adding it again.
			for (j = 0; j < name_node->nr_of_tasks; j++) {
				if (name_node->task_meta[j].task == current)
					break;
			}

			// Add the task if it doesn't exist.
			if (j >= name_node->nr_of_tasks) {
				if (name_node->nr_of_tasks < MAX_TASKS_PER_CVCORE_NAME) {
					name_node->task_meta[name_node->nr_of_tasks].task_node = task_node;
					name_node->task_meta[name_node->nr_of_tasks].task = current;

					if (name_node->notify_only_one_enabled)
						name_node->task_meta[name_node->nr_of_tasks].task_wait_order_index =
						INVALID_TASK_WAIT_ORDER_INDEX;

					// add the cvcore name node address to the task node.
					for (j = 0; j < task_node->nr_of_registered_cvcore_names; j++) {
						if (task_node->registered_name_meta[j].registered_name ==
								name_node->name) {
							task_node->registered_name_meta[j].name_node = name_node;
							task_node->registered_name_meta[j].name_node_task_index =
								name_node->nr_of_tasks;
							break;
						}
					}

					++name_node->nr_of_tasks;

				} else {
					dprintk(0, "Number of tasks %d exceeding %d for name %llu\n",
                                                name_node->nr_of_tasks, MAX_TASKS_PER_CVCORE_NAME, name_node->name);
					write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
					// Return from here with failure. This task will exist
					// in cvcore name nodes it has added itself to before
					// we hit this condition but it shouldn't do any harm.
					return MERO_NOTIFIER_TASK_NR_QUOTA_EXCEEDED;
				}
			} else {
				continue;
			}
		} else {
			// We didn't find any such cvcore name entry in cvcore name rbtree.
			// Hence, create a new node for it.
			name_node = kmalloc(sizeof(struct cvcore_name_meta_priv), GFP_ATOMIC);
			if (!name_node) {
				dprintk(0, "kmalloc failed to allocate struct cvcore_name_meta_priv\n");
				write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
				return -ENOMEM;
			}

			// Initialize node.
			name_node->name = meta->cvcore_names_register_list[i];
			name_node->task_meta[0].task = current;
			name_node->task_meta[0].task_node = task_node;
			name_node->signal_serial_num = 0;
			name_node->nr_of_tasks = 1;
			name_node->latest_signaling_task = NULL;
			name_node->dsp_meta_name_index = -1;
			name_node->signal_consumed = true;
			name_node->nr_of_tasks_waiting = 0;
			name_node->tasks_interested_nr_of_times = 0;

			if (meta->flags & NOTIFY_ONLY_ONE_MODE) {
				name_node->notify_only_one_enabled = true;
				name_node->task_meta[0].task_wait_order_index = INVALID_TASK_WAIT_ORDER_INDEX;
			} else {
				name_node->notify_only_one_enabled = false;
			}

			// Init other entries to null.
			for (j = 1; j < MAX_TASKS_PER_CVCORE_NAME; j++)
				name_node->task_meta[j].task = NULL;

			// add the cvcore name node address to the task node.
			for (j = 0; j < task_node->nr_of_registered_cvcore_names; j++) {
				if (task_node->registered_name_meta[j].registered_name ==
						name_node->name) {
					task_node->registered_name_meta[j].name_node = name_node;
					task_node->registered_name_meta[j].name_node_task_index = 0;
					break;
				}
			}

			rwlock_init(&name_node->name_rw_lock);

			add_cvcore_name_node_to_rbtree(name_node);
		}

		if (meta->flags & DSP_SHARED) {

			if (!dsp_notifier_meta) {
				// It is uninitialized in the case where user space app hasn't
				// called mero_notifier_init successfully.
				dprintk(0, "%s: dsp_notifier_meta is not initialized\n", __func__);
				write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
				return -EINVAL;
			}

			ret = mero_notifier_register_dsp_shared_names(name_node, NULL);
			if (ret != 0) {
				dprintk(0, "%s: register dsp shared names failed, ret=%d\n",
					__func__, ret);
				write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
				return ret;
			}

			sync_task_with_dsp_meta(name_node, REGISTER_TASK);
		}
	}

	task_node->wait_all = wait_all;

	if (mydebug)
		debug_print_out_cvcore_name_rbtree();

	write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
	return 0;
}

static int copy_meta_from_user(struct mero_notifier_meta *meta,
	struct mero_notifier_meta *userspace_meta) {
	if (!mero_notifier_access_ok(VERIFY_WRITE, userspace_meta,
		sizeof(struct mero_notifier_meta))) {
		dprintk(0, "Invalid user-space buffer.\n");
		return -EINVAL;
	}

	if (copy_from_user((void *)meta, (void *)userspace_meta, sizeof(struct mero_notifier_meta))) {
		dprintk(0, "copy_from_user failed to copy meta structure\n");
		return -EFAULT;
	}
	return 0;
}

// This function must be called while holding the spinlock.
static int mero_notifier_copy_ready_to_user(struct task_meta_priv *task_node,
	struct local_user_meta_s *local_user_meta, uint8_t woken_up_task)
{
	int i, j = 0;
	int index;
	int nm_task_index;
	bool notify_only_one_ready = false;
	unsigned long rw_lock_irq_flags;

	struct cvcore_name_meta_priv *name_node;

	for (i = 0; i < task_node->nr_of_watch_cvcore_names; i++) {

		index = task_node->watch_cvcore_name_index[i];
		name_node = task_node->registered_name_meta[index].name_node;

		if (name_node->notify_only_one_enabled) {

			notify_only_one_ready = false;

			write_lock_irqsave(&name_node->name_rw_lock, rw_lock_irq_flags);

			if ((name_node->nr_of_tasks_waiting >= 1) &&
				!woken_up_task) {
				// There's atleast one task waiting on this
				// cvcore name that came before us. Don't even
				// try to consume this cvcore name's signal
				// if it has any.
				write_unlock_irqrestore(&name_node->name_rw_lock, rw_lock_irq_flags);
				continue;
			} else {

				if (!name_node->signal_consumed) {

					notify_only_one_ready = true;
					name_node->signal_consumed = true;

					nm_task_index =
					task_node->registered_name_meta[index].name_node_task_index;

					if (name_node->task_meta[nm_task_index].task_wait_order_index !=
					    INVALID_TASK_WAIT_ORDER_INDEX) {
						// This task isn't waiting anymore, it just got a signal.
						name_node->task_meta[nm_task_index].task_wait_order_index =
						INVALID_TASK_WAIT_ORDER_INDEX;

						--(name_node->nr_of_tasks_waiting);
					}
				}
			}
			write_unlock_irqrestore(&name_node->name_rw_lock, rw_lock_irq_flags);
		}

		read_lock_irqsave(&name_node->name_rw_lock, rw_lock_irq_flags);

		if ((task_node->registered_name_meta[index].signal_serial_num !=
			name_node->signal_serial_num && name_node->latest_signaling_task
			!= current && !name_node->notify_only_one_enabled) ||
			notify_only_one_ready) {

			local_user_meta->meta.cvcore_names_ready_list[j] =
				task_node->registered_name_meta[index].registered_name;

			if (name_node->payload_len_bytes != 0) {
				memcpy((void *)&local_user_meta->payload[j][0],
				       (void *)name_node->payload, name_node->payload_len_bytes);
				local_user_meta->meta.payload_len_bytes[j] = name_node->payload_len_bytes;
			}

			task_node->registered_name_meta[index].signal_serial_num =
			  name_node->signal_serial_num;

			++j;
		}

		read_unlock_irqrestore(&name_node->name_rw_lock, rw_lock_irq_flags);
	}

	if (j) {
		local_user_meta->meta.nr_of_ready_cvcore_names = j;
		return NEW_SIG;
	}

	return NO_NEW_SIG;
}

static int copy_local_meta_to_user(struct local_user_meta_s *local_user_meta,
	struct mero_notifier_meta *userspace_meta)
{
	int i;

	if (copy_to_user((void *)&userspace_meta->nr_of_ready_cvcore_names,
		(void *)&local_user_meta->meta.nr_of_ready_cvcore_names,
		sizeof(local_user_meta->meta.nr_of_ready_cvcore_names))) {
		dprintk(0, "%s: copy_to_user of nr_of_ready_names failed.\n", __func__);
		return -EFAULT;
	}

	for (i = 0; i < local_user_meta->meta.nr_of_ready_cvcore_names; ++i) {

		if (copy_to_user((void *)&userspace_meta->cvcore_names_ready_list[i],
			(void *)&local_user_meta->meta.cvcore_names_ready_list[i],
			sizeof(local_user_meta->meta.cvcore_names_ready_list[0]))) {
			dprintk(0, "%s: copy_to_user of ready names failed.\n", __func__);
			return -EFAULT;
		}

		if (local_user_meta->meta.payload_len_bytes[i]) {
			// the userspace buffer must have enough space to accommodate the payload.
			if (!mero_notifier_access_ok(VERIFY_WRITE,
				(void *)local_user_meta->meta.payload_buf_addr[i],
				local_user_meta->meta.payload_len_bytes[i])) {
				dprintk(0, "Invalid user-space buffer.\n");
				return -EINVAL;
			}

			if (copy_to_user((void *)local_user_meta->meta.payload_buf_addr[i],
				(void *)&local_user_meta->payload[i][0],
				local_user_meta->meta.payload_len_bytes[i])) {
				dprintk(0, "%s: copy_to_user of payload.\n", __func__);
				return -EFAULT;
			}

			if (copy_to_user((void *)&userspace_meta->payload_len_bytes[i],
				(void *)&local_user_meta->meta.payload_len_bytes[i],
				sizeof(local_user_meta->meta.payload_len_bytes[0]))) {
				dprintk(0, "%s: copy_to_user of payload length failed.\n", __func__);
				return -EFAULT;
			}
		}
	}

	return 0;
}

static void update_signal_one_name_node_meta(struct task_meta_priv *task_node)
{
	int i;
	int index;
	int nm_task_index;
	unsigned long rw_lock_irq_flags;
	struct cvcore_name_meta_priv *name_node;

	for (i = 0; i < task_node->nr_of_watch_cvcore_names; i++) {

		index = task_node->watch_cvcore_name_index[i];
		name_node = task_node->registered_name_meta[index].name_node;

		write_lock_irqsave(&name_node->name_rw_lock, rw_lock_irq_flags);

		nm_task_index = task_node->registered_name_meta[index].name_node_task_index;
		name_node->task_meta[nm_task_index].task_wait_order_index =
			name_node->tasks_interested_nr_of_times;

		++name_node->tasks_interested_nr_of_times;

		++name_node->nr_of_tasks_waiting;

		//dprintk(0, "(pid=%d, name=%llx) index = %llu\n\n", current->pid,
		//	name_node->name, name_node->task_meta[nm_task_index].task_wait_order_index);

		write_unlock_irqrestore(&name_node->name_rw_lock, rw_lock_irq_flags);
	}
}

// Remove this task from notify_only_one cvcore names' meta since it is about to
// return to userspace and won't be actively waiting on any notify_only_one cvcore name.
static void notify_only_one_cleanup(struct task_meta_priv *task_node) {
	int i, j;
	int nr_of_tasks;
	int index;
	unsigned long rw_lock_irq_flags;
	struct cvcore_name_meta_priv *name_node;

	for (i = 0; i < task_node->nr_of_watch_cvcore_names; i++) {

		index = task_node->watch_cvcore_name_index[i];
		name_node = task_node->registered_name_meta[index].name_node;

		if (name_node->notify_only_one_enabled) {

			write_lock_irqsave(&name_node->name_rw_lock, rw_lock_irq_flags);

			nr_of_tasks = name_node->nr_of_tasks;

			for (j = 0; j < nr_of_tasks; j++) {

				if (name_node->task_meta[j].task == task_node->task) {

					if (name_node->task_meta[j].task_wait_order_index !=
					    INVALID_TASK_WAIT_ORDER_INDEX) {
						name_node->task_meta[j].task_wait_order_index =
							INVALID_TASK_WAIT_ORDER_INDEX;
						--(name_node->nr_of_tasks_waiting);
					}
					break;
				}
			}
			write_unlock_irqrestore(&name_node->name_rw_lock, rw_lock_irq_flags);
		}
	}
}

static int mero_notifier_wait(unsigned long arg, bool timed_poll)
{
	struct task_meta_priv *temp_task_node, *task_node = NULL;
	struct mero_notifier_meta_drv meta_drv;
	struct mero_notifier_meta *userspace_meta;
	long timeout;
	int retval = 0;
	raw_spinlock_t *lock = NULL;
	unsigned long rw_lock_irq_flags;
	unsigned long raw_spinlock_irq_flags;
	struct local_user_meta_s local_user_meta;
	int new_signal_stat = NO_NEW_SIG;

	read_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);

	list_for_each_entry(temp_task_node, &task_meta_nodes, list) {
		if (temp_task_node->task == current) {
			task_node = temp_task_node;
			break;
		}
	}

	read_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);

	if (copy_from_user((void *)&meta_drv, (void *)arg,
		sizeof(struct mero_notifier_meta_drv))) {
		dprintk(0, "copy_from_user failed to copy meta_drv\n");
		return -EFAULT;
	}

	// copy timeout.
	if (timed_poll)
		timeout = usecs_to_jiffies(meta_drv.timeout_us);
	else
		timeout = MAX_SCHEDULE_TIMEOUT;

	userspace_meta = (struct mero_notifier_meta *)meta_drv.meta_ptr;

	retval = copy_meta_from_user(&local_user_meta.meta, userspace_meta);
	if (retval != 0) {
		dprintk(0, "wait() failed to copy meta from user\n");
		return retval;
	}

	retval = mero_notifier_register_cvcore_names(&local_user_meta.meta,
				&task_node, meta_drv.wait_all);
	if (retval != 0)
		return retval;

	lock = &task_node->lock;

	raw_spin_lock_irqsave(lock, raw_spinlock_irq_flags);

	// Check if the task already has ready cvcore names. If it has,
	// return with the list.
	if (!mero_notifier_copy_ready_to_user(task_node, &local_user_meta,
		TASK_NO_WAKE_UP)) {
		raw_spin_unlock_irqrestore(lock, raw_spinlock_irq_flags);
		retval = copy_local_meta_to_user(&local_user_meta, userspace_meta);
		return retval;
	}

	if (local_user_meta.meta.flags & NOTIFY_ONLY_ONE_MODE)
		update_signal_one_name_node_meta(task_node);

	// The task is going to suspend soon.
	task_node->is_task_waiting = true;

	for (;;) {
		// Does this task have any signal pending?
		if (signal_pending_state(TASK_INTERRUPTIBLE, current)) {
			/* Due to signal usage on Nova we can't return ERESTART_RESTARTBLOCK
			 * here. More on that in CVOS-339 ticket or
			 * https://gerrit.magicleap.com/c/leapcore/cvcore/kernel/+/1097975
			 * Just update the timeout in user structure and restart
			 * the syscall. ERESTARTSYS is a safe option in case we'd
			 * want to interrupt the syscall with non-fatal signal.
			 */
			if (timed_poll) {
				meta_drv.timeout_us = timeout;
				arg += offsetof(struct mero_notifier_meta_drv, timeout_us);
				retval = copy_to_user((void *)arg, (void *)&(meta_drv.timeout_us),
								sizeof(meta_drv.timeout_us));
				if (retval) {
					dprintk(0, "copy_to_user failed to copy meta_drv.timeout_us\n");
					retval = -EFAULT;
					goto notifier_return;
				}
			}

			retval = -ERESTARTSYS;
			goto notifier_return;
		}

		if (unlikely(timeout <= 0)) {
			retval = -ETIME;
			goto notifier_return;
		}

		// We would like the task to receive other signals while
		// they are sleeping.
		__set_current_state(TASK_INTERRUPTIBLE);

		// Free the lock before the task actually starts sleeping.
		raw_spin_unlock_irqrestore(lock, raw_spinlock_irq_flags);

		// let the task sleep now.
		timeout = schedule_timeout(timeout);

		// The task woke up. Check if it woke up for the expected reason.
		raw_spin_lock_irqsave(lock, raw_spinlock_irq_flags);

		new_signal_stat = mero_notifier_copy_ready_to_user(task_node, &local_user_meta,
						      TASK_WOKE_UP);
		if (new_signal_stat == NEW_SIG) {
			goto notifier_return;
		}
	}

notifier_return:
	task_node->is_task_waiting = false;
	raw_spin_unlock_irqrestore(lock, raw_spinlock_irq_flags);
	notify_only_one_cleanup(task_node);
	if (new_signal_stat == NEW_SIG)
		retval = copy_local_meta_to_user(&local_user_meta, userspace_meta);
	return retval;
}

static int update_cvcore_name_node(struct cvcore_name_meta_priv *name_node,
	struct mero_notifier_meta *meta, int meta_index, uint8_t *payload, bool user_space_call)
{
	int i = meta_index;
	int retval = 0;
	__maybe_unused int xpc_target_mask;
	__maybe_unused int interested_cores;
	unsigned long rw_lock_irq_flags;

	retval = check_cvip_state(meta, user_space_call);
	if (retval)
		return retval;

	write_lock_irqsave(&name_node->name_rw_lock, rw_lock_irq_flags);

	if (meta->payload_len_bytes[i] != 0) {
		if (meta->payload_len_bytes[i] > MAX_PAYLOAD_SIZE_BYTES) {
			retval = -E2BIG;
			goto cleanup;
		}
		if (user_space_call)
			memcpy((void *)name_node->payload, (void *)payload,
			       meta->payload_len_bytes[i]);
		else
			memcpy((void *)name_node->payload, (void *)meta->payload_buf_addr[i],
			       meta->payload_len_bytes[i]);
		name_node->payload_len_bytes = meta->payload_len_bytes[i];
	} else {
		name_node->payload_len_bytes = 0;
	}

	if (name_node->notify_only_one_enabled)
		name_node->signal_consumed = false;

	++(name_node->signal_serial_num);

	if (user_space_call && name_node->latest_signaling_task != current)
		name_node->latest_signaling_task = current;
	else {
		if (!user_space_call)
			// NULL here signifies crosscore signal
			name_node->latest_signaling_task = NULL;
	}

	if(CVIP_ARM) {

		if (meta->flags & DSP_SHARED && user_space_call) {

			if (!dsp_notifier_meta) {
				// It is uninitialized in the case where user space app hasn't
				// called mero_notifier_init successfully.
				dprintk(0, "%s: dsp_notifier_meta is not initialized\n", __func__);
				retval = -EINVAL;
				goto cleanup;
			}

			retval = mero_notifier_register_dsp_shared_names(name_node,
				&interested_cores);
			if (retval != 0) {
				dprintk(0, "%s: register dsp shared names failed,ret=%d",
					__func__, retval);
				goto cleanup;
			}

			// prepare the ipc interrupt mask looking at interested cores.
			xpc_target_mask = interested_cores & ~(1 << CVCORE_ARM);

			if (xpc_target_mask)
				retval = signal_cross_core(name_node, xpc_target_mask);
		}

		if (meta->flags & X86_SHARED && user_space_call)
			retval = signal_cross_core(name_node, 1 << CVCORE_X86);

	} else {

		if(MERO_X86) {

			if (meta->flags & ARM_SHARED && user_space_call)
				retval = signal_cross_core(name_node, 1 << CVCORE_ARM);
		}
	}

cleanup:
	write_unlock_irqrestore(&name_node->name_rw_lock, rw_lock_irq_flags);
	return retval;
}

// Make sure to call this function while holding write lock (meta_rw_lock).
static int create_cvcore_name_node(struct mero_notifier_meta *meta,
	int meta_index, struct cvcore_name_meta_priv **new_name_node)
{
	int i;
	struct cvcore_name_meta_priv *name_node;

	// We didn't find any such cvcore name entry in cvcore name rbtree.
	// Hence, create a new node for it.
	name_node = kmalloc(sizeof(struct cvcore_name_meta_priv), GFP_ATOMIC);
	if (!name_node) {
		return -ENOMEM;
	}

	// Initialize node.
	name_node->name = meta->cvcore_names_ready_list[meta_index];
	name_node->signal_serial_num = 0;
	name_node->nr_of_tasks = 0;
	name_node->latest_signaling_task = NULL;
	name_node->dsp_meta_name_index = -1;
	name_node->signal_consumed = true;
	name_node->nr_of_tasks_waiting = 0;
	name_node->tasks_interested_nr_of_times = 0;

	if (meta->flags & NOTIFY_ONLY_ONE_MODE)
		name_node->notify_only_one_enabled = true;
	else
		name_node->notify_only_one_enabled = false;

	// Init other entries to null.
	for (i = 0; i < MAX_TASKS_PER_CVCORE_NAME; i++) {
		name_node->task_meta[i].task = NULL;
		name_node->task_meta[i].task_node = NULL;
	}

	rwlock_init(&name_node->name_rw_lock);

	add_cvcore_name_node_to_rbtree(name_node);

	*new_name_node = name_node;

	return 0;
}

static bool task_node_is_processed(pid_t *processed_pids,
		pid_t pid,
		int nr_of_processed_pids)
{
	int i;

	for (i = 0; i < nr_of_processed_pids; i++) {
		if (processed_pids[i] == pid)
			return true;
	}
	return false;
}

// This function checks if any one ready cvcore name matches with the
// watch list of this task. If it does, then the task is indeed waiting
// on at least one cvcore name from the ready cvcore name list.
static bool task_wakeup_needed(struct task_meta_priv *task_node,
	struct mero_notifier_meta *meta) {
	int i, j;
	int index;
	uint64_t registered_name;

	for (i = 0; i < meta->nr_of_ready_cvcore_names; i++) {
		for (j = 0; j < task_node->nr_of_watch_cvcore_names; j++) {

			index = task_node->watch_cvcore_name_index[j];
			registered_name = task_node->registered_name_meta[index].registered_name;

			if (meta->cvcore_names_ready_list[i] == registered_name)
				return true;
		}
	}

	return false;
}

static bool is_wait_all_ready(struct task_meta_priv *task_node) {
	int i;
	int index;
	struct cvcore_name_meta_priv *name_node;

	if (task_node->wait_all) {

		for (i = 0; i < task_node->nr_of_watch_cvcore_names; i++) {

			index = task_node->watch_cvcore_name_index[i];
			name_node = task_node->registered_name_meta[index].name_node;

			if (task_node->registered_name_meta[index].signal_serial_num ==
					name_node->signal_serial_num)
				return false;
		}
	}
	return true;
}

static struct task_meta_priv *get_first_waiting_task_node(
	struct cvcore_name_meta_priv *name_node)
{
	int i;
	unsigned long rw_lock_irq_flags;
	uint64_t min;
	struct task_meta_priv *task_node;

	read_lock_irqsave(&name_node->name_rw_lock, rw_lock_irq_flags);

	min = name_node->task_meta[0].task_wait_order_index;
	task_node = name_node->task_meta[0].task_node;

	for (i = 1; i < name_node->nr_of_tasks; ++i) {
		if ((name_node->task_meta[i].task_wait_order_index !=
			INVALID_TASK_WAIT_ORDER_INDEX) &&
			(name_node->task_meta[i].task_wait_order_index < min)) {
			min = name_node->task_meta[i].task_wait_order_index;
			task_node = name_node->task_meta[i].task_node;
		}
	}

	read_unlock_irqrestore(&name_node->name_rw_lock, rw_lock_irq_flags);

	return task_node;
}

static int mero_notifier_signal(unsigned long arg, bool user_space_call)
{
	struct mero_notifier_meta meta;
	struct cvcore_name_meta_priv *name_node;
	struct task_meta_priv *task_node;
	raw_spinlock_t *lock;
	unsigned long rw_lock_irq_flags;
	unsigned long raw_spinlock_irq_flags;

	int i, j, retval;
	bool node_update_needed = true;
	int nr_of_processed_pids = 0;
	uint8_t payload[MAX_CVCORE_NAMES_PER_META_INSTANCE][MAX_PAYLOAD_SIZE_BYTES];

	// Maintain a local copy of already processed task pids.
	// Maintaining local copy because it only applies to a single task
	// calling signal() and shouldn't interfere with signal() getting called
	// concurrently from other tasks.
	pid_t processed_pids[MAX_PROCESSED_PIDS];

	if (user_space_call) {

		retval = copy_meta_from_user(&meta, (struct mero_notifier_meta *)arg);
		if (retval != 0) {
			dprintk(0, "signal() failed to copy meta from user\n");
			return retval;
		}

	} else {
		memcpy((void *)&meta, (void *)arg, sizeof(struct mero_notifier_meta));
	}

	if (meta.nr_of_ready_cvcore_names > MAX_CVCORE_NAMES_PER_META_INSTANCE) {
		dprintk(0, "num of names %d exceeds maximum %d (per meta instance %d)\n",
			meta.nr_of_ready_cvcore_names, MAX_CVCORE_NAMES_PER_META_INSTANCE,
            current->pid);
		return MERO_NOTIFIER_NAME_NR_QUOTA_EXCEEDED;
	}

	if (!meta.nr_of_ready_cvcore_names) {
		dprintk(0, "num of names can't be 0 (in pid %d)\n", current->pid);
		return -EINVAL;
	}

	if (user_space_call) {
		for (i = 0; i < meta.nr_of_ready_cvcore_names; i++) {
			if ((meta.payload_len_bytes[i] != 0)) {
				if (meta.payload_len_bytes[i] > MAX_PAYLOAD_SIZE_BYTES)
					return -E2BIG;
				if (copy_from_user((void *)payload[i],
						   (void *)meta.payload_buf_addr[i],
						   meta.payload_len_bytes[i])) {
					dprintk(0, "copy_from_user failed to copy payload\n");
					return -EFAULT;
				}
			}
		}
	}

	read_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);

	for (i = 0; i < meta.nr_of_ready_cvcore_names; i++) {

		name_node = search_cvcore_name_in_rbtree(meta.cvcore_names_ready_list[i]);

		if (name_node) {

			if (node_update_needed) {
				// update the cvcore name node fields.
				retval = update_cvcore_name_node(name_node, &meta, i, payload[i],
								 user_space_call);
				if (retval != 0) {
					read_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
					return retval;
				}
			} else {
				node_update_needed = true;
			}

			for (j = 0; j < name_node->nr_of_tasks; j++) {

				if (name_node->notify_only_one_enabled)
					task_node = get_first_waiting_task_node(name_node);
				else
					task_node = name_node->task_meta[j].task_node;

				if (!task_node_is_processed(processed_pids, task_node->task->pid,
						nr_of_processed_pids) && is_wait_all_ready(task_node)) {

					lock = &task_node->lock;

					raw_spin_lock_irqsave(lock, raw_spinlock_irq_flags);

					// Wake up the task if it was in waiting.
					if (task_node->is_task_waiting) {

						// Does this task really needs a wakeup?
						if (task_wakeup_needed(task_node, &meta))
							wake_up_process(task_node->task);
					}

					raw_spin_unlock_irqrestore(lock, raw_spinlock_irq_flags);

					if (nr_of_processed_pids < MAX_PROCESSED_PIDS) {
						processed_pids[nr_of_processed_pids++] =
							task_node->task->pid;
					} else {
						/*
						 * There's a highly likely chance of tasks getting
						 * spurious wakeups if this condition is hit. Increasing
						 * the size processed_pids array should resolve this issue.
						 */
						dprintkrl(0, "Max pids limit reached. Notifier behavior will be affected.\n");
					}
				}

				/*
				 * We are supposed to wake up only one task if this feature is enabled.
				 * We keep scanning though the tasks until we wake up someone successfully.
				 */
				if (name_node->notify_only_one_enabled)
					break;
			}
		} else {

			read_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
			write_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);

			// Check again if the node is still not present in the cvcore name node rb-tree.
			// We do this check here as we did give up the read lock before acquiring the write
			// lock and there's some possibility that another waiting task might have already
			// created the cvcore name node.
			name_node = search_cvcore_name_in_rbtree(meta.cvcore_names_ready_list[i]);
			if (!name_node) {
				// cvcore name node is not present. Create it.
				retval = create_cvcore_name_node(&meta, i, &name_node);
				if (retval != 0) {
					write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
					dprintk(0, "Failed to create cvcore name node\n");
					return retval;
				}

				// Call update_cvcore_name_node() here and don't call it from the first
				// update_cvcore_name_node within the for loop if called here.
				// Otherwise there's a chance of signal getting delivered out-of-order in case
				// someone else delivers signal to this newly created cvcore name and then we
				// update it after looping for this node again.
				retval = update_cvcore_name_node(name_node, &meta, i, payload[i], user_space_call);
				if (retval != 0) {
					write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
					return retval;
				}
				node_update_needed = false;
			}

			// Let the for loop run for this name node again.
			--i;

			write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
			read_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);
		}
	}

	read_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
	return 0;
}

static void mero_notifier_cleanup(struct task_struct *task)
{
	struct task_meta_priv *task_node;
	struct cvcore_name_meta_priv *name_node;
	struct rb_node *node;
	bool task_node_found = false;
	bool latest_signaling_task_matched = false;
	unsigned long rw_lock_irq_flags;

	// Expect cleanup to be called for literally every terminating
	// task (even those that aren't registered with the mero_notifier
	// system). So, grabbing a reader lock instead of a writer lock
	// here makes more sense.
	read_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);

	list_for_each_entry(task_node, &task_meta_nodes, list) {
		if (task_node->task->pid == task->pid) {
			task_node_found = true;
			break;
		}
	}

	for (node = rb_first(&cvcore_name_rbtree_root); node; node = rb_next(node)) {
		name_node = rb_entry(node, struct cvcore_name_meta_priv, node);
		if (name_node->latest_signaling_task == current) {
			latest_signaling_task_matched = true;
			break;
		}
	}

	read_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);

	if (!task_node_found && !latest_signaling_task_matched)
		return;

	write_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);

	// Recheck task_node in case task_meta_nodes changed while we released the lock
	task_node_found = false;
	list_for_each_entry(task_node, &task_meta_nodes, list) {
		if (task_node->task->pid == task->pid) {
			task_node_found = true;
			break;
		}
	}

	if (!task_node_found && !latest_signaling_task_matched) {
		write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
		return;
	}

	if (task_node_found) {
		// remove the dying task's meta.
		list_del(&task_node->list);
		kfree(task_node);
	}

	for (node = rb_first(&cvcore_name_rbtree_root); node; node = rb_next(node)) {
		name_node = rb_entry(node, struct cvcore_name_meta_priv, node);

		// Cleanup the task pid entries from all the relevant cvcore names entries
		// as well.
		if (task_node_found)
			cleanup_task_from_name_node(name_node, task);

		if (latest_signaling_task_matched &&
			name_node->latest_signaling_task == current) {
			name_node->latest_signaling_task = NULL;
		}
	}

	write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);
}

static int task_notifier(struct notifier_block *self, unsigned long cmd, void *v)
{
	mero_notifier_cleanup(v);
	return 0;
}

static long mero_notifier_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	if (_IOC_TYPE(cmd) != MERO_NOTIFIER_IOC_MAGIC) {
		dprintk(0, "Invalid ioctl.\n");
		return -EINVAL;
	}

	switch (cmd) {

	case MERO_NOTIFIER_IOC_WAIT_ANY:
		trace_ml_mero_notifier_ioctl_wait_sleep(cmd, arg, 0);
		retval = mero_notifier_wait(arg, false);
		trace_ml_mero_notifier_ioctl_wait_wake(cmd, arg, retval);
		break;

	case MERO_NOTIFIER_IOC_TIMED_WAIT_ANY:
		trace_ml_mero_notifier_ioctl_wait_sleep(cmd, arg, 1);
		retval = mero_notifier_wait(arg, true);
		trace_ml_mero_notifier_ioctl_wait_wake(cmd, arg, retval);
		break;

	case MERO_NOTIFIER_IOC_SIGNAL:
		trace_ml_mero_notifier_ioctl_signal(cmd, arg, 1);
		retval = mero_notifier_signal(arg, true);
		break;

	default:
		return -EINVAL;
	}

	return retval;
}


static const struct file_operations mero_notifier_fops = {
	.owner		= THIS_MODULE,
	.open		= mero_notifier_open,
	.unlocked_ioctl	= mero_notifier_unlocked_ioctl,
	.compat_ioctl	= mero_notifier_unlocked_ioctl,
	.release	= mero_notifier_close,
};

struct miscdevice mero_notifier_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mero_notifier",
	.fops = &mero_notifier_fops,
};

// callback for whenever a task (that includes threads
// and processes) exits. We're using this feature, primarily
// because we want cleanup to be called for just not the
// userspace processes but for the threads as well.
static struct notifier_block task_notifier_block = {
	.notifier_call	= task_notifier,
};

#if defined(ARM_CVIP) && defined(CONFIG_DEBUG_FS)
static struct dentry *debugfs_root;
#endif

static int __init mero_notifier_init(void)
{
	int ret;

	ret = register_xpc_handlers();
	if (ret) {
		dprintk(0, "register_xpc_handlers failed, ret=%d\n", ret);
		return ret;
	}

	dprintk(0, "Successfully loaded mero_notifier misc module\n");

	rwlock_init(&meta_rw_lock);

	ret = profile_event_register(PROFILE_TASK_EXIT, &task_notifier_block);
	if (ret) {
		dprintk(0, "Failed to register task exit event.\n");
		deregister_xpc_handlers();
		return ret;
	}

	ret = init_gsm_spinlock();
	if (ret) {
		dprintk(0, "init_gsm_spinlock failed, ret=%d\n", ret);
		return ret;
	}

	ret = register_sid_meta();
	if (ret) {
		dprintk(0, "register_sid_meta failed. ret=%d\n", ret);
		return ret;
	}

	ret = misc_register(&mero_notifier_device);
	if (ret) {
		dprintk(0, "Failed to register mero_notifier misc module.\n");
		return ret;
	}

#if defined(ARM_CVIP) && defined(CONFIG_DEBUG_FS)
	debugfs_root = debugfs_create_dir("mero_notifier", NULL);
	debugfs_create_file("dsp_shared_names", 0444, debugfs_root, NULL,
		&nodes_fops);
#endif

	return 0;
}

static void __exit mero_notifier_exit(void)
{
	struct rb_node *node, *next_node;
	struct task_meta_priv *task_node, *temp_node;
	struct cvcore_name_meta_priv *name_node;
	unsigned long rw_lock_irq_flags;

	dprintk(0, "Unloading mero_notifier misc module.\n");

#if defined(ARM_CVIP) && defined(CONFIG_DEBUG_FS)
	debugfs_remove(debugfs_root);
#endif

	profile_event_unregister(PROFILE_TASK_EXIT, &task_notifier_block);

	write_lock_irqsave(&meta_rw_lock, rw_lock_irq_flags);

	deregister_xpc_handlers();

	list_for_each_entry_safe(task_node, temp_node, &task_meta_nodes, list) {
		list_del(&task_node->list);
		kfree(task_node);
	}

	for (node = rb_first(&cvcore_name_rbtree_root); node;) {
		name_node = rb_entry(node, struct cvcore_name_meta_priv, node);
		dprintk(2, "freeing cvcore name 0x%llx name_node %p node %p\n",
			name_node->name, name_node, node);
		next_node = rb_next(node);
		rb_erase(&name_node->node, &cvcore_name_rbtree_root);
		kfree(name_node);
		node = next_node;
	}

	write_unlock_irqrestore(&meta_rw_lock, rw_lock_irq_flags);

	free_dsp_dma_buf_resources();

	deinit_gsm_spinlock();

	deregister_sids();

	misc_deregister(&mero_notifier_device);
}

#ifdef ARM_CVIP
late_initcall_sync(mero_notifier_init);
#else
module_init(mero_notifier_init);
#endif
module_exit(mero_notifier_exit);

MODULE_AUTHOR("Sumit Jain<sjain@magicleap.com>");
MODULE_DESCRIPTION("Mero notifier misc module");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.8");

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
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/iommu.h>
#include <asm/set_memory.h>
#include <linux/profile.h>
#include <linux/cvip_status.h>
#include <linux/delay.h>
#include <linux/cred.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif

#include "shregion_status.h"
#include "shregion_types.h"
#include "shregion_internal.h"
#include "shregionctl_uapi.h"
#include "mero_xpc_kapi.h"
#include "xstat.h"
#include "cvcore_processor_common.h"
#include "cvcore_xchannel_map.h"

static unsigned int mydebug;
module_param(mydebug, uint, 0644);
MODULE_PARM_DESC(mydebug, "Activates local debug info");

#define MODULE_NAME "shregion"

#define dprintk(level, fmt, arg...)             \
  do {                                          \
    if (level <= mydebug)                       \
      pr_info(MODULE_NAME ": "fmt, ## arg);     \
  } while (0)


#define PCI_DEVICE_ID_CVIP		(0x149d)
#define CVIP_S_INTF_BAR			(2)
#define SCM_BAR_OFFSET			(0x0)

#define SECS_TO_MS			(1000)
#define CVIP_XPC_RESP_TIMEOUT_MS	(10 * SECS_TO_MS)
#define CVIP_MAX_XPC_XSTAT_RETRIES	(10)

// Don't start logging new high water marks until this level is reached.
#define MAX_REF_PIDS_QUIET_WATERMARK    (SHREGION_MAX_REF_PIDS / 2)

extern void *get_sintf_base(int);

static struct iommu_domain *domain = NULL;
static struct device *cdev = NULL;
struct pci_dev *pci_dev = NULL;
static struct x86_shared_shregion_meta *shared_meta = NULL;

extern struct device *cvip_get_mb_device(void);
extern struct cvip_status *cvip_get_status(void);

// structure private to this module.
struct shr_meta_priv {
	struct list_head list;
	struct shregion_metadata_drv meta_drv;
	pid_t ref_pids[SHREGION_MAX_REF_PIDS];
	uint32_t num_of_ref_pids;
	// Stores PID and it's corresponding buffer fd.
	struct {
		pid_t pid;
		uint64_t shr_uvaddr;
		uint8_t shr_uvaddr_valid;
	} pid_info[SHREGION_MAX_REF_PIDS];
	// Stores access permissions for shregion
	struct list_head perm_list;
};

struct shr_perm_priv {
	struct list_head list;
	pid_t pid;
};

struct pid_meta_s {
	pid_t pid;
};

// structure holding tgid of all the
// processes interacting with shregion
// driver.
struct registered_processes_s {
	struct pid_meta_s pid_meta[SHREGION_MAX_REF_PIDS];
	int nr_of_valid_pids;
	int nr_of_valid_pids_high_water;
} registered_processes;

// Declare and init the head node of the linked list
LIST_HEAD(shregion_nodes);

static DEFINE_MUTEX(shr_meta_mutex);

static const uid_t SHREGION_MIN_USER_UID = 10000;

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_root;

static int permissions_show(struct seq_file *s, void *data)
{
	struct shr_meta_priv *meta_node;
	struct shr_perm_priv *perm_node;

	seq_puts(s, "name\t\t\tpid\n");
	seq_puts(s, "---------------------------------\n");

	mutex_lock(&shr_meta_mutex);
	list_for_each_entry(meta_node, &shregion_nodes, list) {
		seq_printf(s, "%llx\n", meta_node->meta_drv.meta.name);

		list_for_each_entry(perm_node, &meta_node->perm_list, list)
			seq_printf(s, "\t\t\t%d\n", perm_node->pid);

		seq_puts(s, "---------------------------------\n");
	}
	mutex_unlock(&shr_meta_mutex);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(permissions);
#endif

static int is_shregion_mmap_denied(uint64_t shregion_name)
{
	struct shr_meta_priv *meta_node;
	struct shr_perm_priv *perm_node;

	if (__kuid_val(current_uid()) > SHREGION_MIN_USER_UID) {

		mutex_lock(&shr_meta_mutex);

		list_for_each_entry(meta_node, &shregion_nodes, list) {
			if (meta_node->meta_drv.meta.name == shregion_name) {
				list_for_each_entry(perm_node, &meta_node->perm_list, list) {
					if (perm_node->pid == current->tgid) {
						mutex_unlock(&shr_meta_mutex);
						return 0;
					}
				}
				break;
			}
		}

		mutex_unlock(&shr_meta_mutex);

		//TODO(kamanowicz): don't enforce permissions yet, just print log
		dprintk(0, "Shregion permission violation: shregion=0x%llx pid=%d (%s)\n",
				shregion_name, current->tgid, current->comm);
	}
	return 0;
}

static int is_mem_type_cvip_sram(uint8_t mem_type)
{
	if (mem_type == SHREGION_Q6_SCM_0)
		return 1;
	return 0;
}

static int is_mem_type_xport_access(uint8_t mem_type)
{
	if (mem_type == SHREGION_DDR_SHARED)
		return 1;
	return 0;
}

static void get_proc_shr_uvaddr(struct shr_meta_priv *meta_node,
				struct shr_prexist_meta *spm)
{
	int i;

	for (i = 0; i < meta_node->num_of_ref_pids; i++) {

		if (meta_node->pid_info[i].pid == current->tgid) {

			if (meta_node->pid_info[i].shr_uvaddr_valid) {
				spm->shr_uvaddr = meta_node->pid_info[i].shr_uvaddr;
				spm->shr_uvaddr_valid = meta_node->pid_info[i].shr_uvaddr_valid;
			}

			return;
		}
	}
}

// Must be called while holding mutex
static int get_xstat_status_ready(void)
{
	int i = 0;
	int ret;
	struct xstat_core_info info;

	// This logic would roughly spend around ~55s in worst case to get
	// expected status once the cvip alive msg is received. ~55s is
	// a lot of time and we expect to receive the expected
	// XSTAT_STATE_MODULES_LOADED status or better within a couple seconds
	// in normal case after the cvip alive message is received by x86.
	do {
		memset(&info, 0, sizeof(struct xstat_core_info));
		// check first if cvip has loaded all its modules and ready to respond to
		// our xpc cmds
		ret = xstat_request_cvip_info(&info, CVIP_XPC_RESP_TIMEOUT_MS);
		if (ret) {
			dprintk(0, "xstat_request_cvip_info failed, ret=%d\n", ret);
			return -1;
		}

		if (info.core_state >= XSTAT_STATE_MODULES_LOADED &&
			info.core_state < XSTAT_STATE_INVALID)
			return 0;

		msleep((i + 1) * SECS_TO_MS);

	} while (++i <= CVIP_MAX_XPC_XSTAT_RETRIES);

	return -2;
}

// Must be called while holding mutex
static int get_shared_meta_addr_from_arm(void)
{
	int ret;
	int status;
	XpcTarget destination = CVCORE_ARM;
	uint8_t send_buffer[MAX_XPC_MSG_LEN_BYTES] = { 0 };
	uint8_t response_buffer[MAX_XPC_MSG_LEN_BYTES] = { 0 };
	size_t response_length;
	void *ioremapped_shared_meta;
	struct shr_xpc_msg_resp msg_resp;
	struct shr_xpc_msg_req msg_req;

	if (shared_meta)
		return 0;

	ret = get_xstat_status_ready();
	if (ret) {
		dprintk(0, "get_xstat_status_ready failed");
		return ret;
	}

	msg_req.src_id = X86_SRC_CORE_ID;

	memcpy((void *)send_buffer, (void *)&msg_req, sizeof(struct shr_xpc_msg_req));

	status = xpc_command_send(CVCORE_XCHANNEL_SHREGION, destination, send_buffer,
		sizeof(send_buffer), response_buffer, sizeof(response_buffer),
		&response_length);

	if (status != 0) {
		dprintk(0, "xpc_command_send failed, status=%d\n", status);
		return -3;
	}

	memcpy((void *)&msg_resp, (void *)response_buffer, sizeof(struct shr_xpc_msg_resp));

	if (msg_resp.entry_status != ENTRY_IS_VALID) {
	    dprintk(0, "Received unexpected response from arm, entry_status=%d\n",
	    	msg_resp.entry_status);
	    return -4;
	}

	dprintk(0, "Got x86 shared meta address 0x%llx\n", msg_resp.addr);

	ioremapped_shared_meta = ioremap_wc(msg_resp.addr,
		sizeof(struct x86_shared_shregion_meta));

	shared_meta = (struct x86_shared_shregion_meta *)ioremapped_shared_meta;

	return 0;
}

static int check_arm_meta(void)
{
	const struct cvip_status *cvip_stat;
	int ret;

	cvip_stat = cvip_get_status();
	if (cvip_stat->is_alive) {
		if (!shared_meta) {
			ret = get_shared_meta_addr_from_arm();
			if (ret) {
				dprintk(0, "get_shared_meta_addr_from_arm failed, ret=%d", ret);
				mutex_unlock(&shr_meta_mutex);
				return -1;
			}
		}
	} else {
		shared_meta = NULL;
		dprintk(2, "cvip is not alive");
		return  -1;
	}

	if (shared_meta->meta_init_done != META_INIT_MAGIC_NUM ||
		shared_meta->nr_of_entries > MAX_x86_SHARED_SHR_ENTRIES) {
		dprintk(2, "shared meta structure is not valid yet.\n");
		return -1;
	}

	return 0;
}

static void shregion_query_entry(struct shregion_metadata_drv *meta_drv,
	struct file *filp)
{
	int i;
	uint8_t shregion_mem_type;
	int nr_of_entries;
	struct shr_meta_priv *meta_node;

	if (!shared_meta) {
		mutex_lock(&shr_meta_mutex);
		if (check_arm_meta()) {
			meta_drv->status = SHREGION_MEM_NONEXISTENT;
			mutex_unlock(&shr_meta_mutex);
			return;
		}
		mutex_unlock(&shr_meta_mutex);
	}

	nr_of_entries = shared_meta->nr_of_entries;

	if (filp)
		filp->private_data = NULL;

	for (i = 0; i < nr_of_entries; i++) {
		if (meta_drv->meta.name == shared_meta->cvcore_name_entry[i].name) {
			if (filp)
				filp->private_data = (void *)meta_drv->meta.name;
			meta_drv->meta.size_bytes = shared_meta->cvcore_name_entry[i].size_bytes;
			shregion_mem_type = shared_meta->cvcore_name_entry[i].type;
			if (is_mem_type_xport_access(shregion_mem_type)) {
				meta_drv->phys_addr = iommu_iova_to_phys(domain,
					(dma_addr_t)shared_meta->cvcore_name_entry[i].x86_phys_addr);
			} else {
				meta_drv->phys_addr = shared_meta->cvcore_name_entry[i].x86_phys_addr;
			}

			// Since we aren't using flags member element for anything here, we hack it
			// to propagate info whether guard bands are enabled on this shregion to
			// x86 userspace.
			meta_drv->meta.flags = shared_meta->cvcore_name_entry[i].guard_bands_config;

			meta_drv->status = SHREGION_MEM_PREEXISTENT;
		}
	}

	if (meta_drv->status == SHREGION_MEM_PREEXISTENT) {

		meta_drv->spm.shr_uvaddr_valid = 0;

		mutex_lock(&shr_meta_mutex);

		list_for_each_entry(meta_node, &shregion_nodes, list) {

			if (meta_node->meta_drv.meta.name == meta_drv->meta.name) {

				dprintk(1, "shregion(name=%llx) node already present\n",
					meta_drv->meta.name);

				get_proc_shr_uvaddr(meta_node, &meta_drv->spm);

				break;
			}
		}

		mutex_unlock(&shr_meta_mutex);

	} else {
		meta_drv->status = SHREGION_MEM_NONEXISTENT;
	}
}

// Call this while holding the mutex
static int register_pid(void)
{
	int i;
	int idx;

	for (i = 0; i < registered_processes.nr_of_valid_pids; ++i) {
		if (registered_processes.pid_meta[i].pid == current->tgid) {
			return 0;
		}
	}

	if (i >= registered_processes.nr_of_valid_pids) {

		if (i >= SHREGION_MAX_REF_PIDS) {
			dprintk(0, "Exceeding max shregion refs(%d)\n", SHREGION_MAX_REF_PIDS);
			return -EDQUOT;
		}

		// Registering this PID for the first time
		idx = registered_processes.nr_of_valid_pids;
		registered_processes.pid_meta[idx].pid = current->tgid;
		++registered_processes.nr_of_valid_pids;
		if (registered_processes.nr_of_valid_pids > registered_processes.nr_of_valid_pids_high_water) {
			registered_processes.nr_of_valid_pids_high_water = registered_processes.nr_of_valid_pids;
			if (registered_processes.nr_of_valid_pids_high_water > MAX_REF_PIDS_QUIET_WATERMARK) {
				dprintk(0, "Increasing max shregion refs to %d/%d for pid %d\n",
					registered_processes.nr_of_valid_pids_high_water, SHREGION_MAX_REF_PIDS,
					current->tgid);
			}
		}
	}

	return 0;
}

// Must be called while holding shr_meta_mutex lock.
// It returns true if the calling PID exists in pid info table
// of this shregion node.
// Returns the first free/vacant index (*idx) in the table if the pid wasn't
// found or else returns the idx of the pid found.
static bool this_pid_exists(struct shr_meta_priv *node, uint32_t *idx)
{
	int i;

	*idx = -1;

	// check if this PID entry already exists.
	for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {
		if (node->pid_info[i].pid == current->tgid) {
			*idx = i;
			return true;
		}

		if ((*idx == -1) && (node->pid_info[i].pid == -1))
			*idx = i;
	}

	return false;
}

static int register_shregion_pid_uvaddr(struct shregion_metadata_drv *meta_drv)
{
	int idx;
	int ret = 0;
	bool pid_entry_found = false;
	struct shr_meta_priv *meta_node;

	meta_drv->spm.shr_uvaddr_valid = 0;

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if (meta_node->meta_drv.meta.name == meta_drv->meta.name) {

			dprintk(1, "shregion(name=%llx) node already created\n",
				meta_drv->meta.name);

			pid_entry_found = this_pid_exists(meta_node, &idx);

			// Does it not exist and did shr ref reached
			// the max already?
			if (!pid_entry_found && (idx == -1)) {
				mutex_unlock(&shr_meta_mutex);
				dprintk(0, "Exceeding max shr refs(%d).\n", SHREGION_MAX_REF_PIDS);
				return -EDQUOT;
			}

			if (pid_entry_found) {
				// Likely some other thread belonging to the same process
				// got into the else part and registered this pid meta
				// for this shregion name.
				// Convey message to the userspace that this process now
				// already has a valid userspace virtual address, so munmap
				// the new address that it wanted to register with the kernel
				// driver for this shregion name
				get_proc_shr_uvaddr(meta_node, &meta_drv->spm);
			} else {
				ret = register_pid();
				if (ret == 0) {
					// Register the PID and corresponding info.
					meta_node->pid_info[idx].pid = current->tgid;
					meta_node->pid_info[idx].shr_uvaddr = meta_drv->spm.shr_uvaddr;
					meta_node->pid_info[idx].shr_uvaddr_valid = 1;
					meta_node->num_of_ref_pids++;
				}
			}

			mutex_unlock(&shr_meta_mutex);
			return ret;
		}
	}

	mutex_unlock(&shr_meta_mutex);
	return ret;
}

// Call this while holding the mutex
static int create_shregion_node(struct shregion_metadata_drv *meta_drv)
{
	int i;
	int ret = 0;
	struct shr_meta_priv *meta_node;

	meta_node = kmalloc(sizeof(struct shr_meta_priv), GFP_KERNEL);
	if (!meta_node)
		return -ENOMEM;

	memcpy(&(meta_node->meta_drv.meta), &(meta_drv->meta),
		sizeof(struct shregion_metadata));

	meta_node->num_of_ref_pids = 0;

	for (i = 0; i < SHREGION_MAX_REF_PIDS; i++) {
		meta_node->pid_info[i].pid = -1;
		meta_node->pid_info[i].shr_uvaddr_valid = 0;
	}

	INIT_LIST_HEAD(&meta_node->perm_list);
	list_add(&meta_node->list, &shregion_nodes);

	return ret;
}

int kmap_shregion(struct shregion_metadata *meta, void **vaddr) {
	struct shregion_metadata_drv shr_meta_drv;

	*vaddr = NULL;

	memcpy(&(shr_meta_drv.meta), meta, sizeof(struct shregion_metadata));

	shregion_query_entry(&shr_meta_drv, NULL);
	if (shr_meta_drv.status != SHREGION_MEM_PREEXISTENT) {
		dprintk(0, "kmap_shregion failed\n");
		return -ENOENT;
	}

	*vaddr = phys_to_virt(shr_meta_drv.phys_addr);

	return 0;
}
EXPORT_SYMBOL(kmap_shregion);

static int shregion_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int shregion_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int retval, i;
	phys_addr_t shregion_phys_addr;
	int nr_of_entries;
	int shregion_size_bytes = 0;
	int shregion_size_pg_sz_aligned;
	uint8_t shregion_mem_type;
	__u32 vma_length;
	int sram_offset;
	struct shr_meta_priv *meta_node;
	uint32_t cache_mask;

	if (!shared_meta) {
		dprintk(0, "shared shregion metadata is null\n");
		return -EINVAL;
	}

	if (shared_meta->meta_init_done != META_INIT_MAGIC_NUM) {
		dprintk(0, "shared shregion metadata is not valid\n");
		return -EINVAL;
	}

	nr_of_entries = shared_meta->nr_of_entries;

	vma_length = vma->vm_end - vma->vm_start;

	if (!filp->private_data) {
		dprintk(0, "private data is uninitialized\n");
		return -EINVAL;
	}

	if (is_shregion_mmap_denied((uint64_t)filp->private_data))
		return -EPERM;

	// Make sure the shregion meta is present.
	for (i = 0; i < nr_of_entries; i++) {
		if ((uint64_t)filp->private_data == shared_meta->cvcore_name_entry[i].name) {
			shregion_mem_type = shared_meta->cvcore_name_entry[i].type;
			if (is_mem_type_xport_access(shregion_mem_type)) {
				shregion_phys_addr = iommu_iova_to_phys(domain,
					(dma_addr_t)shared_meta->cvcore_name_entry[i].x86_phys_addr);
			} else {
				shregion_phys_addr = shared_meta->cvcore_name_entry[i].x86_phys_addr;
			}
			shregion_size_bytes = shared_meta->cvcore_name_entry[i].size_bytes;
			cache_mask = shared_meta->cvcore_name_entry[i].cache_mask;
			break;
		}
	}

	if (!shregion_size_bytes) {
		dprintk(0, "shregion(name=%llx) size can't be 0\n", (uint64_t)filp->private_data);
		return -EINVAL;
	}

	// Round of shregion_size_bytes to the next page size multiple.
	if (shregion_size_bytes % PAGE_SIZE)
		shregion_size_pg_sz_aligned = (((shregion_size_bytes / PAGE_SIZE) + 1) * PAGE_SIZE);
	else
		shregion_size_pg_sz_aligned = shregion_size_bytes;

	if (shregion_size_pg_sz_aligned != vma_length) {
		dprintk(0, "shregion(name=%llx) sizes (vma_len=%d, shr_size=%d) doesn't match.\n",
			(uint64_t)filp->private_data, vma_length, shregion_size_pg_sz_aligned);
		return -EINVAL;
	}

	mutex_lock(&shr_meta_mutex);

	list_for_each_entry(meta_node, &shregion_nodes, list) {
		if ((uint64_t)filp->private_data == meta_node->meta_drv.meta.name && !meta_node->num_of_ref_pids) {
			if (!is_mem_type_cvip_sram(shregion_mem_type) && !(cache_mask & SHREGION_X86_CACHED))
				set_memory_uc((unsigned long)phys_to_virt(shregion_phys_addr),
					shregion_size_pg_sz_aligned / PAGE_SIZE);
			break;
		}
	}

	mutex_unlock(&shr_meta_mutex);

	// NOTE: There's cache coherency benefit to be taken if
	// X-port accesses are in play. But FMR_10, FMR_2_10,
	// CVIP SRAM (S-port) accesses from cvip or x86 don't go
	// through X-port.

	if (is_mem_type_cvip_sram(shregion_mem_type)) {

		// Here shregion_phys_addr actually contains offset in this case.
		if (shregion_mem_type == SHREGION_Q6_SCM_0)
			sram_offset = SCM_BAR_OFFSET;
		else {
			dprintk(0, "Invalid cvip sram type=%d\n", shregion_mem_type);
			return -EINVAL;
		}

		dprintk(1, "memtype=%d,sram_off=%d,phys_offset=%d,name=%llx\n",
			shregion_mem_type, sram_offset, shregion_phys_addr,
			(uint64_t)filp->private_data);

		vma->vm_pgoff = (sram_offset + shregion_phys_addr) >> PAGE_SHIFT;

		retval = pci_mmap_resource_range(pci_dev, CVIP_S_INTF_BAR, vma, pci_mmap_mem, 1);

	} else {
		if (!(cache_mask & SHREGION_X86_CACHED))
			vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

		retval = remap_pfn_range(vma, vma->vm_start, shregion_phys_addr >> PAGE_SHIFT,
			vma_length, vma->vm_page_prot);
	}

	if (retval < 0) {
		dprintk(0, "mmap failed with err %d\n", retval);
		return retval;
	}

	return 0;
}

static void shregion_cleanup(struct task_struct *task)
{
	struct shr_meta_priv *meta_node;
	int i;
	int last_valid_idx;

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

	// Deregister process id.
	last_valid_idx = registered_processes.nr_of_valid_pids - 1;
	memcpy((void *)&registered_processes.pid_meta[i],
		(void *)&registered_processes.pid_meta[last_valid_idx],
		sizeof(struct pid_meta_s));
	--registered_processes.nr_of_valid_pids;

	list_for_each_entry(meta_node, &shregion_nodes, list) {

		if (meta_node->num_of_ref_pids > 0) {

			for (i = 0; i < SHREGION_MAX_REF_PIDS &&
					meta_node->num_of_ref_pids > 0; i++) {

				if (meta_node->pid_info[i].pid == task->pid) {
					meta_node->pid_info[i].pid = -1;
					meta_node->pid_info[i].shr_uvaddr_valid = 0;
					meta_node->num_of_ref_pids--;
				}
			}
		}
	}

	mutex_unlock(&shr_meta_mutex);
}

static int shregion_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static long shregion_unlocked_ioctl(struct file *filp, unsigned int cmd,
				    unsigned long arg)
{
	int retval = 0;
	struct shregion_metadata_drv meta_drv;

	if (!access_ok((void *)arg, sizeof(struct shregion_metadata_drv))) {
		dprintk(0, "Invalid user-space buffer.\n");
		return -EINVAL;
	}

	if (_IOC_TYPE(cmd) != SHREGION_IOC_MAGIC) {
		dprintk(0, "Invalid ioctl.\n");
		return -EINVAL;
	}

	if (copy_from_user((void *)&meta_drv, (void *)arg,
		sizeof(struct shregion_metadata_drv))) {
		dprintk(0, "copy_from_user failed.\n");
		return -EFAULT;
	}

	switch (cmd) {

	case SHREGION_IOC_QUERY:
		shregion_query_entry(&meta_drv, filp);
		if (copy_to_user((void *)arg, &meta_drv,
			sizeof(struct shregion_metadata_drv))) {
			dprintk(0, "query ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}
		break;

	case SHREGION_IOC_REGISTER:
		retval = register_shregion_pid_uvaddr(&meta_drv);
		if (copy_to_user((void *)arg, &meta_drv,
			sizeof(struct shregion_metadata_drv))) {
			dprintk(0, "register ioctl cmd: copy_to_user failed.\n");
			return -EFAULT;
		}
		break;

	default:
		return -EINVAL;
	}

	return retval;
}

static long shregionctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct shregion_permissions shr_perm;
	struct shregion_metadata_drv meta_drv;
	struct shr_meta_priv *meta_node;
	struct shr_perm_priv *perm_node, *perm_new;
	long ret = -EINVAL;
	int i;
	int nr_of_entries;

	switch (cmd) {
	case SHREGION_IOC_SET_PID_PERM:
		if (copy_from_user(&shr_perm, (void *)arg, sizeof(shr_perm))) {
			dprintk(0, "shregionctl: copy_from_user failed.\n");
			return -EFAULT;
		}

		mutex_lock(&shr_meta_mutex);
		if (shr_perm.operation == SHREGION_PID_GRANT) {
again:
			list_for_each_entry(meta_node, &shregion_nodes, list) {

				if (meta_node->meta_drv.meta.name == shr_perm.shr_name) {
					list_for_each_entry(perm_node, &meta_node->perm_list, list) {
						if (perm_node->pid == shr_perm.pid) {
							mutex_unlock(&shr_meta_mutex);
							return -EEXIST;
						}
					}

					perm_new = kmalloc(sizeof(struct shr_perm_priv), GFP_KERNEL);
					if (!perm_new) {
						mutex_unlock(&shr_meta_mutex);
						return -ENOMEM;
					}

					perm_new->pid = shr_perm.pid;
					list_add_tail(&perm_new->list, &meta_node->perm_list);
					ret = 0;
					break;
				}
			}

			if (ret == -EINVAL) {
				if (check_arm_meta()) {
					mutex_unlock(&shr_meta_mutex);
					return -EFAULT;
				}

				nr_of_entries = shared_meta->nr_of_entries;

				for (i = 0; i < nr_of_entries; i++) {
					if (shr_perm.shr_name == shared_meta->cvcore_name_entry[i].name) {
						meta_drv.meta.name = shared_meta->cvcore_name_entry[i].name;
						meta_drv.meta.size_bytes = shared_meta->cvcore_name_entry[i].size_bytes;
						meta_drv.meta.type = shared_meta->cvcore_name_entry[i].type;
						ret = create_shregion_node(&meta_drv);
						if (ret) {
							mutex_unlock(&shr_meta_mutex);
							dprintk(0, "shregionctl: create_shregion_node failed with err %d\n", ret);
							return ret;
						}
						goto again;
					}
				}
			}
		} else if (shr_perm.operation == SHREGION_PID_REVOKE) {

			list_for_each_entry(meta_node, &shregion_nodes, list) {
				if (meta_node->meta_drv.meta.name == shr_perm.shr_name) {
					list_for_each_entry(perm_node, &meta_node->perm_list, list) {
						if (perm_node->pid == shr_perm.pid) {
							list_del(&perm_node->list);
							kfree(perm_node);
							mutex_unlock(&shr_meta_mutex);
							return 0;
						}
					}
				}
			}
			ret = -ENOENT;
		}
		mutex_unlock(&shr_meta_mutex);

		if (ret == -EINVAL)
			pr_err("shregionctl: shregion name 0x%lx not found\n", shr_perm.shr_name);

		return ret;

	default:
		return -EINVAL;
	}
}

static const struct file_operations shregionctl_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = shregionctl_ioctl,
};

struct miscdevice shregionctl_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "shregionctl",
	.fops = &shregionctl_fops,
};

static const struct file_operations shregion_fops = {
	.owner		= THIS_MODULE,
	.open		= shregion_open,
	.mmap		= shregion_mmap,
	.unlocked_ioctl = shregion_unlocked_ioctl,
	.release	= shregion_close,
};

struct miscdevice shregion_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "shregion",
	.fops = &shregion_fops,
};

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

static int __init shregion_init(void)
{
	int error;
	int ret;

	pci_dev = pci_get_device(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_CVIP, pci_dev);
	if (!pci_dev) {
		dprintk(0, "Failed to find pci device. pci_get_device failed.\n");
		return -ENODEV;
	}

	error = misc_register(&shregion_device);
	if (error) {
		dprintk(0, "Failed to register shregion misc module.\n");
		return error;
	}

	error = misc_register(&shregionctl_device);
	if (error) {
		dprintk(0, "Failed to register shregionctl misc device.\n");
		return error;
	}

	shregion_app_register();

	/* I'm using this to get the device that has the correct iommu
	 * domain, only query once, the device doesn't change.
	 */
	if (!cdev) {
		cdev = cvip_get_mb_device();
	}

	if (!domain) {
		domain = iommu_get_domain_for_dev(cdev);
	}

	ret = profile_event_register(PROFILE_TASK_EXIT, &task_notifier_block);
	if (ret) {
		dprintk(0, "Failed to register task exit event, ret=%d\n", ret);
		return ret;
	}

	registered_processes.nr_of_valid_pids = 0;
	registered_processes.nr_of_valid_pids_high_water = 0;

#ifdef CONFIG_DEBUG_FS
	debugfs_root = debugfs_create_dir("shregion", NULL);
	debugfs_create_file("permissions", 0444, debugfs_root, NULL, &permissions_fops);
#endif

	pr_info("Successfully loaded x86 shregion misc module.\n");
	return 0;
}

static void __exit shregion_exit(void)
{
	pr_info("Unloading x86 shregion misc module.\n");

#ifdef CONFIG_DEBUG_FS
	debugfs_remove(debugfs_root);
#endif

	profile_event_unregister(PROFILE_TASK_EXIT, &task_notifier_block);

	shregion_app_deregister();
	misc_deregister(&shregion_device);
	misc_deregister(&shregionctl_device);
}

module_init(shregion_init);
module_exit(shregion_exit);

MODULE_AUTHOR("Sumit Jain <sjain@magicleap.com>");
MODULE_DESCRIPTION("x86 shregion misc module");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

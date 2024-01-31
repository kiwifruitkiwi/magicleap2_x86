// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2023
// Magic Leap, Inc. (COMPANY)
//

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/dsp_manager_kapi.h>

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif

#include "mero_smmu_private.h"
#include "mero_smmu_uapi.h"
#include "gsm_spinlock.h"
#include "gsm_cvip.h"
#include "cvcore_xchannel_map.h"
#include "cvcore_status.h"
#include "mero_xpc_types.h"
#include "nibble.h"
#include "cvcore_processor_common.h"

static unsigned int debug_level;
module_param(debug_level, uint, 0644);
MODULE_PARM_DESC(debug_level, "Get/set local debug level");

#define MODULE_NAME "mero_smmu"

#define dprintk(level, fmt, arg...)             \
  do {                                          \
    if (level <= debug_level)                   \
      pr_info(MODULE_NAME ": "fmt, ## arg);     \
  } while (0)

// stream id and pid owner lookup
struct {
	sid_t stream_id;
	pid_t owner_pid;
	uint32_t gsm_slot_idx;
} sid_pid_meta[TOTAL_NR_OF_STREAM_IDS];

// cross core gsm backed spinlock
static struct gsm_spinlock_context gsm_spinlock_ctx;
static uint32_t ctx_base_addr;

extern int shregion_smmu_map(sid_t stream_id);
extern void shregion_smmu_unmap(sid_t stream_id);
extern int xpc_register_command_handler(XpcChannel channel,
	XpcCommandHandler handler, XpcHandlerArgs args);

/*
 * write gsm_sid_ctx structure back to gsm memory. Currently its size is
 * 96 bits so we use gsm raw write 32 command
 */
static void gsm_sid_ctx_write(int index, gsm_stream_id_ctx_t *gsm_sid_ctx);
/*
 * read gsm_sid_ctx structure from gsm memory. Currently its size is
 * 96 bits so we use gsm raw read 32 command.
 */
static void gsm_sid_ctx_read(int index, gsm_stream_id_ctx_t *gsm_sid_ctx);

static int smmu_check_ref_count(gsm_stream_id_ctx_t *gsm_sid_ctx)
{
	int i;
	uint32_t current_ref_count = 0;
	/* Global reference count needs to be equal to all dsp
	 * ref counts + 1 (arm process). At this point it may
	 * be equal or greater by 2. We need to check and correct it
	 * if needed.
	 */
	for (i = 0; i < CVCORE_NUM_DSPS; i++)
		current_ref_count += gsm_sid_ctx->sid_ctx.dsp_ref[i];

	if (gsm_sid_ctx->sid_ctx.ref_count == current_ref_count) {
	/* Dsp may have been shutdown in the middle of task unbinding.
	 * It decremented global ref count but was shutdown before it
	 * updated per dsp ref count. All we need to do is increment
	 * the global ref count (if it's not orphaned)
	 */
		if (!gsm_sid_ctx->sid_ctx.is_orphaned)
			gsm_sid_ctx->sid_ctx.ref_count++;
	}
	else if ((gsm_sid_ctx->sid_ctx.ref_count == (current_ref_count + 2)) ||
			((gsm_sid_ctx->sid_ctx.ref_count == (current_ref_count + 1)) &&
			 gsm_sid_ctx->sid_ctx.is_orphaned)) {
	/* Dsp shutdown occured on task binding. Global ref
	 * has been incremeneted but per dsp ref count has not.
	 * Decrement global ref count
	 */
		gsm_sid_ctx->sid_ctx.ref_count--;
	}
	else {
	/* Some other case that we didn't expect and don't know how to
	 * recover from. Just return error to the caller.
	 */
		return -1;
	}

	return 0;
}

/* Called from dsp manager when dsp is shutdown */
static int smmu_on_dsp_shutdown(struct notifier_block *nb,
			unsigned long dsp_id, void *data)
{
	int err;
	int i;
	gsm_stream_id_ctx_t gsm_sid_ctx;
	int unclean_shutdown = 0;
	uint32_t nibble_value;
	uint32_t cpu_id = (uint32_t)-1;
	uint16_t core_id = (uint16_t)CVCORE_DSP_RENUMBER_Q6C5_TO_C5Q6(dsp_id);


	/* Check who owns the spinlock, if it's core that's been shutdown
	 * we need to do unlock the spinlock and do additional checks to
	 * ensure reference count is valid
	 */
	nibble_value = nibble_test_and_set_axi_sync(gsm_spinlock_ctx.nibble_addr,
			gsm_spinlock_ctx.nibble_id);

	if (nibble_value) {
		switch (nibble_value) {
			case CVCORE_AXI_Q6_P0:
				cpu_id = CVCORE_DSP_Q6_0;
				break;
			case CVCORE_AXI_Q6_P1:
				cpu_id = CVCORE_DSP_Q6_1;
				break;
			case CVCORE_AXI_Q6_P2:
				cpu_id = CVCORE_DSP_Q6_2;
				break;
			case CVCORE_AXI_Q6_P3:
				cpu_id = CVCORE_DSP_Q6_3;
				break;
			case CVCORE_AXI_Q6_P4:
				cpu_id = CVCORE_DSP_Q6_4;
				break;
			case CVCORE_AXI_Q6_P5:
				cpu_id = CVCORE_DSP_Q6_5;
				break;
			case CVCORE_AXI_C5_P0:
				cpu_id = CVCORE_DSP_C5_0;
				break;
			case CVCORE_AXI_C5_P1:
				cpu_id = CVCORE_DSP_C5_1;
				break;
			default:
				break;
		}

		if (unlikely(cpu_id == core_id)) {
			unclean_shutdown = 1;
			nibble_write_sync(gsm_spinlock_ctx.nibble_addr,
					gsm_spinlock_ctx.nibble_id, CVCORE_AXI_A55);
		}
	}

	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {
		gsm_sid_ctx_read(i, &gsm_sid_ctx);

		if (unlikely(unclean_shutdown)) {
			err = smmu_check_ref_count(&gsm_sid_ctx);
			if (err) {
				pr_err("FATAL: smmu sid 0x%x ref count is broken.\n",
						gsm_sid_ctx.sid_ctx.stream_id);
				return -1;
			}
		}

		gsm_sid_ctx.sid_ctx.ref_count -= gsm_sid_ctx.sid_ctx.dsp_ref[core_id];
		gsm_sid_ctx.sid_ctx.dsp_ref[core_id] = 0;

		/* if the ref count is 0 we can unmap the shregions */
		if (!gsm_sid_ctx.sid_ctx.ref_count) {
			shregion_smmu_unmap(gsm_sid_ctx.sid_ctx.stream_id);
			gsm_sid_ctx.sid_ctx.is_active = 0;
			gsm_sid_ctx.sid_ctx.is_orphaned = 0;
		}
		gsm_sid_ctx_write(i, &gsm_sid_ctx);
	}

	gsm_spin_unlock(&gsm_spinlock_ctx);
	return 0;
}

static struct notifier_block dsp_core_shutdown_nb = {
	.notifier_call = smmu_on_dsp_shutdown
};

// Must be called while holding the lock
static void update_sid_pid_meta(gsm_stream_id_ctx_t *gsm_sid_ctx,
	int gsm_slot_idx, pid_t pid)
{
	sid_pid_meta[gsm_slot_idx].stream_id = gsm_sid_ctx->sid_ctx.stream_id;
	sid_pid_meta[gsm_slot_idx].gsm_slot_idx = gsm_slot_idx;
	sid_pid_meta[gsm_slot_idx].owner_pid = pid;
}

static void gsm_sid_ctx_write(int index, gsm_stream_id_ctx_t *gsm_sid_ctx) {
    int i;
    uint32_t sid_ctx_addr = ctx_base_addr + (index * sizeof(gsm_stream_id_ctx_t));

    if (!gsm_sid_ctx) {
        pr_err("gsm_sid_ctx cannot be NULL\n");
        return;
    }

    for (i = 0; i < sizeof(gsm_stream_id_ctx_t) / sizeof(uint32_t); i++) {
        gsm_raw_write_32(sid_ctx_addr, gsm_sid_ctx->ctx_val[i]);
        sid_ctx_addr += sizeof(uint32_t);
    }
}

static void gsm_sid_ctx_read(int index, gsm_stream_id_ctx_t *gsm_sid_ctx) {
	int i;
    uint32_t sid_ctx_addr = ctx_base_addr + (index * sizeof(gsm_stream_id_ctx_t));

    if (!gsm_sid_ctx) {
        pr_err("gsm_sid_ctx cannot be NULL\n");
        return;
    }

    for (i = 0; i < sizeof(gsm_stream_id_ctx_t) / sizeof(uint32_t); i++) {
        gsm_sid_ctx->ctx_val[i] = gsm_raw_read_32(sid_ctx_addr);
        sid_ctx_addr += sizeof(uint32_t);
    }
}

static int _smmu_bind_process_to_stream_id_exclusive(sid_t stream_id, pid_t pid, int do_map)
{
	int i;
	int err;
	int ret;
	gsm_stream_id_ctx_t gsm_sid_ctx;

	// Acquire gsm spinlock first
	err = gsm_spin_lock(&gsm_spinlock_ctx);
	if (err) {
		dprintk(0, "%s: failed to grab gsm spinlock, err=%d\n", __func__, err);
		return -1;
	}

	// Check our list of known stream IDs in the global list (resides in GSM) and
	// attempt to acquire the stream id for exclusive use.
	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {
		if (sid_pid_meta[i].stream_id != stream_id)
			continue;

		gsm_sid_ctx.ctx_val[0] = gsm_raw_read_32(ctx_base_addr +
						      (i * sizeof(gsm_stream_id_ctx_t)));
		if (gsm_sid_ctx.sid_ctx.stream_id == stream_id) {
			// Is the stream ID active?
			if (gsm_sid_ctx.sid_ctx.is_active) {
				if (gsm_sid_ctx.sid_ctx.is_orphaned) {
					// The streamID is active but it is not associated with an ARM process.
					// This is likely a teardown scenario and clients may want to try
					// again soon
					err = -EAGAIN;
				} else {
					// The streamID is active with no signs of being inactive soon.
					// Clients should try again later
					err = -EBUSY;
				}
				gsm_spin_unlock(&gsm_spinlock_ctx);
				return err;
			}

			if (do_map) {
				ret = shregion_smmu_map(stream_id);
				if (ret) {
					gsm_spin_unlock(&gsm_spinlock_ctx);
					return ret;
				}
			}
			// Now set the reference count to one since ARM is now the sole owner of the
			// StreamID
			gsm_sid_ctx.sid_ctx.ref_count = 1;
			gsm_sid_ctx.sid_ctx.is_active = 1;
			// Signify that this StreamID is not an orphan
			gsm_sid_ctx.sid_ctx.is_orphaned = 0;
			// Clear per dsp ref counters
			gsm_sid_ctx.ctx_val[1] = 0;
			gsm_sid_ctx.ctx_val[2] = 0;
			// write changes back to the gsm
			gsm_sid_ctx_write(i, &gsm_sid_ctx);

			update_sid_pid_meta(&gsm_sid_ctx, i, pid);

			gsm_spin_unlock(&gsm_spinlock_ctx);

			return 0;
		}
	}

	gsm_spin_unlock(&gsm_spinlock_ctx);
	// Otherwise we didn’t find the requested StreamID in our known set of known stream IDs
	return -ENOENT;
}

int smmu_bind_process_to_stream_id_exclusive(sid_t stream_id)
{
	return _smmu_bind_process_to_stream_id_exclusive(stream_id, current->tgid, 1);
}
EXPORT_SYMBOL(smmu_bind_process_to_stream_id_exclusive);

static int _smmu_unbind_process_from_stream_id(sid_t stream_id, int do_unmap) {
	int i;
	int err;
	int gsm_idx;
	gsm_stream_id_ctx_t gsm_sid_ctx;

	// Acquire gsm spinlock first
	err = gsm_spin_lock(&gsm_spinlock_ctx);
	if (err) {
		dprintk(0, "%s: failed to grab gsm spinlock, err=%d\n", __func__, err);
		return -1;
	}

	// find the gsm slot
	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {
		if (sid_pid_meta[i].stream_id == stream_id) {
			break;
		}
	}

	if (i >= TOTAL_NR_OF_STREAM_IDS) {
		gsm_spin_unlock(&gsm_spinlock_ctx);
		return -ENOENT;
	}

	gsm_idx = sid_pid_meta[i].gsm_slot_idx;

	gsm_sid_ctx.ctx_val[0] = gsm_raw_read_32(ctx_base_addr +
					      (gsm_idx * sizeof(gsm_stream_id_ctx_t)));
	if ((gsm_sid_ctx.sid_ctx.stream_id == stream_id) &&
		 gsm_sid_ctx.sid_ctx.is_active) {
		// decrement reference count
		--gsm_sid_ctx.sid_ctx.ref_count;
		// This process no longer owns the stream id
		gsm_sid_ctx.sid_ctx.is_orphaned = 1;
		sid_pid_meta[i].owner_pid = -1;

		if (!gsm_sid_ctx.sid_ctx.ref_count) {
			// We decremented from 1 to 0 so let’s unmap
			if (do_unmap)
				shregion_smmu_unmap(stream_id);
			gsm_sid_ctx.sid_ctx.is_active = 0;
			gsm_sid_ctx.sid_ctx.is_orphaned = 0;
		}

		// write changes back to the gsm
		gsm_raw_write_32(ctx_base_addr + (gsm_idx * sizeof(gsm_stream_id_ctx_t)),
						 gsm_sid_ctx.ctx_val[0]);
		update_sid_pid_meta(&gsm_sid_ctx, i, 0);
		gsm_spin_unlock(&gsm_spinlock_ctx);
		return 0;
	} else {
		// We should never get into this condition
		if (gsm_sid_ctx.sid_ctx.stream_id != stream_id) {
			dprintk(0, "FATAL: local gsm info (%d) doesn't match with gsm (%d)\n",
				gsm_sid_ctx.sid_ctx.stream_id, stream_id);
			gsm_spin_unlock(&gsm_spinlock_ctx);
			return -EFAULT;
		}
	}
	gsm_spin_unlock(&gsm_spinlock_ctx);
	return 0;
}

int smmu_unbind_process_from_stream_id(sid_t stream_id)
{
	return _smmu_unbind_process_from_stream_id(stream_id, 1);
}
EXPORT_SYMBOL(smmu_unbind_process_from_stream_id);

static int mero_smmu_open(struct inode *inode, struct file *filp)
{
	filp->private_data = (void *)(uintptr_t)CVCORE_STATUS_FAILURE_DSP_STREAM_ID_INVALID;
	return 0;
}

/* ioctl interface for deprecated userspace api */
static long mero_smmu_ioctl(struct file *filp, unsigned int cmd, unsigned long data)
{
	struct MeroSmmuRequest req;
	int ret = 0;

	if ((uintptr_t)filp->private_data != CVCORE_STATUS_FAILURE_DSP_STREAM_ID_INVALID) {
		ret = smmu_unbind_process_from_stream_id((uintptr_t)filp->private_data);
		if (ret)
			return ret;
	}

	if (copy_from_user(&req, (void *)data, sizeof(req)))
		return -EFAULT;

	switch (cmd) {
	case MERO_SMMU_BIND_STREAM_ID:
		ret = smmu_bind_process_to_stream_id_exclusive(req.stream_id);
		if (!ret)
			filp->private_data = (void *)(uintptr_t)req.stream_id;
		break;
	default:
		ret = -ENOTTY;
	}
	return ret;
}

static int mero_smmu_release(struct inode *inode, struct file *filp)
{
	int ret;

	if ((uintptr_t)filp->private_data == CVCORE_STATUS_FAILURE_DSP_STREAM_ID_INVALID) {
		return 0;
	}

	ret = smmu_unbind_process_from_stream_id((uintptr_t)filp->private_data);
	return ret;
}

static struct file_operations mero_smmu_fops = {
	.owner = THIS_MODULE,
	.open = mero_smmu_open,
	.release = mero_smmu_release,
	.unlocked_ioctl = mero_smmu_ioctl,
};

static struct miscdevice mero_smmu_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &mero_smmu_fops,
};

int mero_smmu_command_handler(XpcChannel channel, XpcHandlerArgs args, uint8_t *command_buffer,
        size_t command_buffer_length, uint8_t *response_buffer, size_t resize_buffer_length,
        size_t *response_length)
{
	int i;
	int err;
	struct smmu_maint_req req;
	int gsm_idx;
	gsm_stream_id_ctx_t gsm_sid_ctx;

	dprintk(2 ,"mero_smmu: received xpc interrupt\n");

	if (channel != CVCORE_XCHANNEL_MERO_SMMU) {
		dprintk(0, "Not the vchannel(got=%d,interested=%d) mero_smmu is interested in.\n",
			channel, CVCORE_XCHANNEL_MERO_SMMU);
		goto err_handle;
	}

	memcpy((void *)&req, (void *)command_buffer, sizeof(struct smmu_maint_req));

	if (req.type != SMMU_MAINT_TYPE_UNMAP) {
		dprintk(2, "unknown req type (%d)\n", req.type);
		goto err_handle;
	}

	dprintk(2 ,"unmap req for sid = %d\n", req.stream_id);

	// Acquire gsm spinlock first
	err = gsm_spin_lock(&gsm_spinlock_ctx);
	if (err) {
		dprintk(0, "%s: failed to grab gsm spinlock, err=%d\n", __func__, err);
		goto err_handle;
	}

	// find the gsm slot
	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {
		if (sid_pid_meta[i].stream_id == req.stream_id) {
			break;
		}
	}

	if (i >= TOTAL_NR_OF_STREAM_IDS) {
		gsm_spin_unlock(&gsm_spinlock_ctx);
		dprintk(0, "stream id (%d) not found in gsm table\n", req.stream_id);
		goto err_handle;
	}

	gsm_idx = sid_pid_meta[i].gsm_slot_idx;

	gsm_sid_ctx.ctx_val[0] = gsm_raw_read_32(ctx_base_addr +
					      (gsm_idx * sizeof(gsm_stream_id_ctx_t)));
	if ((gsm_sid_ctx.sid_ctx.stream_id == req.stream_id) &&
		 gsm_sid_ctx.sid_ctx.is_active) {

		shregion_smmu_unmap(req.stream_id);

		gsm_sid_ctx.sid_ctx.is_active = 0;
		gsm_sid_ctx.sid_ctx.is_orphaned = 0;

		// write changes back to the gsm
		gsm_raw_write_32(ctx_base_addr + (gsm_idx * sizeof(gsm_stream_id_ctx_t)),
				 gsm_sid_ctx.ctx_val[0]);
	}

	gsm_spin_unlock(&gsm_spinlock_ctx);

	err = 0;
	memcpy((void *)response_buffer, (void *)&err, sizeof(err));
	*response_length = sizeof(err);

	return err;

err_handle:
	err = -1;
	memcpy((void *)response_buffer, (void *)&err, sizeof(err));
	*response_length = sizeof(err);
	return err;
}

static int init_gsm_stream_id_global(void)
{
	int i;
	int err;
	gsm_stream_id_ctx_t gsm_sid_ctx = {0};

	err = gsm_spinlock_create(&gsm_spinlock_ctx);
	if (err) {
		dprintk(0, "Failed to create gsm cross core spinlock,err=%d\n", err);
		return err;
	}

	// Save spinlock info in gsm so that it can be shared with the dsps.
	// The purpose of this spinlock is to provide mutual exclusive access
	// to the stream id ctx table stored in gsm.
	gsm_raw_write_32(GSM_RESERVED_STREAM_ID_GLOBAL, gsm_spinlock_ctx.nibble_addr);
	gsm_raw_write_32(GSM_RESERVED_STREAM_ID_GLOBAL + sizeof(gsm_spinlock_ctx.nibble_addr),
		gsm_spinlock_ctx.nibble_id);

	ctx_base_addr = GSM_RESERVED_STREAM_ID_GLOBAL + sizeof(struct gsm_spinlock_context);

	// Init gsm_sid_ctx for all stream ids
	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {
		gsm_sid_ctx.sid_ctx.stream_id = cvcore_stream_ids[i];

		gsm_sid_ctx_write(i, &gsm_sid_ctx);
		update_sid_pid_meta(&gsm_sid_ctx, i, 0);
	}

	return 0;
}

static void deregister_xpc_mero_smmu_command_handler(void)
{
	int ret;
	ret = xpc_register_command_handler(CVCORE_XCHANNEL_MERO_SMMU, NULL, NULL);
	if (ret != 0)
		dprintk(0, "failed to deregister xpc command handler,ret=%d\n", ret);
}

static int register_xpc_mero_smmu_command_handler(void)
{
	int ret;
	// Register shregion cmd handler with xpc
	ret = xpc_register_command_handler(CVCORE_XCHANNEL_MERO_SMMU,
				&mero_smmu_command_handler, NULL);
	if (ret != 0) {
		dprintk(0, "failed to register command handler,ret=%d\n", ret);
		return -1;
	}
	return 0;
}

#ifdef CONFIG_DEBUG_FS
static int bind_map_show(struct seq_file *s, void *data)
{
	int i, err;
	gsm_stream_id_ctx_t gsm_sid_ctx;

	err = gsm_spin_lock(&gsm_spinlock_ctx);
	if (err) {
		seq_printf(s, "gsm_spin_lock failed (%d): try again\n", err);
		return 0;
	}

	seq_puts(s, "SID\tORPHAN\tACTIVE\tREF\tPID\n");

	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {
		gsm_sid_ctx.ctx_val[0] = gsm_raw_read_32(ctx_base_addr +
						      (i * sizeof(gsm_stream_id_ctx_t)));
		seq_printf(s, "0x%hx\t%u\t%u\t%u\t%d\n",
			gsm_sid_ctx.sid_ctx.stream_id,
			gsm_sid_ctx.sid_ctx.is_orphaned,
			gsm_sid_ctx.sid_ctx.is_active,
			gsm_sid_ctx.sid_ctx.ref_count,
			sid_pid_meta[i].owner_pid);

	}

	gsm_spin_unlock(&gsm_spinlock_ctx);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(bind_map);

static ssize_t debug_bind_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int pid, ret;
	sid_t sid;
	char tmp_buf[64] = { 0 };

	if (count >= 64) {
		return -EINVAL;
	}

	ret = copy_from_user(tmp_buf, buf, count);
	if (ret) {
		return -EINVAL;
	}

	if (sscanf(tmp_buf, "%hx %d", &sid, &pid) != 2) {
		return -EINVAL;
	}

	ret =_smmu_bind_process_to_stream_id_exclusive(sid, pid, 0);
	if (ret)
		return -EINVAL;
	return count;
}

static const struct file_operations debug_bind_fops = {
	.owner = THIS_MODULE,
	.write = debug_bind_write
};

static ssize_t debug_unbind_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int pid, ret, i;
	sid_t sid;
	char tmp_buf[64] = { 0 };

	if (count >= 64) {
		return -EINVAL;
	}

	ret = copy_from_user(tmp_buf, buf, count);
	if (ret) {
		return -EINVAL;
	}

	if (sscanf(tmp_buf, "%hx %d", &sid, &pid) != 2) {
		return -EINVAL;
	}

	for (i = 0; i < TOTAL_NR_OF_STREAM_IDS; ++i) {
		if (sid_pid_meta[i].stream_id == sid &&
				sid_pid_meta[i].owner_pid != pid)
			return -EINVAL;
	}

	ret = _smmu_unbind_process_from_stream_id(sid, 0);
	if (ret)
		return -EINVAL;
	return count;
}

static const struct file_operations debug_unbind_fops = {
	.owner = THIS_MODULE,
	.write = debug_unbind_write
};

static struct dentry *mero_smmu_dir;
#endif

static int __init mero_smmu_init(void)
{
	int ret;

	// Init GSM SMMU space with known stream ids
	ret = init_gsm_stream_id_global();
	if (ret) {
		dprintk(0, "init_gsm_stream_id_global failed, ret=%d\n", ret);
		return ret;
	}

	ret = misc_register(&mero_smmu_dev);
	if (ret) {
		dprintk(0, "failed to register mero smmu dev (%d)", ret);
		return ret;
	}

	ret = register_xpc_mero_smmu_command_handler();
	if (ret != 0)
		return ret;

	dsp_mode_register_notify(&dsp_core_shutdown_nb, DSP_OFF_MASK);

#ifdef CONFIG_DEBUG_FS
	mero_smmu_dir = debugfs_create_dir("mero_smmu", NULL);
	debugfs_create_file("bind_map", 0400, mero_smmu_dir, NULL, &bind_map_fops);
	debugfs_create_file("bind", 0600, mero_smmu_dir, NULL, &debug_bind_fops);
	debugfs_create_file("unbind", 0600, mero_smmu_dir, NULL, &debug_unbind_fops);
#endif

	dprintk(0, "mero_smmu module loaded\n");

	return ret;
}

static void __exit mero_smmu_exit(void)
{
	int err;
#ifdef CONFIG_DEBUG_FS
	debugfs_remove(mero_smmu_dir);
#endif

	dsp_mode_deregister_notify(&dsp_core_shutdown_nb);

	deregister_xpc_mero_smmu_command_handler();

	misc_deregister(&mero_smmu_dev);

	err = gsm_spinlock_destroy(&gsm_spinlock_ctx);
	if (err)
		dprintk(0, "Failed to destroy gsm cross core spinlock, err=%d\n", err);

	dprintk(0, "mero_smmu module unloaded\n");
}

module_init(mero_smmu_init);
module_exit(mero_smmu_exit);

MODULE_AUTHOR("Sumit Jain <sjain@magicleap.com>");
MODULE_DESCRIPTION("mero smmu module");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2020-2023
// Magic Leap, Inc. (COMPANY)
//

#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/version.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs_struct.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/errno.h> // EINVAL

#include "cvcore_processor_common.h"
#include "cvcore_status.h"
#include "cvcore_xchannel_map.h"
#include "dsp_core_common.h"
#include "dsp_core_control_packets.h"
#include "dsp_core_response_packets.h"
#include "dsp_core_status_packets.h"
#include "dsp_core_types.h"
#include "dsp_core_uapi.h"
#include "mero_smmu_private.h"
#include "mero_xpc.h"
#include "mero_xpc_kapi.h"

#define MODULE_NAME "dsp_core"
#define DEVICE_NAME "dsp_core"
#define CLASS_NAME "dsp_tasking"

#define DSP_CORE_FILE_COUNT 1
#define MAX_TASK_MANAGERS (8)

struct dsp_core_monitor_struct {
	struct task_struct *task;
	wait_queue_head_t wq;
	spinlock_t lock;
	int state;
};

static struct dsp_core_monitor_struct dsp_core_monitor_info;

static int dsp_core_kthread(void *arg);
static void dsp_core_start_listener(void);

static const int kMaxXpcRetryCount = 10;
static const int kMaxTaskManagers = 8;
static const int kMaxBuiltInTasks = 18;
static const int kMaxBuiltInDelegates = 16;
static const uint16_t kCoreMask = 0xFF;

static int stream_manager_open(struct inode *inode, struct file *filp);
static int stream_manager_release(struct inode *inode, struct file *filp);
static long stream_manager_ioctl(struct file *filp, unsigned int cmd,
				 unsigned long data);
static int stream_manager_create_fs_device(void);
static void stream_manager_destroy_fs_device(void);
static int
stream_manager_handle_bind_request(struct stream_manager_t *client,
				   struct DspCoreBindRequest *request);
static int
stream_manager_handle_info_request(struct stream_manager_t *client,
				   struct DspCoreStreamInfoRequest *request);
static bool task_manager_active_[MAX_TASK_MANAGERS];
static struct dsp_core_task_manager_info task_manager_info_[MAX_TASK_MANAGERS];
static uint16_t task_manager_supported_commands_[MAX_TASK_MANAGERS]
						[TM_COMMAND_COUNT];
static uint64_t built_in_task_ptr_[MAX_TASK_MANAGERS][kBuiltInTaskID_LAST];
static uint64_t built_in_delegate_ptr_[MAX_TASK_MANAGERS]
				      [kBuiltInDelegateID_LAST];

static int dsp_core_major;
struct dsp_core_t *stream_manager_master;

extern int smmu_bind_process_to_stream_id_exclusive(sid_t stream_id);
extern int smmu_unbind_process_from_stream_id(sid_t stream_id);

bool is_valid_core(uint8_t core_id)
{
	return ((kCoreMask >> core_id) & 0x1) ? true : false;
}

static struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.open = stream_manager_open,
	.release = stream_manager_release,
	.unlocked_ioctl = stream_manager_ioctl,
	.compat_ioctl = stream_manager_ioctl,
};

static bool get_command_channel(uint8_t command, uint8_t core_id,
				uint8_t *channel)
{
	uint16_t channel_mask = 0;
	uint8_t i;
	if (core_id == CVCORE_ARM) {
		if (command == kCmdWorkQueueEnqueue) {
			*channel = CVCORE_XCHANNEL_DSPCORE_QUEUE;
			return true;
		}
		return false;
	}

	if (command == kCmdManagerGetInfo ||
	    command == kCmdManagerGetCommands) {
		*channel = CVCORE_XCHANNEL_DSPCORE_ASYNC;
		return true;
	} else {
		channel_mask =
			task_manager_supported_commands_[core_id][command];
		if (channel_mask) {
			for (i = 0; i < 16; i++) {
				if (channel_mask & (0x1 << i)) {
					*channel = i;
					return true;
				}
			}
		}
	}
	return false;
}

int submit_command_packet(uint8_t tgt_core_id,
			  struct task_manager_control_packet *cmd_packet,
			  struct task_manager_response_packet *rsp_packet)
{
	int status;
	size_t response_length;
	uint8_t xchannel = 0x0;
	int retry_count = kMaxXpcRetryCount;
	enum task_manager_command_id cmd_id;

	if (!is_valid_core(tgt_core_id))
		return -ENODEV;

	cmd_id = task_manager_command_get_type(cmd_packet);

	if (!get_command_channel(cmd_id, tgt_core_id, &xchannel)) {
		return -EINVAL;
	}

	do {
		status = xpc_command_send(
			xchannel, (XpcTarget)tgt_core_id, cmd_packet->bytes,
			sizeof(cmd_packet->bytes), rsp_packet->bytes,
			sizeof(rsp_packet->bytes), &response_length);

		/*
			TODO(kirick): Once CVCORE_STATUS_FAILURE_TRY_AGAIN is reported from xpc, convert
			this conditional to look specifically for (status == CVCORE_STATUS_FAILURE_TRY_AGAIN).
			For now, treat all non-CVCORE_STATUS_SUCCESS as a reason to try again.
		*/
		retry_count--;
		if (status && (retry_count > 0)) {
			//std::this_thread::sleep_for(std::chrono::microseconds(kXpcRetryDelayUs));
		}
	} while (status && (retry_count > 0));

	if (status) {
		return -EIO;
	}

	return task_manager_response_get_status(rsp_packet);
}

int submit_command_packet_no_response(
	uint8_t tgt_core_id, struct task_manager_control_packet *cmd_packet)
{
	int status;
	uint8_t xchannel = 0x0;
	int retry_count = kMaxXpcRetryCount;
	enum task_manager_command_id cmd_id;

	if (!is_valid_core(tgt_core_id))
		return -ENODEV;

	cmd_id = task_manager_command_get_type(cmd_packet);

	if (!get_command_channel(cmd_id, tgt_core_id, &xchannel)) {
		return -EINVAL;
	}

	do {
		status = xpc_command_send_no_response(
			xchannel, (XpcTarget)tgt_core_id, cmd_packet->bytes,
			sizeof(cmd_packet->bytes));

		/*
			TODO(kirick): Once CVCORE_STATUS_FAILURE_TRY_AGAIN is reported from xpc, convert
			this conditional to look specifically for (status == CVCORE_STATUS_FAILURE_TRY_AGAIN).
			For now, treat all non-CVCORE_STATUS_SUCCESS as a reason to try again.
		*/
		retry_count--;
		if (status && (retry_count > 0)) {
			//std::this_thread::sleep_for(std::chrono::microseconds(kXpcRetryDelayUs));
		}
	} while (status && (retry_count > 0));

	if (status) {
		return -EIO;
	}

	return 0;
}

/* Get general info about the task manager on a core */
int query_task_manager(uint8_t core_id, struct dsp_core_task_manager_info *info)
{
	int err;
	struct task_manager_control_packet cmd_packet;
	struct task_manager_response_packet rsp_packet;
	struct task_manager_get_info_response *rsp;

	/* Build the command then send it*/
	initialize_manager_info_command(&cmd_packet);
	err = submit_command_packet(core_id, &cmd_packet, &rsp_packet);
	if (err) {
		DSP_CORE_ERROR("Initialize manager info failed\n");
		return -ENOTTY;
	}

	/* Parse the response */
	rsp = get_manager_info_response(&rsp_packet);
	*info = rsp->info;

	return 0;
}

static int discover_supported_commands(uint8_t core_id, uint16_t *cmd_array)
{
	int err;
	uint8_t cmd;
	struct task_manager_control_packet cmd_packet;
	struct task_manager_response_packet rsp_packet;
	struct task_manager_get_command_info_response *rsp;

	/* Discover supported commands and their channels */
	for (cmd = TM_COMMAND_FIRST; cmd <= TM_COMMAND_LAST; cmd++) {
		/* Build the command then send it*/
		initialize_manager_command_info_command(
			&cmd_packet, (enum task_manager_command_id)cmd);
		err = submit_command_packet(core_id, &cmd_packet, &rsp_packet);
		if (err) {
			return -ENOTTY;
		}

		/* Parse the response */
		rsp = get_manager_command_info_response(&rsp_packet);
		cmd_array[cmd] = rsp->command_channels;
	}

	return 0;
}

/* Find all built-in symbols on a core */
int discover_built_in_symbols(uint8_t core_id, uint64_t *task_ptrs,
			      uint64_t *delegate_ptrs)
{
	int err;
	uint8_t task_id, delegate_id;
	struct task_manager_control_packet cmd_packet;
	struct task_manager_response_packet rsp_packet;
	struct task_manager_get_built_in_symbol_response *rsp;

	for (task_id = 0; task_id < kMaxBuiltInTasks; task_id++) {
		initialize_manager_get_built_in_task_command(&cmd_packet,
							     task_id);

		err = submit_command_packet(core_id, &cmd_packet, &rsp_packet);
		if (err) {
			return -ENOTTY;
		}

		rsp = get_manager_built_in_symbol_response(&rsp_packet);
		task_ptrs[task_id] = rsp->ptr;
	}

	for (delegate_id = 0; delegate_id < kMaxBuiltInDelegates;
	     delegate_id++) {
		initialize_manager_get_built_in_delegate_command(&cmd_packet,
								 delegate_id);

		err = submit_command_packet(core_id, &cmd_packet, &rsp_packet);
		if (err) {
			return -ENOTTY;
		}

		rsp = get_manager_built_in_symbol_response(&rsp_packet);
		delegate_ptrs[delegate_id] = rsp->ptr;
	}
	return 0;
}

static int discover_task_managers(void)
{
	uint8_t core_id;
	int err;
	int dev_cnt = 0;

	/* Get general manager info */
	for (core_id = CVCORE_DSP_ID_MIN; core_id <= CVCORE_DSP_ID_MAX;
	     core_id++) {
		if (!is_valid_core(core_id)) {
			task_manager_active_[core_id] = false;
			continue;
		}

		err = query_task_manager(core_id, &task_manager_info_[core_id]);
		if (err) {
			DSP_CORE_ERROR("Query task managers failed\n");
			task_manager_active_[core_id] = false;
			continue;
		}

		err = discover_supported_commands(
			core_id, task_manager_supported_commands_[core_id]);
		if (err) {
			DSP_CORE_ERROR("Discover supported commands failed\n");
			task_manager_active_[core_id] = false;
			continue;
		}

		err = discover_built_in_symbols(
			core_id, built_in_task_ptr_[core_id],
			built_in_delegate_ptr_[core_id]);
		if (err) {
			DSP_CORE_ERROR("Discover built-in symbols failed\n");
			task_manager_active_[core_id] = false;
			continue;
		}

		task_manager_active_[core_id] = true;
		++dev_cnt;
		//print_manager_info(&task_manager_info_[core_id]);
	}
	return dev_cnt;
}

static struct dsp_core_t *dsp_core_allocate(void)
{
	struct dsp_core_t *device;

	device = kzalloc(sizeof(*device), GFP_KERNEL);
	if (!device) {
		goto error;
	}

	/* Initialize mutex */
	mutex_init(&device->mtx);

	/* Initialize client_list */
	INIT_LIST_HEAD(&device->managers);
	return device;

error:
	return ERR_PTR(-ENOMEM);
}

static void dsp_core_deallocate(struct dsp_core_t *device)
{
	struct list_head *pos;
	struct list_head *q;
	struct stream_manager_t *manager;

	if (device) {
		list_for_each_safe (pos, q, &device->managers) {
			manager =
				list_entry(pos, struct stream_manager_t, node);
			list_del(pos);
			kfree(manager);
		}

		kfree(device);
	}
}

static void dsp_core_start_listener(void)
{
	spin_lock(&(dsp_core_monitor_info.lock));
	if (dsp_core_monitor_info.state == 0) {
		dsp_core_monitor_info.state = 1;
		spin_unlock(&(dsp_core_monitor_info.lock));
		dsp_core_monitor_info.task =
			kthread_run(dsp_core_kthread, NULL, "DSP_CORE_MONITOR");

		DSP_CORE_INFO("Started dsp_core_kernel listener\n");
	} else {
		spin_unlock(&(dsp_core_monitor_info.lock));
	}
}

static void dsp_core_stop_listener(void)
{
	kthread_stop(dsp_core_monitor_info.task);
}

static int dsp_core_kthread(void *arg)
{
	int ret;
	struct task_manager_status_packet packet;
	enum task_manager_status_type_id packet_type;
	struct task_control_task_status_message *task_status_message;

	while (!kthread_should_stop()) {
		ret = xpc_queue_recv(CVCORE_XCHANNEL_DSPCORE_ASYNC, 0xC,
				     packet.bytes, XPC_MAX_QUEUE_LENGTH);
		if (ret == 0) {
			packet_type = task_manager_status_get_type(&packet);

			switch (packet_type) {
			case kStatTaskState: {
				task_status_message =
					get_task_control_task_status_message(
						&packet);

				task_control_handle_task_status_message(
					task_status_message);
			} break;
			default:
				break;
			}
		} else if (ret == -EPERM) {
			DSP_CORE_INFO(
				"Exiting dsp_core_kernel listener "
				"because the xpc recv queue was deregisterd\n");
			break;
		}
	}

	return 0;
}

static int __init dsp_core_init(void)
{
	int ret;

	// Register for notifications from DSP
	ret = xpc_queue_register(CVCORE_XCHANNEL_DSPCORE_ASYNC, 0xC);
	if (ret) {
		DSP_CORE_ERROR("Could not register stream channel - %d\n", ret);
		return ret;
	}

	ret = stream_manager_create_fs_device();
	if (ret) {
		DSP_CORE_ERROR("Could not create stream manager device - %d\n",
			       ret);
		return ret;
	}

	ret = task_control_create_fs_device();
	if (ret) {
		DSP_CORE_ERROR("Could not create task control device - %d\n",
			       ret);
		return ret;
	}

	ret = task_manager_create_fs_device();
	if (ret) {
		DSP_CORE_ERROR("Could not create task manager device - %d\n",
			       ret);
		return ret;
	}

	// Start listener thread
	dsp_core_monitor_info.state = 0;
	dsp_core_start_listener();

	DSP_CORE_DBG("Successfully loaded dsp_core module\n");
	return 0;
}

static void __exit dsp_core_exit(void)
{
	// Deregister for notifications from DSP
	xpc_queue_deregister(CVCORE_XCHANNEL_DSPCORE_ASYNC, 0xC);

	dsp_core_stop_listener();

	task_control_destroy_fs_device();
	task_manager_destroy_fs_device();
	stream_manager_destroy_fs_device();

	DSP_CORE_DBG("Successfully removed dsp_core module\n");
}

static int stream_manager_open(struct inode *inode, struct file *filp)
{
	int ret;
	struct list_head *pos;
	struct stream_manager_t *new_manager;

	ret = mutex_lock_interruptible(&stream_manager_master->mtx);
	if (ret) {
		DSP_CORE_ERROR("Failed to lock dsp core.\n");
		goto error;
	}

	/* Make sure a stream manager for this process does not exist in the master list*/
	list_for_each (pos, &stream_manager_master->managers) {
		struct stream_manager_t *old_manager;

		old_manager = list_entry(pos, struct stream_manager_t, node);
		if (old_manager->pid == current->tgid) {
			ret = -EBUSY;
			mutex_unlock(&stream_manager_master->mtx);
			goto error;
		}
	}

	new_manager = kzalloc(sizeof(*new_manager), GFP_KERNEL);
	if (!new_manager) {
		DSP_CORE_ERROR("Failed to allocate new client.\n");
		ret = -ENOMEM;
		goto error;
	}

	new_manager->pid = current->tgid;
	new_manager->stream_id = 0xFFFF;
	new_manager->active_cores = 0x0;
	new_manager->is_bound = false;

	mutex_init(&new_manager->mtx);
	INIT_LIST_HEAD(&new_manager->tcontrol_list);

	list_add(&new_manager->node, &stream_manager_master->managers);
	mutex_unlock(&stream_manager_master->mtx);

	filp->private_data = new_manager;
	DSP_CORE_DBG("Opened new instance of dsp_core for PID %d\n",
		     new_manager->pid);
	return 0;

error:
	return ret;
}

static int stream_manager_release(struct inode *inode, struct file *filp)
{
	int ret;
	struct list_head *pos;
	struct list_head *q;
	struct stream_manager_t *manager;
	struct task_control_handle_t *task_handle;

	ret = mutex_lock_interruptible(&stream_manager_master->mtx);
	if (ret) {
		DSP_CORE_ERROR("Failed to lock dsp_core master device.\n");
		goto error;
	}

	manager = (struct stream_manager_t *)filp->private_data;
	if (!list_empty(&manager->tcontrol_list)) {
		list_for_each_safe (pos, q, &manager->tcontrol_list) {
			task_handle = list_entry(
				pos, struct task_control_handle_t, node);
			task_control_detach_from_parent(task_handle);
		}

		/* Unbind this dsp_core client*/
		if (manager->is_bound) {
			u8 core_id;
			struct task_manager_control_packet cmd_packet;
			struct task_manager_response_packet rsp_packet;
			struct task_manager_abort_stream_id_response *abort_rsp;

			DSP_CORE_DBG(
				"Cleaning up dsp_core for PID %d and StreamID %d\n",
				current->pid, manager->stream_id);

			ret = smmu_unbind_process_from_stream_id(
				manager->stream_id);
			if (ret) {
				DSP_CORE_ERROR(
					"Error unbinding smmu stream id: %d\n",
					ret);
				goto error_unlock;
			}

			initialize_task_manager_abort_stream_id_command(
				&cmd_packet, manager->stream_id);

			for (core_id = CVCORE_DSP_ID_MIN;
			     core_id <= CVCORE_DSP_ID_MAX; core_id++) {
				if ((manager->active_cores >> core_id) & 0x1) {
					DSP_CORE_DBG(
						"Requesting TaskManager on Core %d to abort all tasks with StreamID %d\n",
						core_id, manager->stream_id);
					ret = submit_command_packet(
						core_id, &cmd_packet,
						&rsp_packet);
					if (ret) {
						DSP_CORE_ERROR(
							"Error sending abort stream_id command - status was %d\n",
							ret);
					} else {
						abort_rsp =
							get_manager_abort_stream_id_response(
								&rsp_packet);
						DSP_CORE_DBG(
							"TaskManager on Core %d reported %d tasks aborted with StreamID %d\n",
							core_id,
							abort_rsp->num_aborted,
							manager->stream_id);
					}
				}
			}
		}
	} else {
		if (manager->is_bound) {
			ret = smmu_unbind_process_from_stream_id(
				manager->stream_id);
			if (ret) {
				DSP_CORE_ERROR(
					"Error unbinding smmu stream id: %d\n",
					ret);
				goto error_unlock;
			}
		}
	}

	DSP_CORE_DBG("Closed instance of dsp_core for PID %d\n", manager->pid);

	manager->is_bound = false;
	manager->stream_id = 0;
	list_del(&manager->node);
	kfree(manager);
error_unlock:
	mutex_unlock(&stream_manager_master->mtx);
error:
	return ret;
}

/**
 * dsp_core_ioctl - This is the main dispatch routine
 * for ioctls issued from userspace.  Ioctls are decoded and
 * dispacted to one of the dsp_core_handle_* routines.
 */
static long stream_manager_ioctl(struct file *filp, unsigned int cmd,
				 unsigned long data)
{
	int ret = 0;
	struct stream_manager_t *manager =
		(struct stream_manager_t *)filp->private_data;

	if ((void *)data == NULL)
		return -EINVAL;

	switch (cmd) {
	/**
	 * Bind this stream manager to the requested stream_id
	 */
	case DSP_CORE_BIND_STREAM_ID: {
		struct DspCoreBindRequest req;

		if (copy_from_user((void *)&req, (void *)data,
				   sizeof(struct DspCoreBindRequest))) {
			return -EFAULT;
		}
		ret = stream_manager_handle_bind_request(manager, &req);
	} break;
	/**
	 * Get status of this stream manager
	 */
	case DSP_CORE_STREAM_CLIENT_GET_INFO: {
		struct DspCoreStreamInfoRequest req;

		if (copy_from_user((void *)&req, (void *)data,
				   sizeof(struct DspCoreStreamInfoRequest))) {
			return -EFAULT;
		}
		ret = stream_manager_handle_info_request(manager, &req);
	} break;
	default:
		ret = -ENOTTY;
	}
	return ret;
}

static int
stream_manager_handle_bind_request(struct stream_manager_t *manager,
				   struct DspCoreBindRequest *request)
{
	int ret;
	int num_tm;
	uint8_t core_id;

	/* Find the task managers on all DSP */
	num_tm = discover_task_managers();
	if (num_tm <= 0) {
		DSP_CORE_ERROR("Failed discovering task managers\n");
		return -ENODEV;
	}

	for (core_id = CVCORE_DSP_ID_MIN; core_id <= CVCORE_DSP_ID_MAX;
	     core_id++) {
		if ((request->requested_cores >> core_id) & 0x1) {
			if (!task_manager_active_[core_id]) {
				return -ENODEV;
			}
		}
	}

	/* bind to smmu stream id, if the stream id is not ready yet just
	 * try until binding succeeds or fails hard */
	do {
		ret = smmu_bind_process_to_stream_id_exclusive(
			request->stream_id);
		if (ret == -EAGAIN) {
			if (msleep_interruptible(1))
				return -EINTR;
		}
	} while (ret == -EAGAIN);

	if (!ret) {
		manager->stream_id = request->stream_id;
		manager->active_cores = request->requested_cores;
		manager->is_bound = true;

		DSP_CORE_DBG("Handled request to bind stream id 0x%04x\n",
			     request->stream_id);
	} else {
		DSP_CORE_DBG(
			"Failed to handle request to bind stream id 0x%04x",
			request->stream_id);
	}

	return ret;
}

static int
stream_manager_handle_info_request(struct stream_manager_t *manager,
				   struct DspCoreStreamInfoRequest *request)
{
	struct task_control_handle_t *pos, *task_handle;

	request->num_task_managers = 0;
	list_for_each_entry_safe (pos, task_handle, &manager->tcontrol_list,
				  node) {
		request->num_task_managers++;
	}
	request->is_bound = manager->is_bound;

	return 0;
}

static int stream_manager_create_fs_device(void)
{
	int ret = 0;

	/* Register char device */
	ret = register_chrdev(0, DEVICE_NAME, &f_ops);
	if (ret < 0) {
		DSP_CORE_ERROR("Failed to register with error %d\n", ret);
		goto error;
	}
	dsp_core_major = ret;

	DSP_CORE_DBG("Device registered at %d\n", dsp_core_major);

	/* Allocate memory for master object */
	stream_manager_master = dsp_core_allocate();
	if (IS_ERR(stream_manager_master)) {
		DSP_CORE_ERROR("Failed to allocate dsp_core_master object.\n");
		ret = PTR_ERR(stream_manager_master);
		goto post_chrdev_error;
	}

	/* Register the device class */
	stream_manager_master->char_class =
		class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(stream_manager_master->char_class)) {
		ret = PTR_ERR(stream_manager_master->char_class);
		goto post_allocate_error;
	}

	/* Register the device driver */
	stream_manager_master->char_device =
		device_create(stream_manager_master->char_class, NULL,
			      MKDEV(dsp_core_major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(stream_manager_master->char_device)) {
		DSP_CORE_ERROR("Failed to create the device.\n");
		ret = PTR_ERR(stream_manager_master->char_device);
		goto post_class_error;
	}

	DSP_CORE_DBG("Device initialized successfully.\n");
	return 0;

post_class_error:
	class_destroy(stream_manager_master->char_class);
post_allocate_error:
	dsp_core_deallocate(stream_manager_master);
post_chrdev_error:
	unregister_chrdev(dsp_core_major, DEVICE_NAME);
error:
	return ret;
}

static void stream_manager_destroy_fs_device(void)
{
	device_destroy(stream_manager_master->char_class,
		       MKDEV(dsp_core_major, 0));
	//	class_unregister(stream_manager_master->char_class);
	class_destroy(stream_manager_master->char_class);
	unregister_chrdev(dsp_core_major, DEVICE_NAME);
	dsp_core_deallocate(stream_manager_master);
}

module_init(dsp_core_init);
module_exit(dsp_core_exit);

MODULE_AUTHOR("Kevin Irick<kirick_siliconscapes@magicleap.com>");
MODULE_DESCRIPTION("dsp_core module");
MODULE_LICENSE("GPL");

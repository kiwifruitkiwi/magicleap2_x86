// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2020-2023
// Magic Leap, Inc. (COMPANY)
//

#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <asm/io.h>
#include <linux/jiffies.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs_struct.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/errno.h> // EINVAL

#include "cvcore_processor_common.h"
#include "cvcore_status.h"
#include "cvcore_xchannel_map.h"
#include "dsp_core_control_packets.h"
#include "dsp_core_response_packets.h"
#include "dsp_core_status_packets.h"
#include "dsp_core_common.h"
#include "dsp_core_types.h"
#include "dsp_core_uapi.h"
#include "mero_xpc.h"

#define MODULE_NAME "dsp_core"
#define TASK_CTRL_DEVICE_NAME "dsp_core_tctrl"
#define CLASS_NAME "dsp_tasking"

static const char *dsp_core_run_state_str[] = {
	"kReady",     "kStarting", "kStarted", "kRunning", "kSuspending",
	"kSuspended", "kResuming", "kExiting", "kExited",  "kScheduled",
	"kAborting",  "kAborted",  "kInvalid", "kUnknown"
};

static int task_control_open(struct inode *inode, struct file *filp);
static int task_control_release(struct inode *inode, struct file *filp);
static long task_control_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long data);
static int
task_control_exclusive_access_start(struct task_control_handle_t *handle);
static void
task_control_exclusive_access_end(struct task_control_handle_t *handle);
static int task_control_attach_to_parent(struct task_control_handle_t *handle);
int task_control_detach_from_parent(struct task_control_handle_t *handle);
static int task_control_handle_find_and_attach(
	struct task_control_handle_t *t_handle,
	struct TaskControlFindAndAttachRequest *request);
static int task_control_handle_abort(struct task_control_handle_t *t_handle);
static int
task_control_handle_abort_no_lock(struct task_control_handle_t *t_handle);
static int task_control_handle_start(struct task_control_handle_t *t_handle,
				     struct TaskControlStartRequest *request);
static int
task_control_handle_set_task_priority(struct task_control_handle_t *t_handle,
				      struct TaskControlPriorityInfo *request);
static int
task_control_handle_get_task_priority(struct task_control_handle_t *t_handle,
				      struct TaskControlPriorityInfo *response);
static int
task_control_handle_get_task_state(struct task_control_handle_t *t_handle,
				   struct TaskControlTaskStateInfo *response);
static int task_control_handle_wait_task_state(
	struct task_control_handle_t *t_handle,
	struct TaskControlTaskWaitStateInfo *request);

static struct task_control_t *tcontrol;

static struct file_operations f_ops = { .owner = THIS_MODULE,
					.open = task_control_open,
					.release = task_control_release,
					.unlocked_ioctl = task_control_ioctl,
					.compat_ioctl = task_control_ioctl };

static struct task_control_t *task_control_allocate(void)
{
	struct task_control_t *device;
	device = kzalloc(sizeof(*device), GFP_KERNEL);
	if (!device) {
		goto error;
	}
	return device;
error:
	return ERR_PTR(-ENOMEM);
}

static void task_control_deallocate(struct task_control_t *device)
{
	kfree(device);
}

int task_control_create_fs_device(void)
{
	int ret = 0;
	int dev_major;
	dev_t first_dev;

	/* Allocate character device region */
	ret = alloc_chrdev_region(&first_dev, 0, 1, TASK_CTRL_DEVICE_NAME);
	if (ret < 0) {
		DSP_CORE_ERROR("tctrl alloc_chrdev_region error: %d\n", ret);
		goto error;
	}
	dev_major = MAJOR(first_dev);

	/* Allocate task control global object */
	tcontrol = task_control_allocate();
	if (IS_ERR(tcontrol)) {
		DSP_CORE_ERROR("Failed to allocate dsp_core_master object.\n");
		ret = PTR_ERR(tcontrol);
		goto post_chrdev_error;
	}

	/* Initialize char device */
	cdev_init(&tcontrol->cdev, &f_ops);
	tcontrol->cdev.owner = THIS_MODULE;
	tcontrol->dev = MKDEV(dev_major, 0);

	ret = cdev_add(&tcontrol->cdev, tcontrol->dev, 1);
	if (ret < 0) {
		DSP_CORE_ERROR("tctrl cdev_add error: %d\n", ret);
		goto post_allocate_error;
	}

	/* Create device node */
	tcontrol->char_device =
		device_create(stream_manager_master->char_class, NULL,
			      tcontrol->dev, NULL, TASK_CTRL_DEVICE_NAME);
	if (IS_ERR(&tcontrol->char_device)) {
		DSP_CORE_ERROR("task_control device_create\n");
		goto post_cdev_add_error;
	}

	/* Success */
	return 0;

post_cdev_add_error:
	cdev_del(&tcontrol->cdev);
post_allocate_error:
	task_control_deallocate(tcontrol);
post_chrdev_error:
	unregister_chrdev_region(first_dev, 1);
error:
	return ret;
}

int task_control_destroy_fs_device(void)
{
	cdev_del(&tcontrol->cdev);
	device_destroy(stream_manager_master->char_class, tcontrol->dev);
	unregister_chrdev_region(tcontrol->dev, 1);
	task_control_deallocate(tcontrol);
	return 0;
}

static int task_control_open(struct inode *inode, struct file *filp)
{
	struct task_control_handle_t *t_handle;
	int ret;

	t_handle = kzalloc(sizeof(*t_handle), GFP_KERNEL);
	if (!t_handle) {
		DSP_CORE_ERROR("Failed to allocate new tctrl client.\n");
		ret = -ENOMEM;
		goto error;
	}

	INIT_TASK_LOCATOR(&t_handle->locator);
	mutex_init(&t_handle->mtx);

	t_handle->is_attached = false;
	t_handle->is_aborted = false;
	t_handle->is_exited = false;
	t_handle->is_started = false;
	filp->private_data = t_handle;

	init_waitqueue_head(&t_handle->wait_queue);

	task_control_attach_to_parent(t_handle);
	return 0;

error:
	return ret;
}

static int task_control_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct task_control_handle_t *t_handle =
		(struct task_control_handle_t *)filp->private_data;

	task_control_detach_from_parent(t_handle);
	// Should be safe to free memory since we are not owned anymore
	kfree(t_handle);
	return ret;
}

static long task_control_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long data)
{
	int ret = 0;
	struct task_control_handle_t *t_handle =
		(struct task_control_handle_t *)filp->private_data;

	if ((void *)data == NULL)
		return -EINVAL;

	switch (cmd) {
	case DSP_CORE_TASK_CONTROL_FIND_AND_ATTACH_TASK: {
		struct TaskControlFindAndAttachRequest request;

		if (copy_from_user(
			    (void *)&request, (void *)data,
			    sizeof(struct TaskControlFindAndAttachRequest))) {
			return -EFAULT;
		}

		ret = task_control_handle_find_and_attach(t_handle, &request);
	} break;
	case DSP_CORE_TASK_CONTROL_START_TASK: {
		struct TaskControlStartRequest request;

		if (copy_from_user((void *)&request, (void *)data,
				   sizeof(struct TaskControlStartRequest))) {
			return -EFAULT;
		}

		ret = task_control_handle_start(t_handle, &request);
	} break;
	case DSP_CORE_TASK_CONTROL_WAIT_TASK_STATE: {
		struct TaskControlTaskWaitStateInfo request;

		if (copy_from_user(
			    (void *)&request, (void *)data,
			    sizeof(struct TaskControlTaskWaitStateInfo))) {
			return -EFAULT;
		}

		// Sleep wait on a task state
		ret = task_control_handle_wait_task_state(t_handle, &request);

		if (copy_to_user((void *)data, (void *)&request,
				 sizeof(struct TaskControlTaskWaitStateInfo))) {
			return -EFAULT;
		}
	} break;
	case DSP_CORE_TASK_CONTROL_ABORT_TASK: {
		ret = task_control_handle_abort(t_handle);
	} break;
	case DSP_CORE_TASK_CONTROL_SET_TASK_PRIORITY: {
		struct TaskControlPriorityInfo request;

		if (copy_from_user((void *)&request, (void *)data,
				   sizeof(struct TaskControlPriorityInfo))) {
			return -EFAULT;
		}

		ret = task_control_handle_set_task_priority(t_handle, &request);
	} break;
	case DSP_CORE_TASK_CONTROL_GET_TASK_PRIORITY: {
		struct TaskControlPriorityInfo response;

		ret = task_control_handle_get_task_priority(t_handle,
							    &response);

		if (copy_to_user((void *)data, (void *)&response,
				 sizeof(struct TaskControlPriorityInfo))) {
			return -EFAULT;
		}
	} break;
	case DSP_CORE_TASK_CONTROL_GET_TASK_STATE: {
		struct TaskControlTaskStateInfo response;

		ret = task_control_handle_get_task_state(t_handle, &response);

		if (copy_to_user((void *)data, (void *)&response,
				 sizeof(struct TaskControlTaskStateInfo))) {
			return -EFAULT;
		}
	} break;
	default:
		ret = -ENOTTY;
	}
	return ret;
}

static int
task_control_exclusive_access_start(struct task_control_handle_t *handle)
{
	int ret;

	if (!handle->owner)
		return -EOWNERDEAD; // already detached

	ret = mutex_lock_interruptible(&handle->mtx);
	if (ret) {
		DSP_CORE_ERROR("Failed to lock dsp core.\n");
		return ret;
	}

	if (!handle->owner) {
		mutex_unlock(&handle->mtx);
		return -EOWNERDEAD; // already detached
	}
	// We have the lock
	return 0;
}

// Do NOT call this without a previous call to task_control_exclusive_access_start()
static void
task_control_exclusive_access_end(struct task_control_handle_t *handle)
{
	mutex_unlock(&handle->mtx);
}

static int task_control_find_handle(uint32_t stream_id, uint64_t run_id,
				    struct task_control_handle_t **thandle)
{
	int ret;
	struct list_head *lpos;
	struct list_head *pos;
	struct list_head *q;
	struct stream_manager_t *owner = NULL;

	if (thandle == NULL) {
		return -EINVAL;
	}

	/* Find parent with matching stream_id */
	ret = mutex_lock_interruptible(&stream_manager_master->mtx);
	if (ret) {
		DSP_CORE_ERROR("Failed to lock dsp_core.\n");
		return ret;
	}

	if (list_empty(&stream_manager_master->managers)) {
		DSP_CORE_ERROR("dsp core list is empty.\n");
		mutex_unlock(&stream_manager_master->mtx);
		return -EFAULT;
	}

	/* Find this client's parent in the parent list*/
	list_for_each (lpos, &stream_manager_master->managers) {
		owner = list_entry(lpos, struct stream_manager_t, node);
		if (owner->stream_id == stream_id) {
			break;
		}
		if (list_is_last(lpos, &stream_manager_master->managers)) {
			mutex_unlock(&stream_manager_master->mtx);
			DSP_CORE_DBG(
				"Task control find: no stream manager matching SID 0x%04x\n",
				stream_id);
			return -EINVAL;
		}
	}

	/* Find the task handle matching run_id */
	if (!list_empty(&owner->tcontrol_list)) {
		list_for_each_safe (pos, q, &owner->tcontrol_list) {
			*thandle = list_entry(pos, struct task_control_handle_t,
					      node);

			if ((*thandle)->run_id == run_id) {
				break;
			}
			if (list_is_last(pos, &owner->tcontrol_list)) {
				*thandle = NULL;
				mutex_unlock(&stream_manager_master->mtx);
				DSP_CORE_DBG(
					"Task control find: no task control matching run_id %llu\n",
					run_id);
				return -EINVAL;
			}
		}
	}

	mutex_unlock(&stream_manager_master->mtx);

	return 0;
}

static int
task_control_task_handle_update_state(struct task_control_handle_t *t_handle,
				      enum dsp_core_task_run_state state,
				      uint32_t exit_code)
{
	int ret = 0;

	if (t_handle == NULL) {
		return -EINVAL;
	}

	mutex_lock(&t_handle->mtx);

	switch (state) {
	case kTaskRunStateStarted:
	case kTaskRunStateStarting:
	case kTaskRunStateRunning:
	case kTaskRunStateSuspending:
	case kTaskRunStateSuspended:
	case kTaskRunStateResuming:
		t_handle->is_started = true;
		t_handle->run_state = state;
		break;
	case kTaskRunStateExiting:
	case kTaskRunStateExited:
		t_handle->is_exited = true;
		t_handle->exit_code = exit_code;
		t_handle->run_state = state;
		break;
	case kTaskRunStateAborted:
	case kTaskRunStateAborting:
		t_handle->is_exited = true;
		t_handle->is_aborted = true;
		t_handle->exit_code = exit_code;
		t_handle->run_state = state;
		break;
	default:
		ret = -EINVAL;
		break;
	};

	mutex_unlock(&t_handle->mtx);

	if (ret == 0) {
		wake_up_interruptible(&t_handle->wait_queue);

		DSP_CORE_DBG(
			"Task control update: run_id = %llu - exit_code = 0x%08x"
			" - locator.core_id = 0x%02x - locator.unique_id = 0x%02x - run_state = %s\n",
			t_handle->run_id, t_handle->exit_code,
			t_handle->locator.core_id, t_handle->locator.unique_id,
			dsp_core_run_state_str[t_handle->run_state]);
	}

	return ret;
}

static int task_control_attach_to_parent(struct task_control_handle_t *handle)
{
	int ret;
	struct list_head *lpos;
	struct stream_manager_t *owner = NULL;

	if (handle->owner) {
		DSP_CORE_ERROR("handle already has a parrent");
		return -EINVAL;
	}

	// Find our parent and add ourself to the list
	ret = mutex_lock_interruptible(&stream_manager_master->mtx);
	if (ret) {
		DSP_CORE_ERROR("Failed to lock dsp core.\n");
		return ret;
	}

	if (list_empty(&stream_manager_master->managers)) {
		DSP_CORE_ERROR("dsp core list is empty.\n");
		mutex_unlock(&stream_manager_master->mtx);
		return -EFAULT;
	}

	/* Find this client's parent in the parent list*/
	list_for_each (lpos, &stream_manager_master->managers) {
		owner = list_entry(lpos, struct stream_manager_t, node);
		if (owner->pid == current->tgid) {
			break;
		}
		if (list_is_last(lpos, &stream_manager_master->managers)) {
			mutex_unlock(&stream_manager_master->mtx);
			return -EINVAL;
		}
	}

	ret = mutex_lock_interruptible(&handle->mtx);
	if (ret) {
		DSP_CORE_ERROR("Failed to lock task_control handle");
		mutex_unlock(&stream_manager_master->mtx);
		return -EFAULT;
	}

	handle->owner = owner;
	list_add(&handle->node, &owner->tcontrol_list);

	mutex_unlock(&handle->mtx);
	mutex_unlock(&stream_manager_master->mtx);

	return 0;
}

int task_control_detach_from_parent(struct task_control_handle_t *handle)
{
	int ret;

	if (!handle->owner) {
		DSP_CORE_DBG("Task control already detached\n");
		return 0; // already detached
	}

	ret = mutex_lock_interruptible(&handle->mtx);
	if (ret) {
		DSP_CORE_ERROR("Failed to lock dsp core.\n");
		return ret;
	}

	if (!handle->owner) {
		mutex_unlock(&handle->mtx);
		return 0; // already detached
	}

	// Send an abort commnd to attached task
	task_control_handle_abort_no_lock(handle);

	// Find us in the owner list and remove
	list_del(&handle->node);
	handle->owner = NULL;

	mutex_unlock(&handle->mtx);

	return 0;
}

static int task_control_handle_find_and_attach(
	struct task_control_handle_t *t_handle,
	struct TaskControlFindAndAttachRequest *request)
{
	int ret;
	uint8_t core_id;

	struct task_manager_control_packet cmd_packet;
	struct task_manager_response_packet rsp_packet;

	ret = task_control_exclusive_access_start(t_handle);
	if (ret) {
		DSP_CORE_ERROR("Could not get exclusive access to t_handle\n");
		return ret;
	}

	if (t_handle->is_attached) {
		task_control_exclusive_access_end(t_handle);
		return -EBUSY;
	}

	initialize_task_control_task_find_command(&cmd_packet,
						  request->unique_id);

	for (core_id = CVCORE_DSP_ID_MIN; core_id <= CVCORE_DSP_ID_MAX;
	     core_id++) {
		if ((request->core_mask >> core_id) & 0x1) {
			ret = submit_command_packet(core_id, &cmd_packet,
						    &rsp_packet);
			if (!ret) {
				struct task_control_task_find_response *find_rsp;

				find_rsp = get_task_control_task_find_response(
					&rsp_packet);

				if (rsp_packet.status ==
				    CVCORE_STATUS_SUCCESS) {
					t_handle->locator.core_id =
						find_rsp->task_locator.core_id;
					t_handle->locator.unique_id =
						find_rsp->task_locator.unique_id;
					t_handle->is_attached = true;

					DSP_CORE_DBG(
						"Found and attached to task with locator %02X.%02X\n",
						find_rsp->task_locator.core_id,
						find_rsp->task_locator
							.unique_id);
					task_control_exclusive_access_end(
						t_handle);
					return 0;
				}
			}
		}
	}

	DSP_CORE_DBG("Could not find the requested unique_id\n");

	task_control_exclusive_access_end(t_handle);
	return -ENODEV;
}

static int task_control_handle_abort(struct task_control_handle_t *t_handle)
{
	int ret;
	struct task_manager_control_packet cmd_packet;
	struct task_manager_response_packet rsp_packet;

	ret = task_control_exclusive_access_start(t_handle);
	if (ret) {
		DSP_CORE_ERROR("Could not get exclusive access %d\n", ret);
		return ret;
	}

	if (!t_handle->is_attached) {
		task_control_exclusive_access_end(t_handle);
		return -EBUSY;
	}

	initialize_task_control_task_abort_command(
		&cmd_packet, t_handle->locator, t_handle->run_id);

	ret = submit_command_packet(t_handle->locator.core_id, &cmd_packet,
				    &rsp_packet);
	if (!ret) {
		struct task_control_task_abort_response *abort_rsp;

		abort_rsp = get_task_control_task_abort_response(&rsp_packet);

		if (rsp_packet.status == CVCORE_STATUS_SUCCESS) {
			DSP_CORE_DBG(
				"Aborted task with locator %02X.%02X - run_state was %d\n",
				t_handle->locator.core_id,
				t_handle->locator.unique_id,
				abort_rsp->task_state_on_abort.run_state);
			task_control_exclusive_access_end(t_handle);
			return 0;
		}
	}

	task_control_exclusive_access_end(t_handle);
	return -ENODEV;
}

static int
task_control_handle_abort_no_lock(struct task_control_handle_t *t_handle)
{
	struct task_manager_control_packet cmd_packet;

	if (!t_handle->is_attached) {
		return -EBUSY;
	}

	initialize_task_control_task_abort_command(
		&cmd_packet, t_handle->locator, t_handle->run_id);

	return submit_command_packet_no_response(t_handle->locator.core_id,
						 &cmd_packet);
}

static int task_control_handle_start(struct task_control_handle_t *t_handle,
				     struct TaskControlStartRequest *request)
{
	int ret;

	struct task_manager_control_packet cmd_packet;
	struct task_manager_response_packet rsp_packet;

	ret = task_control_exclusive_access_start(t_handle);
	if (ret) {
		DSP_CORE_ERROR("Could not get exclusive access to t_handle\n");
		return ret;
	}

	if (!t_handle->is_attached) {
		task_control_exclusive_access_end(t_handle);
		return -ENOTTY;
	}

	if (t_handle->is_started && !t_handle->is_exited) {
		task_control_exclusive_access_end(t_handle);
		return -EBUSY;
	}

	initialize_task_control_task_start_command(
		&cmd_packet, t_handle->locator, request->flags,
		(uint8_t *)&request->arguments, request->trigger_event_name,
		request->timeout_us, t_handle->owner->stream_id,
		request->priority);

	ret = submit_command_packet(t_handle->locator.core_id, &cmd_packet,
				    &rsp_packet);
	if (!ret) {
		struct task_control_task_start_response *start_rsp;

		start_rsp = get_task_control_task_start_response(&rsp_packet);

		if (rsp_packet.status == CVCORE_STATUS_SUCCESS) {
			DSP_CORE_DBG(
				"Started task with locator %02X.%02X - run_id is %llu\n",
				t_handle->locator.core_id,
				t_handle->locator.unique_id,
				start_rsp->task_run_id);

			t_handle->run_id = start_rsp->task_run_id;
			t_handle->is_aborted = false;
			t_handle->is_started = false;
			t_handle->is_exited = false;
			task_control_exclusive_access_end(t_handle);
			return 0;
		} else {
			DSP_CORE_DBG(
				"Could not start task with locator %02X.%02X - status is %d\n",
				t_handle->locator.core_id,
				t_handle->locator.unique_id, rsp_packet.status);
		}
	}

	task_control_exclusive_access_end(t_handle);
	return -ENODEV;
}

static int
task_control_handle_set_task_priority(struct task_control_handle_t *t_handle,
				      struct TaskControlPriorityInfo *request)
{
	return 0;
}

static int
task_control_handle_get_task_priority(struct task_control_handle_t *t_handle,
				      struct TaskControlPriorityInfo *response)
{
	return 0;
}

static int
task_control_handle_get_task_state(struct task_control_handle_t *t_handle,
				   struct TaskControlTaskStateInfo *response)
{
	int ret;

	struct task_manager_control_packet cmd_packet;
	struct task_manager_response_packet rsp_packet;

	ret = task_control_exclusive_access_start(t_handle);
	if (ret) {
		return ret;
	}

	if (!t_handle->is_attached) {
		task_control_exclusive_access_end(t_handle);
		return -ENOTTY;
	}

	initialize_task_control_task_get_status_command(
		&cmd_packet, t_handle->locator, t_handle->run_id);

	ret = submit_command_packet(t_handle->locator.core_id, &cmd_packet,
				    &rsp_packet);
	if (!ret) {
		struct task_control_task_get_status_response *status_rsp;

		status_rsp =
			get_task_control_task_get_status_response(&rsp_packet);
		if (rsp_packet.status == CVCORE_STATUS_SUCCESS) {
			response->run_state = status_rsp->run_state;
			response->exit_code = status_rsp->exit_code;
			task_control_task_handle_update_state(
				t_handle,
				(enum dsp_core_task_run_state)
					status_rsp->run_state,
				status_rsp->exit_code);
			task_control_exclusive_access_end(t_handle);
			return 0;
		}
	}

	task_control_exclusive_access_end(t_handle);
	return ret;
}

int task_control_handle_task_status_message(
	struct task_control_task_status_message *message)
{
	int ret;
	struct task_control_handle_t *task_handle = NULL;

	ret = task_control_find_handle(message->stream_id, message->task_run_id,
				       &task_handle);
	if (ret) {
		return ret;
	}

	return task_control_task_handle_update_state(
		task_handle, (enum dsp_core_task_run_state)message->run_state,
		message->exit_code);
}

static int
task_control_handle_check_state(struct task_control_handle_t *t_handle,
				struct TaskControlTaskWaitStateInfo *request)
{
	int ret = 0;

	mutex_lock(&t_handle->mtx);
	if (t_handle->is_aborted) {
		request->current_run_state = kTaskRunStateAborted;
		request->current_exit_code = t_handle->exit_code;
		ret = 1;
	} else if (t_handle->is_exited) {
		request->current_run_state = kTaskRunStateExited;
		request->current_exit_code = t_handle->exit_code;
		ret = 1;
	} else {
		switch (request->wait_state) {
		case kTaskRunStateStarted:
		case kTaskRunStateStarting:
		case kTaskRunStateRunning:
		case kTaskRunStateSuspending:
		case kTaskRunStateSuspended:
		case kTaskRunStateResuming:
			if (t_handle->is_started) {
				request->current_run_state =
					kTaskRunStateStarted;
				request->current_exit_code =
					t_handle->exit_code;
				ret = 1;
			}
			break;
		default:
			if (t_handle->run_state == request->wait_state) {
				request->current_run_state =
					t_handle->run_state;
				request->current_exit_code =
					t_handle->exit_code;
				ret = 1;
			}
			break;
		};
	}
	mutex_unlock(&t_handle->mtx);

	return ret;
}

/**
 * Handle userspace request to wait for a specific
 * task state.
 */
static int task_control_handle_wait_task_state(
	struct task_control_handle_t *t_handle,
	struct TaskControlTaskWaitStateInfo *request)
{
	ulong to;
	int ret = 0;

	to = usecs_to_jiffies(5000000);

	ret = wait_event_interruptible_timeout(
		t_handle->wait_queue,
		(task_control_handle_check_state(t_handle, request) == 1), to);

	if (ret == 0) {
		/* timeout occurred */
		return -EAGAIN;
	} else if (ret == -ERESTARTSYS) {
		/* signal received */
		return -ERESTARTSYS;
	} else if (ret < 0) {
		DSP_CORE_ERROR("wait_event_interruptible_timeout()"
			       " error - (ret=%i)\n",
			       ret);
		return ret;
	}

	return 0;
}

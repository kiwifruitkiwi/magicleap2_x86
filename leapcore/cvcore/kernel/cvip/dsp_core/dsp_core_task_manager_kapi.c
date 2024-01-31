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
#include "dsp_core_common.h"
#include "dsp_core_types.h"
#include "dsp_core_uapi.h"
#include "mero_xpc.h"

#define TASK_MANAGER_DEVICE_NAME "dsp_core_tmanager"

static long task_manager_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long data);

static struct file_operations f_ops = { .owner = THIS_MODULE,
					.unlocked_ioctl = task_manager_ioctl,
					.compat_ioctl = task_manager_ioctl };

static struct task_manager_t tmanager;

int task_manager_create_fs_device(void)
{
	int ret = 0;
	int dev_major;
	dev_t first_dev;

	/* Allocate character device region */
	ret = alloc_chrdev_region(&first_dev, 0, 1, TASK_MANAGER_DEVICE_NAME);
	if (ret < 0) {
		DSP_CORE_ERROR("dsp_core_debug alloc_chrdev_region error: %d\n",
			       ret);
		goto error;
	}
	dev_major = MAJOR(first_dev);

	/* Initialize char device */
	cdev_init(&tmanager.cdev, &f_ops);
	tmanager.cdev.owner = THIS_MODULE;
	tmanager.dev = MKDEV(dev_major, 0);

	ret = cdev_add(&tmanager.cdev, tmanager.dev, 1);
	if (ret < 0) {
		DSP_CORE_ERROR("dsp_core_debug cdev_add error: %d\n", ret);
		goto post_chrdev_error;
	}

	/* Create device node */
	tmanager.char_device =
		device_create(stream_manager_master->char_class, NULL,
			      tmanager.dev, NULL, TASK_MANAGER_DEVICE_NAME);
	if (IS_ERR(&tmanager.char_device)) {
		DSP_CORE_ERROR("Failed to create tmanager device\n");
		goto post_cdev_add_error;
	}

	/* Success */
	return 0;

post_cdev_add_error:
	cdev_del(&tmanager.cdev);
post_chrdev_error:
	unregister_chrdev_region(first_dev, 1);
error:
	return ret;
}

int task_manager_destroy_fs_device(void)
{
	cdev_del(&tmanager.cdev);
	device_destroy(stream_manager_master->char_class, tmanager.dev);
	unregister_chrdev_region(tmanager.dev, 1);
	return 0;
}

static int
task_manager_handle_get_info_request(struct DspCoreGetInfoRequest *request)
{
	int ret = 0;

	if (!is_valid_core(request->core_id)) {
		return -ENODEV;
	}

	ret = query_task_manager(request->core_id, &request->info);
	if (ret) {
		DSP_CORE_ERROR("Query task managers failed\n");
		return -ENODEV;
	}
	return 0;
}

static int
task_manager_handle_create_task(struct TaskManagerCreateTaskRequest *request)
{
	int ret;
	struct task_manager_control_packet cmd_packet;
	struct task_manager_response_packet rsp_packet;
	struct task_manager_task_create_response *create_rsp;

	if (request->task_id < kBuiltInTaskID_LAST) {
		uint64_t ptrs_to_tasks[kBuiltInTaskID_LAST];
		uint64_t ptrs_to_delegates[kBuiltInDelegateID_LAST];

		ret = discover_built_in_symbols(request->core_id, ptrs_to_tasks,
						ptrs_to_delegates);
		if (ret) {
			DSP_CORE_ERROR("Failed to discover built in symbols\n");
			return ret;
		}

		request->ptr_to_creator = ptrs_to_tasks[request->task_id];
	}

	if (!request->ptr_to_creator) {
		return -EINVAL;
	}

	initialize_task_manager_task_create_command(
		&cmd_packet, request->ptr_to_creator, request->unique_id,
		request->stack_size, request->params, request->priority);

	ret = submit_command_packet(request->core_id, &cmd_packet, &rsp_packet);
	if (ret) {
		return ret;
	}

	create_rsp = get_task_manager_task_create_response(&rsp_packet);

	if (rsp_packet.status == CVCORE_STATUS_SUCCESS) {
		DSP_CORE_DBG("Created task with locator %02X.%02X\n",
			     create_rsp->task_locator.core_id,
			     create_rsp->task_locator.unique_id);
		return 0;
	} else {
		DSP_CORE_ERROR("Create task response failure - %d\n",
			       rsp_packet.status);
	}

	return -ENODEV;
}

static long task_manager_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long data)
{
	int res = 0;
	if ((void *)data == NULL)
		return -EINVAL;

	switch (cmd) {
	case DSP_CORE_TASK_MANAGER_GET_INFO: {
		struct DspCoreGetInfoRequest req;

		if (copy_from_user((void *)&req, (void *)data,
				   sizeof(struct DspCoreGetInfoRequest))) {
			return -EFAULT;
		}

		res = task_manager_handle_get_info_request(&req);
		if (!res) {
			if (copy_to_user((void *)data, (void *)&req,
					 sizeof(struct DspCoreGetInfoRequest))) {
				return -EFAULT;
			}
		}
	} break;
	case DSP_CORE_TASK_MANAGER_CREATE_TASK: {
		struct TaskManagerCreateTaskRequest req;

		if (copy_from_user(
			    (void *)&req, (void *)data,
			    sizeof(struct TaskManagerCreateTaskRequest))) {
			return -EFAULT;
		}
		res = task_manager_handle_create_task(&req);
	} break;
	default:
		res = -ENOTTY;
	}
	return res;
}

// SPDX-License-Identifier: GPL-2.0-only
/*
 * dsp request handler for fclk switching
 *
 * Copyright (C) 2023 Magic Leap, Inc. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/dsp_manager_kapi.h>
#include "cvcore_status.h"
#include "cvcore_xchannel_map.h"
#include "cvcore_processor_common.h"
#include "mero_xpc_types.h"
#include "mero_xpc.h"
#include "mero_xpc_kapi.h"

#define MAX_XPC_MSG_LEN_BYTES (27)

#define FCLK_ENABLE_SWITCHING_REQ  1
#define FCLK_DISABLE_SWITCHING_REQ 2
#define FCLK_SYNC_STATE_REQ        3

#define FAULT_INJECTION_ENABLE_FCLK  1
#define FAULT_INJECTION_DISABLE_FCLK 2

extern int fclkctl_enable_switching(void);
extern int fclkctl_disable_switching(void);

DEFINE_MUTEX(fclk_req_lock);
static uint8_t core_id_vote = 0;

struct fclk_req {
	uint32_t id;
	uint8_t type;
	uint8_t core_id;
	uint8_t err_notify;
	uint8_t data;
};

struct fclk_work {
	struct work_struct work;
	int (*fn)(struct fclk_work *);
	uint32_t req_id;
	uint8_t req_type;
	uint8_t core_id;
	uint8_t err_notify;
	uint8_t data;
};

static struct kobject *cvip_kobj;
static uint32_t FAULT_INJECTION_TYPE;
static uint32_t enable_req_cnt;
static uint32_t disable_req_cnt;

static ssize_t fault_injection_type_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	int ret;

	mutex_lock(&fclk_req_lock);
	ret = sprintf(ptr, "%u\n", FAULT_INJECTION_TYPE);
	mutex_unlock(&fclk_req_lock);

	return ret;
}

static ssize_t fault_injection_type_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	uint32_t fault;

	if (sscanf(ptr, "%u\n", &fault) != 1)
		return -EINVAL;

	mutex_lock(&fclk_req_lock);
	FAULT_INJECTION_TYPE = fault;
	mutex_unlock(&fclk_req_lock);

	return len;
}

static ssize_t enable_req_cnt_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	int ret;

	mutex_lock(&fclk_req_lock);
	ret = sprintf(ptr, "%u\n", enable_req_cnt);
	if (ret > 0)
		enable_req_cnt = 0;
	mutex_unlock(&fclk_req_lock);

	return ret;
}

static ssize_t disable_req_cnt_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	int ret;

	mutex_lock(&fclk_req_lock);
	ret = sprintf(ptr, "%u\n", disable_req_cnt);
	if (ret > 0)
		disable_req_cnt = 0;
	mutex_unlock(&fclk_req_lock);

	return ret;
}

static const DEVICE_ATTR_RW(fault_injection_type);
static const DEVICE_ATTR_RO(enable_req_cnt);
static const DEVICE_ATTR_RO(disable_req_cnt);

const struct attribute *fclk_attrs[] = {
	&dev_attr_fault_injection_type.attr,
	&dev_attr_enable_req_cnt.attr,
	&dev_attr_disable_req_cnt.attr,
	NULL
};

static const struct attribute_group fclk_group = {
	.name = "fclk",
	.attrs = (struct attribute **)fclk_attrs,
};

/* called by dsp manager on dsp core shutdown */
static int cvip_fclk_on_dsp_core_shutdown(struct notifier_block *nb, unsigned long dsp_id, void *data)
{
	int status = 0;
	struct fclk_req req;

	uint16_t core_id = (uint16_t)CVCORE_DSP_RENUMBER_Q6C5_TO_C5Q6(dsp_id);
	XpcTargetMask destination = (1 << CVCORE_ARM);

	req.type = FCLK_ENABLE_SWITCHING_REQ;
	req.core_id = (uint8_t)core_id;
	req.err_notify = 0;

	mutex_lock(&fclk_req_lock);
	if ((core_id_vote & (1 << core_id)))
		status = xpc_notification_send(CVCORE_XCHANNEL_FCLK, destination, (uint8_t *)&req,
							  sizeof(req), XPC_NOTIFICATION_MODE_POSTED);
	mutex_unlock(&fclk_req_lock);

	if (status)
		pr_err("fclk xpc_notification_send on dsp core shutdown failed, status=%d\n", status);

	return status;
}

static struct notifier_block dsp_core_shutdown_nb = {
	.notifier_call = cvip_fclk_on_dsp_core_shutdown
};

static void fclk_request_handler(struct work_struct *work)
{
	XpcTargetMask destination;
	struct fclk_work *fclk_work;
	struct fclk_req req;
	int status;

	fclk_work = container_of(work, struct fclk_work, work);

	if (!fclk_work->fn(fclk_work)) {
		kfree(fclk_work);
		return;
	}

	pr_err("fclk req(%u) failed for core id %u\n", fclk_work->req_type, fclk_work->core_id);
	if (fclk_work->err_notify) {
		req.core_id = fclk_work->core_id;
		req.data = fclk_work->data;
		req.type = fclk_work->req_type;
		req.id = fclk_work->req_id;

		destination = (1 << req.core_id);

		status = xpc_notification_send(CVCORE_XCHANNEL_FCLK, destination, (uint8_t *)&req,
									   sizeof(req), XPC_NOTIFICATION_MODE_POSTED);

		if (status)
			pr_err("fclk xpc_notification_send failed, status=%d\n", status);
	}
	kfree(fclk_work);
}

int fclk_enable_switching_fn(struct fclk_work *fclk_work)
{
	int ret = 0;
	uint8_t core_id = fclk_work->core_id;

	mutex_lock(&fclk_req_lock);
	if (FAULT_INJECTION_TYPE == FAULT_INJECTION_ENABLE_FCLK) {
		enable_req_cnt++;
		mutex_unlock(&fclk_req_lock);
		return -1;
	}

	if ((core_id_vote & (1 << core_id))) {
		ret = fclkctl_enable_switching();
		if (!ret)
			core_id_vote &= ~(1 << core_id);
	}
	enable_req_cnt++;
	mutex_unlock(&fclk_req_lock);

	return ret;
}

int fclk_disable_switching_fn(struct fclk_work *fclk_work)
{
	int ret = 0;
	uint8_t core_id = fclk_work->core_id;

	mutex_lock(&fclk_req_lock);
	if (FAULT_INJECTION_TYPE == FAULT_INJECTION_DISABLE_FCLK) {
		disable_req_cnt++;
		mutex_unlock(&fclk_req_lock);
		return -1;
	}

	if (!(core_id_vote & (1 << core_id))) {
		ret = fclkctl_disable_switching();
		if (!ret)
			core_id_vote |= (1 << core_id);
	}
	disable_req_cnt++;
	mutex_unlock(&fclk_req_lock);

	return ret;
}

int fclk_sync_state_fn(struct fclk_work *fclk_work)
{
	int ret;
	XpcTargetMask destination;
	struct fclk_req req;
	uint8_t core_id = fclk_work->core_id;

	mutex_lock(&fclk_req_lock);
	/* just read the vote state and send it back to dsp core */
	fclk_work->data = (core_id_vote >> core_id) & 1;
	mutex_unlock(&fclk_req_lock);

	req.core_id = fclk_work->core_id;
	req.type = fclk_work->req_type;
	req.data = fclk_work->data;

	destination = (1 << req.core_id);

	ret = xpc_notification_send(CVCORE_XCHANNEL_FCLK, destination, (uint8_t *)&req,
								   sizeof(req), XPC_NOTIFICATION_MODE_POSTED);

	if (ret)
		pr_err("fclk failed to send sync state, status=%d\n", ret);

	return ret;
}

static int fclk_notification_handler(XpcChannel channel, XpcHandlerArgs args,
		uint8_t *notification_buffer, size_t notification_buffer_length)
{
	struct fclk_req *req;
	struct fclk_work *work;
	int (*work_fn)(struct fclk_work *);
	int ret = 0;

	if (channel != CVCORE_XCHANNEL_FCLK) {
		pr_err("not the xchannel (got=%d, interested=%d) fclk is interested in.\n",
			channel, CVCORE_XCHANNEL_FCLK);
		return -1;
	}

	req = (struct fclk_req *)notification_buffer;

	switch(req->type) {
	case FCLK_ENABLE_SWITCHING_REQ:
		work_fn = fclk_enable_switching_fn;
		break;

	case FCLK_DISABLE_SWITCHING_REQ:
		work_fn = fclk_disable_switching_fn;
		break;

	case FCLK_SYNC_STATE_REQ:
		work_fn = fclk_sync_state_fn;
		break;

	default:
		pr_err("invalid fclk request type\n");
		return -1;
	}

	work = kmalloc(sizeof(struct fclk_work), GFP_ATOMIC);

	if (!work) {
		pr_err("unable to handle fclk request (type=%u)\n", req->type);
		return -1;
	}

	work->req_type = req->type;
	work->core_id = req->core_id;
	work->err_notify = req->err_notify;
	work->data = req->data;
	work->req_id = req->id;
	work->fn = work_fn;

	INIT_WORK(&work->work, fclk_request_handler);

	ret = schedule_work(&work->work);

	return ret;
}

int cvip_fclk_init(struct kobject *kobj)
{
	int ret = 0;
	ret = xpc_register_notification_handler(CVCORE_XCHANNEL_FCLK,
				fclk_notification_handler, NULL);
	if (ret != 0) {
		pr_err("failed to register xpc fclk notification handler,ret=%d\n", ret);
		return ret;
	}

	dsp_mode_register_notify(&dsp_core_shutdown_nb, DSP_OFF_MASK);

	cvip_kobj = kobj;
	if (cvip_kobj)
		ret = sysfs_create_group(cvip_kobj, &fclk_group);
	else
		ret = -1;

	if (ret < 0)
		pr_err("fclk sysfs group create failed\n");

	return ret;
}

void cvip_fclk_exit(void)
{
	int ret;
	ret = xpc_register_notification_handler(CVCORE_XCHANNEL_FCLK, NULL, NULL);
	if (ret != 0)
		pr_err("failed to deregister xpc fclk notification handler,ret=%d\n", ret);

	if (cvip_kobj)
		sysfs_remove_group(cvip_kobj, &fclk_group);

	dsp_mode_deregister_notify(&dsp_core_shutdown_nb);
}

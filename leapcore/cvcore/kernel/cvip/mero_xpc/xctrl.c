/*
 * ---------------------------------------------------------------------
 * %COPYRIGHT_BEGIN%
 *
 * \copyright
 * Copyright (c) 2021 Magic Leap, Inc. (COMPANY) All Rights Reserved.
 * Magic Leap, Inc. Confidential and Proprietary
 *
 *  NOTICE:  All information contained herein is, and remains the property
 *  of COMPANY. The intellectual and technical concepts contained herein
 *  are proprietary to COMPANY and may be covered by U.S. and Foreign
 *  Patents, patents in process, and are protected by trade secret or
 *  copyright law.  Dissemination of this information or reproduction of
 *  this material is strictly forbidden unless prior written permission is
 *  obtained from COMPANY.  Access to the source code contained herein is
 *  hereby forbidden to anyone except current COMPANY employees, managers
 *  or contractors who have executed Confidentiality and Non-disclosure
 *  agreements explicitly covering such access.
 *
 *  The copyright notice above does not evidence any actual or intended
 *  publication or disclosure  of  this source code, which includes
 *  information that is confidential and/or proprietary, and is a trade
 *  secret, of  COMPANY.   ANY REPRODUCTION, MODIFICATION, DISTRIBUTION,
 *  PUBLIC  PERFORMANCE, OR PUBLIC DISPLAY OF OR THROUGH USE  OF THIS
 *  SOURCE CODE  WITHOUT THE EXPRESS WRITTEN CONSENT OF COMPANY IS
 *  STRICTLY PROHIBITED, AND IN VIOLATION OF APPLICABLE LAWS AND
 *  INTERNATIONAL TREATIES.  THE RECEIPT OR POSSESSION OF  THIS SOURCE
 *  CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY RIGHTS
 *  TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE,
 *  USE, OR SELL ANYTHING THAT IT  MAY DESCRIBE, IN WHOLE OR IN PART.
 *
 * %COPYRIGHT_END%
 * --------------------------------------------------------------------
 */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/poll.h>

#include "xctrl.h"
#include "cvcore_xchannel_map.h"
#include "mero_xpc_common.h"
#include "mero_xpc_notification.h"
#include "mero_xpc_types.h"

#define XCTRL_DEV_COUNT (XPC_NUM_VCHANNELS)
#define XCTRL_DEV_NAME "xctrl"
#define XCTRL_BUFF_SIZE 64
#define str(x) #x
#define xstr(x) str(x)

// xctrl device list struct
struct xctrl_dev_list {
	bool enable;
	bool initialized;
	char dev_name[XCTRL_BUFF_SIZE];
	dev_t dev;
	struct cdev cdev;
	struct device *device;
};

// Entries for each notification channel
static struct xctrl_dev_list notif_dev_list[XCTRL_DEV_COUNT] = {
	{ .enable = true, .dev_name = "platform" }, // 0
	{ .enable = false },
	{ .enable = false },
	{ .enable = false },
	{ .enable = false },
	{ .enable = false },
	{ .enable = true, .dev_name = "dspcore_queue" }, // 6
	{ .enable = false },
	{ .enable = true, .dev_name = "pm" }, // 8
	{ .enable = false },
	{ .enable = false },
	{ .enable = false },
	{ .enable = false },
	{ .enable = false },
	{ .enable = true, .dev_name = "test0" }, // 14
	{ .enable = true, .dev_name = "test1" }, // 15
};

// First assigned device number
static dev_t xctrl_first_dev;

static int xctrl_fops_open(struct inode *inode, struct file *filp)
{
	xpc_debug(1, "%s\n", __func__);
	return 0;
}
static int xctrl_fops_release(struct inode *inode, struct file *filp)
{
	xpc_debug(1, "%s\n", __func__);
	return 0;
}

static long xctrl_fops_ioctl(struct file *file, unsigned int cmd,
			     unsigned long data)
{
	xpc_debug(1, "%s\n", __func__);
	return 0;
}

static ssize_t xctrl_fops_read(struct file *filp, char __user *buf,
			       size_t count, loff_t *ppos)
{
	int ret;
	int status;
	XpcChannel channel;
	loff_t pos;
	size_t rsp_length;
	uint8_t rsp[XPC_MAX_NOTIFICATION_LENGTH + 1] = { 0 };

	channel = iminor(file_inode(filp));
	xpc_debug(1, "%s: channel %d\n", __func__, channel);

	pos = *ppos;
	if (pos > 0) {
		// pos can be greater than 0 if read is called more than once, like by cat.
		// return 0 to signal end of file.
		return 0;
	}

	// Wait for notification
	status = xpc_notification_recv(channel, rsp,
				       XPC_MAX_NOTIFICATION_LENGTH);
	if (status != 0) {
		xpc_debug(0, "%s: xpc_notification_recv error:%d channel:%d\n",
			  __func__, status, channel);
		return -EFAULT;
	}

	// TODO(sadadi): We need to come up with a better solution for getting the size
	// if this interface will be used for more than strings.
	rsp_length = strlen(rsp);
	if (count > rsp_length) {
		count = rsp_length;
	}

	xpc_debug(2, "%s: read %s len:%zu\n", __func__, rsp, count);

	ret = copy_to_user(buf, rsp, count);
	if (ret) {
		xpc_debug(0, "%s: copy_to_user ret:%d channel:%d\n", __func__,
			  ret, channel);
		return -EFAULT;
	}

	// Update position
	*ppos = pos + count;

	return count;
}

static ssize_t xctrl_fops_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *pos)
{
	int ret;
	int status;
	XpcChannel channel;
	char cmd;
	XpcTargetMask target_mask;
	size_t msg_length;
	char str_buf[XCTRL_BUFF_SIZE + 1];
	uint8_t xpc_msg[XCTRL_BUFF_SIZE + 1] = { 0 };

	channel = iminor(file_inode(filp));
	xpc_debug(1, "%s: channel %d\n", __func__, channel);

	// Copy up to buffer size
	if (count > XCTRL_BUFF_SIZE) {
		count = XCTRL_BUFF_SIZE;
	}

	ret = strncpy_from_user(str_buf, buf, count);
	if (ret < 0) {
		return -EFAULT;
	}

	// Manually null terminate.
	// This is needed because sometimes the user buffer isn't
	// null terminated, so we have to terminate it ourselves to be safe.
	str_buf[ret] = '\0';

	// x:target_mask[:data]
	// Example: x:0x200:12345
	// note: The xstr(XCTRL_BUFF_SIZE) is used to limit what's copied to the buffer,
	// it will resolve to "%c:%i:%64s", where 64 is the value from XCTRL_BUFF_SIZE.
	ret = sscanf(str_buf, "%c:%i:%" xstr(XCTRL_BUFF_SIZE) "s", &cmd,
		     &target_mask, xpc_msg);

	// 2 or 3 to allow sending a notification with no data
	if (ret != 2 && ret != 3) {
		xpc_debug(
			0,
			"%s: invalid input:%s. Expected format x:target_mask[:data]\n",
			__func__, str_buf);
		return -EINVAL;
	}

	// Check for xctrl char x
	if (cmd != 'x') {
		xpc_debug(
			0,
			"%s: invalid input:%s. Expected format x:target_mask[:data]\n",
			__func__, str_buf);
		return -EINVAL;
	}

	// Only allow ARM and x86 targets for now
	if (((BIT(CORE_ID_A55) & target_mask) == 0) &&
	    ((BIT(CORE_ID_X86) & target_mask) == 0)) {
		xpc_debug(
			0,
			"%s: invalid target mask:0x%x. Only 0x%lx(ARM) or 0x%lx(x86) allowed\n",
			__func__, target_mask, BIT(CORE_ID_A55),
			BIT(CORE_ID_X86));
		return -EINVAL;
	}

	// Size sanity check
	msg_length = strlen(xpc_msg);
	if (msg_length > XPC_MAX_NOTIFICATION_LENGTH) {
		xpc_debug(
			0,
			"%s: invalid message length:%zu. Max allowed:%u Message:%s\n",
			__func__, msg_length, XPC_MAX_NOTIFICATION_LENGTH,
			xpc_msg);
		return -EINVAL;
	}

	xpc_debug(2, "%s: sending %s len: %zu\n", __func__, xpc_msg,
		  msg_length);

	// Send notification
	status = xpc_notification_send(channel, target_mask, xpc_msg,
				       XPC_MAX_NOTIFICATION_LENGTH,
				       XPC_NOTIFICATION_MODE_POSTED);
	if (status != 0) {
		xpc_debug(0, "%s: xpc_notification_send error:%d channel:%d\n",
			  __func__, status, channel);
		return -EFAULT;
	}

	return count;
}

static unsigned int xctrl_fops_poll(struct file *filp,
				    struct poll_table_struct *wait)
{
	XpcChannel channel;
	unsigned int mask = 0;

	channel = iminor(file_inode(filp));
	xpc_debug(1, "%s: channel %d\n", __func__, channel);

	// The name is misleading... This doesn't actually wait, but will add a
	// wait_queue to the poll_table_struct and return immediately.
	// The waiting behavior is dependent on the mask returned from this function.
	poll_wait(filp, &notification_mailbox[channel].wait_queue, wait);

	if (xpc_check_mailbox_count(&notification_mailbox[channel]) != 0) {
		// Data is available, so device is readable
		mask |= POLLIN | POLLRDNORM | POLLPRI;
	}

	// Device is writable
	mask |= POLLOUT | POLLWRNORM;

	return mask;
}

// xctrl file operations
static const struct file_operations xctrl_fops = {
	.owner = THIS_MODULE,
	.open = xctrl_fops_open,
	.release = xctrl_fops_release,
	.unlocked_ioctl = xctrl_fops_ioctl,
	.read = xctrl_fops_read,
	.write = xctrl_fops_write,
	.poll = xctrl_fops_poll,
};

int xctrl_create_fs_device(struct class *xpc_class)
{
	int ret, i;
	int dev_major;

	// Allocate character device region
	ret = alloc_chrdev_region(&xctrl_first_dev, 0, XCTRL_DEV_COUNT,
				  XCTRL_DEV_NAME);
	if (ret < 0) {
		xpc_debug(0, "xctrl alloc_chrdev_region error: %d\n", ret);
		return ret;
	}

	dev_major = MAJOR(xctrl_first_dev);

	// Create devices
	for (i = 0; i < XCTRL_DEV_COUNT; i++) {
		if (notif_dev_list[i].enable) {
			char name_buff[XCTRL_BUFF_SIZE];

			// Init char device
			cdev_init(&notif_dev_list[i].cdev, &xctrl_fops);
			notif_dev_list[i].cdev.owner = THIS_MODULE;
			notif_dev_list[i].dev = MKDEV(dev_major, i);

			ret = cdev_add(&notif_dev_list[i].cdev,
				       notif_dev_list[i].dev, 1);
			if (ret < 0) {
				xpc_debug(
					0,
					"xctrl cdev_add error: %d minor num:%d\n",
					ret, i);
				return ret;
			}

			if (notif_dev_list[i].dev_name[0] != 0) {
				// Custom name
				snprintf(name_buff, XCTRL_BUFF_SIZE,
					 "%s_notif_%s", XCTRL_DEV_NAME,
					 notif_dev_list[i].dev_name);
			} else {
				// Default name
				snprintf(name_buff, XCTRL_BUFF_SIZE,
					 "%s_notif_%d", XCTRL_DEV_NAME, i);
			}

			// Create device
			notif_dev_list[i].device =
				device_create(xpc_class, NULL,
					      notif_dev_list[i].dev, NULL, "%s",
					      name_buff);
			if (IS_ERR(&notif_dev_list[i].device)) {
				xpc_debug(0,
					  "xctrl device_create minor num:%d\n",
					  i);
				return -1;
			}

			// Set to initialized
			notif_dev_list[i].initialized = true;
		}
	}

	return 0;
}

int xctrl_destory_fs_device(struct class *xpc_class)
{
	int i;

	for (i = 0; i < XCTRL_DEV_COUNT; i++) {
		if (notif_dev_list[i].initialized) {
			cdev_del(&notif_dev_list[i].cdev);
			device_destroy(xpc_class, notif_dev_list[i].dev);
			notif_dev_list[i].initialized = false;
		}
	}

	unregister_chrdev_region(xctrl_first_dev, XCTRL_DEV_COUNT);

	return 0;
}

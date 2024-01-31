
/*
 * ---------------------------------------------------------------------
 * %COPYRIGHT_BEGIN%
 *
 * \copyright
 * Copyright (c) 2020-2023 Magic Leap, Inc. (COMPANY) All Rights Reserved.
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

// clang-format off
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/circ_buf.h>
#include <linux/completion.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h> /* register_chrdev, unregister_chrdev */
#include <linux/fs_struct.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irqreturn.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeirq.h>
#include <linux/pm_wakeup.h>
#include <linux/seq_file.h> /* seq_read, seq_lseek, single_release */
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "cvcore_processor_common.h"
#include "cvcore_xchannel_map.h"
#include "mero_xpc_command.h"
#include "mero_xpc_common.h"
#include "mero_xpc_configuration.h"
#include "mero_xpc_kapi.h"
#include "mero_xpc_notification.h"
#include "mero_xpc_queue.h"
#include "mero_xpc_test.h"
#include "mero_xpc_types.h"
#include "mero_xpc_uapi.h"
#include "xctrl.h"
#include "xstat.h"
#include "xstat_internal.h"
#include "xdebug_internal.h"

#include "pl320_hal.h"
#include "pl320_types.h"
// clang-format on

int xpc_debug_flag = 0;

module_param(xpc_debug_flag, uint, 0644);
MODULE_PARM_DESC(xpc_debug_flag, "Activates xpc debug info");

static atomic_t notification_read_calls;
static atomic_t notification_wake_ups;
static atomic_t notification_claimed;
static atomic_t notification_lost_claims;
static atomic_t queue_read_calls;
static atomic_t queue_wake_ups;
static atomic_t queue_lost_claims;
static atomic_t queue_discards;
static atomic_t queue_attempt_wake_up;

/* PCIe functions that are only applicable
 * on x86 not CVIP
 */
#if defined(__amd64__) || defined(__i386__)

#define PCI_DEVICE_ID_CVIP (0x149d)
#define CVIP_MAILBOX_OFFSET (0x00400000)

/**
  * struct cvip_dev - per CVIP PCI device context
  * @pci_dev: PCI driver node
  * @ipc_dev: IPC mailbox device
  * @completion: used for sync between cvip and x86
  * @power_completion: used for sync power messaging between cvip and x86
  * @is_state_d3: used to check if CVIP entered D3
  * @domain: iommu domain
  */
struct cvip_dev {
	struct pci_dev *pci_dev;
	struct completion completion;
	struct completion power_completion;
	u32 is_state_d3;
	struct iommu_domain *domain;
};
static struct cvip_dev *cvip_p;
#endif

static ssize_t xpc_drv_write(struct file *filp, const char __user *buf,
			     size_t count, loff_t *pos);

static long xpc_drv_unlocked_ioctl(struct file *, unsigned int cmd,
				   unsigned long data);

static int xpc_drv_release(struct inode *inode, struct file *filp);
static int xpc_drv_open(struct inode *inode, struct file *filp);
static int xpc_drv_probe(struct platform_device *pdev);
static int xpc_drv_remove(struct platform_device *pdev);
static void xpc_drv_release_by_pid(pid_t pid);
static int __init xpc_drv_init(void);
static void __exit xpc_drv_exit(void);
static irqreturn_t xpc_irq_handler(int irq, void *dev);
static void xpc_irq_tasklet(unsigned long data);
static uint8_t
xpc_populate_global_mailbox(struct xpc_mailbox_info *mailbox_info,
			    XpcMessageHeader *header,
			    Pl320MailboxIRQReason *irq_cause);

static int xpc_handle_command_send_command(unsigned long data,
					   struct xpc_client_t *client);
static int xpc_handle_command_wait_response(unsigned long data,
					    struct xpc_client_t *client);
static int xpc_handle_command_send_and_wait(unsigned long data,
					    struct xpc_client_t *client);
static int xpc_handle_command_send_response(unsigned long data,
					    struct xpc_client_t *client);
static int xpc_handle_command_wait_receive(unsigned long data,
					   struct xpc_client_t *client);
static int xpc_handle_notification_send(unsigned long data,
					struct xpc_client_t *client);
static int xpc_handle_notification_wait_receive(unsigned long data,
						struct xpc_client_t *client);
static int xpc_handle_queue_send(unsigned long data,
				 struct xpc_client_t *client);
static int xpc_handle_queue_wait_receive(unsigned long data,
					 struct xpc_client_t *client);
static int xpc_handle_queue_register(unsigned long data,
				     struct xpc_client_t *client);
static int xpc_handle_queue_deregister(unsigned long data,
				       struct xpc_client_t *client);
static int xpc_handle_query_mailbox_usage(unsigned long data);
static void xpc_do_kernel_work(XpcChannel channel, XpcMode xpc_mode,
			       int global_mailbox_number);

static atomic_t xpc_drv_inited;
static dev_t dev = 0;
static int major_number;
static struct device *device;
static struct cdev cdev;
static int16_t irq_table[512];
static int ipc_initialized[XPC_NUM_IPC_DEVICES];
static int ipc_count = 0;

#if XPC_CORE_ID == CORE_ID_A55
// Only ARM should be able to set cvip status
static CLASS_ATTR_RW(cvip_state);
static CLASS_ATTR_RO(x86_state);
static CLASS_ATTR_RW(wearable_state);
#elif XPC_CORE_ID == CORE_ID_X86
// Only x86 should be able to set x86 status
static CLASS_ATTR_RO(cvip_state);
static CLASS_ATTR_RW(x86_state);
static CLASS_ATTR_RO(wearable_state);
#else
#error Invalid Core ID
#endif

static struct attribute *xpc_class_attrs[] = {
	&class_attr_cvip_state.attr,
	&class_attr_x86_state.attr,
	&class_attr_wearable_state.attr,
	NULL,
};
ATTRIBUTE_GROUPS(xpc_class);

static struct class xpc_class = {
	.name = DEVICE_NAME,
	.owner = THIS_MODULE,
	.class_groups = xpc_class_groups,
};

static const struct of_device_id xpc_match[] = { {
	.compatible = "mero-xpc",
} };

static struct platform_driver xpc_pldriver = {
	.probe = xpc_drv_probe,
	.remove = xpc_drv_remove,
	.driver =
		{
			.name = "mero-xpc",
			.of_match_table = of_match_ptr(xpc_match),
		},
};

static const struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = xpc_drv_unlocked_ioctl,
	.compat_ioctl = xpc_drv_unlocked_ioctl,
	.open = xpc_drv_open,
	.release = xpc_drv_release,
#if !defined(__i386__) && !defined(__amd64__)
	.write = xpc_drv_write
#endif
};

static int inline xpc_channel_mask_to_idx(uint32_t mask)
{
	int i;

	if (mask == 0)
		return -1;

	for (i = 0; i < 32; i++) {
		if ((mask >> i) & 0x1) {
			return CVCORE_DSP_RENUMBER_Q6C5_TO_C5Q6(i);
		}
	}
	return -1;
}

bool xpc_initialized(void)
{
	return (atomic_read(&xpc_drv_inited) == 1);
}

void create_fs_device(void)
{
	int err = 0;
	err = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (err < 0) {
		xpc_debug(2, "[target] alloc_chrdev_region() failed\n");
	}

	major_number = MAJOR(dev);
	err = class_register(&xpc_class);
	if (err) {
		xpc_debug(0, "Error registering xpc_class: %d\n", err);
		unregister_chrdev_region(dev, 1);
		return;
	}

	cdev_init(&cdev, &f_ops);
	cdev.owner = THIS_MODULE;
	dev = MKDEV(major_number, 0);
	err = cdev_add(&cdev, dev, 1);
	if (err < 0) {
		xpc_debug(2, "error adding minor number\n");
		cdev_del(&cdev);
		class_unregister(&xpc_class);
		unregister_chrdev_region(dev, 1);
		return;
	}
	device = device_create(&xpc_class, NULL, /* no parent device */ dev,
			       NULL,
			       /* no additional data */ DEVICE_NAME);
	if (IS_ERR(&device)) {
		xpc_debug(2, "error creating deivce\n");
	}

	err = xctrl_create_fs_device(&xpc_class);
	if (err < 0) {
		xpc_debug(0, "Error creating xctrl fs\n");
	}

	// Initialize xstat
	err = xstat_init(&xpc_class);
	if (err < 0) {
		xpc_debug(0, "Error creating initializing xstat\n");
	}
}

void destroy_fs_device(void)
{
	xctrl_destory_fs_device(&xpc_class);
	cdev_del(&cdev);
	device_destroy(&xpc_class, MKDEV(major_number, 0));

	class_unregister(&xpc_class);
	unregister_chrdev_region(dev, 1);
}

static void xpc_drv_release_by_pid(pid_t pid)
{
	int ipc_id, mb_idx, global_mb_idx;

	for (ipc_id = 0; ipc_id < XPC_NUM_IPC_DEVICES; ipc_id++) {
		if (test_bit(ipc_id, &enabled_mailboxes) == 0) {
			continue;
		}
		for (mb_idx = 0; mb_idx < XPC_IPC_NUM_MAILBOXES; mb_idx++) {
			global_mb_idx = ipc_id * XPC_IPC_NUM_MAILBOXES + mb_idx;
			/**
			 * Check for open mailboxes for which this
			 * PID is the source of the message.
			 */
			if (xpc_check_global_mailbox_pid(
				    global_mb_idx, pid,
				    XPC_REQUEST_IS_SOURCE)) {
				xpc_print_global_mailbox(global_mb_idx,
							 XPC_REQUEST_IS_SOURCE);

				xpc_clr_global_mailbox_pid(
					global_mb_idx, XPC_REQUEST_IS_SOURCE);

				xpc_close_global_mailbox(global_mb_idx);
			}

			/**
			 * Check for open mailboxes for which this
			 * PID is a target of the message.
			 *
			 * TODO(kirick): In a follow-up CR targets
			 * will reply with error rather than close
			 * the mailbox. Sources will be solely
			 * responsible for closing all mailboxes.
			 */
			if (xpc_check_global_mailbox_pid(
				    global_mb_idx, pid,
				    XPC_REQUEST_IS_TARGET)) {
				xpc_print_global_mailbox(global_mb_idx,
							 XPC_REQUEST_IS_TARGET);

				xpc_clr_global_mailbox_pid(
					global_mb_idx, XPC_REQUEST_IS_TARGET);

				xpc_close_global_mailbox(global_mb_idx);
			}
		}
	}
}

static int xpc_drv_release(struct inode *inode, struct file *filp)
{
	int vchannel, schannel;
	struct xpc_client_t *tmp_client =
		(struct xpc_client_t *)filp->private_data;

	// Deregister this process for any vchannel/schannel queue it was registered on
	for (vchannel = 0; vchannel < XPC_NUM_VCHANNELS; vchannel++) {
		for (schannel = 0; schannel < XPC_NUM_SCHANNELS; schannel++) {
			xpc_queue_deregister_user(
				vchannel, schannel,
				(uint8_t *)&(
					tmp_client->registered_subchannels[0]
									  [0]));
		}
	}

	xpc_drv_release_by_pid(tmp_client->tid);
	kfree(tmp_client);

	return 0;
}

static int xpc_drv_open(struct inode *inode, struct file *filp)
{
	struct xpc_client_t *tmp_client;
	int i;

	tmp_client = kzalloc(sizeof(*tmp_client), GFP_KERNEL);
	if (!tmp_client) {
		xpc_debug(2, "Failed to allocate new client.\n");
		return -ENOMEM;
	}

	// Clear the active mailboxes state for
	// all possible IPC devices
	for (i = 0; i < XPC_NUM_IPC_DEVICES; i++) {
		tmp_client->active_sends[i] = 0;
		tmp_client->active_recvs[i] = 0;
	}

	tmp_client->tid = current->pid;
	tmp_client->use_kernel_api = false;

	if (filp->f_flags & O_SYNC) {
		tmp_client->use_kernel_api = true;
		xpc_debug(
			0,
			"PID %d - XPC driver opened in kernel direct mode - flags = 0x%08X.\n",
			current->pid, filp->f_flags);
	}

	filp->private_data = tmp_client;
	return 0;
}

static int xpc_drv_probe(struct platform_device *pdev)
{
	int i;
	int irq;
	int irq_count;
	int ret;
	int ipc_id = 0;
	int irq_affinity_idx = 0;
	int status;
	struct resource *res;
	size_t remap_size;
	uintptr_t base_addr;
	uint32_t channel_ids = 0;
	uint32_t mailbox_mask = 0;
	uint32_t channel_mask = 0;
	char *irq_description = "mero_xpc";
	struct cpumask irq_cpu_mask;

	irq_count = platform_irq_count(pdev);
	res = platform_get_resource(pdev, IORESOURCE_MEM,
				    0); // get resource info
	remap_size = res->end - res->start + 1; // get resource memory size
	base_addr = (uintptr_t)ioremap(res->start, remap_size); // map it

	// Identify and setup pl320 based on physical start address
	switch (res->start) {
	case IPC0_ADDRESS:
		xpc_debug(2, "Found IPC0\n");
		ipc_id = IPC0_IDX;
		mailbox_mask = (XPC_CORE_ID == CORE_ID_A55) ? 0xFFFFFFFF : 0x0;
		channel_mask = 0xFFFFFFFF;
		irq_description = "mero_xpc, IPC0";
		break;
	case IPC1_ADDRESS:
		xpc_debug(2, "Found IPC1\n");
		ipc_id = IPC1_IDX;
		mailbox_mask = (XPC_CORE_ID == CORE_ID_A55) ? 0xFFFFFFFF : 0x0;
		channel_mask = 0xFFFFFFFF;
		irq_description = "mero_xpc, IPC1";
		break;
	case IPC2_ADDRESS:
		xpc_debug(2, "Found IPC2\n");
		ipc_id = IPC2_IDX;
		mailbox_mask = (XPC_CORE_ID == CORE_ID_A55) ? 0xFFFFFFFF : 0x0;
		channel_mask = 0xFFFFFFFF;
		irq_description = "mero_xpc, IPC2";
		break;
	case IPC3_ADDRESS:
		xpc_debug(2, "Found IPC3\n");
		ipc_id = IPC3_IDX;
		mailbox_mask = IPC3_USEABLE_MAILBOX_MASK;
		channel_mask = get_core_channel_mask(XPC_CORE_ID, ipc_id);
		irq_description = "mero_xpc, IPC3";
		break;
	case IPC4_ADDRESS:
		xpc_debug(2, "Found IPC4\n");
		ipc_id = IPC4_IDX;
		mailbox_mask = (XPC_CORE_ID == CORE_ID_A55) ? 0xFFFFFFFF : 0x0;
		channel_mask = 0xFFFFFFFF;
		irq_description = "mero_xpc, IPC4";
		break;
	default:
		ipc_id = -1;
		break;
	}

	if (ipc_id >= 0) {
		ipc_count++;
	}
	// Initialize pl320's before enabling interrupts
	status = pl320_initialize_hardware(ipc_id, base_addr, channel_mask,
					   mailbox_mask);
	if (status == PL320_STATUS_SUCCESS) {
		ipc_initialized[ipc_id] = 1;
		set_bit(ipc_id, &enabled_mailboxes);
	} else {
		xpc_debug(0, "Error initializing IPC(id:%u): Status:%u\n",
			  ipc_id, status);
		return -1;
	}

	channel_ids = get_core_channel_mask(XPC_CORE_ID, ipc_id);
	for (i = 0; i < irq_count; ++i) {
		if ((channel_ids & 0x1) == 0x1) {
			irq = platform_get_irq(pdev, i);
			irq_table[irq] = (ipc_id << 6) | i;

			ret = request_irq(irq, xpc_irq_handler, 0,
					  irq_description,
					  (void *)xpc_irq_handler);
			if (ret < 0) {
				xpc_debug(0,
					  "Unable to register interrupt %i\n",
					  i);
				xpc_debug(0, "error %i\n", ret);
			} else {
				irq_affinity_idx =
					(irq_affinity_idx + 1) % NR_CPUS;
				irq_affinity_idx =
					irq_affinity_idx == 0 ?
						irq_affinity_idx + 1 :
						irq_affinity_idx;

				cpumask_clear(&irq_cpu_mask);
				cpumask_set_cpu(irq_affinity_idx,
						&irq_cpu_mask);

				WARN_ON(irq_set_affinity_hint(irq,
							      &irq_cpu_mask));

				pr_info("Registering irq_table[%i] = %i affine to cpu_id %i\n",
					irq, (ipc_id << 6) | i,
					irq_affinity_idx);
			}
		}
		channel_ids = channel_ids >> 1;
	}

	if (ipc_id == IPC2_IDX) {
		// Run self tests
		bool had_failure = false;

		if (xpc_test_command_response(kCRNormalPriority) < 0) {
			had_failure = true;
		}

		if (xpc_test_command_response(kCRHighPriority) < 0) {
			had_failure = true;
		}

		if (had_failure) {
			xpc_debug(0, "FAILURE: XPC self-tests had failures\n");
		} else {
			xpc_debug(
				0,
				"SUCCESS: XPC self-tests completed successfully\n");
		}
	}
	return 0;
}

static int xpc_drv_remove(struct platform_device *pdev)
{
	int ipc_id = 0;
	// get resource info
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	switch (res->start) {
	case IPC0_ADDRESS:
		xpc_debug(2, "Unloading IPC0\n");
		ipc_id = IPC0_IDX;
		clear_bit(0, &enabled_mailboxes);
		break;
	case IPC1_ADDRESS:
		xpc_debug(2, "Unloading IPC1\n");
		ipc_id = IPC1_IDX;
		clear_bit(1, &enabled_mailboxes);
		break;
	case IPC2_ADDRESS:
		xpc_debug(2, "Unloading IPC2\n");
		ipc_id = IPC2_IDX;
		clear_bit(2, &enabled_mailboxes);
		break;
	case IPC3_ADDRESS:
		xpc_debug(2, "Unloading IPC3\n");
		ipc_id = IPC3_IDX;
		clear_bit(3, &enabled_mailboxes);
		break;
	case IPC4_ADDRESS:
		xpc_debug(2, "Unloading IPC4\n");
		ipc_id = IPC4_IDX;
		clear_bit(4, &enabled_mailboxes);
		break;
	default:
		ipc_id = -1;
		break;
	}
	get_core_channel_mask(XPC_CORE_ID, ipc_id);

	if (ipc_id >= 0 && ipc_id < XPC_NUM_IPC_DEVICES) {
		ipc_initialized[ipc_id] = 0;
		clear_bit(ipc_id, &enabled_mailboxes);
	}
	return 0;
}

static void dump_mailbox_info(void)
{
	int i, j = 0;
	pl320_debug_info dinfo;
	for (j = 0; j < XPC_NUM_IPC_DEVICES; j++) {
		if (test_bit(j, &enabled_mailboxes) != 0) {
			/**
			* Print divider
			*/
			xpc_debug(
				0,
				"--- IPC[%u]"
				"------------------------------------------------"
				"-------\n",
				j);

			pl320_get_debug_info(&dinfo, j);

			for (i = 0; i < XPC_IPC_NUM_MAILBOXES; i++) {
				xpc_debug(
					0,
					"IPC_%u - mailbox[%02u] - target_state=0x%02X, "
					"source_state=0x%02X\n",
					j, i,
					mailboxes[j * XPC_IPC_NUM_MAILBOXES + i]
						.mailbox_info.target_state,
					mailboxes[j * XPC_IPC_NUM_MAILBOXES + i]
						.mailbox_info.source_state);
			}

			for (i = 0; i < XPC_IPC_NUM_CHANNELS; i++) {
				xpc_debug(
					0,
					"IPC_%u - channel[%02u] - mask=0x%08X\n",
					j, i, dinfo.channel_id_mask[i]);
			}

			/**
			* Print divider
			*/
			xpc_debug(0,
				  "----------------"
				  "--------------------------------------------"
				  "--------"
				  "-----\n\n");
		}
	}
	for (i = 0; i < XPC_NUM_VCHANNELS; i++) {
		xpc_debug(0, "command.vchannel[%2u]=%2u", i,
			  atomic_read(&(command_mailbox[i].mailbox_count)));
	}

	xpc_debug(0,
		  "notification_reads = %u, notification_wake_ups = %u, "
		  "notifications_claimed = %u, notification_lost_claims = %u\n",
		  atomic_read(&notification_read_calls),
		  atomic_read(&notification_wake_ups),
		  atomic_read(&notification_claimed),
		  atomic_read(&notification_lost_claims));
	xpc_debug(0,
		  "queue_reads = %u, queue_wake_ups = %u, "
		  "queue_lost_claims = %u, queue_discards = %u,"
		  "queue_attempt_wake_up = %u\n",
		  atomic_read(&queue_read_calls), atomic_read(&queue_wake_ups),
		  atomic_read(&queue_lost_claims), atomic_read(&queue_discards),
		  atomic_read(&queue_attempt_wake_up));
}

static int clear_mailboxes(const char *buf)
{
	char cmd;
	int ipc, i;
	uint32_t mbox_mask = 0xFFFFFFFF;

	sscanf(buf, "%c:%i", &cmd, &ipc);

	if (cmd != 'f' || ((ipc != 2) && (ipc != 3))) {
		return -EINVAL;
	}

	if (ipc == IPC3_IDX) {
		mbox_mask = IPC3_USEABLE_MAILBOX_MASK;
	}
	for (i = 0; i < XPC_IPC_NUM_MAILBOXES; i++) {
		if (mbox_mask & (0x1 << i)) {
			xpc_close_ipc_mailbox(ipc, i);
		}
	}

	return 0;
}

int xtrace_parse_command(const char *buf)
{
	char cmd;
	int ipc_idx;
	int mbox_idx;

	sscanf(buf, "%c:%i:%i", &cmd, &ipc_idx, &mbox_idx);

	if (cmd != 't') {
		return -EINVAL;
	}

	xdebug_event_dump(ipc_idx, mbox_idx);
	return 0;
}

static int dump_routes(const char *buf)
{
	int i;
	int k;
	struct xpc_route_solution route_solution;
	int ret;
	char cmd;
	int src_core;
	uint32_t target;

	ret = sscanf(buf, "%c:%i", &cmd, &src_core);

	if (ret != 2 || cmd != 'r' || ((src_core != 8) && (src_core != 9))) {
		return -EINVAL;
	}

	for (i = 0; i < XPC_NUM_DESTINATIONS; i++) {
		target = 0x1 << i;

		if (xpc_compute_route(target, &route_solution) != 0) {
			for (k = 0; k < XPC_NUM_IPC_DEVICES; k++) {
				if ((route_solution.valid_routes >> k) & 0x1) {
					printk("RS_F: S=%d,T=0x%03X - r[%d].ipc_id = 0x%02X, "
					       "r[%d].src_mask = 0x%08X, r[%d].tgt_mask = 0x%08X\n",
					       (int)src_core, (int)target, k,
					       (int)route_solution.routes[k]
						       .ipc_id,
					       k,
					       (int)route_solution.routes[k]
						       .ipc_source_irq_mask,
					       k,
					       (int)route_solution.routes[k]
						       .ipc_target_irq_mask);
				}
			}
		}
	}
	return 0;
}

static ssize_t __attribute__((unused))
xpc_drv_write(struct file *filp, const char __user *user_buf, size_t count,
	      loff_t *pos)
{
	char buf[256];
	size_t buf_size;
	int ret = 0;

	buf_size = min(count, sizeof(buf) - 1);
	if (strncpy_from_user(buf, user_buf, buf_size) < 0)
		return -EFAULT;

	buf[buf_size] = 0;

	switch (buf[0]) {
	case 'c':
		atomic_set(&notification_read_calls, 0);
		atomic_set(&notification_wake_ups, 0);
		atomic_set(&notification_claimed, 0);
		atomic_set(&notification_lost_claims, 0);
		atomic_set(&queue_read_calls, 0);
		atomic_set(&queue_wake_ups, 0);
		atomic_set(&queue_lost_claims, 0);
		atomic_set(&queue_discards, 0);
		atomic_set(&queue_attempt_wake_up, 0);
		break;
	case 'd':
		dump_mailbox_info();
		break;
	case 'l':
		if (XPC_CORE_ID == CORE_ID_A55) {
			ret = xdebug_parse_command(buf);
		}
		break;
	case 't':
		ret = xtrace_parse_command(buf);
		break;
	case 'f':
		// Flush all ipc mailboxes
		if (XPC_CORE_ID == CORE_ID_A55) {
			ret = clear_mailboxes(buf);
		}
		break;
	case 'r':
		ret = dump_routes(buf);
		break;
	default:
		pr_err("Command not supported: %c\n", buf[0]);
		return -EFAULT;
	}
	return ret != 0 ? ret : count;
}

/**
 * xpc_handle_command_send_command - Handle userspace request
 * to send a command.
 */
static int xpc_handle_command_send_command(unsigned long data,
					   struct xpc_client_t *client)
{
	int retval;
	struct XpcCommandInfo req;

	(void)client;

	if (copy_from_user((void *)&req, (void *)data,
			   sizeof(struct XpcCommandInfo))) {
		return -EFAULT;
	}

	if (client->use_kernel_api) {
		retval = xpc_command_send_async(
			req.channel, xpc_channel_mask_to_idx(req.dst_mask),
			req.data, req.length, &req.ticket);
	} else {
		retval = xpc_send_command_internal(&req, true,
						   XPC_CLIENT_IS_DIRECT);
	}

	if (retval)
		return retval;

	if (copy_to_user((void *)data, (void *)&req,
			 sizeof(struct XpcCommandInfo))) {
		return -EFAULT;
	}
	return 0;
}

/**
 * xpc_handle_command_wait_response - Handle userspace wait
 * for response from a previous command send.
 */
static int xpc_handle_command_wait_response(unsigned long data,
					    struct xpc_client_t *client)
{
	int retval;
	struct XpcResponseInfo req;

	(void)client;
	if (copy_from_user((void *)&req, (void *)data,
			   sizeof(struct XpcResponseInfo))) {
		return -EFAULT;
	}

	if (client->use_kernel_api) {
		retval =
			xpc_command_wait_response_async(&req.ticket, req.data,
							XPC_MAX_RESPONSE_LENGTH,
							&req.length);
	} else {
		// Check if the ticket provided belongs to the client
		if (!xpc_check_global_mailbox_pid(req.ticket, client->tid,
						  XPC_REQUEST_IS_SOURCE)) {
			xpc_debug(0, "Requesting client does not own ticket\n");
			return -EINVAL;
		}

		retval = xpc_wait_command_response_internal(&req);
	}

	if (retval)
		return retval;

	if (copy_to_user((void *)data, (void *)&req,
			 sizeof(struct XpcResponseInfo))) {
		return -EFAULT;
	}
	return 0;
}

/**
 * xpc_handle_command_send_command_and_wait_response - Handle userspace request
 * to send a command and wait for a response.
 */
static int xpc_handle_command_send_and_wait(unsigned long data,
					    struct xpc_client_t *client)
{
	int retval;
	struct XpcCommandResponseInfo req;

	(void)client;

	if (copy_from_user((void *)&req, (void *)data,
			   sizeof(struct XpcCommandResponseInfo))) {
		return -EFAULT;
	}

	if (client->use_kernel_api) {
		retval = xpc_command_send_async(
			req.command.channel,
			xpc_channel_mask_to_idx(req.command.dst_mask),
			req.command.data, req.command.length,
			&req.command.ticket);
	} else {
		retval = xpc_send_command_internal(&req.command, true,
						   XPC_CLIENT_IS_DIRECT);
	}

	if (retval)
		return retval;

	req.response.ticket = req.command.ticket;
	req.response.timeout_us = 0;

	if (client->use_kernel_api) {
		retval = xpc_command_wait_response_async(
			&req.response.ticket, req.response.data,
			XPC_MAX_RESPONSE_LENGTH, &req.response.length);
	} else {
		retval = xpc_wait_command_response_internal(&req.response);
	}

	if (retval)
		return retval;

	if (copy_to_user((void *)data, (void *)&req,
			 sizeof(struct XpcCommandResponseInfo))) {
		return -EFAULT;
	}

	return 0;
}

/**
 * xpc_handle_command_send_response - Handle userspace request
 * to send response for an incomming command.
 */
static int xpc_handle_command_send_response(unsigned long data,
					    struct xpc_client_t *client)
{
	struct XpcResponseInfo req;
	(void)client;

	if (copy_from_user((void *)&req, (void *)data,
			   sizeof(struct XpcResponseInfo))) {
		return -EFAULT;
	}

	// Check if the ticket provided belongs to the client
	if (!xpc_check_global_mailbox_pid(req.ticket, client->tid,
					  XPC_REQUEST_IS_TARGET)) {
		xpc_debug(0, "Requesting client does not own ticket\n");
		return -EINVAL;
	}

	return xpc_send_command_response_internal(&req);
}

/**
 * xpc_handle_command_wait_receive - Handle userspace request
 * to wait for an incomming command.
 */
static int xpc_handle_command_wait_receive(unsigned long data,
					   struct xpc_client_t *client)
{
	int retval = 0;
	int claimed_mailbox = -1;
	struct XpcReadMailboxInfo req;

	(void)client;
	if (copy_from_user(&req, (void *)data,
			   sizeof(struct XpcReadMailboxInfo))) {
		return -EINVAL;
	}

	do {
		spin_lock_irq(&(command_mailbox[req.channel].wait_queue.lock));

		retval = wait_event_interruptible_locked_irq(
			command_mailbox[req.channel].wait_queue,
			xpc_check_mailbox_count(
				&command_mailbox[req.channel]) != 0);

		if (retval == -ERESTARTSYS) {
			xpc_debug(1, "restart_sys\n");
			spin_unlock_irq(&(
				command_mailbox[req.channel].wait_queue.lock));
			return -ERESTARTSYS;
		}

		if (retval == 0) {
			int ipc_id;
			int mailbox_idx;
			struct xpc_info info;
			info.xpc_channel = req.channel;
			info.xpc_mode = XPC_MODE_COMMAND;

			claimed_mailbox = xpc_claim_global_mailbox(info);
			if (claimed_mailbox == -ECANCELED) {
				// The sender has timed out the mailbox.. We should
				// decrement the mailbox count to remove it from the queue
				atomic_dec(&(command_mailbox[req.channel]
						     .mailbox_count));
			} else if (claimed_mailbox >= 0) {
				atomic_dec(&(command_mailbox[req.channel]
						     .mailbox_count));
			}

			xpc_get_ipc_id_and_mailbox(claimed_mailbox, &ipc_id,
						   &mailbox_idx);
			xdebug_event_record(ipc_id, mailbox_idx,
					    XPC_EVT_CMD_WAITER_WOKE, 0);
		}

		spin_unlock_irq(
			&(command_mailbox[req.channel].wait_queue.lock));

	} while (retval != 0 || claimed_mailbox < 0);

	memcpy((void *)req.data,
	       (void *)mailboxes[claimed_mailbox].mailbox_data.u8,
	       XPC_MAX_PAYLOAD_LENGTH);

	req.length = XPC_MAX_COMMAND_LENGTH;
	req.ticket = claimed_mailbox;
	// We received a message start tracking the
	// mailbox association
	xpc_set_global_mailbox_pid(req.ticket, current->pid,
				   XPC_REQUEST_IS_TARGET, XPC_CLIENT_IS_DIRECT);

	if (copy_to_user((void *)data, (void *)&req,
			 sizeof(struct XpcReadMailboxInfo))) {
		// Should not fail here
		// We need to release the mailbox here
		BUG();
		return -EFAULT;
	}

	return 0;
}

/**
 * xpc_handle_notification_send - Handle userspace request
 * to send a notification.
 */
static int xpc_handle_notification_send(unsigned long data,
					struct xpc_client_t *client)
{
	int retval;
	struct XpcNotificationInfo req;
	(void)client;
	if (copy_from_user((void *)&req, (void *)data,
			   sizeof(struct XpcNotificationInfo))) {
		return -EFAULT;
	}

	if (client->use_kernel_api) {
		retval = xpc_notification_send(req.channel,
					       XPC_UNREMASK(req.dst_mask),
					       req.data, req.length, req.mode);
	} else {
		retval = xpc_send_notification_internal(&req,
							XPC_CLIENT_IS_DIRECT);
	}

	if (retval)
		return retval;

	if (copy_to_user((void *)data, (void *)&req,
			 sizeof(struct XpcNotificationInfo))) {
		return -EFAULT;
	}
	return 0;
}

/**
 * xpc_handle_notification_wait_receive - Handle userspace request
 * to wait for an incomming notification.
 */
static int xpc_handle_notification_wait_receive(unsigned long data,
						struct xpc_client_t *client)
{
	int retval = 0;
	int claimed_mailbox = -1;
	struct XpcReadMailboxInfo req;
	(void)client;
	if (copy_from_user(&req, (void *)data,
			   sizeof(struct XpcReadMailboxInfo))) {
		return -EINVAL;
	}

	if (client->use_kernel_api) {
		retval = xpc_notification_recv(req.channel, req.data,
					       req.length);
	} else {
		atomic_inc(&notification_read_calls);
		do {
			spin_lock_irq(&notification_mailbox[req.channel]
					       .wait_queue.lock);

			retval = wait_event_interruptible_locked_irq(
				notification_mailbox[req.channel].wait_queue,
				xpc_check_mailbox_count(
					&notification_mailbox[req.channel]) !=
					0);

			if (retval == -ERESTARTSYS) {
				xpc_debug(1, "restart_sys\n");
				spin_unlock_irq(
					&notification_mailbox[req.channel]
						 .wait_queue.lock);
				return -ERESTARTSYS;
			}

			if (retval == 0) {
				struct xpc_info info;
				info.xpc_channel = req.channel;
				info.xpc_mode = XPC_MODE_NOTIFICATION;

				atomic_inc(&notification_wake_ups);
				claimed_mailbox =
					xpc_claim_global_mailbox(info);
				if (claimed_mailbox == -ECANCELED) {
					// The sender has timed out the mailbox.. We should
					// decrement the mailbox count to remove it from the queue
					atomic_dec(&(
						notification_mailbox[req.channel]
							.mailbox_count));
				} else if (claimed_mailbox >= 0) {
					atomic_dec(&(
						notification_mailbox[req.channel]
							.mailbox_count));
				}

				if (claimed_mailbox < 0) {
					atomic_inc(&notification_lost_claims);
				}
			}

			spin_unlock_irq(&notification_mailbox[req.channel]
						 .wait_queue.lock);
		} while (retval != 0 || claimed_mailbox < 0);

		atomic_inc(&notification_claimed);

		memcpy((void *)req.data,
		       (void *)mailboxes[claimed_mailbox].mailbox_data.u8,
		       XPC_MAX_PAYLOAD_LENGTH);

		req.length = XPC_MAX_NOTIFICATION_LENGTH;

		/**
		 * We received a message so start tracking the
		 * mailbox to PID association.
		 */
		xpc_set_global_mailbox_pid(claimed_mailbox, current->pid,
					   XPC_REQUEST_IS_TARGET,
					   XPC_CLIENT_IS_DIRECT);

		// Acknowledge the notification
		retval = xpc_acknowledge_notification_internal(claimed_mailbox);
	}

	if (retval)
		return retval;

	if (copy_to_user((void *)data, (void *)&req,
			 sizeof(struct XpcReadMailboxInfo))) {
		// Should not fail here
		// We need to release the mailbox here
		BUG();
		return -EFAULT;
	}
	return 0;
}

/**
 * xpc_handle_queue_send - Handle userspace request
 * to send on a specified xpc queue.
 */
static int xpc_handle_queue_send(unsigned long data,
				 struct xpc_client_t *client)
{
	int retval = 0;
	struct XpcQueueInfo req;

	if (copy_from_user(&req, (void *)data, sizeof(struct XpcQueueInfo))) {
		return -EINVAL;
	}

	if (client->use_kernel_api) {
		retval = xpc_queue_send(req.channel, req.sub_channel,
					XPC_UNREMASK(req.dst_mask), req.data,
					req.length);
	} else {
		retval = xpc_send_queue_internal(&req, XPC_CLIENT_IS_DIRECT);
	}

	return retval;
}

/**
 * xpc_handle_queue_wait_receive - Handle userspace request
 * to wait for data on a specified xpc queue.
 */
static int xpc_handle_queue_wait_receive(unsigned long data,
					 struct xpc_client_t *client)
{
	int retval = 0;
	int claimed_mailbox = -1;
	struct XpcQueueInfo req;
	unsigned long flags;

	if (copy_from_user(&req, (void *)data, sizeof(struct XpcQueueInfo))) {
		return -EINVAL;
	}

	if (client->use_kernel_api) {
		retval = xpc_queue_recv(req.channel, req.sub_channel, req.data,
					req.length);
	} else {
		atomic_inc(&queue_read_calls);
		spin_lock_irqsave(&(registered_queue_lock), flags);
		// Now that we're in an atomic context, check that this process
		// is registered for this queue.
		if (!xpc_queue_check_user_registration(
			    req.channel, req.sub_channel,
			    (uint8_t *)&(
				    client->registered_subchannels[0][0]))) {
			xpc_debug(
				2,
				"user not registered for channel:%u sub_channel:%u\n",
				req.channel, req.sub_channel);
			spin_unlock_irqrestore(&(registered_queue_lock), flags);
			return -EADDRINUSE;
		}
		spin_unlock_irqrestore(&(registered_queue_lock), flags);

		do {
			spin_lock_irq(
				&queue_mailbox[req.channel].wait_queue.lock);

			// After mailbox count increases for this channel, check if mailbox
			// exists for this sub channel
			retval = wait_event_interruptible_locked_irq(
				queue_mailbox[req.channel].wait_queue,
				(xpc_queue_check_and_claim_mailbox(
					 &queue_mailbox[req.channel],
					 req.channel, req.sub_channel,
					 &claimed_mailbox) != 0));

			if (retval == 0) {
				atomic_inc(&queue_wake_ups);
			}

			if (retval == -ERESTARTSYS) {
				spin_unlock_irq(&queue_mailbox[req.channel]
							 .wait_queue.lock);
				return -ERESTARTSYS;
			}

			if (claimed_mailbox == -ECANCELED) {
				// The sender has timed out the mailbox.. We should decrement
				// the mailbox count to remove it from the queue
				atomic_dec(&(queue_mailbox[req.channel]
						     .mailbox_count));
				atomic_inc(&queue_lost_claims);
			} else if (claimed_mailbox >= 0) {
				atomic_dec(&(queue_mailbox[req.channel]
						     .mailbox_count));
			}

			spin_unlock_irq(
				&queue_mailbox[req.channel].wait_queue.lock);
		} while (retval != 0 || claimed_mailbox < 0);

		memcpy((void *)req.data,
		       (void *)mailboxes[claimed_mailbox].mailbox_data.u8,
		       XPC_MAX_PAYLOAD_LENGTH);

		req.length = XPC_MAX_PAYLOAD_LENGTH;

		/**
		 * We received a message so start tracking the
		 * mailbox to PID association.
		 */
		xpc_set_global_mailbox_pid(claimed_mailbox, current->pid,
					   XPC_REQUEST_IS_TARGET,
					   XPC_CLIENT_IS_DIRECT);

		// Acknowledge the queue
		retval = xpc_acknowledge_queue_internal(claimed_mailbox);
	}

	if (retval)
		return retval;

	if (copy_to_user((void *)data, (void *)&req,
			 sizeof(struct XpcQueueInfo))) {
		// Should not fail here
		// We need to release the mailbox here
		BUG();
		return -EFAULT;
	}
	return 0;
}

/**
 * xpc_handle_queue_register - Handle userspace request
 * to register (start listening) on a specific xpc queue.
 */
static int xpc_handle_queue_register(unsigned long data,
				     struct xpc_client_t *client)
{
	int retval;
	struct XpcQueueConfigInfo req;

	if (copy_from_user(&req, (void *)data,
			   sizeof(struct XpcQueueConfigInfo))) {
		return -EINVAL;
	}

	if (client->use_kernel_api) {
		retval = xpc_queue_register(req.channel, req.sub_channel);
	} else {
		retval = xpc_queue_register_user(
			req.channel, req.sub_channel,
			(uint8_t *)&(client->registered_subchannels[0][0]));
	}
	return retval;
}

/**
 * xpc_handle_queue_deregister - Handle userspace request
 * to deregister (stop listening) on a specific xpc queue.
 */
static int xpc_handle_queue_deregister(unsigned long data,
				       struct xpc_client_t *client)
{
	int retval;
	struct XpcQueueConfigInfo req;

	if (copy_from_user(&req, (void *)data,
			   sizeof(struct XpcQueueConfigInfo))) {
		return -EINVAL;
	}

	if (client->use_kernel_api) {
		retval = xpc_queue_deregister(req.channel, req.sub_channel);
	} else {
		retval = xpc_queue_deregister_user(
			req.channel, req.sub_channel,
			(uint8_t *)&(client->registered_subchannels[0][0]));
	}
	return retval;
}

/**
 * xpc_handle_query_mailbox_usage - Handle userspace request
 * to query mailbox states.
 */
static int xpc_handle_query_mailbox_usage(unsigned long data)
{
	int i, j = 0;
	bool filter_by_pid = false;
	struct XpcMailboxUsageQuery req;
	struct xpc_mailbox *mb;
	bool found = false;

	if (copy_from_user(&req, (void *)data,
			   sizeof(struct XpcMailboxUsageQuery))) {
		return -EINVAL;
	}

	if (req.query_pid >= 0)
		filter_by_pid = true;

	req.total_in_use = 0;
	req.total_restarts = 0;

	for (j = 0; j < XPC_NUM_IPC_DEVICES; j++) {
		req.mailboxes[j] = 0;

		if (test_bit(j, &enabled_mailboxes) != 0) {
			for (i = 0; i < XPC_IPC_NUM_MAILBOXES; i++) {
				mb = xpc_get_global_mailbox_from_ipc_id_and_mailbox_idx(
					j, i);

				if (mb == NULL)
					return -EINVAL;

				if (!filter_by_pid) {
					if (mb->mailbox_info.source_state !=
						    MAILBOX_STATE_FREE ||
					    mb->mailbox_info.target_state !=
						    MAILBOX_STATE_FREE) {
						req.mailboxes[j] |= 0x1 << i;
						req.total_in_use++;
					}
					found = true;
				} else if (req.query_pid ==
						   mb->mailbox_info.source_pid ||
					   req.query_pid ==
						   mb->mailbox_info.target_pid) {
					req.mailboxes[j] |= 0x1 << i;
					req.total_in_use++;
					req.total_restarts =
						mb->mailbox_info.source_restarts;
					found = true;
				}
			}
		}
	}

	if (!found)
		return -EINVAL;

	if (copy_to_user((void *)data, (void *)&req,
			 sizeof(struct XpcMailboxUsageQuery))) {
		return -EFAULT;
	}

	return 0;
}

/**
 * xpc_drv_unlocked_ioctl - This is the main dispatch routine
 * for ioctls issued from userspace.  Ioctls are decoded and
 * dispacted to one of the xpc_handle_* routines.
 */
static long xpc_drv_unlocked_ioctl(struct file *filp, unsigned int cmd,
				   unsigned long data)
{
	int res = 0;

	struct xpc_client_t *client = (struct xpc_client_t *)filp->private_data;

	if ((void *)data == NULL)
		return -EINVAL;

	if (client == NULL)
		return -ENOMEM;

	switch (cmd) {
	case XPC_COMMAND_SEND_COMMAND:
		res = xpc_handle_command_send_command(data, client);
		break;
	case XPC_COMMAND_SEND_RESPONSE:
		res = xpc_handle_command_send_response(data, client);
		break;
	case XPC_COMMAND_WAIT_RESPONSE:
		res = xpc_handle_command_wait_response(data, client);
		break;
	case XPC_COMMAND_SEND_COMMAND_AND_WAIT_RESPONSE:
		res = xpc_handle_command_send_and_wait(data, client);
		break;
	case XPC_COMMAND_WAIT_RECEIVE:
		res = xpc_handle_command_wait_receive(data, client);
		break;
	case XPC_NOTIFICATION_SEND:
		res = xpc_handle_notification_send(data, client);
		break;
	case XPC_NOTIFICATION_WAIT_RECEIVE:
		res = xpc_handle_notification_wait_receive(data, client);
		break;
	case XPC_QUEUE_SEND:
		res = xpc_handle_queue_send(data, client);
		break;
	case XPC_QUEUE_WAIT_RECEIVE:
		res = xpc_handle_queue_wait_receive(data, client);
		break;
	case XPC_QUEUE_REGISTER:
		res = xpc_handle_queue_register(data, client);
		break;
	case XPC_QUEUE_DEREGISTER:
		res = xpc_handle_queue_deregister(data, client);
		break;
	case XPC_QUERY_MAILBOX_USAGE:
		res = xpc_handle_query_mailbox_usage(data);
		break;
	default:
		res = -ENOTTY;
		break;
	}
	return res;
}

static irqreturn_t xpc_irq_handler(int irq, void *dev)
{
	struct xpc_mailbox_info mbox_info;
	uint16_t ipc_id;
	uint8_t global_mbox_id;
	Pl320HalStatus status;
	XpcMessageHeader header;
	Pl320MailboxIRQReason irq_cause;
	bool did_nothing = true;
	uint64_t ts;

	ts = xdebug_event_get_ts();

	// If an interrupt comes in before everything is initialized
	// this will early exit.
	ipc_id = (irq_table[irq] >> 6);
	if (ipc_id >= XPC_NUM_IPC_DEVICES ||
	    test_bit(ipc_id, &enabled_mailboxes) == 0) {
		xpc_debug(2, "xpc hasn't been initialized yet\n");
		return IRQ_NONE;
	}

	mbox_info.pl320_info.mailbox_idx = 0;

	while (1) {
		mbox_info.pl320_info.ipc_id = ipc_id;
		mbox_info.pl320_info.ipc_irq_id = irq_table[irq] & 0x3F;

		status = pl320_read_mailbox_irq(&(mbox_info.pl320_info));
		if (status != PL320_STATUS_SUCCESS) {
			xpc_debug(
				0,
				"failed to read pl320_mailbox_irq - status %i\n",
				status);

			return IRQ_NONE;
		}

		if (mbox_info.pl320_info.mailbox_irq_mask == 0) {
			// no work to do, so might as well end early
			if (did_nothing) {
				// Record the case we did nothing at all in
				// during this IRQ event
				xdebug_event_record_all(
					ipc_id, XPC_EVT_IRQ_HANDLER_EXIT_EARLY,
					__LINE__);
			}
			break;
		}

		did_nothing = false;

		xdebug_event_record_timed(
			ipc_id, mbox_info.pl320_info.mailbox_idx,
			XPC_EVT_IRQ_ENTER,
			(0x1 << mbox_info.pl320_info.ipc_irq_id), ts);

		// Perform mailbox read and do some pre-checking
		global_mbox_id = xpc_populate_global_mailbox(
			&mbox_info, &header, &irq_cause);

		if (irq_cause == PL320_IRQ_REASON_SOURCE_ACK) {
			// Fast path
			switch (header.fields.mode) {
			case XPC_MODE_COMMAND_RESPONSE: // command_response
			case XPC_MODE_COMMAND:
				xdebug_event_record(
					ipc_id,
					mbox_info.pl320_info.mailbox_idx,
					XPC_EVT_IRQ_COMMAND_ACK_SOURCE_COMPLETED_START,
					header.byte_);

				if (mailboxes[global_mbox_id]
					    .mailbox_info.source_state ==
				    MAILBOX_STATE_COMMAND_WAIT_NO_RESPONSE) {
					xpc_close_global_mailbox(
						global_mbox_id);
				} else {
					complete(&(mailboxes[global_mbox_id]
							   .target_replied));
				}

				xdebug_event_record(
					ipc_id,
					mbox_info.pl320_info.mailbox_idx,
					XPC_EVT_IRQ_COMMAND_ACK_SOURCE_COMPLETED_END,
					header.byte_);
				break;
			case XPC_MODE_NOTIFICATION:
				if (mailboxes[global_mbox_id]
					    .mailbox_info.source_state ==
				    MAILBOX_STATE_NOTIFICATION_WAIT_POSTED) {
					xdebug_event_record(
						ipc_id,
						mbox_info.pl320_info.mailbox_idx,
						XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_CLOSED,
						header.byte_);

					xpc_clr_global_mailbox_pid(
						global_mbox_id,
						XPC_REQUEST_IS_SOURCE);

					xpc_close_global_mailbox(
						global_mbox_id);
				} else {
					xdebug_event_record(
						ipc_id,
						mbox_info.pl320_info.mailbox_idx,
						XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_COMPLETED_START,
						header.byte_);

					complete(&(mailboxes[global_mbox_id]
							   .target_replied));

					xdebug_event_record(
						ipc_id,
						mbox_info.pl320_info.mailbox_idx,
						XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_COMPLETED_END,
						header.byte_);
				}
				break;
			case XPC_MODE_QUEUE:
				xdebug_event_record(
					ipc_id,
					mbox_info.pl320_info.mailbox_idx,
					XPC_EVT_IRQ_QUEUE_ACK_SOURCE_CLOSED,
					header.byte_);

				xpc_clr_global_mailbox_pid(
					global_mbox_id, XPC_REQUEST_IS_SOURCE);

				xpc_close_global_mailbox(global_mbox_id);
				break;
			}
		} else if (((header.fields.mode == XPC_MODE_COMMAND) ||
			    (header.fields.mode == XPC_MODE_COMMAND_RESPONSE)) &&
			   xpc_command_channel_is_hi_priority(
				   header.fields.vchannel)) {
			// The vchannel has been configured for fastpath execution.
			// The registered handler will be called in hardIRQ context.

			xdebug_event_record(
				ipc_id, mbox_info.pl320_info.mailbox_idx,
				XPC_EVT_IRQ_COMMAND_IN_FAST_HANDLE_START,
				header.byte_);

			xpc_do_kernel_work(header.fields.vchannel,
					   XPC_MODE_COMMAND, global_mbox_id);

			xdebug_event_record(
				ipc_id, mbox_info.pl320_info.mailbox_idx,
				XPC_EVT_IRQ_COMMAND_IN_FAST_HANDLE_END,
				header.byte_);

		} else if ((header.fields.mode == XPC_MODE_NOTIFICATION) &&
			   xpc_notification_channel_is_hi_priority(
				   header.fields.vchannel)) {
			// The vchannel has been configured for fastpath execution.
			// The registered handler will be called in hardIRQ context.
			xdebug_event_record(
				ipc_id, mbox_info.pl320_info.mailbox_idx,
				XPC_EVT_IRQ_NOTIFICATION_IN_FAST_HANDLE_START,
				header.byte_);

			xpc_do_kernel_work(header.fields.vchannel,
					   XPC_MODE_NOTIFICATION,
					   global_mbox_id);

			xdebug_event_record(
				ipc_id, mbox_info.pl320_info.mailbox_idx,
				XPC_EVT_IRQ_NOTIFICATION_IN_FAST_HANDLE_END,
				header.byte_);
		} else {
			// deferred work

			tasklet_hi_schedule(
				&(mailboxes[global_mbox_id].tasklet));

			xdebug_event_record(ipc_id,
					    mbox_info.pl320_info.mailbox_idx,
					    XPC_EVT_IRQ_WORK_SCHEDULED,
					    header.byte_);
		}
	}

	return IRQ_HANDLED;
}

uint8_t xpc_populate_global_mailbox(struct xpc_mailbox_info *mailbox_info,
				    XpcMessageHeader *header,
				    Pl320MailboxIRQReason *irq_cause)
{
	uint16_t length = 0;
	uint8_t mailbox_id;
	mailbox_id = xpc_get_global_mailbox(mailbox_info);

	pl320_read_mailbox_data(&(mailbox_info->pl320_info), 0,
				mailboxes[mailbox_id].mailbox_data.u8,
				XPC_MAILBOX_DATA_SIZE, &length);

	header->byte_ =
		mailboxes[mailbox_id].mailbox_data.u8[XPC_MAILBOX_HEADER_LOC];
	mailboxes[mailbox_id].mailbox_info.pl320_info =
		mailbox_info->pl320_info;
	mailboxes[mailbox_id].mailbox_info.xpc_mode = mailbox_info->xpc_mode;
	mailboxes[mailbox_id].mailbox_info.xpc_channel =
		header->fields.vchannel;
	mailboxes[mailbox_id].mailbox_info.global_mailbox_number =
		mailbox_info->global_mailbox_number;
	mailboxes[mailbox_id].mailbox_info.send_register =
		mailbox_info->send_register;
	*irq_cause = mailboxes[mailbox_id].mailbox_info.pl320_info.irq_cause;
	return mailbox_id;
}

/**
 * xpc_do_kernel_work - For handlers registered in the kernel, this
 * routine calls the command or notification handler for the specified
 * vchannel.  This routine runs in tasklet context.
 */
void xpc_do_kernel_work(XpcChannel channel, XpcMode xpc_mode,
			int global_mailbox_number)
{
	switch (xpc_mode) {
	case XPC_MODE_COMMAND: {
		struct XpcResponseInfo response;

		command_handlers[channel](
			channel, command_handler_args[channel],
			&mailboxes[global_mailbox_number].mailbox_data.u8[0],
			XPC_MAX_COMMAND_LENGTH, response.data,
			XPC_MAX_RESPONSE_LENGTH, &response.length);

		response.length = XPC_MAX_RESPONSE_LENGTH;
		response.ticket = global_mailbox_number;

		xpc_send_command_response_internal(&response);
	} break;
	case XPC_MODE_NOTIFICATION: {
		notification_handlers[channel](
			channel, notification_handler_args[channel],
			&mailboxes[global_mailbox_number].mailbox_data.u8[0],
			XPC_MAX_NOTIFICATION_LENGTH);

		xpc_acknowledge_notification_internal(global_mailbox_number);
	} break;
	default:
		break;
	}
}

/**
 * xpc_irq_tasklet - This is the tasklet that runs deferred
 * irq processing as determined by the main irq_handler
 * (xpc_irq_handler). This tasklet processes incomming mailbox
 * data and determines if it is a command, notification, or
 * queue related message.
 */
void xpc_irq_tasklet(unsigned long data)
{
	XpcMessageHeader header;
	Pl320MailboxIRQReason irq_cause;
	uint8_t mailbox_id;
	unsigned long flags;
	unsigned long wq_flags;
	struct xpc_queue *ch_queue;
	u8 schannel;
	int is_registered_user;
	int is_registered_kernel;

	mailbox_id = (int)data;

	header.byte_ =
		mailboxes[mailbox_id].mailbox_data.u8[XPC_MAILBOX_HEADER_LOC];
	irq_cause = mailboxes[mailbox_id].mailbox_info.pl320_info.irq_cause;

	xdebug_event_record(
		mailboxes[mailbox_id].mailbox_info.pl320_info.ipc_id,
		mailboxes[mailbox_id].mailbox_info.pl320_info.mailbox_idx,
		XPC_EVT_TASKLET_START, irq_cause);

	switch (header.fields.mode) {
	case XPC_MODE_COMMAND_RESPONSE: // command_response
	case XPC_MODE_COMMAND:
		if (irq_cause == PL320_IRQ_REASON_DESTINATION_INT) {
			ch_queue = &command_mailbox[header.fields.vchannel];

			spin_lock_irqsave(&ch_queue->wait_queue.lock, wq_flags);

			mailboxes[mailbox_id].mailbox_info.target_state =
				MAILBOX_STATE_COMMAND_PENDING;

			// Check if this is a platform channel message
			if (header.fields.vchannel ==
			    CVCORE_XCHANNEL_PLATFORM) {
				struct XpcResponseInfo response;
				int ret;

				ret = xpc_platform_channel_command_handler(
					header.fields.vchannel, NULL,
					mailboxes[mailbox_id].mailbox_data.u8,
					XPC_MAX_COMMAND_LENGTH, response.data,
					XPC_MAX_RESPONSE_LENGTH,
					&response.length);

				// Send response if the command was handled. Else fall through
				// and let others handle it.
				if (ret == 0) {
					// Let's let go of our spinlock
					spin_unlock_irqrestore(
						&ch_queue->wait_queue.lock,
						wq_flags);

					response.length =
						XPC_MAX_RESPONSE_LENGTH;
					response.ticket = mailbox_id;

					xpc_send_command_response_internal(
						&response);
					// Break out
					break;
				}
			}

			if (command_handlers[header.fields.vchannel] == NULL) {
				uint32_t queue_is_empty = list_empty_careful(
					&ch_queue->wait_queue.head);
				// No kernel mode handler installed, pass on to user wait_queue
				xdebug_event_record(
					mailboxes[mailbox_id]
						.mailbox_info.pl320_info.ipc_id,
					mailboxes[mailbox_id]
						.mailbox_info.pl320_info
						.mailbox_idx,
					XPC_EVT_CMD_WAKEUP_QUEUE,
					queue_is_empty);

				atomic_inc(&ch_queue->mailbox_count);
				wake_up_locked(&ch_queue->wait_queue);
				spin_unlock_irqrestore(
					&ch_queue->wait_queue.lock, wq_flags);
			} else {
				// Let's let go of our spinlock
				spin_unlock_irqrestore(
					&ch_queue->wait_queue.lock, wq_flags);

				// We handle this request directly in a tasklet
				xpc_do_kernel_work(header.fields.vchannel,
						   XPC_MODE_COMMAND,
						   mailbox_id);
			}
		}
		break;
	case XPC_MODE_NOTIFICATION:
		if (irq_cause == PL320_IRQ_REASON_DESTINATION_INT) {
			ch_queue =
				&notification_mailbox[header.fields.vchannel];

			spin_lock_irqsave(&ch_queue->wait_queue.lock, wq_flags);

			// Mark the mailbox as pending needing attention
			mailboxes[mailbox_id].mailbox_info.target_state =
				MAILBOX_STATE_NOTIFICATION_PENDING;

			if (notification_handlers[header.fields.vchannel] ==
			    NULL) {
				// No kernel mode handler installed, pass on to user wait_queue
				atomic_inc(&ch_queue->mailbox_count);
				wake_up_locked(&ch_queue->wait_queue);
				spin_unlock_irqrestore(
					&ch_queue->wait_queue.lock, wq_flags);
			} else {
				spin_unlock_irqrestore(
					&ch_queue->wait_queue.lock, wq_flags);

				// We handle this request directly in a tasklet
				xpc_do_kernel_work(header.fields.vchannel,
						   XPC_MODE_NOTIFICATION,
						   mailbox_id);
			}
		}
		break;
	case XPC_MODE_QUEUE:
		spin_lock_irqsave(&(registered_queue_lock), flags);

		// Mark the mailbox as pending needing attention
		mailboxes[mailbox_id].mailbox_info.target_state =
			MAILBOX_STATE_QUEUE_PENDING;

		schannel = mailboxes[mailbox_id]
				   .mailbox_data.u8[XPC_SUB_CHANNEL_BYTE];

		// Release mailbox if there is not a user process or kernel registered for
		// this channel/sub channel for queue mode
		is_registered_user = xpc_queue_check_user_registration(
			header.fields.vchannel, schannel,
			(uint8_t *)&(registered_user_subchannels[0][0]));

		if (!is_registered_user) {
			// Well is it registered by the kernel?
			is_registered_kernel =
				xpc_queue_check_kernel_registration(
					header.fields.vchannel, schannel);
			if (!is_registered_kernel) {
				atomic_inc(&queue_discards);
				xpc_acknowledge_queue_internal(mailbox_id);
				spin_unlock_irqrestore(&(registered_queue_lock),
						       flags);
				break;
			}
		}

		ch_queue = &queue_mailbox[header.fields.vchannel];

		spin_lock_irqsave(&ch_queue->wait_queue.lock, wq_flags);
		atomic_inc(&queue_attempt_wake_up);

		atomic_inc(&ch_queue->mailbox_count);
		wake_up_locked(&ch_queue->wait_queue);
		spin_unlock_irqrestore(&ch_queue->wait_queue.lock, wq_flags);

		spin_unlock_irqrestore(&(registered_queue_lock), flags);
		break;
	default:
		break;
	}

	xdebug_event_record(
		mailboxes[mailbox_id].mailbox_info.pl320_info.ipc_id,
		mailboxes[mailbox_id].mailbox_info.pl320_info.mailbox_idx,
		XPC_EVT_TASKLET_END, irq_cause);
}

#if defined(__i386__) || defined(__amd64__)
#define CVIP_S_INTF_BAR 2

extern void *get_sintf_base(int);

static int cvip_probe(struct pci_dev *pci_dev, const struct pci_device_id *id)
{
	struct cvip_dev *cvip;
	struct device *dev = &pci_dev->dev;
	int ret = -ENODEV;
	int nr_vecs, i;
	int local_irq = 0;
	int status = 0;
	uint32_t core_channel_mask = 0;
	unsigned long long device_address = 0;

	cvip = devm_kzalloc(dev, sizeof(*cvip), GFP_KERNEL);
	if (!cvip)
		return -ENOMEM;

	cvip_p = cvip;
	cvip->pci_dev = pci_dev;

	/* Retrieve resources */
	ret = pcim_enable_device(pci_dev);
	if (ret) {
		dev_err(dev, "Failed to enable PCI device (%d)\n", ret);
		return ret;
	}

	device_address = (unsigned long long)get_sintf_base(1);
	xpc_debug(2, "mapped io at %llx\n", device_address);

	nr_vecs = pci_msix_vec_count(pci_dev);
	xpc_debug(2, "nr_vecs %i\n", nr_vecs);

	core_channel_mask = get_core_channel_mask(XPC_CORE_ID, IPC3_IDX);

	status = pl320_initialize_hardware(IPC3_IDX, device_address,
					   core_channel_mask,
					   IPC3_USEABLE_MAILBOX_MASK);
	if (status == PL320_STATUS_SUCCESS) {
		ipc_initialized[IPC3_IDX] = 1;
		set_bit(IPC3_IDX, &enabled_mailboxes);
	} else {
		xpc_debug(0, "Error initializing IPC3: status:%u\n", status);
		return -1;
	}

	for (i = 0; i < nr_vecs && i < 32; ++i) {
		if (core_channel_mask & (0x1 << i)) {
			local_irq = pci_irq_vector(pci_dev, i);
			irq_table[local_irq] = (IPC3_IDX << 6) | i;
			xpc_debug(2,
				  "registering interrupt number %i/32 as %i\n",
				  i, local_irq);
			ret = request_irq(local_irq, xpc_irq_handler, 0,
					  "MERO_XPC", (void *)xpc_irq_handler);
			if (ret < 0) {
				xpc_debug(
					0,
					"unable to register of interrupt %i\n",
					i);
				xpc_debug(0, "error %i\n", ret);
			} else {
				xpc_debug(2, "registering irq_table[%i] = %i\n",
					  local_irq, (IPC3_IDX << 6) | i);
				core_channel_mask |= (1 << i);
			}
		} else {
			core_channel_mask = core_channel_mask & ~(0x1 << i);
		}
	}
	ipc_count += 1;
	core_channel_mask &= 0xFFFFF000;
	set_core_channel_mask(CORE_ID_X86, IPC3_IDX, core_channel_mask);

	dev_info(dev, "Successfully probed PCI device\n");

	return 0;
}
static void cvip_remove(struct pci_dev *pci_dev)
{
	pcim_iounmap_regions(pci_dev, 1 << 0);
	pci_set_drvdata(pci_dev, NULL);
	cvip_p = NULL;
}

static const struct pci_device_id cvip_pci_tbl[] = {
	{ PCI_VDEVICE(AMD, PCI_DEVICE_ID_CVIP) },
	{ 0 }
};

MODULE_DEVICE_TABLE(pci, cvip_pci_tbl);

static struct pci_driver cvip_driver = {
	.name = "cvip_xpc",
	.id_table = cvip_pci_tbl,
	.probe = cvip_probe,
	.remove = cvip_remove,
};
#endif

static int __init xpc_drv_init(void)
{
	int i;
	int global_mbox_id;
#if defined(__i386__) || defined(__amd64__)
	int status;
	struct pci_dev *dev = NULL;
#endif

	for (i = 0; i < XPC_NUM_VCHANNELS; ++i) {
		init_waitqueue_head(&notification_mailbox[i].wait_queue);
		init_waitqueue_head(&command_mailbox[i].wait_queue);
		init_waitqueue_head(&queue_mailbox[i].wait_queue);
		command_handlers[i] = NULL;
		command_handler_args[i] = NULL;
		notification_handlers[i] = NULL;
		notification_handler_args[i] = NULL;
	}
	memset(registered_user_subchannels, 0,
	       sizeof(registered_user_subchannels));
	memset(registered_kernel_subchannels, 0,
	       sizeof(registered_kernel_subchannels));
	spin_lock_init(&registered_queue_lock);

	// Set initial xstat states
	if (XPC_CORE_ID == CORE_ID_A55) {
		xstat_struct.cvip_core_info.core_state =
			XSTAT_STATE_INITIALIZING;
		xstat_struct.x86_core_info.core_state = XSTAT_STATE_OFFLINE;
		xstat_struct.cvip_core_info.wearable_state =
			XSTAT_WEARABLE_STATE_OFFLINE;
	} else {
		xstat_struct.x86_core_info.core_state =
			XSTAT_STATE_INITIALIZING;
		xstat_struct.cvip_core_info.core_state = XSTAT_STATE_OFFLINE;
		xstat_struct.cvip_core_info.wearable_state =
			XSTAT_WEARABLE_STATE_OFFLINE;
	}
	init_waitqueue_head(&xstat_struct.wq);
	spin_lock_init(&xstat_struct.lock);

	for (i = 0; i < 512; ++i) {
		irq_table[i] = -1;
	}

	xpc_initialize_global_mailboxes();

	for (global_mbox_id = 0; global_mbox_id < NUM_MAILBOXES;
	     ++global_mbox_id) {
		tasklet_init(&(mailboxes[global_mbox_id].tasklet),
			     &xpc_irq_tasklet, (unsigned long)global_mbox_id);
	}

#if defined(__i386__) || defined(__amd64__)
	status = pci_register_driver(&cvip_driver);
	dev = pci_get_device(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_CVIP, dev);
	cvip_probe(dev, NULL);
	xpc_debug(2, "xpc pcie_driver_register status %i\n", status);
#endif

	platform_driver_register(&xpc_pldriver);
	create_fs_device();
	(void)(xpc_match[0]); //  Prevents "unused variable 'xpc_match'" error

	// Create and start xstat kthread
	xstat_struct.task =
		kthread_run(xstat_kthread, NULL, XSTAT_KTHREAD_NAME);

	if (XPC_CORE_ID == CORE_ID_A55) {
		xdebug_init();
	}

	atomic_set(&xpc_drv_inited, 1);

	if (XPC_CORE_ID == CORE_ID_X86) {
		// If on x86, set the state to MODULES_LOADED.
		// Might not be the best place but the x86 modules are built in.
		xstat_set_state(XSTAT_STATE_MODULES_LOADED);
	}

	return 0;
}

static void __exit xpc_drv_exit(void)
{
	int i = 0;

	atomic_set(&xpc_drv_inited, 0);

	if (xstat_struct.task) {
		kthread_stop(xstat_struct.task);
	}

	if (XPC_CORE_ID == CORE_ID_A55) {
		xdebug_stop();
	}

#if defined(__i386__) || defined(__amd64__)
	pci_unregister_driver(&cvip_driver);
#else
	platform_driver_unregister(&xpc_pldriver);
#endif
	for (i = 0; i < 512; ++i) {
		if (irq_table[i] != -1) {
			free_irq(i, (void *)xpc_irq_handler);
		}
	}
	destroy_fs_device();
	xpc_debug(2, "mero xpc exited\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pzientara_siliconscapes@magicleap.com");
MODULE_AUTHOR("kirick_siliconscapes@magicleap.com");
MODULE_AUTHOR("cpeacock_tklabs@magicleap.com");
MODULE_DESCRIPTION("Mero XPC Kernel Module - Cross Platform Communication");
MODULE_VERSION("1.03");

#if defined(__i386__) || defined(__amd64__)
module_init(xpc_drv_init);
#else
core_initcall(xpc_drv_init);
#endif
module_exit(xpc_drv_exit);

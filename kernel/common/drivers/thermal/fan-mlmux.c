// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/acpi.h>
#include <linux/ml-mux-client.h>
#include <linux/debugfs.h>
#include <linux/thermal.h>
#include "fan-mlmux.h"

#define CREATE_TRACE_POINTS
#include "fan-mlmux-trace.h"

#define FAN_MLMUX_CHANNEL_NAME		"TM_FAN_CHAN"
#define FAN_MLMUX_MAX_INSTANCE		(1)

#define FAN_MLMUX_TX_ACK_TIMEOUT	(2000)
#define FAN_MLMUX_RPM_TOL		(10)
#define FAN_MLMUX_UEVENT_SIZE		(24)

enum fan_mlmux_event_type {
	FAN_MLMUX_EVENT_OPEN,
	FAN_MLMUX_EVENT_CLOSE,
	FAN_MLMUX_EVENT_REGISTER,
};

enum fan_mlmux_instance_state {
	FAN_MLMUX_REGISTERED,
	FAN_MLMUX_FAULT,
	FAN_MLMUX_DEBUG,
};

struct fan_mlmux_event_register_data {
	u8 instance_id;
};

struct fan_mlmux_event {
	struct list_head list;
	enum fan_mlmux_event_type type;
	size_t len;
	uint8_t payload[0];
};

struct fan_mlmux_priv;

struct fan_mlmux_instance {
	struct fan_mlmux_priv *priv;
	struct thermal_cooling_device *cdev;
	uint8_t id;
	uint32_t hw_rpm;
	enum fan_mlmux_dir dir;
	unsigned long state;
	enum fan_mlmux_fault fault;

	unsigned long max_state;
	unsigned long cur_state;
	uint32_t *rpm_table;
	uint32_t reported_rpm;
};

struct fan_mlmux_priv {
	struct platform_device *pdev;
	struct ml_mux_client client;
	struct fan_mlmux_instance instance[FAN_MLMUX_MAX_INSTANCE];
	struct work_struct event_work;
	struct list_head event_queue;
	spinlock_t event_lock;
	struct dentry *dentry;
	struct completion ack_rcvd;
	bool boot_completed;
};

static DEFINE_MUTEX(fan_mlmux_msg);

static int fan_mlmux_send_msg_with_ack(struct fan_mlmux_instance *instance,
				       uint32_t len,
				       struct fan_mlmux_tx_msg *msg)
{
	int ret;
	unsigned long jiffies_left;
	int t = msecs_to_jiffies(FAN_MLMUX_TX_ACK_TIMEOUT);
	struct completion *complete;

	mutex_lock(&fan_mlmux_msg);

	complete = &instance->priv->ack_rcvd;

	reinit_completion(complete);
	ret = ml_mux_send_msg(instance->priv->client.ch, len, msg);
	if (ret) {
		dev_err_ratelimited(&instance->priv->pdev->dev,
			"Failed to send FAN_MLMUX_TX message\n");
		goto unlock_mutex;
	}

	jiffies_left = wait_for_completion_timeout(complete, t);
	if (!jiffies_left) {
		dev_err_ratelimited(&instance->priv->pdev->dev,
			"Timeout on FAN_MLMUX_TX response\n");
		ret = -ETIMEDOUT;
	}

unlock_mutex:
	mutex_unlock(&fan_mlmux_msg);

	return ret;
}

static DEFINE_MUTEX(fan_mlmux_uevent);

static void fan_mlmux_notify_user_rpm(struct device *dev, uint32_t rpm)
{
	struct fan_mlmux_priv *priv = dev_get_drvdata(dev);
	char fan_rpm[FAN_MLMUX_UEVENT_SIZE];
	char *envp[2];

	/* skip sending the same RPM */
	if (priv->instance[0].reported_rpm != rpm || !priv->boot_completed) {
		mutex_lock(&fan_mlmux_uevent);
		snprintf(fan_rpm, sizeof(fan_rpm), "FAN_RPM=%u", rpm);
		envp[0] = fan_rpm;
		envp[1] = NULL;
		kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
		priv->instance[0].reported_rpm = rpm;
		mutex_unlock(&fan_mlmux_uevent);

		dev_dbg(dev, "Notify user space fan rpm: %u", rpm);
	}
}

static int fan_mlmux_state_to_rpm(struct fan_mlmux_instance *instance)
{
	struct fan_mlmux_tx_msg msg;
	uint32_t rpm = instance->rpm_table[instance->cur_state];
	int ret;

	msg.hdr.msg_type = FAN_MLMUX_TX_SET_RPM;
	msg.hdr.instance_id = instance->id;
	msg.rpm = __cpu_to_le16(rpm);

	trace_fan_mlmux_set(instance->id, rpm, instance->cur_state);

	ret = fan_mlmux_send_msg_with_ack(instance,
					   sizeof(msg.hdr) + sizeof(msg.rpm),
					   &msg);
	if (!ret)
		fan_mlmux_notify_user_rpm(&instance->priv->pdev->dev, rpm);

	return ret;
}

static int fan_mlmux_status_read(struct seq_file *s, void *priv)
{
	struct fan_mlmux_instance *instance = s->private;
	struct fan_mlmux_tx_msg msg;
	int ret;

	seq_printf(s, "State: 0x%lx\n", instance->state);
	if (!test_bit(FAN_MLMUX_REGISTERED, &instance->state)) {
		seq_puts(s, "FAN not available\n");
		return 0;
	}

	if (test_bit(FAN_MLMUX_FAULT, &instance->state)) {
		seq_puts(s, "Fault: ");
		switch (instance->fault) {
		case FAN_MLMUX_FAULT_START:
			seq_puts(s, "START\n");
			break;
		case FAN_MLMUX_FAULT_RAMP_UP:
			seq_puts(s, "RAMP_UP\n");
			break;
		case FAN_MLMUX_FAULT_MONITOR:
			seq_puts(s, "MONITOR\n");
			break;
		case FAN_MLMUX_FAULT_HARDWARE:
			seq_puts(s, "HARDWARE\n");
			break;
		}
		return 0;
	}

	msg.hdr.msg_type = FAN_MLMUX_TX_GET_STATUS;
	msg.hdr.instance_id = instance->id;

	ret = fan_mlmux_send_msg_with_ack(instance, sizeof(msg.hdr), &msg);
	if (ret)
		return ret;

	seq_printf(s, "CUR RPM: %u\n", instance->hw_rpm);

	if (instance->hw_rpm)
		seq_printf(s, "DIR: %s\n",
			   instance->dir ? "REVERSE" : "FORWARD");

	return ret;
}

static int fan_mlmux_status_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, fan_mlmux_status_read, inode->i_private);
}

static const struct file_operations fops_status = {
	.open = fan_mlmux_status_seq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};

static ssize_t fan_mlmux_set_read(struct file *file, char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	char buf[] = "<A><[RPM][F/R]> A: Auto Mode, F:Forward, R:Reverse\n";

	return simple_read_from_buffer(user_buf, count, ppos, buf, sizeof(buf));
}

static ssize_t fan_mlmux_set_write(struct file *file,
				   const char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	struct fan_mlmux_instance *instance = file->private_data;
	int ret;
	bool has_dir = false;
	enum fan_mlmux_dir dir;
	char buf[96] = {0};
	char *tmp;
	unsigned int rpm;
	char c = 0;
	struct fan_mlmux_tx_msg msg;
	uint32_t len;

	if (!test_bit(FAN_MLMUX_REGISTERED, &instance->state))
		return -ENODEV;

	ret = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, user_buf,
				     count);

	if (ret <= 0)
		return ret;

	tmp = strchr(buf, '\n');
	if (tmp) {
		*tmp = '\0';
		len = tmp - buf;
	} else {
		len = strlen(buf);
	}

	if (!len)
		return -EINVAL;

	c = buf[len - 1];
	switch (c) {
	case 'F':
	case 'f':
		dir = FAN_MLMUX_DIR_FORWARD;
		has_dir = true;
		buf[len - 1] = '\0';
		break;
	case 'R':
	case 'r':
		dir = FAN_MLMUX_DIR_REVERSE;
		has_dir = true;
		buf[len - 1] = '\0';
		break;
	case 'a':
	case 'A':
		fan_mlmux_state_to_rpm(instance);

		clear_bit(FAN_MLMUX_DEBUG, &instance->state);
		return count;
	default:
		if (c == '\0' || isdigit(c))
			break;
		return -EINVAL;
	}

	if (has_dir) {
		msg.hdr.msg_type = FAN_MLMUX_TX_SET_DIR;
		msg.hdr.instance_id = instance->id;
		msg.dir = dir;
		ret = fan_mlmux_send_msg_with_ack(instance,
				sizeof(msg.hdr) + sizeof(msg.dir), &msg);
		if (ret)
			return ret;
	}
	ret = kstrtouint(buf, 10, &rpm);
	if (ret)
		return ret;

	msg.hdr.msg_type = FAN_MLMUX_TX_SET_RPM;
	msg.hdr.instance_id = instance->id;
	msg.rpm = __cpu_to_le16(rpm);
	ret = fan_mlmux_send_msg_with_ack(instance,
			sizeof(msg.hdr) + sizeof(msg.rpm), &msg);
	if (ret)
		return ret;

	set_bit(FAN_MLMUX_DEBUG, &instance->state);

	return 0;
}

static const struct file_operations fops_set = {
	.read = fan_mlmux_set_read,
	.write = fan_mlmux_set_write,
	.open = simple_open,
	.llseek = default_llseek,
	.owner = THIS_MODULE,
};

static int fan_mlmux_init_debugfs(struct fan_mlmux_priv *priv)
{
	struct fan_mlmux_instance *instance;
	int i;
	struct dentry *dentry;
	char name[16];

	priv->dentry = debugfs_create_dir("mlmux_fan", NULL);

	if (!priv->dentry)
		return -ENOMEM;

	for (i = 0; i < FAN_MLMUX_MAX_INSTANCE; i++) {
		instance = &priv->instance[i];

		if (FAN_MLMUX_MAX_INSTANCE > 1) {
			snprintf(name, sizeof(name), "fan%d", i);
			dentry = debugfs_create_dir(name, priv->dentry);
		} else {
			dentry = priv->dentry;
		}

		debugfs_create_file("status", 0400, dentry, instance,
				    &fops_status);

		debugfs_create_file("set", 0400, dentry, instance, &fops_set);
	}

	return 0;
}

static int fan_mlmux_get_max_state(struct thermal_cooling_device *cdev,
				   unsigned long *state)
{
	struct fan_mlmux_instance *instance = cdev->devdata;

	*state = instance->max_state;

	return 0;
}

static int fan_mlmux_get_cur_state(struct thermal_cooling_device *cdev,
				   unsigned long *state)
{
	struct fan_mlmux_instance *instance = cdev->devdata;

	*state = instance->cur_state;

	return 0;
}

static int fan_mlmux_set_cur_state(struct thermal_cooling_device *cdev,
				   unsigned long state)
{
	struct fan_mlmux_instance *instance = cdev->devdata;

	if (WARN_ON(state > instance->max_state))
		return -EINVAL;

	if (instance->cur_state == state)
		return 0;

	instance->cur_state = state;

	if (test_bit(FAN_MLMUX_DEBUG, &instance->state))
		return 0;

	return fan_mlmux_state_to_rpm(instance);
}

static struct thermal_cooling_device_ops fan_mlmux_cooling_ops = {
	.get_max_state = fan_mlmux_get_max_state,
	.get_cur_state = fan_mlmux_get_cur_state,
	.set_cur_state = fan_mlmux_set_cur_state,
};

static void fan_mlmux_event_register(struct fan_mlmux_priv *priv,
				     struct fan_mlmux_event_register_data *reg)
{
	struct fan_mlmux_instance *instance;
	struct thermal_cooling_device *cdev;
	struct fan_mlmux_tx_msg msg;

	if (reg->instance_id >= FAN_MLMUX_MAX_INSTANCE) {
		dev_err(&priv->pdev->dev, "Invalid Instance ID %u\n",
			reg->instance_id);
		return;
	}

	instance = &priv->instance[reg->instance_id];

	if (test_bit(FAN_MLMUX_REGISTERED, &instance->state)) {
		dev_err(&priv->pdev->dev, "FAN instance %u already registered\n",
			reg->instance_id);
		return;
	}

	instance->priv = priv;
	instance->id = reg->instance_id;
	instance->cur_state = instance->max_state + 1;
	instance->reported_rpm = 0;

	msg.hdr.msg_type = FAN_MLMUX_TX_REGISTER_ACK;
	msg.hdr.instance_id = instance->id;
	ml_mux_send_msg(instance->priv->client.ch, sizeof(msg.hdr), &msg);
	set_bit(FAN_MLMUX_REGISTERED, &instance->state);

	cdev = thermal_of_cooling_device_register(NULL, "Fan", instance,
						  &fan_mlmux_cooling_ops);
	if (IS_ERR(cdev))
		goto fail_mlmux;

	instance->cdev = cdev;

	return;

fail_mlmux:
	msg.hdr.msg_type = FAN_MLMUX_TX_REGISTER_FAILED;
	msg.hdr.instance_id = instance->id;
	ml_mux_send_msg(instance->priv->client.ch, sizeof(msg.hdr), &msg);
}

static void fan_mlmux_event_close(struct fan_mlmux_priv *priv)
{
	struct fan_mlmux_instance *instance;
	int i;

	for (i = 0; i < FAN_MLMUX_MAX_INSTANCE; i++) {
		instance = &priv->instance[i];

		if (!test_bit(FAN_MLMUX_REGISTERED, &instance->state))
			continue;

		thermal_cooling_device_unregister(instance->cdev);

		memset(&priv->instance[i], 0, sizeof(priv->instance[i]));
	}
}

static void fan_mlmux_event_worker(struct work_struct *work)
{
	struct fan_mlmux_priv *priv =
		container_of(work, struct fan_mlmux_priv, event_work);
	struct fan_mlmux_event *event, *e;

	spin_lock(&priv->event_lock);

	list_for_each_entry_safe(event, e, &priv->event_queue, list) {

		list_del(&event->list);
		spin_unlock(&priv->event_lock);

		switch (event->type) {
		case FAN_MLMUX_EVENT_OPEN:
			/* Nothing to be done */
			break;
		case FAN_MLMUX_EVENT_CLOSE:
			fan_mlmux_event_close(priv);
			break;
		case FAN_MLMUX_EVENT_REGISTER:
			fan_mlmux_event_register(priv, (void *)event->payload);
			break;
		default:
			dev_err(&priv->pdev->dev, "Unknown event %d\n",
				event->type);
			break;
		}

		kfree(event);
		spin_lock(&priv->event_lock);
	}
	spin_unlock(&priv->event_lock);
}

static int fan_mlmux_event_post(struct fan_mlmux_priv *priv,
				  enum fan_mlmux_event_type type,
				  size_t len, void *payload)
{
	struct fan_mlmux_event *event;

	event = kzalloc(sizeof(*event) + len, GFP_KERNEL);
	if (!event)
		return -ENOMEM;

	event->type = type;
	event->len = len;
	if (len && payload)
		memcpy(event->payload, payload, len);

	spin_lock(&priv->event_lock);
	list_add_tail(&event->list, &priv->event_queue);
	spin_unlock(&priv->event_lock);

	queue_work(system_power_efficient_wq, &priv->event_work);

	return 0;
}

static void fan_mlmux_open(struct ml_mux_client *cli, bool is_open)
{
	struct fan_mlmux_priv *priv =
		container_of(cli, struct fan_mlmux_priv, client);

	if (is_open)
		fan_mlmux_event_post(priv, FAN_MLMUX_EVENT_OPEN, 0, NULL);
	else
		fan_mlmux_event_post(priv, FAN_MLMUX_EVENT_CLOSE, 0, NULL);
}

static void fan_mlmux_recv(struct ml_mux_client *cli, uint32_t len, void *pkt)
{
	struct fan_mlmux_priv *priv =
		container_of(cli, struct fan_mlmux_priv, client);
	struct fan_mlmux_rx_msg *msg = pkt;
	struct fan_mlmux_instance *instance;
	struct fan_mlmux_event_register_data reg_data;

	if (len < sizeof(struct fan_mlmux_rx_msg_hdr) || !msg) {
		dev_err(&priv->pdev->dev, "Discarding invalid MLMUX message: %u, %p\n",
			len, pkt);
		return;
	}

	switch (msg->hdr.msg_type) {
	case FAN_MLMUX_RX_REGISTER:
		reg_data.instance_id = msg->hdr.instance_id;
		fan_mlmux_event_post(priv, FAN_MLMUX_EVENT_REGISTER,
				     sizeof(reg_data), &reg_data);
		break;

	case FAN_MLMUX_RX_STATUS:
		if (len < sizeof(msg->hdr) + sizeof(msg->status)) {
			dev_err(&priv->pdev->dev, "Invalid status message %u\n",
				len);
			break;
		}

		if (msg->hdr.instance_id >= FAN_MLMUX_MAX_INSTANCE) {
			dev_err(&priv->pdev->dev, "Invalid Instance ID %u\n",
				msg->hdr.instance_id);
			break;
		}

		instance = &priv->instance[msg->hdr.instance_id];
		instance->hw_rpm = __le16_to_cpu(msg->status.rpm);
		instance->dir = msg->status.dir;

		trace_fan_mlmux_status(instance->id, instance->hw_rpm,
				       instance->dir);

		complete(&instance->priv->ack_rcvd);
		break;

	case FAN_MLMUX_RX_RPM_ACK:
		instance = &priv->instance[msg->hdr.instance_id];
		if (msg->ack_status)
			dev_err_ratelimited(&priv->pdev->dev, "Set RPM ack err = %d\n",
				msg->ack_status);
		complete(&instance->priv->ack_rcvd);
		break;

	case FAN_MLMUX_RX_DIR_ACK:
		instance = &priv->instance[msg->hdr.instance_id];
		if (msg->ack_status)
			dev_err_ratelimited(&priv->pdev->dev, "Set DIR ack err = %d\n",
				msg->ack_status);
		complete(&instance->priv->ack_rcvd);
		break;

	case FAN_MLMUX_RX_SUSPEND_ACK:
		if (msg->ack_status)
			dev_err_ratelimited(&priv->pdev->dev, "x86 suspend ack err = %d\n",
				msg->ack_status);
		complete(&priv->ack_rcvd);
		break;

	case FAN_MLMUX_RX_RESUME_ACK:
		if (msg->ack_status)
			dev_err_ratelimited(&priv->pdev->dev,
				"x86 resume ack err = %d\n", msg->ack_status);
		complete(&priv->ack_rcvd);
		break;

	case FAN_MLMUX_RX_FAULT:
		if (len < sizeof(msg->hdr) + sizeof(msg->fault)) {
			dev_err(&priv->pdev->dev, "Invalid fault message %u\n",
				len);
			break;
		}

		if (msg->hdr.instance_id >= FAN_MLMUX_MAX_INSTANCE) {
			dev_err(&priv->pdev->dev, "Invalid Instance ID %u for Fault\n",
				msg->hdr.instance_id);
			break;
		}

		instance = &priv->instance[msg->hdr.instance_id];
		instance->fault = msg->fault;
		set_bit(FAN_MLMUX_FAULT, &instance->state);

		dev_err(&priv->pdev->dev, "FAN-%u Fault detected: %d\n",
			msg->hdr.instance_id, msg->fault);
		break;

	default:
		dev_err(&priv->pdev->dev, "Unknown msg type: %d, len: %d\n",
			msg->hdr.msg_type, len);
		break;
	}
}

#ifdef CONFIG_ACPI
static int fan_mlmux_parse_acpi(struct fan_mlmux_priv *priv)
{
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *p;
	union acpi_object *elem1, *elem2;
	acpi_status status;
	struct fan_mlmux_instance *fmi;
	int i, j;
	int table_size;

	status = acpi_evaluate_object(ACPI_HANDLE(priv->client.dev),
			"_RPM", NULL, &output);
	if (ACPI_FAILURE(status)) {
		dev_err(priv->client.dev, "Cannot find _RPM method\n");
		goto err_parse_acpi;
	}

	p = output.pointer;
	if (!p || (p->type != ACPI_TYPE_PACKAGE)) {
		dev_err(priv->client.dev, "Invalid data\n");
		goto err_parse_acpi;
	}

	for (i = 0; i < FAN_MLMUX_MAX_INSTANCE; i++) {
		fmi = &priv->instance[i];
		table_size =  p->package.count / 2 * sizeof(*fmi->rpm_table);
		fmi->rpm_table = devm_kzalloc(&priv->pdev->dev, table_size,
			GFP_KERNEL);
		if (!fmi->rpm_table)
			goto err_parse_acpi;
		fmi->max_state = 0;
	}

	for (i = 0; i < p->package.count; i++) {
		elem1 = &p->package.elements[i++];
		if (strncmp(elem1->string.pointer, "step", 4))
			continue;
		elem2 = &p->package.elements[i];
		for (j = 0; j < FAN_MLMUX_MAX_INSTANCE; j++) {
			fmi = &priv->instance[j];
			*(fmi->rpm_table + (i / 2)) = elem2->integer.value;
			fmi->max_state++;
		}
	}

	ACPI_FREE(output.pointer);
	return 0;

err_parse_acpi:
	dev_err(priv->client.dev, "Parsing ACPI data error.\n");
	ACPI_FREE(output.pointer);
	return -ENOENT;
}
#else
static inline int fan_mlmux_parse_acpi(struct fan_mlmux_priv *priv)
{
	return -ENODEV;
}
#endif

static ssize_t boot_completed_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct fan_mlmux_priv *priv = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", priv->boot_completed);
}

static ssize_t boot_completed_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct fan_mlmux_priv *priv = dev_get_drvdata(dev);
	struct fan_mlmux_instance *instance = &priv->instance[0];
	bool input;
	int ret;

	if (!priv->boot_completed) {
		ret = strtobool(buf, &input);
		if (ret < 0) {
			dev_err(dev, "Failed to convert boot_completed value: %d\n",
					ret);
			return -EINVAL;
		}
		if (input)
			fan_mlmux_notify_user_rpm(dev,
				instance->rpm_table[instance->cur_state]);

		/* update boot_completed as the last step */
		priv->boot_completed = input;
		dev_dbg(dev, "boot_completed = %d\n", input);
	}
	return count;
}

/* sys/devices/platform/MXFN0000:00/boot_completed */
static DEVICE_ATTR_RW(boot_completed);

static int fan_mlmux_probe(struct platform_device *pdev)
{
	int ret;
	struct fan_mlmux_priv *priv;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->pdev = pdev;
	priv->client.dev = &pdev->dev;
	priv->client.notify_open = fan_mlmux_open;
	priv->client.receive_cb = fan_mlmux_recv;

	ret = fan_mlmux_parse_acpi(priv);
	if (ret)
		goto end;

	INIT_WORK(&priv->event_work, fan_mlmux_event_worker);
	spin_lock_init(&priv->event_lock);
	INIT_LIST_HEAD(&priv->event_queue);

	ret = fan_mlmux_init_debugfs(priv);
	if (ret)
		goto end;

	ret = ml_mux_request_channel(&priv->client, FAN_MLMUX_CHANNEL_NAME);

	if (ret == ML_MUX_CH_REQ_OPENED)
		ret = fan_mlmux_event_post(priv, FAN_MLMUX_EVENT_OPEN, 0, NULL);

	if (ret) {
		dev_err(&pdev->dev, "MLMUX failed: %d\n", ret);
		goto end;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_boot_completed);
	if (ret < 0) {
		/* continue probe on non-fatal error */
		dev_err(&pdev->dev, "sysfs creation failed\n", ret);
	}

	priv->boot_completed = false;

	platform_set_drvdata(pdev, priv);

	init_completion(&priv->ack_rcvd);
end:
	return ret;
}

static int fan_mlmux_remove(struct platform_device *pdev)
{
	struct fan_mlmux_priv *priv = platform_get_drvdata(pdev);

	ml_mux_release_channel(priv->client.ch);

	flush_work(&priv->event_work);

	debugfs_remove_recursive(priv->dentry);

	device_remove_file(&pdev->dev, &dev_attr_boot_completed);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int fan_mlmux_suspend(struct device *dev)
{
	int ret;
	struct fan_mlmux_tx_msg msg;
	struct fan_mlmux_priv *priv = dev_get_drvdata(dev);

	msg.hdr.msg_type = FAN_MLMUX_TX_SUSPEND;
	msg.hdr.instance_id = 0;
	ret = fan_mlmux_send_msg_with_ack(&priv->instance[0],
					  sizeof(msg.hdr), &msg);

	dev_dbg(dev, "%s: %d", __func__, ret);
	return ret;
}

static int fan_mlmux_resume(struct device *dev)
{
	int i, ret;
	struct fan_mlmux_tx_msg msg;
	struct fan_mlmux_priv *priv = dev_get_drvdata(dev);
	uint32_t rpm;
	uint32_t tol;

	msg.hdr.msg_type = FAN_MLMUX_TX_RESUME;
	msg.hdr.instance_id = 0;
	ret = fan_mlmux_send_msg_with_ack(&priv->instance[0],
					  sizeof(msg.hdr), &msg);
	if (ret)
		goto final;

	msg.hdr.msg_type = FAN_MLMUX_TX_GET_STATUS;
	ret = fan_mlmux_send_msg_with_ack(&priv->instance[0],
					  sizeof(msg.hdr), &msg);
	if (ret)
		goto final;

	rpm = priv->instance[0].hw_rpm;
	for (i = 0; i < priv->instance[0].max_state; i++) {
		tol = priv->instance[0].rpm_table[i] * FAN_MLMUX_RPM_TOL / 100;
		if (rpm <= priv->instance[0].rpm_table[i] + tol &&
		    rpm >= priv->instance[0].rpm_table[i] - tol)
			break;
	}
	priv->instance[0].cur_state = i;
	dev_dbg(dev, "rpm = %d state = %d\n", rpm, i);
final:
	dev_dbg(dev, "%s %d", __func__, ret);
	return ret;
}

static const struct dev_pm_ops fan_mlmux_pm = {
	.suspend = fan_mlmux_suspend,
	.resume = fan_mlmux_resume,
};

#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id fan_mlmux_acpi_match[] = {
	{ "MXFN0000", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, fan_mlmux_acpi_match);
#endif

static struct platform_driver fan_mlmux_driver = {
	.probe = fan_mlmux_probe,
	.remove = fan_mlmux_remove,
	.driver = {
		.name = "mlmux-fan",
#ifdef CONFIG_PM_SLEEP
		.pm = &fan_mlmux_pm,
#endif
		.acpi_match_table = ACPI_PTR(fan_mlmux_acpi_match),
	}
};
module_platform_driver(fan_mlmux_driver);


MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FAN driver for Magic Leap MUX");

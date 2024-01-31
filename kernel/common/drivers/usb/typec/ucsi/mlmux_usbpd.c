// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021, Magic Leap, Inc. All rights reserved.
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

#include <linux/acpi.h>
#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/ml-mux-client.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>
#include <linux/usb/role.h>
#include "ucsi.h"

#define MLMUX_USBPD_CHAN_NAME	"USBPD_CHAN"
#define MLMUX_USBPD_UCSI_VERSION (0x0110)
/* MLMUX TX/RX messages */
struct mlmux_usbpd_ucsi_iface {
	uint16_t version;
	uint16_t reserved;
	uint32_t cci;
	uint64_t control;
	uint8_t msg_in[16];
	uint8_t msg_out[16];
} __attribute__((__packed__));

/* RX */
enum mlmux_usbpd_test_rx_type {
	MLMUX_USBPD_RX_INFO,
	MLMUX_USBPD_RX_UCSI_REGISTER,
	MLMUX_USBPD_RX_UCSI,
};

struct mlmux_usbpd_info {
	__le32 usb_host_present_type : 2;
	__le32 vbus_sts : 2;
	__le32 conn_state : 3;
	__le32 dp_conn : 1;
} __attribute__((__packed__));

struct mlmux_usbpd_rx_info {
	__le32 partner_atch : 1;
	__le32 cable_atch : 1;
	__le32 host : 1;
	__le32 source : 1;
	struct mlmux_usbpd_info info;
	__le32 reserved1 : 20;
	__le32 reserved2[15];
} __attribute__((__packed__));

struct mlmux_usbpd_rx_pkt {
	uint8_t type;
	union {
		struct mlmux_usbpd_rx_info info;
		struct mlmux_usbpd_ucsi_iface ucsi_data;
	} u;
} __attribute__((__packed__));

#define MLMUX_USB_PD_RX_SIZE(msg) \
	(sizeof(struct mlmux_usbpd_rx_pkt) - \
	sizeof(((struct mlmux_usbpd_rx_pkt *)0)->u) + \
	sizeof(((struct mlmux_usbpd_rx_pkt *)0)->u.msg))

/* TX */
enum mlmux_usbpd_tx_type {
	MLMUX_USBPD_TX_UCSI,
};

struct mlmux_usbpd_tx_pkt {
	uint8_t type;
	union {
		struct mlmux_usbpd_ucsi_iface ucsi_data;
	};
} __attribute__((__packed__));

/* MLMUX_USBPD private */
#define MLMUX_USBPD_UCSI_RESET_PEND (0)
#define MLMUX_USBPD_UCSI_CMD_PEND (1)

struct mlmux_usbpd_data {
	struct device *dev;
	struct ucsi *ucsi;
	struct ml_mux_client client;
	struct usb_role_switch *role_sw;
	struct kobject *pd_kobj;

	struct mutex lock;
	struct work_struct event_work;
	struct list_head event_queue;
	spinlock_t event_lock;

	struct mlmux_usbpd_ucsi_iface ucsi_iface;

	unsigned long flags;
	struct completion wr_complete;
	struct completion rd_complete;
};

enum mlmux_usbpd_event_type {
	MLMUX_USBPD_EVENT_OPEN,
	MLMUX_USBPD_EVENT_CLOSE,
	MLMUX_USBPD_EVENT_INFO,
	MLMUX_USBPD_EVENT_UCSI_REG,
	MLMUX_USBPD_EVENT_UCSI,
};

struct mlmux_usbpd_event {
	struct list_head node;
	enum mlmux_usbpd_event_type type;
	size_t len;
	uint8_t payload[0];
};

#define CHK_BOUNDS(x, y) ((const char *)((y > ARRAY_SIZE(x) - 1) ? 0 : x[y]))

static struct mlmux_usbpd_info pd_info;

static const char * const usbHostPrst_txt[] = {
	"far end:VBUS[n],PD pwr.role SRC",
	"far end:VBUS[y],PD[y],USB-comm[n] (med.chrg)",
	"far end:VBUS[y],PD[n]",
	"far end:VBUS[y],PD[y],USB-comm[y]"
};

static const char * const vbusSts_txt[] = {
	"VBUS < 0.8v",
	"VBUS <= 5v",
	"VBUS nego. by PD",
	"VBUS OOR"
};

static const char * const connState_txt[] = {
	"No conn.",
	"port is disabled",
	"audio conn.",
	"dbg. conn.",
	"No conn., Ra detcd.",
	"rsvd.",
	"conn. prst.",
	"conn. prst., PD patnr. swpd. to sink"
};

static ssize_t usbHostPrst_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%s\n", CHK_BOUNDS(usbHostPrst_txt,
						pd_info.usb_host_present_type));
}
static DEVICE_ATTR_RO(usbHostPrst);

static ssize_t vbusSts_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "%s\n", CHK_BOUNDS(vbusSts_txt, pd_info.vbus_sts));
}
static DEVICE_ATTR_RO(vbusSts);

static ssize_t connState_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	return sprintf(buf, "%s\n", CHK_BOUNDS(connState_txt,
						pd_info.conn_state));
}
static DEVICE_ATTR_RO(connState);

static ssize_t dpConnPrst_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	return sprintf(buf, "%s\n", ((pd_info.dp_conn == 1) ? "Yes" : "No"));
}
static DEVICE_ATTR_RO(dpConnPrst);

static struct attribute *mlmux_usbpd_attr[] = {
	&dev_attr_usbHostPrst.attr,
	&dev_attr_vbusSts.attr,
	&dev_attr_connState.attr,
	&dev_attr_dpConnPrst.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = mlmux_usbpd_attr,
};

static void mlmux_usbpd_set_info(struct mlmux_usbpd_rx_info *info,
				 struct mlmux_usbpd_data *data)
{
	pd_info.usb_host_present_type =
			__le32_to_cpu(info->info.usb_host_present_type);
	pd_info.vbus_sts = __le32_to_cpu(info->info.vbus_sts);
	pd_info.conn_state = __le32_to_cpu(info->info.conn_state);
	pd_info.dp_conn = __le32_to_cpu(info->info.dp_conn);
}

static void mlmux_usbpd_event_info(struct mlmux_usbpd_data *data,
				   struct mlmux_usbpd_event *event)
{
	struct mlmux_usbpd_rx_info *info =
		(struct mlmux_usbpd_rx_info *)event->payload;
	enum usb_role role_val;

	if (__le32_to_cpu(info->host))
		role_val = USB_ROLE_HOST;
	else
		role_val = USB_ROLE_DEVICE;

	if (!__le32_to_cpu(info->partner_atch))
		role_val = USB_ROLE_NONE;

	mlmux_usbpd_set_info(info, data);
	usb_role_switch_set_role(data->role_sw, role_val);
}

static void mlmux_usbpd_event_ucsi_reg(struct mlmux_usbpd_data *data,
				       struct mlmux_usbpd_event *event)
{
	struct mlmux_usbpd_ucsi_iface *ucsi_data =
		(struct mlmux_usbpd_ucsi_iface *)event->payload;
	int ret;

	mutex_lock(&data->lock);
	data->ucsi_iface.version = ucsi_data->version;
	data->ucsi_iface.cci = ucsi_data->cci;
	memcpy(data->ucsi_iface.msg_in, ucsi_data->msg_in,
	       sizeof(data->ucsi_iface.msg_in));
	mutex_unlock(&data->lock);

	ret = ucsi_register(data->ucsi);
	if (ret)
		dev_err(data->dev, "fail ucsi reg %d\n", ret);
}

static void mlmux_usbpd_event_ucsi(struct mlmux_usbpd_data *data,
				   struct mlmux_usbpd_event *event)
{
	struct mlmux_usbpd_ucsi_iface *ucsi_data =
		(struct mlmux_usbpd_ucsi_iface *)event->payload;

	mutex_lock(&data->lock);
	memcpy(&data->ucsi_iface, ucsi_data, sizeof(data->ucsi_iface));

	if (UCSI_CCI_CONNECTOR(ucsi_data->cci))
		ucsi_connector_change(data->ucsi,
				      UCSI_CCI_CONNECTOR(ucsi_data->cci));

	if (test_bit(MLMUX_USBPD_UCSI_CMD_PEND, &data->flags) &&
	   ucsi_data->cci & (UCSI_CCI_ACK_COMPLETE | UCSI_CCI_COMMAND_COMPLETE))
		complete(&data->wr_complete);
	mutex_unlock(&data->lock);
}

static void mlmux_usbpd_work(struct work_struct *work)
{
	struct mlmux_usbpd_data *data =
		container_of(work, struct mlmux_usbpd_data, event_work);
	struct mlmux_usbpd_event *event, *e;

	spin_lock(&data->event_lock);
	list_for_each_entry_safe(event, e, &data->event_queue, node) {

		list_del(&event->node);
		spin_unlock(&data->event_lock);

		switch (event->type) {
		case MLMUX_USBPD_EVENT_OPEN:
			break;
		case MLMUX_USBPD_EVENT_CLOSE:
			break;
		case MLMUX_USBPD_EVENT_INFO:
			mlmux_usbpd_event_info(data, event);
			break;
		case MLMUX_USBPD_EVENT_UCSI_REG:
			mlmux_usbpd_event_ucsi_reg(data, event);
			break;
		case MLMUX_USBPD_EVENT_UCSI:
			mlmux_usbpd_event_ucsi(data, event);
			break;
		default:
			dev_err(data->dev, "Unknown event %d\n",
				event->type);
			break;
		}

		kfree(event);
		spin_lock(&data->event_lock);
	}
	spin_unlock(&data->event_lock);
}

static int mlmux_usbpd_event_post(struct mlmux_usbpd_data *data,
				  enum mlmux_usbpd_event_type type,
				  const void *payload, size_t len)
{
	struct mlmux_usbpd_event *event;

	event = kzalloc(sizeof(*event) + len, GFP_KERNEL);
	if (!event)
		return -ENOMEM;

	event->type = type;
	event->len = len;
	if (len && data)
		memcpy(event->payload, payload, len);

	spin_lock(&data->event_lock);
	list_add_tail(&event->node, &data->event_queue);
	spin_unlock(&data->event_lock);

	schedule_work(&data->event_work);

	return 0;
}

static void mlmux_usbpd_open(struct ml_mux_client *cli, bool is_open)
{
	struct mlmux_usbpd_data *data =
		container_of(cli, struct mlmux_usbpd_data, client);

	if (is_open)
		mlmux_usbpd_event_post(data, MLMUX_USBPD_EVENT_OPEN, NULL, 0);
	else
		mlmux_usbpd_event_post(data, MLMUX_USBPD_EVENT_CLOSE, NULL, 0);
}

static inline int mlmux_size_check(struct mlmux_usbpd_data *data,
				   uint32_t type, uint32_t len, size_t exp)
{
	if (len != exp) {
		dev_err(data->dev, "msg type %d, unexpected length. %d vs %zu\n",
			type, len, exp);
		return -EMSGSIZE;
	}
	return 0;
}

static void mlmux_usbpd_recv(struct ml_mux_client *cli, uint32_t len, void *msg)
{
	struct mlmux_usbpd_data *data =
		container_of(cli, struct mlmux_usbpd_data, client);
	struct mlmux_usbpd_rx_pkt *pkt = msg;

	if (len < sizeof(pkt->type) || !msg)
		return;

	switch (pkt->type) {
	case MLMUX_USBPD_RX_INFO:
		if (mlmux_size_check(data, pkt->type, len,
				     MLMUX_USB_PD_RX_SIZE(info)))
			break;

		mlmux_usbpd_event_post(data, MLMUX_USBPD_EVENT_INFO,
				       &pkt->u.info, sizeof(pkt->u.info));
		break;
	case MLMUX_USBPD_RX_UCSI_REGISTER:
		if (mlmux_size_check(data, pkt->type, len,
				     MLMUX_USB_PD_RX_SIZE(ucsi_data)))
			break;

		mlmux_usbpd_event_post(data, MLMUX_USBPD_EVENT_UCSI_REG,
				       &pkt->u.ucsi_data,
				       sizeof(pkt->u.ucsi_data));
		break;
	case MLMUX_USBPD_RX_UCSI:
		if (mlmux_size_check(data, pkt->type, len,
				     MLMUX_USB_PD_RX_SIZE(ucsi_data)))
			break;

		mlmux_usbpd_event_post(data, MLMUX_USBPD_EVENT_UCSI,
				       &pkt->u.ucsi_data,
				       sizeof(pkt->u.ucsi_data));
		break;
	default:
		dev_err(data->dev, "Unknown msg: %d, %d\n", pkt->type, len);
		break;
	}
}

static int mlmux_usbpd_send(const struct mlmux_usbpd_data *data)
{
	struct mlmux_usbpd_tx_pkt tx_pkt = {0};

	tx_pkt.type = MLMUX_USBPD_TX_UCSI;
	memcpy(&tx_pkt.ucsi_data, &data->ucsi_iface, sizeof(tx_pkt.ucsi_data));

	return ml_mux_send_msg(data->client.ch, sizeof(tx_pkt), &tx_pkt);
}

static int mlmux_usbpd_ucsi_read(struct ucsi *ucsi, unsigned int offset,
				 void *val, size_t val_len)
{
	struct mlmux_usbpd_data *data = ucsi_get_drvdata(ucsi);

	mutex_lock(&data->lock);
	memcpy(val, ((uint8_t *)&data->ucsi_iface) + offset, val_len);
	memset(((uint8_t *)&data->ucsi_iface.cci), 0x00,
	       sizeof(data->ucsi_iface.cci));
	mutex_unlock(&data->lock);

	return 0;
}

static int mlmux_usbpd_ucsi_async_write(struct ucsi *ucsi, unsigned int offset,
					const void *val, size_t val_len)
{
	struct mlmux_usbpd_data *data = ucsi_get_drvdata(ucsi);
	int ret;

	memcpy((void *)&data->ucsi_iface + offset, val, val_len);
	ret = mlmux_usbpd_send(data);

	return ret;
}

static int mlmux_usbpd_ucsi_sync_write(struct ucsi *ucsi, unsigned int offset,
			       const void *val, size_t val_len)
{
	struct mlmux_usbpd_data *data = ucsi_get_drvdata(ucsi);
	int ret;


	mutex_lock(&data->lock);
	set_bit(MLMUX_USBPD_UCSI_CMD_PEND, &data->flags);

	memcpy((void *)&data->ucsi_iface + offset, val, val_len);
	ret = mlmux_usbpd_send(data);
	if (ret)
		goto err_clear_bit;

	mutex_unlock(&data->lock);

	if (!wait_for_completion_timeout(&data->wr_complete,
					 msecs_to_jiffies(5000)))
		ret = -ETIMEDOUT;

	mutex_lock(&data->lock);

err_clear_bit:
	clear_bit(MLMUX_USBPD_UCSI_CMD_PEND, &data->flags);
	mutex_unlock(&data->lock);

	return ret;
}


static const struct ucsi_operations mldb_usbpd_ucsi_ops = {
	.read = mlmux_usbpd_ucsi_read,
	.sync_write = mlmux_usbpd_ucsi_sync_write,
	.async_write = mlmux_usbpd_ucsi_async_write,
};

static int mlmux_usbpd_probe(struct platform_device *pdev)
{
	struct mlmux_usbpd_data *data;
	const struct software_node *swnode;
	struct fwnode_handle *fwnode;
	struct usb_role_switch *role_sw;
	int ret;

	swnode = software_node_find_by_name(NULL, "amd-dwc3-usb-sw");
	if (!swnode)
		return -EPROBE_DEFER;

	fwnode = software_node_fwnode(swnode);
	role_sw = usb_role_switch_find_by_fwnode(fwnode);
	if (IS_ERR_OR_NULL(role_sw)) {
		ret = -EINVAL;
		goto err_fwnode_put;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto err_role_put;
	}

	platform_set_drvdata(pdev, data);

	mutex_init(&data->lock);
	INIT_WORK(&data->event_work, mlmux_usbpd_work);
	spin_lock_init(&data->event_lock);
	INIT_LIST_HEAD(&data->event_queue);
	init_completion(&data->rd_complete);
	init_completion(&data->wr_complete);

	data->role_sw = role_sw;
	data->dev = &pdev->dev;
	data->client.dev = &pdev->dev;
	data->client.notify_open = mlmux_usbpd_open;
	data->client.receive_cb = mlmux_usbpd_recv;

	data->ucsi = ucsi_create(data->dev, &mldb_usbpd_ucsi_ops);
	if (IS_ERR(data->ucsi)) {
		ret = PTR_ERR(data->ucsi);
		goto err_mutex_destroy;
	}
	ucsi_set_drvdata(data->ucsi, data);

	ret = ml_mux_request_channel(&data->client, MLMUX_USBPD_CHAN_NAME);
	if (ret == ML_MUX_CH_REQ_OPENED)
		mlmux_usbpd_event_post(data, MLMUX_USBPD_EVENT_OPEN, NULL, 0);
	else if (ret != ML_MUX_CH_REQ_SUCCESS)
		goto err_mutex_destroy;

	data->pd_kobj = kobject_create_and_add("pd_info", &data->dev->kobj);
	if (!data->pd_kobj) {
		ret = -EINVAL;
		goto err_mutex_destroy;
	}

	ret = sysfs_create_group(data->pd_kobj, &attr_group);
	if (ret) {
		kobject_put(data->pd_kobj);
		goto err_mutex_destroy;
	}

	fwnode_handle_put(fwnode);

	return 0;

err_mutex_destroy:
	mutex_destroy(&data->lock);
err_role_put:
	usb_role_switch_put(role_sw);
err_fwnode_put:
	fwnode_handle_put(fwnode);

	return ret;
}

static int mlmux_usbpd_remove(struct platform_device *pdev)
{
	struct mlmux_usbpd_data *data = platform_get_drvdata(pdev);

	ml_mux_release_channel(data->client.ch);
	flush_work(&data->event_work);

	usb_role_switch_put(data->role_sw);
	ucsi_unregister(data->ucsi);
	ucsi_destroy(data->ucsi);
	mutex_destroy(&data->lock);

	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id mlmux_usbpd_acpi_match[] = {
	{ "MXPD1111", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, mlmux_usbpd_acpi_match);
#endif

static struct platform_driver mlmux_usbpd_driver = {
	.probe = mlmux_usbpd_probe,
	.remove = mlmux_usbpd_remove,
	.driver = {
		.name = "mlmux-usbpd",
		.acpi_match_table = ACPI_PTR(mlmux_usbpd_acpi_match),
	}
};
module_platform_driver(mlmux_usbpd_driver);


MODULE_AUTHOR("Ilya Tsunaev <itsunaev@magicleap.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MLMUX USB PD channel");

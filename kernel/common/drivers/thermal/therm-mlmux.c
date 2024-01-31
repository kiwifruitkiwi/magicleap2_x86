// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2017-2019, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/acpi.h>

#include "therm-mlmux.h"


#define MLMUX_THERM_SUSPEND_TO    (2000)
#define MLMUX_THERM_RESUME_TO     (2000)
#define MLMUX_THERM_POLLING_DELAY (2000)
#define MLMUX_THERM_CPU_STEP      (3)

static bool g_disable_shutdown;

static int mlmux_therm_get_temp(struct thermal_zone_device *tzd, int *temp)
{
	struct mlmux_therm_zone *ml_tz = tzd->devdata;

	*temp = ml_tz->temp;
	return 0;
}

static int mlmux_therm_get_weighted_temp(struct thermal_zone_device *tzd,
					 int *temp)
{
	struct mlmux_therm_zone *ml_tz = tzd->devdata;
	struct mlmux_therm_dev_data *tdev = ml_tz->tdev;
	int i;
	int sum_weight = 0;
	int sum_weighted_temp = 0;

	idr_for_each_entry(&tdev->zones, ml_tz, i) {
		if (i == tdev->fn_est_id)
			continue;
		sum_weighted_temp += ml_tz->temp * ml_tz->weight;
		sum_weight += ml_tz->weight;
	}

	if (sum_weight)
		*temp = sum_weighted_temp / sum_weight;
	return 0;
}

static int mlmux_therm_get_trip_type(struct thermal_zone_device *tzd, int trip,
		enum thermal_trip_type *type)
{
	struct mlmux_therm_zone *ml_tz = tzd->devdata;

	*type = ml_tz->trips[trip].trip_type;
	return 0;
}

static int mlmux_therm_get_trip_temp(struct thermal_zone_device *tzd, int trip,
		int *temp)
{
	struct mlmux_therm_zone *ml_tz = tzd->devdata;

	if (g_disable_shutdown &&
	    ml_tz->trips[trip].trip_type == THERMAL_TRIP_CRITICAL)
		*temp = 0;
	else
		*temp = ml_tz->trips[trip].trip_temp;
	return 0;
}

static int mlmux_therm_get_trip_hyst(struct thermal_zone_device *tzd, int trip,
		int *hyst)
{
	struct mlmux_therm_zone *ml_tz = tzd->devdata;

	*hyst = ml_tz->trips[trip].hysteresis;
	return 0;
}

static int mlmux_therm_get_critical_temp(struct thermal_zone_device *tzd,
		int *temp)
{
	struct mlmux_therm_zone *ml_tz = tzd->devdata;

	*temp = ml_tz->critical_temp;
	return 0;
}

static int mlmux_therm_bind(struct thermal_zone_device *thz,
			    struct thermal_cooling_device *cdev)
{
	struct mlmux_therm_zone *ml_tz = thz->devdata;
	int i, j;

	if (!cdev)
		return 0;

	for (i = 0, j = 0; i < ml_tz->num_trips; i++) {
		if (!ml_tz->trips[i].cdev_type)
			continue;

		if (!strncmp(ml_tz->trips[i].cdev_type, cdev->type,
			     strlen(cdev->type))){
			if (ml_tz->trips[i].trip_type == THERMAL_TRIP_PASSIVE) {
				thermal_zone_bind_cooling_device(thz, i, cdev,
					j, j, THERMAL_WEIGHT_DEFAULT);
				j += MLMUX_THERM_CPU_STEP;
			} else {
				thermal_zone_bind_cooling_device(thz, i, cdev,
					i, i, THERMAL_WEIGHT_DEFAULT);
			}
		}
	}

	return 0;
}

static int mlmux_therm_unbind(struct thermal_zone_device *thz,
			      struct thermal_cooling_device *cdev)
{
	struct mlmux_therm_zone *ml_tz = thz->devdata;
	int i;

	if (!cdev)
		return 0;

	for (i = 0; i < ml_tz->num_trips; i++) {
		if (!ml_tz->trips[i].cdev_type)
			continue;

		if (!strcmp(ml_tz->trips[i].cdev_type, cdev->type))
			thermal_zone_unbind_cooling_device(thz, i, cdev);
	}
	return 0;
}

static int mlmux_therm_get_trend(struct thermal_zone_device *thz,
				 int trip,
				 enum thermal_trend *trend)
{
	int trip_temp, trip_hyst;

	thz->ops->get_trip_temp(thz, trip, &trip_temp);
	thz->ops->get_trip_hyst(thz, trip, &trip_hyst);

	if (thz->temperature > thz->last_temperature)
		*trend = THERMAL_TREND_RAISING;
	else if (thz->temperature < trip_temp - trip_hyst)
		*trend = THERMAL_TREND_DROPPING;
	else
		*trend = THERMAL_TREND_STABLE;

	return 0;
}

static struct thermal_zone_device_ops ml_tzd_ops = {
	.get_temp = mlmux_therm_get_temp,
	.get_trip_type = mlmux_therm_get_trip_type,
	.get_trip_temp = mlmux_therm_get_trip_temp,
	.get_trip_hyst = mlmux_therm_get_trip_hyst,
	.get_crit_temp = mlmux_therm_get_critical_temp,
	.bind = mlmux_therm_bind,
	.unbind = mlmux_therm_unbind,
	.get_trend = mlmux_therm_get_trend,
};

static struct thermal_zone_device_ops ml_wtzd_ops = {
	.get_temp = mlmux_therm_get_weighted_temp,
	.get_trip_type = mlmux_therm_get_trip_type,
	.get_trip_temp = mlmux_therm_get_trip_temp,
	.get_trip_hyst = mlmux_therm_get_trip_hyst,
	.bind = mlmux_therm_bind,
	.unbind = mlmux_therm_unbind,
};

static void mlmux_therm_process_reg(struct work_struct *work)
{
	struct mlmux_therm_work *reg_w;
	struct mlmux_therm_dev_data *tdev;
	struct mlmux_therm_zone *ml_tz;
	struct mlmux_therm_tx_msg tx_msg = {.type = MLMUX_THERM_TX_REG_ACK};
	struct mlmux_therm_rx_reg *reg;
	struct device_node *np;
	char prop_name[32];
	int ret;
	int i, j;

	reg_w = container_of(work, struct mlmux_therm_work, work);
	reg = reg_w->data;
	tdev = reg_w->tdev;

	mutex_lock(&tdev->lock);
	for (j = 0; j < MLMUX_MAX_NUM_TZ; j++) {
		if (!(reg + j)->name[0])
			break;

		ml_tz = kzalloc(sizeof(*ml_tz), GFP_KERNEL);
		if (!ml_tz)
			goto free;

		strncpy(ml_tz->name, (char *)(reg + j), MLMUX_TZ_NAME_SIZE);
		ml_tz->num_trips = 0;

		for (i = 0; i < MLMUX_MAX_NUM_TZ; i++) {
			if (!strncmp(ml_tz->name, tdev->tname[i],
				     MLMUX_TZ_NAME_SIZE)) {
				ml_tz->weight = tdev->weight[i];
				ml_tz->critical_temp = tdev->crit_temp[i];
				break;
			}
		}

		/* Add throttling and critical trip point */
		if (ml_tz->critical_temp) {
			tdev->trips[ml_tz->num_trips].hysteresis = 0;
			tdev->trips[ml_tz->num_trips].trip_temp =
				ml_tz->critical_temp - 5000;
			tdev->trips[ml_tz->num_trips].trip_type =
				THERMAL_TRIP_PASSIVE;
			tdev->trips[ml_tz->num_trips].cdev_type = "Processor";
			ml_tz->num_trips++;

			tdev->trips[ml_tz->num_trips].hysteresis = 2000;
			tdev->trips[ml_tz->num_trips].trip_temp =
				ml_tz->critical_temp - 3000;
			tdev->trips[ml_tz->num_trips].trip_type =
				THERMAL_TRIP_PASSIVE;
			tdev->trips[ml_tz->num_trips].cdev_type = "Processor";
			ml_tz->num_trips++;

			tdev->trips[ml_tz->num_trips].trip_temp =
				ml_tz->critical_temp;
			tdev->trips[ml_tz->num_trips].trip_type =
				THERMAL_TRIP_CRITICAL;
			ml_tz->num_trips++;
		}
		ml_tz->tz = thermal_zone_device_register((char *)(reg + j),
			ml_tz->num_trips, 0, ml_tz, &ml_tzd_ops, NULL,
			MLMUX_THERM_POLLING_DELAY,
			MLMUX_THERM_POLLING_DELAY);
		if (IS_ERR(ml_tz->tz)) {
			dev_err(tdev->dev, "Reg therm_zone failed=%ld\n",
				PTR_ERR(ml_tz->tz));
			kfree(ml_tz);
			goto free;
		}
		ret = idr_alloc(&tdev->zones, ml_tz, 0, MLMUX_MAX_NUM_TZ,
				GFP_KERNEL);

		tx_msg.u.ack[j].id = ret;
		strncpy(tx_msg.u.ack[j].name, (char *)(reg + j),
			MLMUX_TZ_NAME_SIZE);

		ml_tz->id = ret;

		np = tdev->dev->of_node;
		if (snprintf(prop_name, sizeof(prop_name),
			     "trip-%s-critical-temp", ml_tz->name) > 0)
			of_property_read_u32(np, prop_name,
					     &ml_tz->critical_temp);
	}
	/* ack with success or failure */
	ml_mux_send_msg(tdev->client.ch, (uint32_t)MLMUX_THERM_TX_MSG_SIZE(ack),
					&tx_msg);
	if (ret < 0) {
		thermal_zone_device_unregister(ml_tz->tz);
		kfree(ml_tz);
	}

free:
	mutex_unlock(&tdev->lock);
	devm_kfree(tdev->dev, reg_w->data);
	devm_kfree(tdev->dev, reg_w);
}

static void mlmux_therm_process_update(struct work_struct *work)
{
	struct mlmux_therm_work *update_w;
	struct mlmux_therm_dev_data *tdev;
	struct mlmux_therm_zone *ml_tz;
	struct mlmux_therm_rx_update *update;
	int i;

	update_w = container_of(work, struct mlmux_therm_work, work);
	update = update_w->data;
	tdev = update_w->tdev;

	mutex_lock(&tdev->lock);
	for (i = 0; i < MLMUX_MAX_NUM_TZ; i++) {
		if ((update + i)->id == -1)
			break;

		ml_tz = (struct mlmux_therm_zone *)idr_find(&tdev->zones,
							    (update + i)->id);
		if (!ml_tz) {
			dev_err(tdev->dev, "Can't find TZ %d\n",
				(update + i)->id);
			mutex_unlock(&tdev->lock);
			return;
		}
		ml_tz->temp = (update + i)->temp;
	}
	mutex_unlock(&tdev->lock);
}

static inline void mlmux_therm_destroy(struct mlmux_therm_dev_data *tdev,
					  struct mlmux_therm_zone *ml_tz)
{
	thermal_zone_device_unregister(ml_tz->tz);
	idr_remove(&tdev->zones, ml_tz->id);
	kfree(ml_tz);
}

static void mlmux_therm_unregister_all(struct mlmux_therm_dev_data *tdev)
{
	struct mlmux_therm_zone *ml_tz;
	int i;

	mutex_lock(&tdev->reg_lock);
	idr_for_each_entry(&tdev->zones, ml_tz, i)
		mlmux_therm_destroy(tdev, ml_tz);
	idr_for_each_entry(&tdev->w_zones, ml_tz, i) {
		thermal_zone_device_unregister(ml_tz->tz);
		idr_remove(&tdev->w_zones, i);
	}
	mutex_unlock(&tdev->reg_lock);
}

static int mlmux_queue_process_msg(struct mlmux_therm_dev_data *tdev,
				   const void *data,
				   uint16_t size,
				   void (*work_func)(struct work_struct *))
{
	int ret = 0;
	struct mlmux_therm_work *therm_work;

	therm_work = devm_kzalloc(tdev->dev, sizeof(*therm_work), GFP_KERNEL);
	if (!therm_work) {
		ret = -1;
		goto exit;
	}

	therm_work->data = devm_kzalloc(tdev->dev, size, GFP_KERNEL);
	if (!therm_work->data) {
		devm_kfree(tdev->dev, therm_work);
		ret = -1;
		goto exit;
	}
	memcpy(therm_work->data, data, size);
	therm_work->tdev = tdev;

	INIT_WORK(&therm_work->work, work_func);
	queue_work(tdev->work_q, &therm_work->work);

exit:
	return ret;
}

static inline int mlmux_size_check(struct mlmux_therm_dev_data *tdev,
					   uint32_t len, size_t exp)
{
	if (len != exp) {
		dev_err(tdev->dev, "Unexpected length %d vs %zu\n", len, exp);
		return -EMSGSIZE;
	}
	return 0;
}

static void mlmux_therm_recv(struct ml_mux_client *cli, uint32_t len, void *msg)
{
	struct mlmux_therm_dev_data *tdev;
	struct mlmux_therm_rx_msg *msg_rx = (struct mlmux_therm_rx_msg *)msg;

	tdev = container_of(cli, struct mlmux_therm_dev_data, client);

	switch (msg_rx->type) {
	case MLMUX_THERM_RX_UPDATE:
		if (!mlmux_size_check(tdev, len, MLMUX_THERM_RX_SIZE(update)))
			mlmux_queue_process_msg(tdev, &msg_rx->u.update,
						len - 1,
						mlmux_therm_process_update);
	break;

	case MLMUX_THERM_RX_REG:
		if (!mlmux_size_check(tdev, len, MLMUX_THERM_RX_SIZE(reg)))
			mlmux_queue_process_msg(tdev, &msg_rx->u.reg, len - 1,
						mlmux_therm_process_reg);
	break;

	case MLMUX_THERM_RX_SUSPEND_ACK:
		if (mlmux_size_check(tdev, len, MLMUX_THERM_RX_SIZE(data)))
			return;
		if (!msg_rx->u.data)
			complete(&tdev->suspend);
		else
			dev_err(tdev->dev, "Suspend failed ret = %d",
				msg_rx->u.data);
	break;

	case MLMUX_THERM_RX_RESUME_ACK:
		if (mlmux_size_check(tdev, len, MLMUX_THERM_RX_SIZE(data)))
			return;
		if (!msg_rx->u.data)
			complete(&tdev->resume);
		else
			dev_err(tdev->dev, "Resume failed ret = %d",
				msg_rx->u.data);
	break;

	default:
		dev_err(tdev->dev, "Unknown cmd type %d\n", msg_rx->type);
		return;
	}
}

static void mlmux_therm_process_open(struct work_struct *work)
{
	struct mlmux_therm_work *open_w;
	struct mlmux_therm_dev_data *tdev;

	open_w = container_of(work, struct mlmux_therm_work, work);
	tdev = open_w->tdev;

	mutex_lock(&tdev->lock);
	tdev->chan_up = *(bool *)open_w->data;
	if (!tdev->chan_up)
		mlmux_therm_unregister_all(tdev);
	mutex_unlock(&tdev->lock);

	devm_kfree(tdev->dev, open_w->data);
	devm_kfree(tdev->dev, open_w);
}

static void mlmux_therm_open(struct ml_mux_client *cli, bool is_open)
{
	struct mlmux_therm_dev_data *tdev;

	tdev = container_of(cli, struct mlmux_therm_dev_data, client);
	dev_info(tdev->dev, "mlmux channel is %s\n",
		 is_open ? "opened" : "closed");

	mlmux_queue_process_msg(tdev, &is_open, sizeof(is_open),
			       mlmux_therm_process_open);
}

#ifdef CONFIG_OF
static int mlmux_therm_parse_dt(struct mlmux_therm_dev_data *tdev)
{
	struct device_node *np = tdev->dev->of_node;

	if (of_property_read_string(np, "ml,chan-name", &tdev->chan_name)) {
		dev_err(tdev->dev, "ml,chan-name undefined\n");
		return -EINVAL;
	}

	/* Thermistor regulator is optional */
	tdev->therm_reg = devm_regulator_get(tdev->dev, "thermistor");

	return 0;
}
#else
static inline int mlmux_therm_parse_dt(struct mlmux_therm_dev_data *tdev)
{
	return -ENODEV;
}
#endif

#ifdef CONFIG_ACPI
static void mlmux_therm_get_hyst_table(struct mlmux_therm_dev_data *tdev)
{
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *p;
	union acpi_object *elem;
	acpi_status status;
	int i;

	status = acpi_evaluate_object(ACPI_HANDLE(tdev->dev), "HYST",
				NULL, &output);
	if (ACPI_FAILURE(status))
		return;

	p = output.pointer;
	if (!p || (p->type != ACPI_TYPE_PACKAGE)) {
		dev_err(tdev->dev, "Invalid data\n");
		goto free_ptr;
	}

	for (i = 0; i < p->package.count; i++) {
		elem = &p->package.elements[i];
		tdev->trips[i].hysteresis = elem->integer.value;
	}

free_ptr:
	ACPI_FREE(output.pointer);
}

static void mlmux_therm_get_crit_table(struct mlmux_therm_dev_data *tdev)
{
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *p;
	union acpi_object *elem;
	acpi_status status;
	int i, j;
	status = acpi_evaluate_object(ACPI_HANDLE(tdev->dev), "CRIT",
				NULL, &output);
	if (ACPI_FAILURE(status))
		return;

	p = output.pointer;
	if (!p || (p->type != ACPI_TYPE_PACKAGE)) {
		dev_err(tdev->dev, "Invalid data\n");
		goto free_ptr;
	}

	for (i = 0, j = 0; i < p->package.count; i++, j++) {
		elem = &p->package.elements[++i];
		tdev->crit_temp[j] = elem->integer.value;
	}
free_ptr:
	ACPI_FREE(output.pointer);
}

static void mlmux_therm_get_weight_table(struct mlmux_therm_dev_data *tdev)
{
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *p;
	union acpi_object *elem1, *elem2;
	acpi_status status;
	int i, j;

	status = acpi_evaluate_object(ACPI_HANDLE(tdev->dev), "WGHT",
				NULL, &output);
	if (ACPI_FAILURE(status))
		return;

	p = output.pointer;
	if (!p || (p->type != ACPI_TYPE_PACKAGE)) {
		dev_err(tdev->dev, "Invalid data\n");
		goto free_ptr;
	}

	for (i = 0, j = 0; i < p->package.count; i++, j++) {
		elem1 = &p->package.elements[i++];
		strlcpy(tdev->tname[j], elem1->string.pointer,
			MLMUX_TZ_NAME_SIZE);
		elem2 = &p->package.elements[i];
		tdev->weight[j] = elem2->integer.value;
	}

free_ptr:
	ACPI_FREE(output.pointer);
}

static void mlmux_therm_get_trip_table(struct mlmux_therm_dev_data *tdev)
{
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *p;
	union acpi_object *elem1, *elem2;
	acpi_status status;
	int i, j;
	static char* cdev = "Fan";

	status = acpi_evaluate_object(ACPI_HANDLE(tdev->dev), "TRIP",
				NULL, &output);
	if (ACPI_FAILURE(status))
		return;

	p = output.pointer;
	if (!p || (p->type != ACPI_TYPE_PACKAGE)) {
		dev_err(tdev->dev, "Invalid data\n");
		goto free_ptr;
	}

	tdev->num_trips = 0;
	for (i = 0, j = 0; i < p->package.count; i++) {
		elem1 = &p->package.elements[i++];
		if (strncmp(elem1->string.pointer, "trip", 4))
			continue;
		elem2 = &p->package.elements[i];
		tdev->trips[j].trip_temp = elem2->integer.value;
		tdev->trips[j].trip_type = THERMAL_TRIP_ACTIVE;
		tdev->trips[j].cdev_type = cdev;
		tdev->num_trips++;
		j++;
	}

free_ptr:
	ACPI_FREE(output.pointer);
}

static int mlmux_therm_process_acpi(struct mlmux_therm_dev_data *tdev)
{
	tdev->chan_name = "THERM_CHAN";
	tdev->therm_reg = ERR_PTR(-ENXIO);

	mlmux_therm_get_weight_table(tdev);
	mlmux_therm_get_trip_table(tdev);
	mlmux_therm_get_crit_table(tdev);
	mlmux_therm_get_hyst_table(tdev);

	return 0;
}
#else
static inline int mlmux_therm_process_acpi(struct mlmux_therm_dev_data *tdev)
{
	return -ENODEV;
}
#endif

static int mlmux_therm_process_fwnode(struct mlmux_therm_dev_data *tdev)
{
	if (tdev->dev->of_node)
		return mlmux_therm_parse_dt(tdev);
	else
		return mlmux_therm_process_acpi(tdev);
}

static int mlmux_therm_disable_shutdown_read(struct seq_file *s, void *priv)
{
	seq_printf(s, "%d\n", g_disable_shutdown);

	return 0;
}

static ssize_t mlmux_therm_disable_shutdown_write(struct file *file,
						  const char __user *user_buf,
						  size_t count, loff_t *ppos)
{
	unsigned long t;
	int err;

	err = kstrtoul_from_user(user_buf, count, 0, &t);
	if (err)
		return err;

	g_disable_shutdown = t;

	return count;
}

static int mlmux_therm_disable_shutdown_open(struct inode *inode,
					     struct file *file)
{
	return single_open(file, mlmux_therm_disable_shutdown_read,
		inode->i_private);
}

static const struct file_operations fops_disable_shutdown = {
	.open = mlmux_therm_disable_shutdown_open,
	.read = seq_read,
	.write = mlmux_therm_disable_shutdown_write,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};

static int mlmux_therm_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mlmux_therm_dev_data *tdev = NULL;
	struct mlmux_therm_zone *ml_tz;
	static char *tz_name = "fn_est";

	tdev = devm_kzalloc(&pdev->dev, sizeof(*tdev), GFP_KERNEL);
	if (!tdev) {
		ret = -ENOMEM;
		goto exit;
	}

	tdev->dentry = debugfs_create_dir("therm_mlmux", NULL);
	if (!tdev->dentry) {
		ret = -ENOMEM;
		goto exit;
	}

	debugfs_create_file("disable_shutdown", 0400, tdev->dentry, tdev,
			    &fops_disable_shutdown);

	tdev->dev = &pdev->dev;
	platform_set_drvdata(pdev, tdev);
	ret = mlmux_therm_process_fwnode(tdev);
	if (ret)
		goto exit;

	/* Enable thermistor regulator if provided */
	if (!IS_ERR(tdev->therm_reg)) {
		ret = regulator_enable(tdev->therm_reg);
		if (ret) {
			dev_err(tdev->dev, "Could not enable regulator\n");
			goto exit;
		}
	}

	/* Create fan estimator thermal zone */
	ml_tz = devm_kzalloc(tdev->dev, sizeof(*ml_tz), GFP_KERNEL);
	if (!ml_tz)
		goto exit;

	ml_tz->tdev = tdev;
	strncpy(ml_tz->name, tz_name, MLMUX_TZ_NAME_SIZE);
	ml_tz->num_trips = tdev->num_trips;
	memcpy(ml_tz->trips, tdev->trips,
		sizeof(struct thermal_trip_info) * THERMAL_MAX_TRIPS);

	ml_tz->tz = thermal_zone_device_register(tz_name,
		ml_tz->num_trips, 0, ml_tz, &ml_wtzd_ops, NULL, 0,
		MLMUX_THERM_POLLING_DELAY);
	if (IS_ERR(ml_tz->tz)) {
		dev_err(tdev->dev, "Reg therm_zone failed=%ld\n",
			PTR_ERR(ml_tz->tz));
			goto exit;
	}

	tdev->client.dev = tdev->dev;
	tdev->client.notify_open = mlmux_therm_open;
	tdev->client.receive_cb = mlmux_therm_recv;

	init_completion(&tdev->suspend);
	init_completion(&tdev->resume);
	mutex_init(&tdev->lock);
	mutex_init(&tdev->reg_lock);
	idr_init(&tdev->zones);
	tdev->fn_est_id = idr_alloc(&tdev->zones, ml_tz, 0,
				    MLMUX_MAX_NUM_TZ, GFP_KERNEL);

	tdev->work_q = alloc_workqueue(tdev->chan_name, WQ_UNBOUND, 1);
	if (!tdev->work_q) {
		dev_info(tdev->dev, "Failed to create workqueue\n");
		ret = -ENOMEM;
		goto exit_idr_destroy;
	}

	/* Revisit: Right now we do not care that channel is already open,
	 *	    the next step in registration is controlled by msgs
	 */
	ret = ml_mux_request_channel(&tdev->client, (char *)tdev->chan_name);
	if (ret < 0)
		goto exit_wq_destroy;

	return 0;

exit_wq_destroy:
	destroy_workqueue(tdev->work_q);
exit_idr_destroy:
	mlmux_therm_unregister_all(tdev);
	idr_destroy(&tdev->zones);
	mutex_destroy(&tdev->lock);
	mutex_destroy(&tdev->reg_lock);
	if (!IS_ERR(tdev->therm_reg))
		regulator_disable(tdev->therm_reg);
exit:
	return ret;
}

int mlmux_therm_remove(struct platform_device *pdev)
{
	struct mlmux_therm_dev_data *tdev = platform_get_drvdata(pdev);

	ml_mux_release_channel(tdev->client.ch);
	destroy_workqueue(tdev->work_q);
	mlmux_therm_unregister_all(tdev);
	idr_destroy(&tdev->zones);
	mutex_destroy(&tdev->lock);
	mutex_destroy(&tdev->reg_lock);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mlmux_therm_suspend(struct device *dev)
{
	int ret;
	unsigned long jiffies_left;
	struct mlmux_therm_dev_data *tdev = dev_get_drvdata(dev);
	struct mlmux_therm_tx_msg tx_msg;

	reinit_completion(&tdev->suspend);
	tx_msg.type = MLMUX_THERM_TX_SUSPEND;
	tx_msg.u.data = 1;
	ret = ml_mux_send_msg(tdev->client.ch,
			(uint32_t)MLMUX_THERM_TX_MSG_SIZE(data), &tx_msg);
	if (ret)
		return ret;

	jiffies_left = wait_for_completion_timeout(&tdev->suspend,
				msecs_to_jiffies(MLMUX_THERM_SUSPEND_TO));
	if (!jiffies_left)
		return -ETIMEDOUT;
	dev_dbg(dev, "suspend\n");

	if (!IS_ERR(tdev->therm_reg))
		regulator_disable(tdev->therm_reg);

	return 0;
}

static int mlmux_therm_resume(struct device *dev)
{
	int ret;
	unsigned long jiffies_left;
	struct mlmux_therm_dev_data *tdev = dev_get_drvdata(dev);
	struct mlmux_therm_tx_msg tx_msg;

	if (!IS_ERR(tdev->therm_reg)) {
		ret = regulator_enable(tdev->therm_reg);
		if (ret) {
			dev_err(tdev->dev, "Failed to re-enable regulator\n");
			return ret;
		}
	}

	reinit_completion(&tdev->resume);
	tx_msg.type = MLMUX_THERM_TX_RESUME;
	tx_msg.u.data = 1;

	ret = ml_mux_send_msg(tdev->client.ch,
			(uint32_t)MLMUX_THERM_TX_MSG_SIZE(data), &tx_msg);
	if (ret)
		return ret;

	jiffies_left = wait_for_completion_timeout(&tdev->resume,
				msecs_to_jiffies(MLMUX_THERM_RESUME_TO));
	if (!jiffies_left)
		return -ETIMEDOUT;
	dev_dbg(dev, "resume\n");

	return 0;
}

static const struct dev_pm_ops mlmux_therm_pm = {
	.suspend = mlmux_therm_suspend,
	.resume = mlmux_therm_resume,
};
#define MLMUX_THERM_PM_OPS	(&mlmux_therm_pm)
#else
#define MLMUX_THERM_PM_OPS	NULL
#endif


static const struct of_device_id ml_therm_match_table[] = {
	{ .compatible = "ml,therm_mlmux", },
	{ },
};
MODULE_DEVICE_TABLE(of, ml_therm_match_table);

static const struct acpi_device_id mlmux_therm_acpi_match[] = {
	{"MXTH0000", 0},
	{ },
};
MODULE_DEVICE_TABLE(acpi, mlmux_therm_acpi_match);

static struct platform_driver ml_therm_driver = {
	.driver = {
		.name = "therm_mlmux",
		.of_match_table = ml_therm_match_table,
		.pm = MLMUX_THERM_PM_OPS,
		.acpi_match_table = mlmux_therm_acpi_match,
	},
	.probe = mlmux_therm_probe,
	.remove = mlmux_therm_remove,
};

module_platform_driver(ml_therm_driver);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("ML MUX client driver for thermal zones");
MODULE_LICENSE("GPL v2");

/*
 * Copyright (C) 2017-2020 InvenSense, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "inv_mpu: " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/math64.h>

#include "../inv_mpu_iio.h"

static int inv_check_fsync(struct inv_mpu_state *st)
{
	u8 data[1];

	if (!st->chip_config.eis_enable)
		return 0;
	inv_plat_read(st, REG_INT_STATUS, 1, data);
	if (data[0] & BIT_INT_STATUS_UI_FSYNC) {
		pr_debug("fsync\n");
		st->eis.eis_triggered = true;
		st->eis.fsync_delay = 1;
		st->eis.prev_state = 1;
		st->eis.frame_count++;
		st->eis.eis_frame = true;
	}
	st->header_count--;

	return 0;
}

static int inv_push_sensor(struct inv_mpu_state *st, int ind, u64 t, s32 *raw, u16 temp)
{
	s32 calib[3] = { 0, 0, 0 };

	switch (ind) {
	case SENSOR_ACCEL:
		inv_push_16bytes_buffer(st, ind, t, raw, temp);
		break;
	case SENSOR_TEMP:
		inv_check_fsync(st);
		break;
	case SENSOR_GYRO:
		inv_push_gyro_data(st, raw, calib, t, temp);
		break;
	default:
		break;
	}

	return 0;
}

static bool inv_validate_fifo_data(s16 *lsb)
{
	bool ret = true;

	if ((lsb[0] == -32768) && (lsb[1] == -32768) && (lsb[2] == -32768))
		ret = false;

	return ret;
}

static int inv_push_42600_data(struct inv_mpu_state *st, u8 *d)
{
	icm406xx_fifo_header_t header;
	s16 acc_lsb[3] = { 0, 0, 0 };
	s16 gyr_lsb[3] = { 0, 0, 0 };
	s32 acc[3], gyr[3];
	u16 temperature = 0;
	bool valid_acc = false;
	bool valid_gyr = false;

	header.Byte = *d;

	/* skip 1 byte header */
	if (header.bits.msg_bit)
		return 0;

	if (header.bits.accel_bit && !header.bits.gyro_bit) {
		/* only accel */
		acc_lsb[0] = (s16)be16_to_cpup((__be16 *) (&d[1]));
		acc_lsb[1] = (s16)be16_to_cpup((__be16 *) (&d[3]));
		acc_lsb[2] = (s16)be16_to_cpup((__be16 *) (&d[5]));
		temperature = d[7];
		valid_acc = inv_validate_fifo_data(acc_lsb);
	} else if (!header.bits.accel_bit && header.bits.gyro_bit) {
		/* only gyro */
		gyr_lsb[0] = (s16)be16_to_cpup((__be16 *) (&d[1]));
		gyr_lsb[1] = (s16)be16_to_cpup((__be16 *) (&d[3]));
		gyr_lsb[2] = (s16)be16_to_cpup((__be16 *) (&d[5]));
		temperature = d[7];
		valid_gyr = inv_validate_fifo_data(gyr_lsb);
	} else if ((header.bits.accel_bit && header.bits.gyro_bit) ||
			header.bits.twentybits_bit) {
		/* accel and gyro */
		acc_lsb[0] = (s16)be16_to_cpup((__be16 *) (&d[1]));
		acc_lsb[1] = (s16)be16_to_cpup((__be16 *) (&d[3]));
		acc_lsb[2] = (s16)be16_to_cpup((__be16 *) (&d[5]));
		gyr_lsb[0] = (s16)be16_to_cpup((__be16 *) (&d[7]));
		gyr_lsb[1] = (s16)be16_to_cpup((__be16 *) (&d[9]));
		gyr_lsb[2] = (s16)be16_to_cpup((__be16 *) (&d[11]));
		valid_acc = inv_validate_fifo_data(acc_lsb);
		valid_gyr = inv_validate_fifo_data(gyr_lsb);

		if (header.bits.twentybits_bit) {
			temperature = (u16)be16_to_cpup((__be16 *) (&d[13]));
		} else {
			temperature = d[13];
		}
	} else
		return 0;

	pr_debug("temperature: %d\n", temperature);

	if (header.bits.twentybits_bit) {
		/* high resolution mode */
		acc[0] = (acc_lsb[0] << 4) | ((d[17] >> 4) & 0xf);
		acc[1] = (acc_lsb[1] << 4) | ((d[18] >> 4) & 0xf);
		acc[2] = (acc_lsb[2] << 4) | ((d[19] >> 4) & 0xf);
		gyr[0] = (gyr_lsb[0] << 4) | (d[17] & 0xf);
		gyr[1] = (gyr_lsb[1] << 4) | (d[18] & 0xf);
		gyr[2] = (gyr_lsb[2] << 4) | (d[19] & 0xf);
	} else {
		/* normal resolution mode */
		acc[0] = acc_lsb[0];
		acc[1] = acc_lsb[1];
		acc[2] = acc_lsb[2];
		gyr[0] = gyr_lsb[0];
		gyr[1] = gyr_lsb[1];
		gyr[2] = gyr_lsb[2];
	}

	/* send accel data */
	if (header.bits.accel_bit) {
		if (st->sensor[SENSOR_ACCEL].on && valid_acc) {
			inv_get_dmp_ts(st, SENSOR_ACCEL);
			if (st->sensor[SENSOR_ACCEL].send &&
					(!st->ts_algo.first_drop_samples[SENSOR_ACCEL])) {
				st->sensor[SENSOR_ACCEL].sample_calib++;
				inv_push_sensor(st, SENSOR_ACCEL,
						st->sensor[SENSOR_ACCEL].ts, acc, temperature);
			}
			if (st->ts_algo.first_drop_samples[SENSOR_ACCEL])
				st->ts_algo.first_drop_samples[SENSOR_ACCEL]--;
		}
	}

	/* send gyro data */
	if (header.bits.gyro_bit) {
		if (st->sensor[SENSOR_GYRO].on && valid_gyr) {
			inv_get_dmp_ts(st, SENSOR_GYRO);
			if (st->sensor[SENSOR_GYRO].send &&
					(!st->ts_algo.first_drop_samples[SENSOR_GYRO])) {
				st->sensor[SENSOR_GYRO].sample_calib++;
				inv_push_sensor(st, SENSOR_GYRO,
						st->sensor[SENSOR_GYRO].ts, gyr, temperature);
			}
			if (st->ts_algo.first_drop_samples[SENSOR_GYRO])
				st->ts_algo.first_drop_samples[SENSOR_GYRO]--;
		}
	}

	st->header_count--;

	return 0;
}

static int inv_prescan_fifo_data(struct inv_mpu_state *st, u8 *data, int len)
{
	int i;
	u8 *dptr;
	icm406xx_fifo_header_t header;

	for (i = 0; i < SENSOR_NUM_MAX; i++)
		st->sensor[i].count = 0;

	dptr = data;
	while (dptr < (data + len)) {
		/* count the number of valid samples
		 * in the buffer for accel and gyro
		 */
		header.Byte = *dptr;
		if (header.bits.msg_bit) {
			pr_warn("Unexpected FIFO msg bit\n");
			return 0;
		}
		/* header */
		dptr++;
		/* accel */
		if (header.bits.accel_bit) {
			st->sensor[SENSOR_ACCEL].count++;
			dptr += st->sensor[SENSOR_ACCEL].sample_size;
		}
		/* gyro */
		if (header.bits.gyro_bit) {
			st->sensor[SENSOR_GYRO].count++;
			dptr += st->sensor[SENSOR_GYRO].sample_size;
		}
		/* temperature */
		if (header.bits.twentybits_bit)
			dptr += 2;
		else
			dptr += 1;
		/* timestamp */
		if ((header.bits.accel_bit && header.bits.gyro_bit) ||
				header.bits.twentybits_bit)
			dptr += 2;
		/* high resolution data */
		if (header.bits.twentybits_bit)
			dptr += 3;
	}

	return 0;
}

static int inv_process_42600_data(struct inv_mpu_state *st)
{
	struct iio_dev *indio_dev = iio_priv_to_dev(st);
	int total_bytes, tmp, res, fifo_count, pk_size, i;
	u8 *dptr, *d;
	u8 data[2];
	bool done_flag;

	res = inv_plat_read(st, REG_INT_STATUS2, 1, data);
	if (res)
		return res;
	if (st->smd.on && (data[0] & BIT_INT_STATUS_SMD)) {
		sysfs_notify(&indio_dev->dev.kobj, NULL, "poll_smd");
		st->smd.on = false;
		st->trigger_state = EVENT_TRIGGER;
		inv_check_sensor_on(st);
		res = set_inv_enable(indio_dev);
		if (res)
			return res;
		st->wake_sensor_received = true;
	}
	if (st->gesture_only_on && (!st->batch.timeout)) {
		pr_debug("ges cnt=%d, statu=%x\n",
			st->gesture_int_count, data[0]);
		if (data[0] & (BIT_INT_STATUS_WOM_XYZ)) {
			if (!st->gesture_int_count) {
				res = inv_plat_read(st,
					REG_INT_SOURCE0, 1, data);
				if (res)
					return res;
				data[0] |= BIT_INT_UI_DRDY_INT1_EN;
				res = inv_plat_single_write(st,
					REG_INT_SOURCE0, data[0]);
				if (res)
					return res;
				data[0] = 0;
				if (st->chip_config.gyro_enable ||
					st->chip_config.accel_enable)
					data[0] |= (BIT_FIFO_ACCEL_EN |
						BIT_FIFO_GYRO_EN |
						BIT_FIFO_WM_TH);
				res = inv_plat_single_write(st,
					REG_FIFO_CONFIG1, data[0]);
				if (res)
					return res;
				/*
				 * First time wake up from WOM
				 * we don't need data in the FIFO
				 */
				res = inv_reset_fifo(st, true);
				if (res)
					return res;
				if (st->chip_config.stationary_detect_enable) {
					st->gesture_int_count =
						STATIONARY_DELAY_THRESHOLD;
				} else {
					st->gesture_int_count =
						WOM_DELAY_THRESHOLD;
				}
				return res;
			}
			if (st->chip_config.stationary_detect_enable) {
				st->gesture_int_count =
					STATIONARY_DELAY_THRESHOLD;
			} else {
				st->gesture_int_count =
					WOM_DELAY_THRESHOLD;
			}
		} else {
			if (!st->gesture_int_count) {
				res = inv_plat_single_write(st,
					REG_FIFO_CONFIG1, 0);
				if (res)
					return res;
				res = inv_plat_read(st,
					REG_INT_SOURCE0, 1, data);
				if (res)
					return res;
				data[0] &= ~BIT_INT_UI_DRDY_INT1_EN;
				res = inv_plat_single_write(st,
					REG_INT_SOURCE0, data[0]);
				if (res)
					return res;

				return res;
			}
			st->gesture_int_count--;
		}
	}
	fifo_count = inv_get_last_run_time_non_dmp_record_mode(st);
	pr_debug("fifc= %d\n", fifo_count);
	if (!fifo_count) {
		pr_debug("REG_FIFO_COUNT_H size is 0\n");
		return 0;
	}
	pk_size = st->batch.pk_size;
	if (!pk_size)
		return -EINVAL;

	if (fifo_count >= (HARDWARE_FIFO_SIZE - st->batch.pk_size) /
				st->batch.pk_size) {
		pr_warn("fifo overflow pkt count=%d pkt sz=%d\n",
			fifo_count, st->batch.pk_size);
		return -EOVERFLOW;
	}

	fifo_count *= st->batch.pk_size;
	st->fifo_count = fifo_count;
	d = st->fifo_data_store;
	dptr = d;
	total_bytes = fifo_count;
	while (total_bytes > 0) {
		if (total_bytes < pk_size * MAX_FIFO_PACKET_READ)
			tmp = total_bytes;
		else
			tmp = pk_size * MAX_FIFO_PACKET_READ;
		res = inv_plat_read(st, REG_FIFO_DATA_REG, tmp, dptr);
		if (res < 0) {
			pr_err("read REG_FIFO_R_W is failed\n");
			return res;
		}
		pr_debug("inside: %x, %x, %x, %x, %x, %x, %x, %x\n",
			dptr[0], dptr[1], dptr[2], dptr[3],
			dptr[4], dptr[5], dptr[6], dptr[7]);
		pr_debug("insid2: %x, %x, %x, %x, %x, %x, %x, %x\n",
			dptr[8], dptr[9], dptr[10], dptr[11],
			dptr[12], dptr[13], dptr[14], dptr[15]);
		dptr += tmp;
		total_bytes -= tmp;
	}

	dptr = d;
	pr_debug("dd: %x, %x, %x, %x, %x, %x, %x, %x\n",
		d[0], d[1], d[2], d[3],
		d[4], d[5], d[6], d[7]);
	pr_debug("dd2: %x, %x, %x, %x, %x, %x, %x, %x\n",
		d[8], d[9], d[10], d[11],
		d[12], d[13], d[14], d[15]);
	total_bytes = fifo_count;

	inv_prescan_fifo_data(st, dptr, total_bytes);

	st->header_count = 0;
	for (i = 0; i < SENSOR_NUM_MAX; i++) {
		if (st->sensor[i].on)
			st->header_count = max(st->header_count,
							st->sensor[i].count);
	}

	st->ts_algo.calib_counter++;
	inv_bound_timestamp(st);

	dptr = d;
	done_flag = false;

	while (!done_flag) {
		pr_debug("total%d, pk=%d\n", total_bytes, pk_size);
		if (total_bytes >= pk_size) {
			res = inv_push_42600_data(st, dptr);
			if (res)
				return res;
			total_bytes -= pk_size;
			dptr += pk_size;
		} else {
			done_flag = true;
		}
	}

	return 0;
}

int inv_get_42600_pedometer_steps(struct inv_mpu_state *st,
	int *ped, int *update)
{
	u8 r[2];
	uint32_t cur_step_cnt;
	int result;

	*update = false;
	result = inv_plat_read(st,
		REG_APEX_DATA0, 2, &r[0]);
	if (result)
		return result;

	cur_step_cnt = (uint32_t)((r[1] << 8) | r[0]);
	if (st->apex_data.step_reset_last_val) {
		st->apex_data.step_reset_last_val = false;
		st->apex_data.step_cnt_last_val = cur_step_cnt;
	}
	if (cur_step_cnt !=
		st->apex_data.step_cnt_last_val) {
		if (cur_step_cnt < st->apex_data.step_cnt_last_val) {
			/* overflow */
			st->apex_data.step_cnt_total +=
				cur_step_cnt + (0xFFFF - st->apex_data.step_cnt_last_val);
		} else {
			st->apex_data.step_cnt_total +=
				cur_step_cnt - st->apex_data.step_cnt_last_val;
		}
	}
	st->apex_data.step_cnt_last_val = cur_step_cnt;
	*ped = st->apex_data.step_cnt_total;
	*update = true;

	return result;
}

static int inv_process_apex_gesture(struct inv_mpu_state *st)
{
	struct iio_dev *indio_dev = iio_priv_to_dev(st);
	u8 r[2], int_status3, apex_data4;
	s16 s[3] = {0,};
	u64 t;
	uint32_t cur_step_cnt;
	u8 tap_dir, tap_axis, tap_num;
	int ped_update;
	int result;

	if (inv_get_apex_enabled(st)) {
		t = get_time_ns();
		result = inv_plat_read(st, REG_APEX_DATA4, 1, &r[0]);
		if (result)
			return result;
		apex_data4 = r[0];
		tap_dir = apex_data4 & 0x01;
		tap_axis = (apex_data4 & 0x06) >> 1;
		if (tap_axis > 2)
			tap_axis = 2;
		tap_num = (apex_data4 & 0x18) >> 3;
		result = inv_plat_read(st, REG_INT_STATUS3, 1, &r[0]);
		if (result)
			return result;
		int_status3 = r[0];
		if (int_status3 & BIT_INT_STATUS_TILT_DET) {
			sysfs_notify(&indio_dev->dev.kobj, NULL, "poll_tilt");
			st->wake_sensor_received = true;
		}
		if (int_status3 & BIT_INT_STATUS_RAISE_DET) {
			sysfs_notify(&indio_dev->dev.kobj, NULL, "poll_pick_up");
			st->chip_config.pick_up_enable = 0;
			inv_check_sensor_on(st);
			set_inv_enable(indio_dev);
			st->wake_sensor_received = true;
		}
		if (int_status3 & BIT_INT_STATUS_STEP_DET) {
			if (st->step_detector_l_on) {
				inv_push_8bytes_buffer(st,
					STEP_DETECTOR_HDR, t, s);
			}
			if (st->step_detector_wake_l_on) {
				inv_push_8bytes_buffer(st,
					STEP_DETECTOR_WAKE_HDR, t, s);
				st->wake_sensor_received = true;
			}
			if (st->ped.int_mode &&
				(st->step_counter_l_on || st->step_counter_wake_l_on)) {
				result = inv_get_42600_pedometer_steps(st,
					&cur_step_cnt, &ped_update);
				if (result)
					return result;
				if (ped_update)
					inv_send_steps(st, st->apex_data.step_cnt_total, t);
				if (st->step_counter_wake_l_on)
					st->wake_sensor_received = true;
			}
		}
		if (int_status3 & BIT_INT_STATUS_TAP_DET && tap_num == 2) {
			s[tap_axis] = (tap_dir ? -1 : 1);
			inv_push_8bytes_buffer(st, TAP_HDR, t, s);
			st->wake_sensor_received = true;
		}
	}
	return 0;
}

/*
 *  inv_read_fifo() - Transfer data from FIFO to ring buffer.
 */
irqreturn_t inv_read_fifo(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct inv_mpu_state *st = iio_priv(indio_dev);
	int result;

	result = wait_event_interruptible_timeout(st->wait_queue,
					st->resume_state, msecs_to_jiffies(300));
	if (result <= 0)
		goto exit_handled;

	mutex_lock(&indio_dev->mlock);
	st->wake_sensor_received = false;

	if (st->apex_supported) {
		result = inv_process_apex_gesture(st);
		if (result)
			goto err_reset_fifo;
	}

	result = inv_process_42600_data(st);
	if (result)
		goto err_reset_fifo;
	mutex_unlock(&indio_dev->mlock);

	if (st->wake_sensor_received)
#ifdef CONFIG_HAS_WAKELOCK
		wake_lock_timeout(&st->wake_lock, msecs_to_jiffies(200));
#else
		__pm_wakeup_event(st->wake_lock, 200); /* 200 msecs */
#endif
	goto exit_handled;

err_reset_fifo:
	if ((!st->chip_config.gyro_enable) &&
		(!st->chip_config.accel_enable) &&
		(!st->chip_config.slave_enable) &&
		(!st->chip_config.pressure_enable) &&
		(!inv_get_apex_enabled(st))) {
		mutex_unlock(&indio_dev->mlock);
		goto exit_handled;
	}

	pr_err("error to reset fifo\n");
	inv_reset_fifo(st, true);
	mutex_unlock(&indio_dev->mlock);

exit_handled:
	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

int inv_flush_batch_data(struct iio_dev *indio_dev, int data)
{
	struct inv_mpu_state *st = iio_priv(indio_dev);

	if (st->chip_config.gyro_enable ||
		st->chip_config.accel_enable ||
		st->chip_config.slave_enable ||
		st->chip_config.pressure_enable ||
		inv_get_apex_enabled(st)) {
		st->wake_sensor_received = false;
		inv_process_42600_data(st);
		if (st->wake_sensor_received)
#ifdef CONFIG_HAS_WAKELOCK
			wake_lock_timeout(&st->wake_lock, msecs_to_jiffies(200));
#else
			__pm_wakeup_event(st->wake_lock, 200); /* 200 msecs */
#endif
	}
	inv_push_marker_to_buffer(st, END_MARKER, data);

	return 0;
}


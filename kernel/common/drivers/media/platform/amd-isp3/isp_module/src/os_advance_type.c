/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "log.h"

#include <linux/time.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/fs.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][os_advance_type]"

#define USING_WAIT_QUEUE

static wait_queue_head_t g_evt_waitq_head;
wait_queue_head_t *g_evt_waitq_headp;

int isp_mutex_lock(struct mutex *p_mutex)
{
	mutex_lock(p_mutex);
	return RET_SUCCESS;
}

int isp_event_init(struct isp_event *p_event, int auto_matic,
			int init_state)
{
	p_event->automatic = auto_matic;
	p_event->event = init_state;
	p_event->result = 0;

	if (g_evt_waitq_headp == NULL) {
		g_evt_waitq_headp = &g_evt_waitq_head;
		init_waitqueue_head(g_evt_waitq_headp);
	}
	return RET_SUCCESS;
};

int isp_event_signal(unsigned int result, struct isp_event *p_event)
{
	p_event->result = result;
	p_event->event = 1;

#ifdef USING_WAIT_QUEUE
	if (g_evt_waitq_headp)
		wake_up_interruptible(g_evt_waitq_headp);
	else
		ISP_PR_ERR("no head");
#endif
	ISP_PR_DBG("signal evt %p,result %d\n", p_event, result);
	return RET_SUCCESS;
};

int isp_event_reset(struct isp_event *p_event)
{
	p_event->event = 0;

	return RET_SUCCESS;
};

int isp_event_wait(struct isp_event *p_event, unsigned int timeout_ms)
{
#ifdef USING_WAIT_QUEUE
	if (g_evt_waitq_headp) {
		int temp;

		if (p_event->event)
			goto quit;

		temp =
			wait_event_interruptible_timeout((*g_evt_waitq_headp),
						p_event->event,
						(timeout_ms * HZ / 1000));

		if (temp == 0)
			return RET_TIMEOUT;
	} else {
		ISP_PR_ERR("no head");
	}
#else
	long long start, end;

	isp_get_cur_time_tick(&start);
	while (!p_event->event) {
		isp_get_cur_time_tick(&end);
		if (p_event->event)
			goto quit;

		if (isp_is_timeout(&start, &end, timeout_ms))
			return RET_TIMEOUT;

		usleep_range(10000, 11000);
	}
#endif

quit:
	if (p_event->automatic)
		p_event->event = 0;

	ISP_PR_DBG("wait evt %p suc\n", p_event);
	return p_event->result;
};

void isp_get_cur_time_tick(long long *ptimetick)
{
	if (ptimetick)
		*ptimetick = get_jiffies_64();

};

int isp_is_timeout(long long *start, long long *end,
		unsigned int timeout_ms)
{
	if ((start == NULL) || (end == NULL) || (timeout_ms == 0))
		return 1;
	if ((*end - *start) * 1000 / HZ >= (long long) timeout_ms)
		return 1;
	else
		return 0;

};

int create_work_thread(struct thread_handler *handle,
			work_thread_prototype working_thread, void *context)
{
	struct task_struct *resp_polling_kthread;

	if ((handle == NULL) || (working_thread == NULL)) {
		ISP_PR_ERR("illegal parameter\n");
		return RET_INVALID_PARM;
	};

	if (handle->thread) {
		ISP_PR_INFO("response thread has already created\n");
		return RET_SUCCESS;
	}

	handle->stop_flag = false;

	isp_event_init(&handle->wakeup_evt, 1, 0);

	resp_polling_kthread =
		kthread_run(working_thread, context, "kernel_thread");

	if (IS_ERR(resp_polling_kthread)) {
		ISP_PR_ERR("create thread fail\n");
		return RET_FAILURE;
	}

	handle->thread = resp_polling_kthread;

	ISP_PR_INFO("success\n");
	return RET_SUCCESS;

};

void stop_work_thread(struct thread_handler *handle)
{

	if (handle == NULL) {
		ISP_PR_ERR("illegal parameter\n");
		return;
	};

	if (handle->thread) {
		handle->stop_flag = true;
		kthread_stop(handle->thread);
		handle->thread = NULL;
	} else {
		ISP_PR_ERR("thread is NULL, do nothing\n");
	}

};

int thread_should_stop(struct thread_handler __maybe_unused *handle)
{
	return kthread_should_stop();
};

/*add for dump image in kernel*/
int isp_write_file_test(struct file *fp, void *buf, ulong *len)
{
	ssize_t ret = -1;

	if (!len)
		return ret;

	ret = __kernel_write(fp, buf, *len, 0);//&fp->f_op

	//will crash when close
	//filp_close(fp, NULL);

	return ret;
}


/*add for conver the format from yuv to rgb (temporary workaround)*/

/*Build RGB565 word from red, green, and blue bytes. */
#define RGB565(r, g, b) (uint16_t)(((((uint16_t)(b) << 6) | g) << 5) | r)

/* Clips a value to the unsigned 0-255 range, treating negative values as zero.
 */
static inline int clamp_ff(int x)
{
	if (x > 255)
		return 255;
	if (x < 0)
		return 0;

	return x;
}

/*
 *YUV -> RGB conversion macros
 */

/*"Optimized" macros that take specialy prepared Y, U, and V values:
 *C = Y - 16
 *D = U - 128
 *E = V - 128
 */
#define YUV2RO(C, D, E) clamp_ff((298 * (C) + 409 * (E) + 128) >> 8)
#define YUV2GO(C, D, E) clamp_ff((298 * (C) - 100 * (D) - 208 * (E) + 128) >> 8)
#define YUV2BO(C, D, E) clamp_ff((298 * (C) + 516 * (D) + 128) >> 8)


/*Converts YUV color to RGB565. */
static uint16_t YUVToRGB565(int y, int u, int v)
{
	/*Calculate C, D, and E values for the optimized macro. */
	uint16_t r, g, b;

	y -= 16;
	u -= 128;
	v -= 128;

	r = (YUV2RO(y, u, v) >> 3) & 0x1f;
	g = (YUV2GO(y, u, v) >> 2) & 0x3f;
	b = (YUV2BO(y, u, v) >> 3) & 0x1f;

	return RGB565(r, g, b);
}

static void _YUV420SToRGB565(uint8_t *Y,
			uint8_t *U,
			uint8_t *V,
			int dUV,
			uint16_t *rgb,
			int width,
			int height)
{
	uint8_t *U_pos = U;
	uint8_t *V_pos = V;
	int x, y;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x += 2, U += dUV, V += dUV) {
			uint8_t nU = *U;
			uint8_t nV = *V;
			*rgb = YUVToRGB565(*Y, nU, nV);
			Y++;
			rgb++;
			*rgb = YUVToRGB565(*Y, nU, nV);
			Y++;
			rgb++;
		}
		if (y & 0x1) {
			U_pos = U;
			V_pos = V;
		} else {
			U = U_pos;
			V = V_pos;
		}
	}
}


/*Common converter for YUV 4:2:0 interleaved to RGB565.
 *y, u, and v point to Y,U, and V panes, where U and V values are interleaved.
 */
static void _NVXXToRGB565(uint8_t *Y,
			uint8_t *U,
			uint8_t *V,
			uint16_t *rgb,
			int width,
			int height)
{
	_YUV420SToRGB565(Y, U, V, 2, rgb, width, height);
}

/*add for conver the format from yuv to rgb (temporary workaround)*/
void NV12ToRGB565(void *nv12, void *rgb, int width, int height)
{
	int pix_total = width * height;
	uint8_t *y = (uint8_t *)(nv12);

	_NVXXToRGB565(y, y + pix_total, y + pix_total + 1,
		(uint16_t *)(rgb), width, height);
}


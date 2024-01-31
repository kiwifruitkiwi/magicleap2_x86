/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef OS_ADVANCE_TYPE_H
#define OS_ADVANCE_TYPE_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/semaphore.h>

#include "isp_module_cfg.h"

#define MAX_ISP_TIME_TICK 0x7fffffffffffffff

#define unreferenced_parameter(X) (X = X)

typedef struct mutex isp_mutex_t;

struct isp_spin_lock {
	spinlock_t lock;
};

typedef int32 isp_ret_status_t;

/*a count of 100-nanosecond intervals since january 1, 1601 */
typedef int64 isp_time_tick_t;

struct isp_event {
	int32 automatic;
	volatile int32 event;
	uint32 result;
};

struct thread_handler {
	int32 stop_flag;
	struct isp_event wakeup_evt;
	struct task_struct *thread;
	isp_mutex_t mutex;
};

typedef int32(*work_thread_prototype) (pvoid start_context);

#define os_read_reg32(address) (*((volatile uint32 *)address))
#define os_write_reg32(address, value) (*((volatile uint32 *)address) = value)

#define isp_sys_mem_alloc(size) kmalloc(size, GFP_KERNEL)
#define isp_sys_mem_free(p) kfree(p)

#define isp_mutex_init(PM)	mutex_init(PM)
#define isp_mutex_destroy(PM)	mutex_destroy(PM)
#define isp_mutex_unlock(PM)	mutex_unlock(PM)

#define isp_spin_lock_init(s_lock)	spin_lock_init(&s_lock.lock)
#define isp_spin_lock_lock(s_lock)	spin_lock(&((s_lock).lock))
#define isp_spin_lock_unlock(s_lock)	spin_unlock(&((s_lock).lock))

isp_ret_status_t isp_mutex_lock(isp_mutex_t *p_mutex);

isp_ret_status_t isp_event_init(struct isp_event *p_event, int32 auto_matic,
				int32 init_state);
isp_ret_status_t isp_event_signal(uint32 result, struct isp_event *p_event);
isp_ret_status_t isp_event_reset(struct isp_event *p_event);
isp_ret_status_t isp_event_wait(struct isp_event *p_event, uint32 timeout_ms);
void isp_get_cur_time_tick(isp_time_tick_t *ptimetick);
int32 isp_is_timeout(isp_time_tick_t *start, isp_time_tick_t *end,
		uint32 timeout_ms);
int32 create_work_thread(struct thread_handler *handle,
			work_thread_prototype working_thread, pvoid context);
void stop_work_thread(struct thread_handler *handle);
bool_t thread_should_stop(struct thread_handler *handle);

int32 polling_thread_wrapper(pvoid context);
int32 idle_detect_thread_wrapper(pvoid context);
int32 work_item_thread_wrapper(pvoid context);

int32 isp_write_file_test(struct file *fp, void *buf, ulong *len);
void NV12ToRGB565(void *nv21, void *rgb, int width, int height);

#endif

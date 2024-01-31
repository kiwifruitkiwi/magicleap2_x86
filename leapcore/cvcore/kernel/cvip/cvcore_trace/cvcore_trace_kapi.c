// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2020-2023
// Magic Leap, Inc. (COMPANY)
//

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
#include <linux/errno.h>

#include "cvcore_name_stub.h"
#include "cvcore_processor_common.h"
#include "cvcore_trace_uapi.h"
#include "cvcore_trace.h"
#include "cvcore_trace_easy.h"
#include "gsm.h"
#include "gsm_jrtc.h"
#include "gsm_cvip.h"
#include "primitive_atomic.h"
#include "shregion_types.h"

#define MODULE_NAME "cvcore_trace"
#define DEVICE_NAME "cvcore_trace"
#define CLASS_NAME "tracing"

#define CVCORE_TRACE_kernel_test_named_event_no_value_step1                    \
	((u64)0xed85b0afa37800)
#define CVCORE_TRACE_kernel_test_named_event_no_value_step2                    \
	((u64)0xd011a7d27ed800)
#define CVCORE_TRACE_kernel_test_named_event_no_value_step3_pause              \
	((u64)0x9fbf22e120f700)
#define CVCORE_TRACE_kernel_test_named_event_no_value_unexpected               \
	((u64)0x6b36876634f300)
#define CVCORE_TRACE_kernel_test_named_event_no_value_step4                    \
	((u64)0xf114659996d800)

#define CVCORE_TRACE_kernel_test_named_event_one_value_step1 (6)
#define CVCORE_TRACE_kernel_test_named_event_one_value_step2 (7)
#define CVCORE_TRACE_kernel_test_named_event_one_value_step3_pause (8)
#define CVCORE_TRACE_kernel_test_named_event_one_value_unexpected (9)
#define CVCORE_TRACE_kernel_test_named_event_one_value_step4 (10)

#define CVCORE_TRACE_kernel_test_named_event_two_values_step1 (11)
#define CVCORE_TRACE_kernel_test_named_event_two_values_step2 (12)
#define CVCORE_TRACE_kernel_test_named_event_two_values_step3_pause (13)
#define CVCORE_TRACE_kernel_test_named_event_two_values_unexpected (14)
#define CVCORE_TRACE_kernel_test_named_event_two_values_step4 (15)

#define CVCORE_TRACE_kernel_test_named_event_three_values_step1 (16)
#define CVCORE_TRACE_kernel_test_named_event_three_values_step2 (17)
#define CVCORE_TRACE_kernel_test_named_event_three_values_step3_pause (18)
#define CVCORE_TRACE_kernel_test_named_event_three_values_unexpected (19)
#define CVCORE_TRACE_kernel_test_named_event_three_values_step4 (20)

#define CVCORE_TRACE_kernel_test_named_event_four_values_step1 (21)
#define CVCORE_TRACE_kernel_test_named_event_four_values_step2 (22)
#define CVCORE_TRACE_kernel_test_named_event_four_values_step3_pause (23)
#define CVCORE_TRACE_kernel_test_named_event_four_values_unexpected (24)
#define CVCORE_TRACE_kernel_test_named_event_four_values_step4 (25)

#define CVCORE_ARM_KERNEL (CVCORE_CORE_ID_MAX + 1)
#define CVCORE_X86_KERNEL (CVCORE_CORE_ID_MAX + 2)

#define CVCORE_EASY_TRACER CVCORE_NAME_STATIC(CVCORE_EASY_TRACER)

#define CAT_FLAGS_CORE(f, c)                                                   \
	(((uint64_t)(((c)&0xFU) | (((f)&0xFU) << 4))) << 56)

#define CAT_FLAGS_CORE_TIMESTAMP(f, c, t)                                      \
	(uint64_t)(((t)&0x00FFFFFFFFFFFFFFU) | CAT_FLAGS_CORE(f, c))

#define EXTRACT_FLAGS(cft)                                                     \
	((enum cvcore_easy_trace_flags)((((uint64_t)(cft)) >> 56) & 0xFU))

#define EXTRACT_CORE(cft) ((uint8_t)((((uint64_t)(cft)) >> 60) & 0xFU))

#define EXTRACT_TIME_NS(cft) ((uint64_t)((cft)&0x00FFFFFFFFFFFFFFU))

static const int k_num_trace_desc_per_ctrl_word = 32;
static bool is_initialized_ = false;
static struct class *dev_class = NULL;
static dev_t dev = 0;
static int major_number;
static struct device *device;
static struct cdev cdev;
static struct mutex trace_buffer_lock;
const char *device_name = "tracer";

#if defined(__amd64__) || defined(__i386__)
extern int kmap_shregion(struct shregion_metadata *meta, void **vaddr);
#else
extern int kalloc_shregion(struct shregion_metadata *meta, void **vaddr);
#endif

static void *trace_buffers_[GSM_MAX_NUM_CVCORE_TRACERS] = { NULL };
// The following hex value corresponds to the hex value
// for cvcore_name "CVCORE_TRACE_BUFFERS".  The assumption
// is that this name to hex value relationship will not change.
#define CVCORE_TRACE_BUFFERS_BASE_NAME (0xb28c7d68c30400UL)
static uint64_t trace_buffer_name_ = CVCORE_TRACE_BUFFERS_BASE_NAME;
static long cvcore_trace_ioctl(struct file *, unsigned int cmd,
			       unsigned long data);
static bool is_initialized(void);
static int cvcore_trace_initialize(void);
static int handle_add_event_ioctl(struct cvcore_trace_add_event_info *info);
static struct cvcore_trace_ctx easy_trace_ctx;

static int
cvcore_easy_trace_add_pre_initialization(uint64_t tp_name, uint32_t tp_value1,
					 uint32_t tp_value2, uint32_t tp_value3,
					 uint32_t tp_value4,
					 enum cvcore_easy_trace_flags tp_flags);

static int cvcore_easy_trace_add_post_initialization(
	uint64_t tp_name, uint32_t tp_value1, uint32_t tp_value2,
	uint32_t tp_value3, uint32_t tp_value4,
	enum cvcore_easy_trace_flags tp_flags);

static int cvcore_easy_trace_add_internal_ts(
	struct cvcore_trace_ctx *trace_ctx, uint64_t tp_name,
	uint32_t tp_value1, uint32_t tp_value2, uint32_t tp_value3,
	uint32_t tp_value4, enum cvcore_easy_trace_flags tp_flags);

typedef int (*cvcore_easy_trace_logger)(uint64_t tp_name, uint32_t tp_value1,
					uint32_t tp_value2, uint32_t tp_value3,
					uint32_t tp_value4,
					enum cvcore_easy_trace_flags tp_flags);

struct cvcore_easy_tracepoint {
	uint64_t tp_flags_core_and_time_ns;
	uint64_t tp_name;
	uint32_t tp_user_value1;
	uint32_t tp_user_value2;
	uint32_t tp_user_value3;
	uint32_t tp_user_value4;
};

static int cvcore_trace_test(void);

static cvcore_easy_trace_logger logger =
	cvcore_easy_trace_add_pre_initialization;

static struct file_operations f_ops = { .owner = THIS_MODULE,
					.unlocked_ioctl = cvcore_trace_ioctl,
					.compat_ioctl = cvcore_trace_ioctl };

static void destroy_fs_device(void)
{
	cdev_del(&cdev);
	device_destroy(dev_class, MKDEV(major_number, 1));
	class_destroy(dev_class);
	unregister_chrdev_region(dev, 1);
}

static int create_fs_device(void)
{
	int ret = 0;

	/* Register char device */
	ret = register_chrdev(0, DEVICE_NAME, &f_ops);
	if (ret < 0) {
		pr_err("Failed to register with error %d\n", ret);
		goto error;
	}
	major_number = ret;
	pr_info("Device registered at %d\n", major_number);

	/* Register the device class */
	dev_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(dev_class)) {
		ret = PTR_ERR(dev_class);
		goto post_chrdev_error;
	}

	/* Register the device driver */
	device = device_create(dev_class, NULL, MKDEV(major_number, 0), NULL,
			       DEVICE_NAME);
	if (IS_ERR(device)) {
		pr_err("Failed to create the device.\n");
		ret = PTR_ERR(device);
		goto post_class_error;
	}

	pr_info("Initialized successfully.\n");
	return 0;

post_class_error:
	class_destroy(dev_class);
post_chrdev_error:
	unregister_chrdev(major_number, DEVICE_NAME);
error:
	return ret;
}

static long cvcore_trace_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long data)
{
	int res = 0;

	if ((void *)data == NULL)
		return -EINVAL;

	switch (cmd) {
	case IOCTL_CVCORE_TRACE_INITIALIZE:
		res = cvcore_trace_initialize();
		break;
	case IOCTL_CVCORE_TRACE_ADD_EVENT: {
		struct cvcore_trace_add_event_info info;

		if (copy_from_user(&info, (void *)data,
				   sizeof(struct cvcore_trace_add_event_info))) {
			return -EINVAL;
		}
		res = handle_add_event_ioctl(&info);
	} break;
	case IOCTL_CVCORE_TRACE_CREATE: {
		struct cvcore_trace_create_info info;

		if (copy_from_user(&info, (void *)data,
				   sizeof(struct cvcore_trace_create_info))) {
			return -EINVAL;
		}
		res = cvcore_trace_create(info.trace_id, info.event_size,
					  info.max_events);
	} break;
	case IOCTL_CVCORE_TRACE_TEST:
		res = cvcore_trace_test();
		break;
	default:
		res = -ENOTTY;
		break;
	}
	return res;
}

static int handle_add_event_ioctl(struct cvcore_trace_add_event_info *info)
{
	int res;
	struct cvcore_trace_ctx ctx;

	res = cvcore_trace_attach(info->trace_id, &ctx);
	if (res) {
		pr_err("Kernel Trace IOCTL: Trace could not attach - %d", res);
		return res;
	}

	res = cvcore_trace_add_event(&ctx, (uint8_t *)info->data);
	if (res) {
		pr_err("Kernel Trace IOCTL: Trace could not add event - %d",
		       res);
		return res;
	}
	return res;
}

static uint64_t cvcore_trace_find_descriptor_by_id(uint64_t unique_id,
						   uint16_t *idx,
						   uint8_t *is_early_trace)
{
	int i;
	volatile struct cvcore_trace_descriptor *ctd;

	if (idx == NULL || is_early_trace == NULL) {
		return (uint64_t)-1;
	}

	ctd = (struct cvcore_trace_descriptor *)(GSM_ADDRESS_8(
		GSMI_READ_COMMAND_RAW, GSM_RESERVED_CVCORE_TRACE_DESC_BASE));

	/**
	 * Support finding free tracers in both trace domains
	 */

	for (i = 0; i < GSM_MAX_NUM_CVCORE_TRACERS; i++) {
		if (ctd[i].trace_unique_id == unique_id) {
			*idx = (uint16_t)i;
			*is_early_trace = (uint8_t) false;
			return GSM_RESERVED_CVCORE_TRACE_DESC_BASE +
			       (uint64_t)i * GSM_CVCORE_TRACE_DESC_SIZE;
		}
	}

	ctd = (struct cvcore_trace_descriptor *)(GSM_ADDRESS_8(
		GSMI_READ_COMMAND_RAW, GSM_RESERVED_EARLY_TRACE_DESC_BASE));

	for (i = 0; i < GSM_NUM_EARLY_TRACERS; i++) {
		if (ctd[i].trace_unique_id == unique_id) {
			*idx = (uint16_t)i;
			*is_early_trace = (uint8_t) true;
			return GSM_RESERVED_EARLY_TRACE_DESC_BASE +
			       (uint64_t)i * GSM_EARLY_TRACE_DESC_SIZE;
		}
	}
	pr_err("Kernel Trace Find Descriptor ID: Can't find Trace ID");

	return (uint64_t)-1;
}

static inline uint64_t cvcore_trace_find_free_descriptor(uint16_t *idx)
{
	int word_indx;

	if (idx == NULL) {
		return (uint64_t)-1;
	}

	/**
	 * In the future support creating both Shregion and GSM based trace buffers,
	 * for now all trace buffers created in the kernel will be GSM targets.
	 */

	/**
	 * Each trace ctx has an associated bit in the control
	 * bitmap.
	 */
	for (word_indx = 0; word_indx < GSM_EARLY_TRACE_BITMAP_TABLE_SIZE / 4;
	     word_indx++) {
		uint32_t ctrl_word = GSM_RESERVED_EARLY_TRACE_BITMAP_BASE +
				     (word_indx * sizeof(uint32_t));
		uint32_t ctrl_indx =
			primitive_atomic_find_and_set_32(ctrl_word);
		if (ctrl_indx != PRIMITIVE_ATOMIC_FIND_AND_SET_ERROR) {
			/**
			 * Found a tracer location that is not in use.
			 * Determine the word address based on the control word and bit indices.
			 */
			*idx = word_indx * k_num_trace_desc_per_ctrl_word +
			       ctrl_indx;
			return GSM_RESERVED_EARLY_TRACE_DESC_BASE +
			       (*idx * GSM_EARLY_TRACE_DESC_SIZE);
		}
	}
	return (uint64_t)-1;
}

static inline uint64_t cvcore_trace_index_to_descriptor(uint16_t index)
{
	/**
	 * In the future support creating both Shregion and GSM based trace buffers,
	 * for now all trace buffers created in the kernel will be GSM targets.
	 */
	return GSM_RESERVED_EARLY_TRACE_DESC_BASE +
	       (uint64_t)(index)*GSM_EARLY_TRACE_DESC_SIZE;
}

static inline void cvcore_trace_release_descriptor(uint16_t trace_idx)
{
	/**
	 * In the future support creating both Shregion and GSM based trace buffers,
	 * for now all trace buffers created in the kernel will be GSM targets.
	 */
	uint32_t ctrl_word = GSM_RESERVED_EARLY_TRACE_BITMAP_BASE +
			     ((trace_idx / 32) * sizeof(uint32_t));
	uint32_t ctrl_indx = trace_idx % 32;

	primitive_atomic_bitwise_clear(ctrl_word, 0x1 << ctrl_indx);
}

static bool is_initialized(void)
{
	return is_initialized_ && (trace_buffer_name_ != 0);
}

static int cvcore_trace_initialize(void)
{
	is_initialized_ = true;
	return is_initialized() ? 0 : -ENODEV;
}

static void cvcore_trace_enable(const struct cvcore_trace_ctx *ctx)
{
	uint32_t trace_enable_bitmap_base =
		(ctx->is_early_trace) ?
			GSM_RESERVED_EARLY_TRACE_ENABLE_BITMAP_BASE :
			GSM_RESERVED_CVCORE_TRACE_ENABLE_BITMAP_BASE;

	uint32_t ctrl_word = trace_enable_bitmap_base +
			     ((ctx->trace_idx / 32) * sizeof(uint32_t));
	uint32_t ctrl_indx = ctx->trace_idx % 32;

	primitive_atomic_bitwise_set(ctrl_word, 0x1 << ctrl_indx);
}

static void cvcore_trace_disable(const struct cvcore_trace_ctx *ctx)
{
	uint32_t trace_enable_bitmap_base =
		(ctx->is_early_trace) ?
			GSM_RESERVED_EARLY_TRACE_ENABLE_BITMAP_BASE :
			GSM_RESERVED_CVCORE_TRACE_ENABLE_BITMAP_BASE;

	uint32_t ctrl_word = trace_enable_bitmap_base +
			     ((ctx->trace_idx / 32) * sizeof(uint32_t));
	uint32_t ctrl_indx = ctx->trace_idx % 32;

	primitive_atomic_bitwise_clear(ctrl_word, 0x1 << ctrl_indx);
}

static bool cvcore_trace_is_enabled(const struct cvcore_trace_ctx *ctx)
{
	uint32_t trace_enable_bitmap_base =
		(ctx->is_early_trace) ?
			GSM_RESERVED_EARLY_TRACE_ENABLE_BITMAP_BASE :
			GSM_RESERVED_CVCORE_TRACE_ENABLE_BITMAP_BASE;

	uint32_t ctrl_word = trace_enable_bitmap_base +
			     ((ctx->trace_idx / 32) * sizeof(uint32_t));
	uint32_t ctrl_indx = ctx->trace_idx % 32;
	uint32_t val =
		primitive_atomic_read_current(ctrl_word) & (0x1 << ctrl_indx);
	return (val == 0) ? false : true;
}

int cvcore_trace_attach(uint64_t unique_id, struct cvcore_trace_ctx *ctx)
{
	struct shregion_metadata meta;
	uint64_t offset;
	uint32_t event_info;
	int s;
	void *tmp_trace_buf;
	int ret;

	if (!is_initialized()) {
		return -ENODEV;
	}

	// Unique ID cannot be 0
	if (unique_id == 0x0) {
		return -EINVAL;
	}

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx->trace_buffer_vaddr = (uintptr_t)NULL;

	offset = cvcore_trace_find_descriptor_by_id(
		unique_id, &(ctx->trace_idx), &(ctx->is_early_trace));
	if (offset == (uint64_t)-1) {
		return -ENODEV;
	}

	ctx->idx_ctx =
		offset + offsetof(struct cvcore_trace_descriptor, trace_cnt);

	if (ctx->is_early_trace == true) {
		ctx->trace_buffer_vaddr = ((uintptr_t)GSM_ADDRESS_32(
			GSMI_WRITE_COMMAND_RAW,
			GSM_RESERVED_EARLY_TRACE_BUF_BASE +
				(ctx->trace_idx *
				 GSM_EARLY_TRACE_MAX_BUF_SIZE)));
	} else {
		if (trace_buffers_[ctx->trace_idx] == NULL) {
			/*
			 * Grab a lock and check again.
			 */
			ret = mutex_lock_killable(&trace_buffer_lock);
			if (ret) {
				return ret;
			}

			if (trace_buffers_[ctx->trace_idx] != NULL) {
				mutex_unlock(&trace_buffer_lock);
			} else {
				meta.name = (trace_buffer_name_ & ~0xFFUL) |
					    (ctx->trace_idx & 0xFF);
				meta.type = SHREGION_DDR_PRIVATE;
				meta.flags = SHREGION_DSP_MAP_ALL_STREAM_IDS |
					     SHREGION_PERSISTENT |
					     SHREGION_SHAREABLE |
					     SHREGION_DEVICE_TYPE;

				meta.cache_mask = 0;
				meta.size_bytes = CVCORE_TRACE_MAX_BUFFER_SIZE;
				meta.fixed_dsp_v_addr =
					CVCORE_TRACE_BUFFER_DSP_VADDR +
					(ctx->trace_idx *
					 CVCORE_TRACE_MAX_BUFFER_SIZE);

#if defined(__amd64__) || defined(__i386__)
				if ((s = kmap_shregion(&meta,
						       &tmp_trace_buf)) != 0) {
					pr_err("kmap_shregion error: %d\n", s);
					mutex_unlock(&trace_buffer_lock);
					return -ENOMEM;
				} else {
					trace_buffers_[ctx->trace_idx] =
						tmp_trace_buf;
					pr_info("kmap_shregion successful: %px",
						trace_buffers_[ctx->trace_idx]);
				}
#else
				if ((s = kalloc_shregion(
					     &meta, &tmp_trace_buf)) != 0) {
					pr_err("kalloc_shregion error: %d\n",
					       s);
					mutex_unlock(&trace_buffer_lock);
					return -ENOMEM;
				} else {
					trace_buffers_[ctx->trace_idx] =
						tmp_trace_buf;

					pr_info("kalloc_shregion successful: %px",
						trace_buffers_[ctx->trace_idx]);
				}
#endif
				mutex_unlock(&trace_buffer_lock);
			}
		}

		ctx->trace_buffer_vaddr =
			(uintptr_t)trace_buffers_[ctx->trace_idx];
	}

	event_info = gsm_raw_read_32(
		offset + offsetof(struct cvcore_trace_descriptor, event_size));
	ctx->event_size = event_info & 0xFFFF;
	ctx->max_events = (event_info >> 16) & 0xFFFF;
	return 0;
}
EXPORT_SYMBOL(cvcore_trace_attach);

int cvcore_trace_reset(struct cvcore_trace_ctx *ctx)
{
	int i;
	uint8_t *trace_buf_ptr;

	if (!is_initialized()) {
		return -ENODEV;
	}

	if (ctx == NULL) {
		return -EINVAL;
	}

	gsm_raw_write_32(ctx->idx_ctx, 0);

	trace_buf_ptr = (uint8_t *)(ctx->trace_buffer_vaddr);
	if (!trace_buf_ptr) {
		return -ENOMEM;
	}

	for (i = 0; i < ctx->max_events * ctx->event_size; i = i + 4) {
		gsm_raw_write_32(trace_buf_ptr + i, 0);
	}

	return 0;
}
EXPORT_SYMBOL(cvcore_trace_reset);

int cvcore_trace_add_event(const struct cvcore_trace_ctx *ctx,
			   const uint8_t *data)
{
	int32_t write_idx;
	uint8_t *write_ptr;
	uint8_t *trace_buf_ptr;

	if (!is_initialized()) {
		return -ENODEV;
	}

	if (ctx == NULL || data == NULL) {
		return -EINVAL;
	}

	if (ctx->trace_idx >= GSM_MAX_NUM_CVCORE_TRACERS) {
		return -EINVAL;
	}

	if (!cvcore_trace_is_enabled(ctx)) {
		return -EAGAIN;
	}

	write_idx = primitive_atomic_increment_ordered(ctx->idx_ctx);

	trace_buf_ptr = (uint8_t *)(ctx->trace_buffer_vaddr);
	if (!trace_buf_ptr) {
		return -ENOMEM;
	}

	write_idx = write_idx % ctx->max_events;
	write_ptr = (uint8_t *)(trace_buf_ptr + (write_idx * ctx->event_size));

	memcpy(write_ptr, data, ctx->event_size);
	return 0;
}
EXPORT_SYMBOL(cvcore_trace_add_event);

int cvcore_trace_get_events(const struct cvcore_trace_ctx *ctx, uint8_t *buf,
			    uint32_t buf_size, uint32_t *num_events)
{
	int32_t current_idx = 0;
	int copy_start;
	int copy_length;
	uint8_t *trace_buf_ptr;

	if (!is_initialized()) {
		return -ENODEV;
	}

	if (ctx == NULL || buf == NULL || num_events == NULL) {
		return -EINVAL;
	}

	if (ctx->trace_idx >= GSM_MAX_NUM_CVCORE_TRACERS) {
		return -EINVAL;
	}

	if (buf_size < (ctx->max_events * ctx->event_size)) {
		return -EINVAL;
	}

	current_idx = primitive_atomic_read_current(ctx->idx_ctx);

	trace_buf_ptr = (uint8_t *)(ctx->trace_buffer_vaddr);
	if (!trace_buf_ptr) {
		return -ENOMEM;
	}

	if (current_idx == 0) {
		*num_events = 0;
	} else if (current_idx > (int32_t)ctx->max_events) {
		// The trace buffer wrapped around at least once.
		// Copy the earliest trace data into the output buffer
		copy_start = (current_idx % ctx->max_events) * ctx->event_size;
		copy_length = (ctx->max_events * ctx->event_size) - copy_start;
		memcpy(buf, (uint8_t *)(trace_buf_ptr + copy_start),
		       copy_length);

		// Copy the most recent trace data into the output buffer
		copy_start = copy_length;
		copy_length = (ctx->max_events * ctx->event_size) - copy_start;

		memcpy(&buf[copy_start], trace_buf_ptr, copy_length);

		*num_events = ctx->max_events;
	} else {
		copy_length = current_idx * ctx->event_size;

		memcpy(&buf[0], trace_buf_ptr, copy_length);

		*num_events = current_idx;
	}

	return 0;
}
EXPORT_SYMBOL(cvcore_trace_get_events);

int cvcore_trace_pause(const struct cvcore_trace_ctx *ctx, const uint8_t *data)
{
	int status = 0;

	if (!is_initialized()) {
		return -ENODEV;
	}

	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->trace_idx >= GSM_MAX_NUM_CVCORE_TRACERS) {
		return -EINVAL;
	}

	if (data != NULL) {
		status = cvcore_trace_add_event(ctx, data);
	}

	cvcore_trace_disable(ctx);

	return status;
}
EXPORT_SYMBOL(cvcore_trace_pause);

int cvcore_trace_resume(const struct cvcore_trace_ctx *ctx)
{
	if (!is_initialized()) {
		return -ENODEV;
	}

	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->trace_idx >= GSM_MAX_NUM_CVCORE_TRACERS) {
		return -EINVAL;
	}

	cvcore_trace_enable(ctx);

	return 0;
}
EXPORT_SYMBOL(cvcore_trace_resume);

int cvcore_trace_create(uint64_t unique_id, uint32_t event_size,
			uint32_t max_events)
{
	int status;
	volatile uintptr_t desc_p;
	struct cvcore_trace_ctx ctx;
	uint64_t offset;

	/*
	 * Unique ID cannot be 0
	 */
	if (unique_id == 0x0) {
		pr_err("cvcore_trace create: invalid trace name created\n");
		return -ENODEV;
	}

	if (event_size == 0 || max_events == 0) {
		pr_err("cvcore_trace create: bad trace params passed\n");
		return -EINVAL;
	}

	if ((event_size * max_events) > GSM_EARLY_TRACE_MAX_BUF_SIZE) {
		pr_err("cvcore_trace create: trace requested too large\n");
		return -EINVAL;
	}

	if (cvcore_trace_find_descriptor_by_id(unique_id, &(ctx.trace_idx),
					       &(ctx.is_early_trace)) !=
	    (uint64_t)-1) {
		pr_err("cvcore_trace create: found pre-registered trace id \n");
		return -EINVAL;
	}

	offset = cvcore_trace_find_free_descriptor(&(ctx.trace_idx));
	if (offset == (uint64_t)-1) {
		pr_err("cvcore_trace create: could not find free trace descriptor\n");
		return -ENOMEM;
	}

	desc_p = (uintptr_t)(GSM_ADDRESS_8(GSMI_READ_COMMAND_RAW, offset));
	((struct cvcore_trace_descriptor *)(desc_p))->trace_unique_id =
		unique_id;
	((struct cvcore_trace_descriptor *)(desc_p))->event_size = event_size;
	((struct cvcore_trace_descriptor *)(desc_p))->max_events = max_events;

	status = cvcore_trace_attach(unique_id, &ctx);
	if (status != 0) {
		pr_err("cvcore_trace create: failed to attach to created tracer\n");
		return status;
	}

	cvcore_trace_reset(&ctx);
	cvcore_trace_enable(&ctx);

	return 0;
}
EXPORT_SYMBOL(cvcore_trace_create);

int cvcore_trace_destroy(uint64_t unique_id)
{
	int status;
	volatile uintptr_t desc_p;
	int i;

	uint64_t offset;
	uint8_t *trace_buf_ptr;

	struct cvcore_trace_ctx ctx;

	status = cvcore_trace_attach(unique_id, &ctx);
	if (status != 0) {
		return status;
	}

	cvcore_trace_disable(&ctx);

	offset = cvcore_trace_index_to_descriptor(ctx.trace_idx);

	desc_p = ((uintptr_t)(GSM_ADDRESS_8(GSMI_READ_COMMAND_RAW, offset)));
	((struct cvcore_trace_descriptor *)(desc_p))->trace_unique_id = 0;
	((struct cvcore_trace_descriptor *)(desc_p))->event_size = 0;
	((struct cvcore_trace_descriptor *)(desc_p))->max_events = 0;

	trace_buf_ptr = (uint8_t *)(ctx.trace_buffer_vaddr);
	if (!trace_buf_ptr) {
		return -EINVAL;
	}

	for (i = 0; i < ctx.max_events * ctx.event_size; i = i + 4) {
		gsm_raw_write_32(trace_buf_ptr + i, 0);
	}

	cvcore_trace_release_descriptor(ctx.trace_idx);

	return 0;
}
EXPORT_SYMBOL(cvcore_trace_destroy);

int cvcore_easy_trace_initialize(void)
{
	int status;

	status = cvcore_trace_initialize();
	if (status != 0) {
		return status;
	}

	logger = cvcore_easy_trace_add_post_initialization;

	return cvcore_trace_attach(CVCORE_EASY_TRACER, &easy_trace_ctx);
}
EXPORT_SYMBOL(cvcore_easy_trace_initialize);

int cvcore_easy_trace_pause(uint64_t tp_name, uint32_t tp_value1,
			    uint32_t tp_value2, uint32_t tp_value3,
			    uint32_t tp_value4,
			    enum cvcore_easy_trace_flags tp_flags)
{
	if (tp_flags == kFlagIsPauseNoLog) {
		return cvcore_trace_pause(&easy_trace_ctx, NULL);
	} else {
		struct cvcore_easy_tracepoint tp;
		uint8_t core_id;
		uint64_t ts_ns;

#if defined(__arm__) || defined(__aarch64__)
		core_id = CVCORE_ARM_KERNEL;
#elif defined(__i386__) || defined(__amd64__)
		core_id = CVCORE_X86_KERNEL;
#else
		return -1;
#endif
		ts_ns = gsm_jrtc_get_time_ns();

		tp.tp_flags_core_and_time_ns =
			CAT_FLAGS_CORE_TIMESTAMP(tp_flags, core_id, ts_ns);
		tp.tp_name = tp_name;
		tp.tp_user_value1 = tp_value1;
		tp.tp_user_value2 = tp_value2;
		tp.tp_user_value3 = tp_value3;
		tp.tp_user_value4 = tp_value4;

		return cvcore_trace_pause(&easy_trace_ctx, (uint8_t *)&tp);
	}
}
EXPORT_SYMBOL(cvcore_easy_trace_pause);

int cvcore_easy_trace_resume(void)
{
	return cvcore_trace_resume(&easy_trace_ctx);
}
EXPORT_SYMBOL(cvcore_easy_trace_resume);

int cvcore_easy_trace_add_pre_initialization(
	uint64_t tp_name, uint32_t tp_value1, uint32_t tp_value2,
	uint32_t tp_value3, uint32_t tp_value4,
	enum cvcore_easy_trace_flags tp_flags)
{
	int status;

	status = cvcore_easy_trace_initialize();
	if (status) {
		printk("Could not initialize tracer %d", status);
		return status;
	}

	return cvcore_easy_trace_add_post_initialization(
		tp_name, tp_value1, tp_value2, tp_value3, tp_value4, tp_flags);
}

int cvcore_easy_trace_add_post_initialization(
	uint64_t tp_name, uint32_t tp_value1, uint32_t tp_value2,
	uint32_t tp_value3, uint32_t tp_value4,
	enum cvcore_easy_trace_flags tp_flags)
{
	int status;

	status = cvcore_easy_trace_add_internal_ts(&easy_trace_ctx, tp_name,
						   tp_value1, tp_value2,
						   tp_value3, tp_value4,
						   tp_flags);
	if (status) {
		printk("Could not add to trace %d", status);
	}
	return status;
}

int cvcore_easy_trace_add(uint64_t tp_name, uint32_t tp_value1,
			  uint32_t tp_value2, uint32_t tp_value3,
			  uint32_t tp_value4,
			  enum cvcore_easy_trace_flags tp_flags)
{
	return logger(tp_name, tp_value1, tp_value2, tp_value3, tp_value4,
		      tp_flags);
}
EXPORT_SYMBOL(cvcore_easy_trace_add);

static int cvcore_easy_trace_add_internal_ts(
	struct cvcore_trace_ctx *trace_ctx, uint64_t tp_name,
	uint32_t tp_value1, uint32_t tp_value2, uint32_t tp_value3,
	uint32_t tp_value4, enum cvcore_easy_trace_flags tp_flags)
{
	struct cvcore_easy_tracepoint tp;
	uint8_t core_id;
	uint64_t ts_ns;

#if defined(__arm__) || defined(__aarch64__)
	core_id = CVCORE_CORE_ID_MAX + 1; // ARM Kernel
#elif defined(__i386__) || defined(__amd64__)
	core_id = CVCORE_CORE_ID_MAX + 2; // X86 Kernel
#else
	return -1;
#endif

	ts_ns = gsm_jrtc_get_time_ns();

	tp.tp_flags_core_and_time_ns =
		CAT_FLAGS_CORE_TIMESTAMP(tp_flags, core_id, ts_ns);
	tp.tp_name = tp_name;
	tp.tp_user_value1 = tp_value1;
	tp.tp_user_value2 = tp_value2;
	tp.tp_user_value3 = tp_value3;
	tp.tp_user_value4 = tp_value4;

	return cvcore_trace_add_event(trace_ctx, (uint8_t *)&tp);
}

static int cvcore_trace_test(void)
{
	// Test anonymous events
	cvcore_easy_tracepoint_begin();
	cvcore_easy_tracepoint_add();
	cvcore_easy_tracer_pause();
	cvcore_easy_tracepoint_add();
	cvcore_easy_tracer_resume();
	cvcore_easy_tracepoint_end();

	// Test named event with no values
	cvcore_easy_tracepoint_begin_event(
		CVCORE_TRACE_kernel_test_named_event_no_value_step1);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_no_value_step2);
	cvcore_easy_tracer_pause_event(
		CVCORE_TRACE_kernel_test_named_event_no_value_step3_pause);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_no_value_unexpected);
	cvcore_easy_tracer_resume();
	cvcore_easy_tracepoint_end_event(
		CVCORE_TRACE_kernel_test_named_event_no_value_step4);

	// Test named event with one value
	cvcore_easy_tracepoint_begin_event(
		CVCORE_TRACE_kernel_test_named_event_one_value_step1, 1);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_one_value_step2, 2);
	cvcore_easy_tracer_pause_event(
		CVCORE_TRACE_kernel_test_named_event_one_value_step3_pause, 3);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_one_value_unexpected,
		0xFF);
	cvcore_easy_tracer_resume();
	cvcore_easy_tracepoint_end_event(
		CVCORE_TRACE_kernel_test_named_event_one_value_step4, 4);

	// Test named event with two values
	cvcore_easy_tracepoint_begin_event(
		CVCORE_TRACE_kernel_test_named_event_two_values_step1, 1, 2);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_two_values_step2, 3, 4);
	cvcore_easy_tracer_pause_event(
		CVCORE_TRACE_kernel_test_named_event_two_values_step3_pause, 5,
		6);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_two_values_unexpected,
		0xFF, 0xFF);
	cvcore_easy_tracer_resume();
	cvcore_easy_tracepoint_end_event(
		CVCORE_TRACE_kernel_test_named_event_two_values_step4, 7, 8);

	// Test named event with three values
	cvcore_easy_tracepoint_begin_event(
		CVCORE_TRACE_kernel_test_named_event_three_values_step1, 1, 2,
		3);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_three_values_step2, 4, 5,
		6);
	cvcore_easy_tracer_pause_event(
		CVCORE_TRACE_kernel_test_named_event_three_values_step3_pause,
		7, 8, 9);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_three_values_unexpected,
		0xFF);
	cvcore_easy_tracer_resume();
	cvcore_easy_tracepoint_end_event(
		CVCORE_TRACE_kernel_test_named_event_three_values_step4, 10, 11,
		12);

	// Test named event with four values
	cvcore_easy_tracepoint_begin_event(
		CVCORE_TRACE_kernel_test_named_event_four_values_step1, 1, 2, 3,
		4);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_four_values_step2, 5, 6, 7,
		8);
	cvcore_easy_tracer_pause_event(
		CVCORE_TRACE_kernel_test_named_event_four_values_step3_pause, 9,
		10, 11, 12);
	cvcore_easy_tracepoint_add_event(
		CVCORE_TRACE_kernel_test_named_event_four_values_unexpected,
		0xFF);
	cvcore_easy_tracer_resume();
	cvcore_easy_tracepoint_end_event(
		CVCORE_TRACE_kernel_test_named_event_four_values_step4, 13, 14,
		15, 16);

	return 0;
}

static int __init cvcore_trace_init(void)
{
	int ret;
	int i;

	for (i = 0; i < GSM_MAX_NUM_CVCORE_TRACERS; i++) {
		trace_buffers_[i] = NULL;
	}

	ret = create_fs_device();
	if (ret) {
		pr_info("Failed to create cvcore_trace module\n");
		return ret;
	}

	mutex_init(&trace_buffer_lock);

	printk("Successfully loaded cvcore_trace module\n");
	return 0;
}

static void __exit cvcore_trace_exit(void)
{
	destroy_fs_device();
	printk("Unloading cvcore_trace module\n");
}

module_init(cvcore_trace_init);
module_exit(cvcore_trace_exit);

MODULE_AUTHOR("Kevin Irick<kirick_siliconscapes@magicleap.com>");
MODULE_DESCRIPTION("cvcore_trace module");
MODULE_LICENSE("GPL");

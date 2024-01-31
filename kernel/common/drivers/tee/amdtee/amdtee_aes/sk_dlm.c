// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2020-2020 Advanced Micro Devices, Inc. All rights reserved.
 */
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/sysfs.h>

#include <linux/tee_drv.h>
#include <linux/psp-tee.h>

#include "../amdtee_if.h"
#include "../amdtee_private.h"
#include "sk_api.h"

#ifdef DLM_SUPPORT

static DEFINE_MUTEX(dlm_session_list_mutex);

static int allocate_dlm_buffer(unsigned long *vaddr, u32 size)
{
	/* Size of DLM buffer must be multiple of 4K */
	if (size & (PAGE_SIZE - 1)) {
		pr_err("%s: invalid size %u\n", __func__, size);
		return -EINVAL;
	}

	*vaddr = __get_free_pages(GFP_KERNEL, get_order(size));
	if (NULL == (void *) *vaddr) {
		pr_err("%s: get_free_pages failed\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

static void deallocate_dlm_buffer(unsigned long dlm_buffer_va, u32 size)
{
	free_pages(dlm_buffer_va, get_order(size));
}

static struct dlm_manager *
sk_dlm_find_manager(const struct tee_context *tee_ctx, uint32_t sid)
{
	struct amdtee_context_data *ctxdata =
		(struct amdtee_context_data *)tee_ctx->data;
	struct dlm_manager *dlm_man = NULL;
	int i;

	pr_debug("%s enter\n", __func__);

	mutex_lock(&dlm_session_list_mutex);

	for (i = 0; i < MAX_ALLOWED_DLM_BUFFERS; ++i) {
		if (!ctxdata->dlm_manager[i].in_use)
			continue;

		if (ctxdata->dlm_manager[i].dlm_session_id == sid)
			break;
	}

	mutex_unlock(&dlm_session_list_mutex);

	if (i >= MAX_ALLOWED_DLM_BUFFERS)
		pr_err("%s: Invalid session id %u\n", __func__, sid);
	else
		dlm_man =  &ctxdata->dlm_manager[i];

	pr_debug("%s: exit dlm=0x%llx, i=%d\n", __func__, (uint64_t)dlm_man, i);

	return dlm_man;
}

static TEEC_Result sk_dlm_restart_session(const struct tee_context *tee_ctx,
					const struct TEEC_UUID *ta_uuid,
					int32_t *dlm_sid)
{
	struct tee_cmd_stop_ta_dbg cmd_stop = {0};
	struct tee_cmd_start_ta_dbg cmd_start = { {0} };
	phys_addr_t dlm_buf_paddr;

	int ret = -1;
	u32 ori_ret = 0;

	struct dlm_manager *dlm_man = sk_dlm_find_manager(tee_ctx, *dlm_sid);

	if (!dlm_man) {
		pr_err("%s: wrong sid?\n", __func__);
		ret = TEEC_ERROR_BAD_STATE;
		goto quit;
	}

	pr_debug("%s:stop session %d first\n", __func__, *dlm_sid);

	/* stop TA debug first */
	cmd_stop.sid = *dlm_sid;
	ret = psp_tee_process_cmd(TEE_CMD_ID_STOP_TA_DEBUG, (void *)&cmd_stop,
					sizeof(cmd_stop), &ori_ret);
	if (ret || ori_ret) {
		pr_err("%s:submit to ring failed. 0x%x, 0x%x\n", __func__,
			ret, ori_ret);

		dlm_man->in_use = false;
		deallocate_dlm_buffer(dlm_man->buffer_va, dlm_man->buffer_size);
		dlm_man->buffer_va = 0;
		dlm_man->buffer_size = 0;

		goto quit;
	}

	/*re-start a session*/
	dlm_buf_paddr = virt_to_phys((void *)dlm_man->buffer_va);

	pr_debug("%s: dlm_buf_paddr 0x%llx\n", __func__, dlm_buf_paddr);

	if (dlm_buf_paddr & (PAGE_SIZE - 1)) {
		pr_err("%s: page unaligned. DLM buffer 0x%llx", __func__,
			dlm_buf_paddr);
		ret =  TEEC_ERROR_BAD_STATE;
		goto quit;
	}
	cmd_start.low_addr = lower_32_bits(dlm_buf_paddr);
	cmd_start.hi_addr = upper_32_bits(dlm_buf_paddr);
	cmd_start.size = dlm_man->buffer_size;
	memcpy(cmd_start.ta_uuid, ta_uuid, sizeof(cmd_start.ta_uuid));

	ret = psp_tee_process_cmd(TEE_CMD_ID_START_TA_DEBUG, (void *)&cmd_start,
					sizeof(cmd_start), &ori_ret);
	if (ret || ori_ret) {
		pr_err("%s: psp_tee_process_cmd ret %d, %d\n", __func__,
				ret, ori_ret);

		dlm_man->in_use = false;
		deallocate_dlm_buffer(dlm_man->buffer_va, dlm_man->buffer_size);
		dlm_man->buffer_va = 0;
		dlm_man->buffer_size = 0;

		goto quit;
	}

	dlm_man->dlm_session_id = cmd_start.sid;
	*dlm_sid = cmd_start.sid;

	pr_debug("%s: sid=%d, ret=%d, ori_ret=%d\n",
		__func__, cmd_start.sid, ret, ori_ret);

	return TEEC_SUCCESS;

quit:
	*dlm_sid = -1;
	return ret;
}

static void sk_dlm_restart_all_sessions(struct amdtee_alg_dev *p_dev)
{
	struct amdtee_alg_ctx *tfm_ctx;

	mutex_lock(&p_dev->dev_mutex);
	list_for_each_entry(tfm_ctx, &p_dev->tfm_list, list) {
		if (tfm_ctx->dlm_sid != -1)
			sk_dlm_restart_session(p_dev->ctx, &ta_uuid,
						&tfm_ctx->dlm_sid);
	}
	mutex_unlock(&p_dev->dev_mutex);

}

/* read token from usermode with either one
 * cat /sys/bus/tee/devices/amdtee-clnt0/dlm_debug_token > /data/debug_token.bin
 * cat /sys/devices/amdtee-clnt0/dlm_debug_token > /data/dlm_token.bin
 */
ssize_t sk_dlm_token_data_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buffer,
				loff_t pos, size_t count)
{
	struct amdtee_alg_dev *p_dev =
		container_of(bin_attr, struct amdtee_alg_dev, attr);
	size_t ret = count;

	if (pos >= p_dev->dt.dlm_token_size) {
		ret = 0;
		goto ret;
	}
	if (p_dev->dt.dlm_token_size - pos < count) {
		memcpy(buffer, p_dev->dt.dlm_token + pos,
			p_dev->dt.dlm_token_size - pos);

		pr_debug("%s: token size %d, %ld\n", __func__,
			p_dev->dt.dlm_token_size, count);
		ret = p_dev->dt.dlm_token_size - pos;
		goto ret;
	}

	memcpy(buffer, p_dev->dt.dlm_token + pos, count);
ret:
	pr_debug("%s: token data read ret=%ld\n", __func__, ret);

	return ret;
}

/* write the signed token back
 * cat /data/signed_debug_token.bin > /sys/devices/amdtee-clnt0/dlm_debug_token
 */
ssize_t sk_dlm_token_data_write(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buffer,
				loff_t pos, size_t count)
{
	struct amdtee_alg_dev *p_dev =
		container_of(bin_attr, struct amdtee_alg_dev, attr);

	if (count > MAX_DEBUG_TOKEN_SIZE) {
		pr_err("%s: token size %d, %ld\n", __func__,
			p_dev->dt.dlm_token_size, count);
		return 0;
	}
	memcpy(p_dev->dt.dlm_token + pos, buffer, count);
	p_dev->dt.dlm_token_size = count;

	sk_dlm_restart_all_sessions(p_dev);

	pr_debug("%s: token data written %ld\n", __func__, count);

	return count;
}

TEEC_Result sk_dlm_get_dbg_token(struct debug_token *dt)
{
	struct tee_cmd_get_dbg_token cmd = { {0} };
	u32 ori_ret = -1;
	int ret = 0;

	ret = psp_tee_process_cmd(TEE_CMD_ID_GET_DEBUG_TOKEN, (void *)&cmd,
				sizeof(cmd), &ori_ret);

	if (ori_ret == TEEC_SUCCESS) {
		if (sizeof(cmd.token) >= MAX_DEBUG_TOKEN_SIZE) {
			pr_err("%s: token size %ld bigger than %d",
				__func__, sizeof(cmd.token),
				MAX_DEBUG_TOKEN_SIZE);
			ret = TEEC_ERROR_EXCESS_DATA;
			goto ret;
		}
		memcpy(dt->dlm_token, &cmd.token[0], sizeof(cmd.token));
		dt->dlm_token_size = sizeof(cmd.token);
		ret = TEEC_SUCCESS;

	} else {
		pr_err("%s: cannot get debug token %d, %d\n",
			__func__, ret, ori_ret);
		ret = TEEC_ERROR_COMMUNICATION;
		dt->dlm_token_size = 0;
		goto ret;
	}

	pr_debug("%s: token size %d\n", __func__, dt->dlm_token_size);

ret:
	return ret;
}
EXPORT_SYMBOL(sk_dlm_get_dbg_token);

static struct dlm_manager *
sk_dlm_get_buf_manager(const struct tee_context *tee_ctx,
			const struct debug_token *dt)
{
	struct amdtee_context_data *ctxdata =
		(struct amdtee_context_data *)tee_ctx->data;
	struct dlm_manager *dlm_man = NULL;
	unsigned long vaddr = 0;
	struct amdtee_dlm_buf *dlm_buffer;
	u32 dlm_buffer_size = sizeof(struct amdtee_dlm_buf);
	int i, ret;

	pr_debug("%s enter\n", __func__);

	mutex_lock(&dlm_session_list_mutex);

	for (i = 0; i < MAX_ALLOWED_DLM_BUFFERS; ++i)
		if (!ctxdata->dlm_manager[i].in_use)
			break;

	if (i >= MAX_ALLOWED_DLM_BUFFERS) {
		dlm_man = ERR_PTR(-ENOMEM);
		goto quit;
	}
	ret = allocate_dlm_buffer(&vaddr, dlm_buffer_size);
	if (ret) {
		dlm_man = ERR_PTR(-ENOMEM);
		goto quit;
	}
	pr_debug("%s: DLM buffer vaddr 0x%lx size %d, i=%d\n",
		__func__, vaddr, dlm_buffer_size, i);

	dlm_buffer = (struct amdtee_dlm_buf *)vaddr;
	memset(dlm_buffer, 0, dlm_buffer_size);
	dlm_buffer->dlm_size = dlm_buffer_size;
	dlm_buffer->num_strings = DLM_NUM_STRINGS;
	memcpy(dlm_buffer->debug_token, dt->dlm_token,
		dt->dlm_token_size);

	ctxdata->dlm_manager[i].buffer_va = vaddr;
	ctxdata->dlm_manager[i].buffer_size = dlm_buffer_size;
	ctxdata->dlm_manager[i].in_use = true;

	dlm_man = &ctxdata->dlm_manager[i];

quit:
	mutex_unlock(&dlm_session_list_mutex);

	pr_debug("%s exit 0x%llx\n", __func__, (u64)dlm_man);

	return dlm_man;
}

TEEC_Result sk_dlm_start_session(const struct tee_context *tee_ctx,
				const struct debug_token *dt,
				const struct TEEC_UUID *ta_uuid,
				uint32_t *dlm_sid)
{
	struct dlm_manager *dlm_man = NULL;
	phys_addr_t dlm_buf_paddr;
	struct tee_cmd_start_ta_dbg cmd = { {0} };
	int ret;
	u32 ori_ret = 0;

	dlm_man = sk_dlm_get_buf_manager(tee_ctx, dt);
	if (IS_ERR(dlm_man)) {
		pr_err("%s: failed get DLM buffer manager\n", __func__);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	dlm_buf_paddr = virt_to_phys((void *)dlm_man->buffer_va);

	pr_debug("%s: dlm_buf_paddr 0x%llx\n", __func__, dlm_buf_paddr);

	if (dlm_buf_paddr & (PAGE_SIZE - 1)) {
		pr_err("%s: page unaligned. DLM buffer 0x%llx", __func__,
			dlm_buf_paddr);
		return TEEC_ERROR_BAD_STATE;
	}

	cmd.low_addr = lower_32_bits(dlm_buf_paddr);
	cmd.hi_addr = upper_32_bits(dlm_buf_paddr);
	cmd.size = dlm_man->buffer_size;
	memcpy(cmd.ta_uuid, ta_uuid, sizeof(cmd.ta_uuid));

	ret = psp_tee_process_cmd(TEE_CMD_ID_START_TA_DEBUG, (void *)&cmd,
					sizeof(cmd), &ori_ret);
	if (ret || ori_ret) {
		pr_err("%s: psp_tee_process_cmd ret %d\n", __func__, ori_ret);

		dlm_man->in_use = false;
		deallocate_dlm_buffer(dlm_man->buffer_va, dlm_man->buffer_size);
		dlm_man->buffer_va = 0;
		dlm_man->buffer_size = 0;

		*dlm_sid = -1;
	} else {
		dlm_man->dlm_session_id = cmd.sid;
		*dlm_sid = cmd.sid;
	}

	pr_debug("%s: sid=%d, ret=%d, ori_ret=%d\n",
		__func__, cmd.sid, ret, ori_ret);

	return ret;
}
EXPORT_SYMBOL(sk_dlm_start_session);

TEEC_Result sk_dlm_fetch_dbg_string(const struct tee_context *tee_ctx,
					uint32_t dlm_sid)
{
	struct dlm_manager *dlm_man = NULL;
	struct amdtee_dlm_buf *dlm_buf = NULL;
	u32 rptr, wptr, string_size, i;

	dlm_man = sk_dlm_find_manager(tee_ctx, dlm_sid);
	if (!dlm_man) {
		pr_err("%s: wrong dlm_sid?\n", __func__);
		return TEEC_ERROR_BAD_STATE;
	}

	dlm_buf = (struct amdtee_dlm_buf *)dlm_man->buffer_va;

	rptr = dlm_buf->rptr;
	wptr = dlm_buf->wptr;
	if (wptr >= rptr)
		string_size = wptr - rptr;
	else
		string_size = DLM_NUM_STRINGS - (rptr - wptr);

	pr_debug("%s: rptr=%d, wptr=%d, string_size=%d\n",
		__func__, rptr, wptr, string_size);

	for (i = 0; i < string_size; i++) {
		pr_info("%s: %s\n", __func__,
			dlm_buf->dlm_string[rptr].string);
		rptr++;
		if (rptr >= DLM_NUM_STRINGS)
			rptr = 0;
	}
	dlm_buf->rptr = rptr;

	return TEEC_SUCCESS;
}
EXPORT_SYMBOL(sk_dlm_fetch_dbg_string);

TEEC_Result
sk_dlm_stop_session(const struct tee_context *tee_ctx, uint32_t dlm_sid)
{
	struct tee_cmd_stop_ta_dbg cmd = {0};
	TEEC_Result ret = TEEC_SUCCESS;
	u32 ori_ret = 0;
	struct dlm_manager *dlm_man = sk_dlm_find_manager(tee_ctx, dlm_sid);

	if (!dlm_man) {
		pr_err("%s: wrong sid?\n", __func__);
		return TEEC_ERROR_BAD_STATE;
	}

	pr_debug("%s:dlm_sid %d\n", __func__, dlm_sid);

	/* stop TA debug first */
	cmd.sid = dlm_sid;
	ret = psp_tee_process_cmd(TEE_CMD_ID_STOP_TA_DEBUG, (void *)&cmd,
				sizeof(cmd), &ori_ret);
	if (ret || ori_ret) {
		pr_err("%s:submit to ring failed. 0x%x, 0x%x\n", __func__,
			ret, ori_ret);
	}
	/* now release the buffer */
	dlm_man->in_use = false;
	deallocate_dlm_buffer(dlm_man->buffer_va, dlm_man->buffer_size);
	dlm_man->buffer_va = 0;
	dlm_man->buffer_size = 0;

	return ret;
}
EXPORT_SYMBOL(sk_dlm_stop_session);

#endif


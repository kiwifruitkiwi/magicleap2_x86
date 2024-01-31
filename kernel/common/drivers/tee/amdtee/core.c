// SPDX-License-Identifier: MIT
/*
 * Copyright 2019-2021 Advanced Micro Devices, Inc.
 */

#include <linux/errno.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include "amdtee_private.h"
#include "../tee_private.h"
#include <linux/psp-tee.h>
#include "../optee/optee_private.h"

static struct amdtee_driver_data *drv_data;
static DEFINE_MUTEX(session_list_mutex);

static void amdtee_get_version(struct tee_device *teedev,
			       struct tee_ioctl_version_data *vers)
{
	struct tee_ioctl_version_data v = {
		.impl_id = TEE_IMPL_ID_AMDTEE,
		.impl_caps = 0,
		.gen_caps = TEE_GEN_CAP_GP,
	};
	*vers = v;
}

static int amdtee_open(struct tee_context *ctx)
{
	struct amdtee_context_data *ctxdata;
	uint8_t i = 0;

	ctxdata = kzalloc(sizeof(*ctxdata), GFP_KERNEL);
	if (!ctxdata)
		return -ENOMEM;

	INIT_LIST_HEAD(&ctxdata->sess_list);
	INIT_LIST_HEAD(&ctxdata->shm_list);
	mutex_init(&ctxdata->shm_mutex);

	for (i = 0; i < MAX_ALLOWED_DLM_BUFFERS; ++i)
		ctxdata->dlm_manager[i].in_use = false;

	ctx->data = ctxdata;
	return 0;
}

static void release_session(struct amdtee_session *sess)
{
	int i;

	/* Close any open session */
	for (i = 0; i < TEE_NUM_SESSIONS; ++i) {
		/* Check if session entry 'i' is valid */
		if (!test_bit(i, sess->sess_mask))
			continue;

		handle_close_session(sess->ta_handle, sess->session_info[i]);
		handle_unload_ta(sess->ta_handle);
	}

	kfree(sess);
}

static void amdtee_release(struct tee_context *ctx)
{
	struct amdtee_context_data *ctxdata = ctx->data;
	int i;

	if (!ctxdata)
		return;

	for (i = 0; i < MAX_ALLOWED_DLM_BUFFERS; ++i) {
		if (ctxdata->dlm_manager[i].in_use == true) {
			struct tee_ioctl_stop_ta_debug_data data;

			ctxdata->dlm_manager[i].in_use = false;
			dma_free_coherent(&ctx->teedev->dev,
					  ctxdata->dlm_manager[i].buffer_size,
					  ctxdata->dlm_manager[i].buffer_va,
					  ctxdata->dlm_manager[i].buffer_dma);
			data.dlm_session_id =
				ctxdata->dlm_manager[i].dlm_session_id;
			pr_info("%s: calling stop sid %u\n", __func__,
					data.dlm_session_id);

			handle_stop_ta_debug(&data);
			pr_info("%s: ret = 0x%x\n", __func__, data.ret);
		}
	}

	while (true) {
		struct amdtee_session *sess;

		sess = list_first_entry_or_null(&ctxdata->sess_list,
						struct amdtee_session,
						list_node);

		if (!sess)
			break;

		list_del(&sess->list_node);
		release_session(sess);
	}
	mutex_destroy(&ctxdata->shm_mutex);
	kfree(ctxdata);

	ctx->data = NULL;
}

/**
 * alloc_session() - Allocate a session structure
 * @ctxdata:    TEE Context data structure
 * @session:    Session ID for which 'struct amdtee_session' structure is to be
 *              allocated.
 *
 * Scans the TEE context's session list to check if TA is already loaded in to
 * TEE. If yes, returns the 'session' structure for that TA. Else allocates,
 * initializes a new 'session' structure and adds it to context's session list.
 *
 * The caller must hold a mutex.
 *
 * Returns:
 * 'struct amdtee_session *' on success and NULL on failure.
 */
static struct amdtee_session *alloc_session(struct amdtee_context_data *ctxdata,
					    u32 session)
{
	struct amdtee_session *sess;
	u32 ta_handle = get_ta_handle(session);

	/* Scan session list to check if TA is already loaded in to TEE */
	list_for_each_entry(sess, &ctxdata->sess_list, list_node)
		if (sess->ta_handle == ta_handle) {
			kref_get(&sess->refcount);
			return sess;
		}

	/* Allocate a new session and add to list */
	sess = kzalloc(sizeof(*sess), GFP_KERNEL);
	if (sess) {
		sess->ta_handle = ta_handle;
		kref_init(&sess->refcount);
		spin_lock_init(&sess->lock);
		list_add(&sess->list_node, &ctxdata->sess_list);
	}

	return sess;
}

/* Requires mutex to be held */
static struct amdtee_session *find_session(struct amdtee_context_data *ctxdata,
					   u32 session)
{
	u32 ta_handle = get_ta_handle(session);
	u32 index = get_session_index(session);
	struct amdtee_session *sess;

	if (index >= TEE_NUM_SESSIONS)
		return NULL;

	list_for_each_entry(sess, &ctxdata->sess_list, list_node)
		if (ta_handle == sess->ta_handle &&
		    test_bit(index, sess->sess_mask))
			return sess;

	return NULL;
}

u32 get_buffer_id(struct tee_shm *shm)
{
	struct amdtee_context_data *ctxdata = shm->ctx->data;
	struct amdtee_shm_data *shmdata;
	u32 buf_id = 0;

	mutex_lock(&ctxdata->shm_mutex);
	list_for_each_entry(shmdata, &ctxdata->shm_list, shm_node)
		if (shmdata->kaddr == shm->kaddr) {
			buf_id = shmdata->buf_id;
			break;
		}
	mutex_unlock(&ctxdata->shm_mutex);
	return buf_id;
}

static DEFINE_MUTEX(drv_mutex);
static int copy_ta_binary(struct tee_context *ctx, void *ptr, void **ta,
			  size_t *ta_size)
{
	const struct firmware *fw;
	char fw_name[TA_PATH_MAX];
	struct {
		u32 lo;
		u16 mid;
		u16 hi_ver;
		u8 seq_n[8];
	} *uuid = ptr;
	int n, rc = 0;

	n = snprintf(fw_name, TA_PATH_MAX,
		     "%s/%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x.bin",
		     TA_LOAD_PATH, uuid->lo, uuid->mid, uuid->hi_ver,
		     uuid->seq_n[0], uuid->seq_n[1],
		     uuid->seq_n[2], uuid->seq_n[3],
		     uuid->seq_n[4], uuid->seq_n[5],
		     uuid->seq_n[6], uuid->seq_n[7]);
	if (n < 0 || n >= TA_PATH_MAX) {
		pr_err("failed to get firmware name\n");
		return -EINVAL;
	}

	mutex_lock(&drv_mutex);
	n = request_firmware(&fw, fw_name, &ctx->teedev->dev);
	if (n) {
		pr_err("failed to load firmware %s\n", fw_name);
		rc = -ENOMEM;
		goto unlock;
	}

	*ta_size = roundup(fw->size, PAGE_SIZE);
	*ta = (void *)__get_free_pages(GFP_KERNEL, get_order(*ta_size));
	if (IS_ERR(*ta)) {
		pr_err("%s: get_free_pages failed 0x%llx\n", __func__,
		       (u64)*ta);
		rc = -ENOMEM;
		goto rel_fw;
	}

	memcpy(*ta, fw->data, fw->size);
#ifdef CONFIG_ARCH_MERO_CVIP
	__dma_map_area(*ta, *ta_size, DMA_TO_DEVICE);
#endif
rel_fw:
	release_firmware(fw);
unlock:
	mutex_unlock(&drv_mutex);
	return rc;
}

static void destroy_session(struct kref *ref)
{
	struct amdtee_session *sess = container_of(ref, struct amdtee_session,
						   refcount);

	mutex_lock(&session_list_mutex);
	list_del(&sess->list_node);
	mutex_unlock(&session_list_mutex);
	kfree(sess);
}

int amdtee_open_session(struct tee_context *ctx,
			struct tee_ioctl_open_session_arg *arg,
			struct tee_param *param)
{
	struct amdtee_context_data *ctxdata = ctx->data;
	struct amdtee_session *sess = NULL;
	u32 session_info, ta_handle;
	size_t ta_size;
	int rc, i;
	void *ta;

	if (arg->clnt_login != TEE_IOCTL_LOGIN_PUBLIC) {
		pr_err("unsupported client login method\n");
		return -EINVAL;
	}

	rc = copy_ta_binary(ctx, &arg->uuid[0], &ta, &ta_size);
	if (rc) {
		pr_err("failed to copy TA binary\n");
		return rc;
	}

	/* Load the TA binary into TEE environment */
	handle_load_ta(ta, ta_size, arg);
	if (arg->ret != TEEC_SUCCESS)
		goto out;

	ta_handle = get_ta_handle(arg->session);

	mutex_lock(&session_list_mutex);
	sess = alloc_session(ctxdata, arg->session);
	mutex_unlock(&session_list_mutex);

	if (!sess) {
		handle_unload_ta(ta_handle);
		rc = -ENOMEM;
		goto out;
	}

	/* Find an empty session index for the given TA */
	spin_lock(&sess->lock);
	i = find_first_zero_bit(sess->sess_mask, TEE_NUM_SESSIONS);
	if (i < TEE_NUM_SESSIONS)
		set_bit(i, sess->sess_mask);
	spin_unlock(&sess->lock);

	if (i >= TEE_NUM_SESSIONS) {
		pr_err("reached maximum session count %d\n", TEE_NUM_SESSIONS);
		handle_unload_ta(ta_handle);
		kref_put(&sess->refcount, destroy_session);
		rc = -ENOMEM;
		goto out;
	}

	/* Open session with loaded TA */
	handle_open_session(arg, &session_info, param);
	if (arg->ret != TEEC_SUCCESS) {
		pr_err("open_session failed %d\n", arg->ret);
		spin_lock(&sess->lock);
		clear_bit(i, sess->sess_mask);
		spin_unlock(&sess->lock);
		handle_unload_ta(ta_handle);
		kref_put(&sess->refcount, destroy_session);
		goto out;
	}

	sess->session_info[i] = session_info;
	set_session_id(ta_handle, i, &arg->session);
out:
	free_pages((u64)ta, get_order(ta_size));
	return rc;
}

int amdtee_close_session(struct tee_context *ctx, u32 session)
{
	struct amdtee_context_data *ctxdata = ctx->data;
	u32 i, ta_handle, session_info;
	struct amdtee_session *sess;

	pr_debug("%s: sid = 0x%x\n", __func__, session);

	/*
	 * Check that the session is valid and clear the session
	 * usage bit
	 */
	mutex_lock(&session_list_mutex);
	sess = find_session(ctxdata, session);
	if (sess) {
		ta_handle = get_ta_handle(session);
		i = get_session_index(session);
		session_info = sess->session_info[i];
		spin_lock(&sess->lock);
		clear_bit(i, sess->sess_mask);
		spin_unlock(&sess->lock);
	}
	mutex_unlock(&session_list_mutex);

	if (!sess)
		return -EINVAL;

	/* Close the session */
	handle_close_session(ta_handle, session_info);
	handle_unload_ta(ta_handle);

	kref_put(&sess->refcount, destroy_session);

	return 0;
}

int amdtee_map_shmem(struct tee_shm *shm)
{
	struct amdtee_context_data *ctxdata;
	struct amdtee_shm_data *shmnode;
	struct shmem_desc shmem;
	int rc, count;
	u32 buf_id;

	if (!shm)
		return -EINVAL;

	shmnode = kmalloc(sizeof(*shmnode), GFP_KERNEL);
	if (!shmnode)
		return -ENOMEM;

	count = 1;
	shmem.kaddr = shm->kaddr;
	shmem.size = shm->size;

	/*
	 * Send a MAP command to TEE and get the corresponding
	 * buffer Id
	 */
	rc = handle_map_shmem(count, &shmem, &buf_id);
	if (rc) {
		pr_err("map_shmem failed: ret = %d\n", rc);
		kfree(shmnode);
		return rc;
	}

	shmnode->kaddr = shm->kaddr;
	shmnode->buf_id = buf_id;
	ctxdata = shm->ctx->data;
	mutex_lock(&ctxdata->shm_mutex);
	list_add(&shmnode->shm_node, &ctxdata->shm_list);
	mutex_unlock(&ctxdata->shm_mutex);

	pr_debug("buf_id :[%x] kaddr[%p]\n", shmnode->buf_id, shmnode->kaddr);

	return 0;
}

void amdtee_unmap_shmem(struct tee_shm *shm)
{
	struct amdtee_context_data *ctxdata;
	struct amdtee_shm_data *shmnode;
	u32 buf_id;

	if (!shm)
		return;

	buf_id = get_buffer_id(shm);
	/* Unmap the shared memory from TEE */
	handle_unmap_shmem(buf_id);

	ctxdata = shm->ctx->data;
	mutex_lock(&ctxdata->shm_mutex);
	list_for_each_entry(shmnode, &ctxdata->shm_list, shm_node)
		if (buf_id == shmnode->buf_id) {
			list_del(&shmnode->shm_node);
			kfree(shmnode);
			break;
		}
	mutex_unlock(&ctxdata->shm_mutex);
}

int amdtee_invoke_func(struct tee_context *ctx,
		       struct tee_ioctl_invoke_arg *arg,
		       struct tee_param *param)
{
	struct amdtee_context_data *ctxdata = ctx->data;
	struct amdtee_session *sess;
	u32 i, session_info;

	/* Check that the session is valid */
	mutex_lock(&session_list_mutex);
	sess = find_session(ctxdata, arg->session);
	if (sess) {
		i = get_session_index(arg->session);
		session_info = sess->session_info[i];
	}
	mutex_unlock(&session_list_mutex);

	if (!sess)
		return -EINVAL;

	handle_invoke_cmd(arg, session_info, param);

	return 0;
}

int amdtee_cancel_req(struct tee_context *ctx, u32 cancel_id, u32 session)
{
	return -EINVAL;
}

static int amdtee_get_debug_token(struct tee_context *ctx,
				struct tee_ioctl_get_debug_token_data *arg)
{
	int res;

	if (arg->debug_token_size < TEE_DLM_TOKEN_SIZE) {
		arg->debug_token_size = TEE_DLM_TOKEN_SIZE;
		arg->ret = TEEC_ERROR_SHORT_BUFFER;
		return 0;
	}

	res = handle_get_debug_token(arg);
	if (res) {
		pr_err("%s: get_debug_token failed 0x%x\n", __func__, res);
		return res;
	}

	return 0;
}

static int amdtee_start_ta_debug(struct tee_context *ctx,
				 struct tee_ioctl_start_ta_debug_data *arg)
{
	struct amdtee_context_data *ctxdata =
		(struct amdtee_context_data *) ctx->data;
	int i, res;
	void *vaddr;
	dma_addr_t dma_addr;
	u32 dlm_buffer_size = sizeof(struct amdtee_dlm_buf);
	struct amdtee_dlm_buf *dlm_buffer;

	if (arg->debug_token_size > MAX_DEBUG_TOKEN_SIZE) {
		pr_err("%s: invalid debug token size %u\n", __func__,
				arg->debug_token_size);
		return -EINVAL;
	}

	mutex_lock(&session_list_mutex);

	for (i = 0; i < MAX_ALLOWED_DLM_BUFFERS; ++i) {
		if (ctxdata->dlm_manager[i].in_use == false)
			break;
	}

	if (i >= MAX_ALLOWED_DLM_BUFFERS) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		res = -ENOMEM;
		goto err;
	}

	vaddr = dma_alloc_coherent(&ctx->teedev->dev, dlm_buffer_size,
				   &dma_addr, GFP_KERNEL);
	if (!vaddr) {
		res = -ENOMEM;
		goto err;
	}

	dlm_buffer = (struct amdtee_dlm_buf *) vaddr;
	memset(dlm_buffer, 0, sizeof(*dlm_buffer));
	dlm_buffer->dlm_size = dlm_buffer_size;
	dlm_buffer->num_strings = DLM_NUM_STRINGS;
	if (copy_from_user(dlm_buffer->debug_token,
				(void __user *) arg->debug_token,
				arg->debug_token_size)) {
		res = -EFAULT;
		goto err;
	}
	/*
	 * Save DLM buffer information into struct amdtee_context_data
	 * so that it can be later freed up
	 */
	ctxdata->dlm_manager[i].buffer_va = vaddr;
	ctxdata->dlm_manager[i].buffer_dma = dma_addr;
	ctxdata->dlm_manager[i].buffer_size = dlm_buffer_size;
	ctxdata->dlm_manager[i].in_use = true;

	mutex_unlock(&session_list_mutex);

	return handle_start_ta_debug(arg, &ctxdata->dlm_manager[i]);
err:
	mutex_unlock(&session_list_mutex);
	return res;
}

static int amdtee_fetch_debug_strings(struct tee_context *ctx,
				      struct tee_ioctl_fetch_debug_strings_data
				      *arg)
{
	struct amdtee_context_data *ctxdata =
		(struct amdtee_context_data *) ctx->data;
	struct amdtee_dlm_buf *dlm_buf;
	int i;
	u32 rptr, wptr;

	mutex_lock(&session_list_mutex);

	for (i = 0; i < MAX_ALLOWED_DLM_BUFFERS; ++i) {
		if (ctxdata->dlm_manager[i].in_use == false)
			continue;

		if (ctxdata->dlm_manager[i].dlm_session_id ==
				arg->dlm_session_id) {
			dlm_buf = (struct amdtee_dlm_buf *)
				ctxdata->dlm_manager[i].buffer_va;

			rptr = dlm_buf->rptr;
			wptr = dlm_buf->wptr;
			break;
		}
	}

	if (i >= MAX_ALLOWED_DLM_BUFFERS) {
		pr_err("%s: Invalid session id %u\n", __func__,
				arg->dlm_session_id);
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		mutex_unlock(&session_list_mutex);
		return 0;
	}

	pr_debug("%s: rptr = 0x%x wptr = 0x%x\n", __func__, rptr, wptr);

	if (rptr != wptr) {
		arg->is_valid_string = 1;
		memcpy(arg->string, dlm_buf->dlm_string[rptr].string,
				sizeof(dlm_buf->dlm_string[rptr].string));

		pr_info("%s: %s\n", __func__,
				dlm_buf->dlm_string[rptr].string);

		rptr = rptr + 1;
		if (rptr >= dlm_buf->num_strings)
			rptr = 0;
		dlm_buf->rptr = rptr;
	} else
		arg->is_valid_string = 0;

	mutex_unlock(&session_list_mutex);
	arg->ret = TEEC_SUCCESS;
	return 0;
}

static int amdtee_stop_ta_debug(struct tee_context *ctx,
				struct tee_ioctl_stop_ta_debug_data *arg)
{
	struct amdtee_context_data *ctxdata =
		(struct amdtee_context_data *) ctx->data;
	int i;

	mutex_lock(&session_list_mutex);

	for (i = 0; i < MAX_ALLOWED_DLM_BUFFERS; ++i) {
		if (ctxdata->dlm_manager[i].dlm_session_id ==
				arg->dlm_session_id) {
			ctxdata->dlm_manager[i].in_use = false;
			dma_free_coherent(&ctx->teedev->dev,
					  ctxdata->dlm_manager[i].buffer_size,
					  ctxdata->dlm_manager[i].buffer_va,
					  ctxdata->dlm_manager[i].buffer_dma);
			ctxdata->dlm_manager[i].buffer_va = 0;
			ctxdata->dlm_manager[i].buffer_size = 0;
			break;
		}
	}

	mutex_unlock(&session_list_mutex);

	if (i == MAX_ALLOWED_DLM_BUFFERS) {
		pr_err("%s: unknown DLM session id %u\n", __func__,
				arg->dlm_session_id);
		arg->ret = TEEC_ERROR_ITEM_NOT_FOUND;
		return 0;
	}

	return handle_stop_ta_debug(arg);
}

static const struct tee_driver_ops amdtee_ops = {
	.get_version = amdtee_get_version,
	.open = amdtee_open,
	.release = amdtee_release,
	.open_session = amdtee_open_session,
	.close_session = amdtee_close_session,
	.invoke_func = amdtee_invoke_func,
	.cancel_req = amdtee_cancel_req,
	.get_debug_token = amdtee_get_debug_token,
	.start_ta_debug = amdtee_start_ta_debug,
	.fetch_debug_strings = amdtee_fetch_debug_strings,
	.stop_ta_debug = amdtee_stop_ta_debug,
};

static const struct tee_desc amdtee_desc = {
	.name = DRIVER_NAME "-clnt",
	.ops = &amdtee_ops,
	.owner = THIS_MODULE,
};

static int __init amdtee_driver_init(void)
{
	struct tee_device *teedev;
	struct tee_shm_pool *pool;
	struct amdtee *amdtee;
	int rc;

	rc = psp_check_tee_status();
	if (rc) {
		pr_err("amd-tee driver: tee not present\n");
		return rc;
	}

	drv_data = kzalloc(sizeof(*drv_data), GFP_KERNEL);
	if (!drv_data)
		return -ENOMEM;

	amdtee = kzalloc(sizeof(*amdtee), GFP_KERNEL);
	if (!amdtee) {
		rc = -ENOMEM;
		goto err_kfree_drv_data;
	}

	pool = amdtee_config_shm();
	if (IS_ERR(pool)) {
		pr_err("shared pool configuration error\n");
		rc = PTR_ERR(pool);
		goto err_kfree_amdtee;
	}

	teedev = tee_device_alloc(&amdtee_desc, NULL, pool, amdtee);
	if (IS_ERR(teedev)) {
		rc = PTR_ERR(teedev);
		goto err_free_pool;
	}

	rc = dma_set_coherent_mask(&teedev->dev, DMA_BIT_MASK(64));
	if (rc) {
		pr_err("failed to set dma coherent mask\n");
		goto err_device_unregister;
	}

	amdtee->teedev = teedev;

	rc = tee_device_register(amdtee->teedev);
	if (rc)
		goto err_device_unregister;

	amdtee->pool = pool;

	drv_data->amdtee = amdtee;

	pr_info("amd-tee driver initialization successful\n");
	return 0;

err_device_unregister:
	tee_device_unregister(amdtee->teedev);

err_free_pool:
	tee_shm_pool_free(pool);

err_kfree_amdtee:
	kfree(amdtee);

err_kfree_drv_data:
	kfree(drv_data);
	drv_data = NULL;

	pr_err("amd-tee driver initialization failed\n");
	return rc;
}
module_init(amdtee_driver_init);

static void __exit amdtee_driver_exit(void)
{
	struct amdtee *amdtee;

	if (!drv_data || !drv_data->amdtee)
		return;

	amdtee = drv_data->amdtee;

	tee_device_unregister(amdtee->teedev);
	tee_shm_pool_free(amdtee->pool);
}
module_exit(amdtee_driver_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION("AMD-TEE driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("Dual MIT/GPL");

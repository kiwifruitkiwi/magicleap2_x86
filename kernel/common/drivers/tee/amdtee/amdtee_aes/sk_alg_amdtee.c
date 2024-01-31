// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2020-2020 Advanced Micro Devices, Inc. All rights reserved.
 */
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/crypto.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/tee_drv.h>

#include <crypto/skcipher.h>
#include <crypto/internal/skcipher.h>
#include <crypto/aes.h>

#include "sk_api.h"
#include "sk_private.h"

MODULE_FIRMWARE("/amdtee/c0fc39b1-dd2e-4753-a66600ba987669f6.bin");

static struct amdtee_alg_dev pvt_data = {
#ifdef DLM_SUPPORT
	.dt = {
		.dlm_token = NULL,
		.dlm_token_size = 0,
	},
#endif
};

static int amdtee_alg_open_ta_session(struct amdtee_alg_ctx *tfm_ctx)
{
	struct tee_ioctl_open_session_arg sess_arg;
	int ret, err;

	memset(tfm_ctx, 0, sizeof(*tfm_ctx));

	/* Open a client session with Trusted App */
	memset(&sess_arg, 0, sizeof(sess_arg));
	memcpy(sess_arg.uuid, (__u8 *)&ta_uuid, TEE_IOCTL_UUID_LEN);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;

	ret = tee_client_open_session(pvt_data.ctx, &sess_arg, NULL);
	if (ret < 0 || sess_arg.ret != 0) {
		pr_err("%s: tee_client_open_session failed, err: %d, %d\n",
			__func__, sess_arg.ret, ret);
		err = -EINVAL;
		goto out_ctx;
	}
	tfm_ctx->session_id = sess_arg.session;
	pr_debug("%s: sess_id=%d\n", __func__, tfm_ctx->session_id);

	tfm_ctx->shm_in = tee_shm_alloc(pvt_data.ctx, AES_BLOCK_SIZE,
				TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
	tfm_ctx->shm_out = tee_shm_alloc(pvt_data.ctx, AES_BLOCK_SIZE,
				TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
	if (IS_ERR(tfm_ctx->shm_in) || IS_ERR(tfm_ctx->shm_out)) {
		pr_err("%s: tee_shm_alloc ret=%ld,%ld\n", __func__,
				PTR_ERR(tfm_ctx->shm_in),
				PTR_ERR(tfm_ctx->shm_out));
		err = TEEC_ERROR_OUT_OF_MEMORY;
		goto out_shm;
	}
	pr_debug("%s: AMD AES shm allocated without error\n", __func__);

#ifdef DLM_SUPPORT
	if (pvt_data.dt.dlm_token_size) {
		ret = sk_dlm_start_session(pvt_data.ctx, &pvt_data.dt,
						&ta_uuid, &tfm_ctx->dlm_sid);
		if (ret) {
			pr_err("%s: cannot start a dlm session\n", __func__);
			tfm_ctx->dlm_sid = -1;
		}
	}
#endif
	return 0;

out_shm:
	if (tfm_ctx->shm_in)
		tee_shm_free(tfm_ctx->shm_in);

	if (tfm_ctx->shm_out)
		tee_shm_free(tfm_ctx->shm_out);

out_ctx:
	return err;

}

static void amdtee_alg_close_ta_session(struct amdtee_alg_ctx *tfm_ctx)
{
#ifdef DLM_SUPPORT
	/* close DLM session first */
	if (tfm_ctx->dlm_sid != -1) {
		sk_dlm_fetch_dbg_string(pvt_data.ctx, tfm_ctx->dlm_sid);
		sk_dlm_stop_session(pvt_data.ctx, tfm_ctx->dlm_sid);
		tfm_ctx->dlm_sid = -1;
	}
#endif
	if (tfm_ctx->shm_in)
		tee_shm_free(tfm_ctx->shm_in);

	if (tfm_ctx->shm_out)
		tee_shm_free(tfm_ctx->shm_out);

	tee_client_close_session(pvt_data.ctx, tfm_ctx->session_id);
	tfm_ctx->session_id = -1;
}

static int amdtee_cra_init(struct crypto_tfm *tfm)
{
	struct amdtee_alg_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	int ret;
	struct amdtee_alg_ctx *p;

	/* each instance should only call init once, in case? */
	mutex_lock(&pvt_data.dev_mutex);
	list_for_each_entry(p, &pvt_data.tfm_list, list) {
		if (p == tfm_ctx) {
			pr_err("%s: found 0x%llx\n", __func__, (u64)tfm_ctx);
			return 1;
		}
	}
	mutex_unlock(&pvt_data.dev_mutex);

	/* open a client session for this instance */
	ret = amdtee_alg_open_ta_session(tfm_ctx);
	if (ret) {
		dev_info(pvt_data.dev,
			"%s: there's no more tee client at this moment\n",
			__func__);
		return -1;
	}

	/* add this to the list */
	INIT_LIST_HEAD(&tfm_ctx->list);
	mutex_lock(&pvt_data.dev_mutex);
	list_add(&tfm_ctx->list, &pvt_data.tfm_list);
	pvt_data.alg_cnt++;
	mutex_unlock(&pvt_data.dev_mutex);

	pr_debug("%s: kenlength=%d , alg_cnt=%d, 0x%llx\n",
		__func__, tfm_ctx->key_len, pvt_data.alg_cnt, (u64)tfm_ctx);
	return 0;
}

static void amdtee_cra_exit(struct crypto_tfm *tfm)
{
	struct amdtee_alg_ctx *tfm_ctx = crypto_tfm_ctx(tfm);

	amdtee_alg_close_ta_session(tfm_ctx);

	mutex_lock(&pvt_data.dev_mutex);
	list_del(&tfm_ctx->list);
	pvt_data.alg_cnt--;
	mutex_unlock(&pvt_data.dev_mutex);

	pr_debug("%s: kenlength=%d, alg_cnt=%d, 0x%llx\n",
		__func__, tfm_ctx->key_len, pvt_data.alg_cnt, (u64)tfm_ctx);
}

static int amdtee_aes_func(uint32_t cmd, u8 *dst, const u8 *src, u32 size,
			struct amdtee_alg_ctx *tfm_ctx)
{
	struct tee_param *params[4];
	struct tee_ioctl_invoke_arg *arg = NULL;
	void *data_in = NULL;
	void *data_out = NULL;
	void *buf = NULL;
	int ret = 0, i;

	data_in = tee_shm_get_va(tfm_ctx->shm_in, 0);
	data_out = tee_shm_get_va(tfm_ctx->shm_out, 0);
	if (IS_ERR(data_in) || IS_ERR(data_out)) {
		pr_err("%s: tee_shm_get_va failed\n", __func__);
		ret = -EINVAL;
		goto quit;
	}
	memcpy(data_in, src, size);
	arg = (struct tee_ioctl_invoke_arg *)
		kzalloc(sizeof(struct tee_ioctl_invoke_arg), GFP_KERNEL);

	if (!arg) {
		ret = -ENOMEM;
		goto quit;
	}

	arg->func = cmd;
	arg->session = tfm_ctx->session_id;
	arg->cancel_id = 0;
	arg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;

	buf = kcalloc(arg->num_params, sizeof(struct tee_param), GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto quit_arg;
	}
	for (i = 0; i < arg->num_params; i++) {
		params[i] = (struct tee_param *)
			(buf + i * sizeof(struct tee_param));
	}

	if (cmd == CMD_AES_ECB_SET_KEY) {
		params[0]->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
		params[0]->u.memref.shm = tfm_ctx->shm_in;
		params[0]->u.memref.size = size;
		params[0]->u.memref.shm_offs = 0;

	} else {
		params[0]->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
		params[0]->u.memref.shm = tfm_ctx->shm_in;
		params[0]->u.memref.size = AES_BLOCK_SIZE;
		params[0]->u.memref.shm_offs = 0;

		params[1]->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
		params[1]->u.memref.shm = tfm_ctx->shm_out;
		params[1]->u.memref.size = AES_BLOCK_SIZE;
		params[1]->u.memref.shm_offs = 0;
	}
	ret = tee_client_invoke_func(pvt_data.ctx, arg, params[0]);
	if (ret) {
		pr_err("%s: tee_client_invoke_func re = %d\n", __func__, ret);
		goto quit_pa;
	}
	if (cmd != CMD_AES_ECB_SET_KEY)
		memcpy(dst, data_out, AES_BLOCK_SIZE);
quit_pa:
	kfree(buf);
quit_arg:
	kfree(arg);
quit:
	return ret;
}

static void amdtee_aes_encrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	struct amdtee_alg_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	int ret;

	ret = amdtee_aes_func(CMD_AES_ECB_ENC, dst, src,
				AES_BLOCK_SIZE, tfm_ctx);
	if (ret)
		pr_err("%s: encrypt failed, ret = %d\n", __func__, ret);
}

static void amdtee_aes_decrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	struct amdtee_alg_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	int ret;

	ret = amdtee_aes_func(CMD_AES_ECB_DEC, dst, src,
				AES_BLOCK_SIZE, tfm_ctx);
	if (ret)
		pr_err("%s: decrypt failed, ret=%d\n", __func__, ret);
}

static int amdtee_aes_setkey(struct crypto_tfm *tfm, const u8 *key,
				unsigned int keylen)
{
	struct amdtee_alg_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	int ret;

	tfm_ctx->key_len = keylen;
	pr_debug("%s called, keylen=%d\n", __func__, keylen);

	ret = amdtee_aes_func(CMD_AES_ECB_SET_KEY, NULL, key, keylen, tfm_ctx);
	if (ret)
		pr_err("%s: key setting failed\n", __func__);

	return ret;
}

static int amdtee_skcipher_setkey(struct crypto_skcipher *tfm, const u8 *key,
					unsigned int keylen)
{
	pr_debug("%s called, keylen=%d\n", __func__, keylen);

	return amdtee_aes_setkey(&tfm->base, key, keylen);
}

static int amdtee_ecb_crypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src,
				u32 blocks, bool enc)
{
	u32 i;

	pr_debug("%s: blocks = %d\n", __func__, blocks);

	for (i = 0; i < blocks; i++) {
		if (enc)
			amdtee_aes_encrypt(tfm, dst, src);
		else
			amdtee_aes_decrypt(tfm, dst, src);

		dst += AES_BLOCK_SIZE;
		src += AES_BLOCK_SIZE;
	}

	return 0;
}

static int amdtee_skcipher_enc(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct amdtee_alg_ctx *alg_ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	u32 blocks;
	int ret = 0;

	memset(&walk, 0, sizeof(struct skcipher_walk));
	ret = skcipher_walk_virt(&walk, req, false);
	if (ret) {
		pr_err("%s: virtual walk ret = %d\n",
			__func__, ret);
		return ret;
	}

	while ((blocks = (walk.nbytes / AES_BLOCK_SIZE))) {
		amdtee_ecb_crypt(&tfm->base,
				walk.dst.virt.addr, walk.src.virt.addr,
				blocks, 1);
		ret = skcipher_walk_done(&walk, walk.nbytes % AES_BLOCK_SIZE);
	}

	pr_debug("%s: ret=%d, keyLen=%d\n", __func__, ret, alg_ctx->key_len);

	return ret;
}

static int amdtee_skcipher_dec(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct amdtee_alg_ctx *alg_ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int blocks;
	int ret = 0;

	memset(&walk, 0, sizeof(struct skcipher_walk));
	ret = skcipher_walk_virt(&walk, req, false);
	if (ret) {
		dev_err(pvt_data.dev, "%s: virtual walk ret = %d\n",
				__func__, ret);
		return ret;
	}

	while ((blocks = (walk.nbytes / AES_BLOCK_SIZE))) {
		amdtee_ecb_crypt(&tfm->base,
				walk.dst.virt.addr, walk.src.virt.addr,
				blocks, 0);
		ret = skcipher_walk_done(&walk, walk.nbytes % AES_BLOCK_SIZE);
	}

	pr_debug("%s: ret=%d, keyLen=%d\n", __func__, ret, alg_ctx->key_len);

	return ret;
}

static int amdtee_skcipher_init(struct crypto_skcipher *tfm)
{
	dev_info(pvt_data.dev, "%s called, 0x%llx\n", __func__, (u64)tfm);
	return amdtee_cra_init(&tfm->base);
}

static void amdtee_skcipher_exit(struct crypto_skcipher *tfm)
{
	dev_info(pvt_data.dev, "%s called, 0x%llx\n", __func__, (u64)tfm);
	amdtee_cra_exit(&tfm->base);
}

static struct skcipher_alg amdtee_skcipher_alg = {
	.setkey		 = amdtee_skcipher_setkey,
	.encrypt	 = amdtee_skcipher_enc,
	.decrypt	 = amdtee_skcipher_dec,
	.init		 = amdtee_skcipher_init,
	.exit		 = amdtee_skcipher_exit,
	.min_keysize	 = AES_MIN_KEY_SIZE,
	.max_keysize	 = AES_MAX_KEY_SIZE,

	.base = {
		.cra_name	 = "ecb(aes)",
		.cra_driver_name = "amdtee_skcipher",
		.cra_flags	 =  CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize	= AES_BLOCK_SIZE,
		.cra_ctxsize    = sizeof(struct amdtee_alg_ctx),
		.cra_priority   = 300,
		.cra_init       = amdtee_cra_init,
		.cra_exit       = amdtee_cra_exit,
		.cra_module     = THIS_MODULE,

		.cra_u = {
			.cipher = {
				.cia_min_keysize = AES_MIN_KEY_SIZE,
				.cia_max_keysize = AES_MAX_KEY_SIZE,
				.cia_setkey	= amdtee_aes_setkey,
				.cia_encrypt    = amdtee_aes_encrypt,
				.cia_decrypt    = amdtee_aes_decrypt
			}
		}
	}
};

static int amdtee_ctx_match(struct tee_ioctl_version_data *ver,
				const void *data)
{
	if (ver->impl_id == TEE_IMPL_ID_AMDTEE)
		return 1;
	else
		return 0;
}

static int amdtee_alg_probe(struct device *dev)
{
	int err = -ENODEV;

	pr_debug("%s called, dev=0x%llx\n", __func__, (uint64_t)dev);

	pvt_data.dev = dev;
	mutex_init(&pvt_data.dev_mutex);
	INIT_LIST_HEAD(&pvt_data.tfm_list);
	pvt_data.alg_cnt = 0;

	/* Open context with TEE driver */
	pvt_data.ctx = tee_client_open_context(NULL, amdtee_ctx_match, NULL,
					       NULL);
	if (IS_ERR(pvt_data.ctx)) {
		pr_err("%s: ctx=0x%llx\n", __func__, (uint64_t)pvt_data.ctx);
		pvt_data.ctx = NULL;
		return err;
	}
	pr_debug("%s: ctx = 0x%llx\n", __func__, (uint64_t)pvt_data.ctx);

#ifdef DLM_SUPPORT
	pvt_data.dt.dlm_token = kzalloc(MAX_DEBUG_TOKEN_SIZE, GFP_KERNEL);
	if (sk_dlm_get_dbg_token(&pvt_data.dt)) {
		kfree(pvt_data.dt.dlm_token);
		pvt_data.dt.dlm_token = NULL;
		goto out_ctx;
	} else {
		pvt_data.attr = amdtee_clint_data_attr;
		if (sysfs_create_bin_file(&dev->kobj, &pvt_data.attr)) {
			pr_err("%s: failed to create file\n",
				__func__);
			kfree(pvt_data.dt.dlm_token);
			pvt_data.dt.dlm_token = NULL;
			pvt_data.dt.dlm_token_size = 0;
			goto out_ctx;
		}
	}
#endif

	crypto_register_skcipher(&amdtee_skcipher_alg);

	pr_debug("%s: return 0\n", __func__);

	return 0;

#ifdef DLM_SUPPORT
out_ctx:
	if (pvt_data.ctx) {
		tee_client_close_context(pvt_data.ctx);
		pvt_data.ctx = NULL;
	}
	return err;
#endif
}

static int amdtee_alg_remove(struct device *dev)
{
	struct tee_client_device *client_dev = to_tee_client_device(dev);
	struct amdtee_alg_ctx *tfm_ctx;

	pr_debug("%s called. 0x%llx - 0x%llx - 0x%llx: %d\n", __func__,
			(uint64_t)dev, (uint64_t)pvt_data.dev,
			(uint64_t)client_dev, pvt_data.alg_cnt);

	crypto_unregister_skcipher(&amdtee_skcipher_alg);

	if (pvt_data.alg_cnt) {
		list_for_each_entry(tfm_ctx, &pvt_data.tfm_list, list) {
#ifdef DLM_SUPPORT
			/* close DLM session first */
			if (tfm_ctx->dlm_sid != -1) {
				sk_dlm_fetch_dbg_string(pvt_data.ctx,
							tfm_ctx->dlm_sid);
				sk_dlm_stop_session(pvt_data.ctx,
							tfm_ctx->dlm_sid);
				tfm_ctx->dlm_sid = -1;
			}
#endif
			if (tfm_ctx->shm_in)
				tee_shm_free(tfm_ctx->shm_in);
			if (tfm_ctx->shm_out)
				tee_shm_free(tfm_ctx->shm_out);
			if (tfm_ctx->session_id != -1) {
				tee_client_close_session(pvt_data.ctx,
							tfm_ctx->session_id);
				tfm_ctx->session_id = -1;
			}
		}
		INIT_LIST_HEAD(&pvt_data.tfm_list);
		pvt_data.alg_cnt = 0;
	}

#ifdef DLM_SUPPORT
		kfree(pvt_data.dt.dlm_token);
		pvt_data.dt.dlm_token = NULL;
		pvt_data.dt.dlm_token_size = 0;
		if (pvt_data.dt.dlm_token)
			sysfs_remove_bin_file(&dev->kobj, &pvt_data.attr);
#endif

	if (pvt_data.ctx) {
		tee_client_close_context(pvt_data.ctx);
		pvt_data.ctx = NULL;
	}

	return 0;
}

static const struct tee_client_device_id amdtee_alg_id_table[] = {
	{UUID_INIT(0xb139fcc0, 0x2edd, 0x5347,
			0xa6, 0x66, 0x00, 0xba, 0x98, 0x76, 0x69, 0xf6)},
	{}
};

static struct tee_client_driver amdtee_alg_driver = {
	.id_table = amdtee_alg_id_table,
	.driver = {
		.name = "amdtee_alg_driver",
		.bus = &tee_bus_type,
		.owner = THIS_MODULE,
		.probe = amdtee_alg_probe,
		.remove = amdtee_alg_remove,
	},
};

static void amdtee_client_dev_release(struct device *dev)
{
	kfree(to_tee_client_device(dev));
}

static int amdtee_register_client_device(void)
{
	int ret = 0;
	struct tee_client_device *amdtee_client_dev = NULL;

	pr_debug("%s called\n", __func__);

	amdtee_client_dev = kzalloc(sizeof(*amdtee_client_dev), GFP_KERNEL);
	if (!amdtee_client_dev)
		return -ENOMEM;

	amdtee_client_dev->dev.bus = &tee_bus_type;
	amdtee_client_dev->dev.release = amdtee_client_dev_release;
	dev_set_name(&amdtee_client_dev->dev, "amdtee-clnt%u", 0);
	memcpy(amdtee_client_dev->id.uuid.b,
		(__u8 *)&ta_uuid, TEE_IOCTL_UUID_LEN);

	ret = device_register(&amdtee_client_dev->dev);
	if (ret) {
		pr_err("%s: device register failed\n", __func__);
		kfree(amdtee_client_dev);
	}
	pr_debug("%s: ret = %d\n", __func__, ret);

	return ret;
}

static int __init amdtee_sk_alg_init(void)
{
	amdtee_register_client_device();
	return driver_register(&amdtee_alg_driver.driver);
}

static void __exit amdtee_sk_alg_exit(void)
{
	driver_unregister(&amdtee_alg_driver.driver);
	device_unregister(pvt_data.dev);
}

late_initcall(amdtee_sk_alg_init);
module_exit(amdtee_sk_alg_exit);

MODULE_DESCRIPTION("ecb(aes) with amdtee");
MODULE_LICENSE("Dual MIT/GPL");

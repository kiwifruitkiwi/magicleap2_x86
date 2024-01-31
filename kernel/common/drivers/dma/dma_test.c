// SPDX-License-Identifier: GPL-2.0-only
/*
 * DMA Engine test module
 *
 * Copyright (C) 2007 Atmel Corporation
 * Copyright (C) 2013 Intel Corporation
 */

#define pr_fmt(fmt) "dma-test: " fmt

#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/freezer.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/sched/task.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/sched/clock.h>

#include "dma_test.h"
#include "../drivers/staging/android/ion/ion.h"

/*track time before and after dma transfer*/
static u64 start_ts[MAX_CHANS], end_ts[MAX_CHANS], dur_ts[MAX_CHANS];

static struct task_struct *tasks[MAX_CHANS];

struct dmatest_done {
	bool done;
	wait_queue_head_t *wait;
	int cnum;
};

/**
 * struct dmatest_params - test parameters.
 * @buf_size:		size of the memcpy test buffer
 * @fd_srcbuf:		fd of the source ion buffer
 * @fd_dstbuf:		fd of the destination ion buffer
 * @channel:		bus ID of the channel to test
 * @device:		bus ID of the DMA Engine to test
 * @timeout:		transfer timeout in msec, -1 for infinite timeout
 */
struct dmatest_params {
	unsigned int	buf_size;
	int		fd_srcbuf;
	int		fd_dstbuf;
	struct dma_buf *dma_src_buf;
	struct dma_buf *dma_dst_buf;
	char		channel[20];
	char		device[32];
	int		timeout;
	wait_queue_head_t wait_done;
	struct dmatest_done test_done;
	int		case_num;
	struct dma_test_data *tstdata;
};

/**
 * struct dmatest_info - test information.
 * @params:		test parameters
 * @lock:		access protection to the fields of this structure
 */
static struct dmatest_info {
	/* Test parameters */
	struct dmatest_params	params[MAX_CHANS];

	/* Internal state */
	struct list_head	channels;
	unsigned int		nr_channels;
	/* mutex lock */
	struct mutex		lock;
	bool			did_init;
} test_info = {
	.channels = LIST_HEAD_INIT(test_info.channels),
	.lock = __MUTEX_INITIALIZER(test_info.lock),
};

struct dma_test_device {
	struct miscdevice misc;
};

/*add src and dst void ptr*/
struct dma_test_data {
	int channel;
	struct dma_buf *dma_src_buf;
	struct dma_buf *dma_dst_buf;
	struct dma_buf_attachment *dma_src_attach;
	struct dma_buf_attachment *dma_dst_attach;
	struct sg_table *sg_tbl_src;
	struct sg_table *sg_tbl_dst;
	void *vaddr_src;
	void *vaddr_dst;
	struct dma_chan *chan;
	struct device *dev;
	int is_dmabuf_imported;
	int is_skip_test;
};

static void dmatest_callback(void *arg)
{
	struct dmatest_done *done = arg;
	/* track end time including overhead */
	end_ts[done->cnum] = local_clock();
	dur_ts[done->cnum] = end_ts[done->cnum] - start_ts[done->cnum];
	done->done = true;
	wake_up_all(done->wait);
}

static void dmatest_callback_result(void *arg, const struct dmaengine_result *result)
{
	struct dmatest_done *done = arg;
	/* track end time including overhead */
	end_ts[done->cnum] = local_clock();
	/* track actual DMA transfer time */
	dur_ts[done->cnum] = result->residue;
	done->done = true;
	wake_up_all(done->wait);
}

static bool filter(struct dma_chan *chan, void *param)
{
	struct dmatest_params *params = param;

	if (strcmp(dma_chan_name(chan), params->channel) == 0)
		return true;
	else
		return false;
}

static int dma_buf_import(struct dmatest_params *params, struct dma_test_data *data)
{
	enum dma_transaction_type type = DMA_MEMCPY;
	dma_cap_mask_t mask;
	struct device *dev;
	int ret = -EINVAL;

	if (data->is_dmabuf_imported)
		return 0;

	if (params->fd_srcbuf < 0 || params->fd_dstbuf < 0) {
		ret = 0;
		goto err_chan;
	}

	dma_cap_zero(mask);
	dma_cap_set(type, mask);

	/*send dma channel request*/
	data->chan = dma_request_channel(mask, filter, params);
	if (!data->chan) {
		pr_err("error request dma channel!");
		goto err_chan;
	}
	dev = data->chan->device->dev;

	/* Get dma src buf from ION fd */
	data->dma_src_buf = dma_buf_get(params->fd_srcbuf);
	if (!data->dma_src_buf) {
		pr_err("fail dma_buf_get srcbuf:%d\n", params->fd_srcbuf);
		goto err_src_buf;
	}

	/* attach SRC dmabuf to iommu dev */
	data->dma_src_attach = dma_buf_attach(data->dma_src_buf, dev);
	if (IS_ERR(data->dma_src_attach)) {
		pr_err("failed to attach SRC buffer to DMA device\n");
		goto err_src_buf_attach;
	}

	/* map the SRC dmabuf to obtain the sg_table */
	data->sg_tbl_src = dma_buf_map_attachment(data->dma_src_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(data->sg_tbl_src)) {
		pr_err("error for dma src buf sg table...\n");
		goto err_src_buf_sg;
	}

	/* Map a kernel VA for CPU access */
	data->vaddr_src = dma_buf_kmap(data->dma_src_buf, 0);
	if (!data->vaddr_src) {
		pr_err("failed to kmap for SRC buffer\n");
		goto err_src_kmap;
	}

	/* get dma dst buf from ion fd */
	data->dma_dst_buf = dma_buf_get(params->fd_dstbuf);
	if (!data->dma_dst_buf) {
		pr_err("fail dma_buf_get dstbuf:%d\n", params->fd_dstbuf);
		goto err_dst_buf;
	}

	/* attach DST dmabuf to iommu dev */
	data->dma_dst_attach = dma_buf_attach(data->dma_dst_buf, dev);
	if (IS_ERR(data->dma_dst_attach)) {
		pr_err("failed to attach DST buffer to DMA device\n");
		goto err_dst_buf_attach;
	}

	/* Map the DST dmabuf to obtain the sg_table */
	data->sg_tbl_dst = dma_buf_map_attachment(data->dma_dst_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(data->sg_tbl_dst)) {
		pr_err("error for dma dst buf sg table...\n");
		goto err_dst_buf_sg;
	}

	/* Map a kernel VA for CPU access */
	data->vaddr_dst = dma_buf_kmap(data->dma_dst_buf, 0);
	if (!data->vaddr_dst) {
		pr_err("failed to kmap for DST buffer\n");
		goto err_dst_kmap;
	}

	data->is_dmabuf_imported = 1;
	data->is_skip_test = 0;
	return 0;

err_dst_kmap:
	dma_buf_unmap_attachment(data->dma_dst_attach, data->sg_tbl_dst, DMA_BIDIRECTIONAL);
err_dst_buf_sg:
	dma_buf_detach(data->dma_dst_buf, data->dma_dst_attach);
err_dst_buf_attach:
	dma_buf_put(data->dma_dst_buf);
err_dst_buf:
	dma_buf_kunmap(data->dma_src_buf, 0, data->vaddr_src);
err_src_kmap:
	dma_buf_unmap_attachment(data->dma_src_attach, data->sg_tbl_src, DMA_BIDIRECTIONAL);
err_src_buf_sg:
	dma_buf_detach(data->dma_src_buf, data->dma_src_attach);
err_src_buf_attach:
	dma_buf_put(data->dma_src_buf);
err_src_buf:
	dma_release_channel(data->chan);
err_chan:
	data->dma_src_buf = NULL;
	data->dma_dst_buf = NULL;
	data->dma_src_attach = NULL;
	data->dma_dst_attach = NULL;
	data->sg_tbl_src = NULL;
	data->sg_tbl_dst = NULL;
	data->vaddr_src = NULL;
	data->vaddr_dst = NULL;
	data->chan = NULL;
	data->is_dmabuf_imported = 0;
	data->is_skip_test = 1;
	return ret;
}

static void dma_buf_cleanup(struct dma_test_data *data)
{
	if (data->vaddr_dst)
		dma_buf_kunmap(data->dma_dst_buf, 0, data->vaddr_dst);
	if (data->dma_dst_attach && data->sg_tbl_dst)
		dma_buf_unmap_attachment(data->dma_dst_attach, data->sg_tbl_dst,
					 DMA_BIDIRECTIONAL);
	if (data->dma_dst_buf && data->dma_dst_attach)
		dma_buf_detach(data->dma_dst_buf, data->dma_dst_attach);
	if (data->dma_dst_buf)
		dma_buf_put(data->dma_dst_buf);

	if (data->vaddr_src)
		dma_buf_kunmap(data->dma_src_buf, 0, data->vaddr_src);
	if (data->dma_src_attach && data->sg_tbl_src)
		dma_buf_unmap_attachment(data->dma_src_attach, data->sg_tbl_src,
					 DMA_BIDIRECTIONAL);
	if (data->dma_src_buf)
		dma_buf_detach(data->dma_src_buf, data->dma_src_attach);
	if (data->dma_src_buf)
		dma_buf_put(data->dma_src_buf);

	data->dma_src_buf = NULL;
	data->dma_dst_buf = NULL;
	data->dma_src_attach = NULL;
	data->dma_dst_attach = NULL;
	data->sg_tbl_src = NULL;
	data->sg_tbl_dst = NULL;
	data->vaddr_src = NULL;
	data->vaddr_dst = NULL;
	data->is_dmabuf_imported = 0;
}

static int dma_chan_threadfn(void *data)
{
	struct dmatest_params *params = data;
	struct dma_test_data *tstdata;
	struct dma_async_tx_descriptor *tx = NULL;
	struct dmaengine_unmap_data *um;
	struct dma_device *dev;
	dma_cookie_t cookie;
	enum dma_ctrl_flags flags;
	enum dma_status status;
	struct dmatest_done *done;
	unsigned int len;
	int src_cnt, dst_cnt;
	int ret = -1;
	int need_cleanup;

	tstdata = params->tstdata;

	if (tstdata->is_skip_test)
		goto err_import;

	if (tstdata->is_dmabuf_imported)
		need_cleanup = 0;
	else
		need_cleanup = 1;

	/* Import ION buffer into dmabuf */
	ret = dma_buf_import(params, tstdata);
	if (ret)
		goto err_import;

	/*add wait queue head*/
	params->test_done.wait = &params->wait_done;
	init_waitqueue_head(&params->wait_done);

	done = &params->test_done;

	/*Initialize variables*/
	len = params->buf_size;
	dev = tstdata->chan->device;
	flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
	src_cnt = 1;
	dst_cnt = 1;

	um = dmaengine_get_unmap_data(dev->dev, src_cnt + dst_cnt, GFP_KERNEL);
	if (!um) {
		pr_err("error getting um with dma engine\n");
		goto err_um;
	}

	/*mapping src buf sg*/
	um->len = len;
	um->addr[0] = tstdata->sg_tbl_src->sgl->dma_address;
	um->to_cnt++;

	/*mapping dst buf sg*/
	um->addr[1] = tstdata->sg_tbl_dst->sgl->dma_address;
	um->bidi_cnt++;

	ret = -1;
	/*prepare dma for memcpy*/
	tx = dev->device_prep_dma_memcpy(tstdata->chan, um->addr[1],
					 um->addr[0], len, flags);
	if (!tx) {
		pr_err("error prepare dma memcpy\n");
		goto err_prep_dma_memcpy;
	}

	/*set descriptor call back and params*/
	done->cnum = params->case_num;
	done->done = false;
	tx->callback = dmatest_callback;
	tx->callback_result = dmatest_callback_result;
	tx->callback_param = done;

	/*Submit dma descriptor*/
	cookie = tx->tx_submit(tx);
	if (dma_submit_error(cookie)) {
		pr_err("error dma submit!!\n");
		goto err_prep_dma_memcpy;
	}

	/*start time here*/
	start_ts[params->case_num] = local_clock();
	dma_async_issue_pending(tstdata->chan);
	wait_event_freezable_timeout(params->wait_done, done->done,
				     msecs_to_jiffies(params->timeout));

	/*check dma return status*/
	status = dma_async_is_tx_complete(tstdata->chan, cookie, NULL, NULL);
	if (!done->done) {
		pr_err("***test time out***\n");
	} else if (status != DMA_COMPLETE) {
		if (status == DMA_ERROR)
			pr_err("***dma error***\n");
		else
			pr_err("***busy status***\n");
	} else {
		pr_debug("***%s COMPLETE***\n", params->channel);
		ret = 0;
	}

err_prep_dma_memcpy:
	um->bidi_cnt = 0;
	um->to_cnt = 0;
	dmaengine_unmap_put(um);
err_um:
	dmaengine_terminate_sync(tstdata->chan);
	dma_release_channel(tstdata->chan);
	if (need_cleanup)
		dma_buf_cleanup(tstdata);
err_import:
	/*wait for thread_stop call and exit thread*/
	while (!kthread_should_stop())
		usleep_range(20, 100);
	return ret;
}

static int request_channels(void *data)
{
	struct dmatest_params *params = data;
	struct dma_async_tx_descriptor *tx = NULL;
	struct dmaengine_unmap_data *um;
	struct dma_device *dev;
	dma_cookie_t cookie;
	enum dma_ctrl_flags flags;
	enum dma_status status;
	struct dmatest_done *done;
	unsigned int len;
	int src_cnt, dst_cnt;
	int ret = -1;
	int need_cleanup;
	struct dma_test_data *tstdata;

	tstdata = params->tstdata;

	if (tstdata->is_dmabuf_imported)
		need_cleanup = 0;
	else
		need_cleanup = 1;

	/* Import ION buffer into dmabuf */
	ret = dma_buf_import(params, tstdata);
	if (ret)
		goto err_import;

	/*add wait queue head*/
	params->test_done.wait = &params->wait_done;
	init_waitqueue_head(&params->wait_done);

	done = &params->test_done;

	/*Initialize variables*/
	len = params->buf_size;
	dev = tstdata->chan->device;
	flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
	src_cnt = 1;
	dst_cnt = 1;

	um = dmaengine_get_unmap_data(dev->dev, src_cnt + dst_cnt, GFP_KERNEL);
	if (!um) {
		pr_err("error getting um with dma engine\n");
		goto err_um;
	}

	/*mapping src buf sg*/
	um->len = len;
	um->addr[0] = tstdata->sg_tbl_src->sgl->dma_address;
	um->to_cnt++;

	/*mapping dst buf sg*/
	um->addr[1] = tstdata->sg_tbl_dst->sgl->dma_address;
	um->bidi_cnt++;

	ret = -1;
	/*prepare dma for memcpy*/
	tx = dev->device_prep_dma_memcpy(tstdata->chan, um->addr[1],
					 um->addr[0], len, flags);
	if (!tx) {
		pr_err("error prepare dma memcpy\n");
		goto err_prep_dma_memcpy;
	}

	/*set descriptor call back and params*/
	done->cnum = params->case_num;
	done->done = false;
	tx->callback = dmatest_callback;
	tx->callback_result = dmatest_callback_result;
	tx->callback_param = done;

	/*Submit dma descriptor*/
	cookie = tx->tx_submit(tx);
	if (dma_submit_error(cookie)) {
		pr_err("error dma submit!!\n");
		goto err_prep_dma_memcpy;
	}

	/*start time here*/
	start_ts[params->case_num] = local_clock();
	dma_async_issue_pending(tstdata->chan);
	wait_event_freezable_timeout(params->wait_done, done->done,
				     msecs_to_jiffies(params->timeout));

	/*check dma return status*/
	status = dma_async_is_tx_complete(tstdata->chan, cookie, NULL, NULL);
	if (!done->done) {
		pr_err("***test time out***\n");
	} else if (status != DMA_COMPLETE) {
		if (status == DMA_ERROR)
			pr_err("***dma error***\n");
		else
			pr_err("***busy status***\n");
	} else {
		pr_debug("***%s COMPLETE***\n", params->channel);
		ret = 0;
	}

err_prep_dma_memcpy:
	um->bidi_cnt = 0;
	um->to_cnt = 0;
	dmaengine_unmap_put(um);
err_um:
	dmaengine_terminate_sync(tstdata->chan);
	dma_release_channel(tstdata->chan);
	if (need_cleanup)
		dma_buf_cleanup(tstdata);
err_import:
	return ret;
}

static long dma_test_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	struct dma_test_data *test_data = filp->private_data;
	struct dmatest_info *info = &test_info;
	struct dmatest_params *params;

	int ret = 0;
	int k, i, cases;
	union {
		struct dma_test_rw_data test_rw;
		struct dma_test_multi_chan_rw_data test_multi_rw;
	} data;

	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (_IOC_DIR(cmd) & _IOC_WRITE)
		if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;

	switch (cmd) {
	case DMA_IOC_TEST_SET_CHANNEL:
	{
		test_data->channel = arg;
		break;
	}
	case DMA_IOC_TEST_IMPORT_FD:
	{
		params = &info->params[0];
		params->buf_size = data.test_rw.buf_size;
		strlcpy(params->channel, strim(data.test_rw.dma_channel),
			sizeof(params->channel));
		params->fd_srcbuf = data.test_rw.src_fd;
		params->fd_dstbuf = data.test_rw.dst_fd;
		ret = dma_buf_import(params, test_data);
		if (ret) {
			pr_err("Fail to import DMA buffer fd\n");
			return -EINVAL;
		}
		break;
	}
	case DMA_IOC_TEST_IMPORT_MULTI_FD:
	{
		/* multiple ION FDs import */
		if (data.test_multi_rw.total_cases > MAX_CHANS) {
			pr_err("Import total cases:%d > MAX_CHANS:%d\n",
			       data.test_multi_rw.total_cases, MAX_CHANS);
			return -E2BIG;
		}
		cases = data.test_multi_rw.total_cases;

		for (k = 0; k < cases; k++) {
			params = &info->params[k];
			params->buf_size = data.test_multi_rw.buf_size[k];
			strlcpy(params->channel,
				strim(data.test_multi_rw.dma_channel[k]),
				sizeof(params->channel));
			params->fd_srcbuf = data.test_multi_rw.src_fd[k];
			params->fd_dstbuf = data.test_multi_rw.dst_fd[k];
			ret = dma_buf_import(params, &test_data[k]);
			if (ret) {
				pr_err("Fail to import DMA buffer[%d] fd\n", k);
				return -EINVAL;
			}
		}
		break;
	}
	case DMA_IOC_TEST_CLEANUP:
	{
		for (k = 0; k < MAX_CHANS; k++)
			dma_buf_cleanup(test_data + k);
		break;
	}
	case DMA_IOC_TEST_RUN_DMA:
	{
		params = &info->params[0];
		params->buf_size = data.test_rw.buf_size;
		strlcpy(params->channel, strim(data.test_rw.dma_channel),
			sizeof(params->channel));
		params->fd_srcbuf = data.test_rw.src_fd;
		params->fd_dstbuf = data.test_rw.dst_fd;
		params->timeout = 1000;
		params->case_num = 0;
		params->tstdata = &test_data[0];
		ret = request_channels(params);

		data.test_rw.runtime_ns = dur_ts[0];
		pr_debug("duration: %llu of channel %s\n",
			(long long)(dur_ts[0]), params->channel);
		break;
	}
	case DMA_IOC_TEST_DMA_CHANS_SEQ:
	{
		int k_ret[MAX_CHANS];

		/*sequential channel submission*/
		if (data.test_multi_rw.total_cases > MAX_CHANS)
			return -E2BIG;
		else
			cases = data.test_multi_rw.total_cases;

		for (k = 0; k < cases; k++) {
			params = &info->params[k];
			params->buf_size = data.test_multi_rw.buf_size[k];
			strlcpy(params->channel,
				strim(data.test_multi_rw.dma_channel[k]),
				sizeof(params->channel));
			params->fd_srcbuf = data.test_multi_rw.src_fd[k];
			params->fd_dstbuf = data.test_multi_rw.dst_fd[k];
			params->timeout = 1000;
			params->case_num = k;
			params->tstdata = &test_data[k];
			k_ret[k] = request_channels(params);
		}
		/*print transfer time results*/
		for (k = 0; k < cases; k++) {
			if (k_ret[k]) {
				ret = k_ret[k];
				pr_debug("failed to test channel %s\n",
					 data.test_multi_rw.dma_channel[k]);
				continue;
			}
			data.test_multi_rw.runtime_ns[k] = dur_ts[k];
			pr_debug("duration: %llu of channel %s\n start time: %llu ----> end time: %llu\n",
				(long long)(dur_ts[k]), data.test_multi_rw.dma_channel[k],
				start_ts[k], end_ts[k]);
		}
		break;
	}
	case DMA_IOC_TEST_DMA_CHANS_PARA:
	{
		pr_debug("inside dma multi chan parallel transfer...\n");
		/*parallel channel submission*/
		if (data.test_multi_rw.total_cases > MAX_CHANS)
			return -E2BIG;
		else
			cases = data.test_multi_rw.total_cases;

		for (k = 0; k < cases; k++) {
			params = &info->params[k];
			params->buf_size = data.test_multi_rw.buf_size[k];
			strlcpy(params->channel,
				strim(data.test_multi_rw.dma_channel[k]),
				sizeof(params->channel));
			params->fd_srcbuf = data.test_multi_rw.src_fd[k];
			params->fd_dstbuf = data.test_multi_rw.dst_fd[k];
			params->timeout = 1000;
			params->case_num = k;
			params->tstdata = &test_data[k];
		}
		for (k = 0; k < cases; k++) {
			tasks[k] = kthread_create(dma_chan_threadfn, &(info->params[k]), "thread%d", k);
			if (!IS_ERR(tasks[k]))
				wake_up_process(tasks[k]);
			else {
				pr_err("failed to create thread for case %d\n", cases);
				for (i = 0; i < k; i++)
					kthread_stop(tasks[i]);
				return PTR_ERR(tasks[k]);
			}
		}
		for (k = 0; k < cases; k++) {
			kthread_stop(tasks[k]);
		}
		/*print transfer time results*/
		for (k = 0; k < cases; k++) {
			data.test_multi_rw.runtime_ns[k] = dur_ts[k];
			pr_debug("duration: %llu of channel %s\n",
				(long long)(dur_ts[k]), data.test_multi_rw.dma_channel[k]);
		}
		break;
	}
	default:
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ) {
		if (copy_to_user((void __user *)arg, &data, sizeof(data)))
			return -EFAULT;
	}
	return ret;
}

static int dma_test_open(struct inode *inode, struct file *file)
{
	struct dma_test_data *data;
	struct miscdevice *miscdev = file->private_data;

	data = kzalloc(sizeof(*data) * MAX_CHANS, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->dev = miscdev->parent;

	file->private_data = data;

	return 0;
}

static int dma_test_release(struct inode *inode, struct file *file)
{
	struct dma_test_data *data = file->private_data;

	kfree(data);

	return 0;
}

static const struct file_operations dma_test_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = dma_test_ioctl,
	.compat_ioctl = dma_test_ioctl,
	.open = dma_test_open,
	.release = dma_test_release,
};

static int __init dma_test_probe(struct platform_device *pdev)
{
	int ret;
	struct dma_test_device *testdev;

	testdev = devm_kzalloc(&pdev->dev, sizeof(struct dma_test_device),
			       GFP_KERNEL);
	if (!testdev)
		return -ENOMEM;

	testdev->misc.minor = MISC_DYNAMIC_MINOR;
	testdev->misc.name = "dma-test";
	testdev->misc.fops = &dma_test_fops;
	testdev->misc.parent = &pdev->dev;
	ret = misc_register(&testdev->misc);
	if (ret) {
		pr_err("failed to register misc device.\n");
		return ret;
	}

	platform_set_drvdata(pdev, testdev);

	return 0;
}

static int dma_test_remove(struct platform_device *pdev)
{
	struct dma_test_device *testdev;

	testdev = platform_get_drvdata(pdev);
	if (!testdev)
		return -ENODATA;

	misc_deregister(&testdev->misc);
	return 0;
}

static struct platform_device *dma_test_pdev;
static struct platform_driver dma_test_platform_driver = {
	.remove = dma_test_remove,
	.driver = {
		.name = "dma-test",
	},
};

static int __init dma_test_init(void)
{
	dma_test_pdev = platform_device_register_simple("dma-test",
							-1, NULL, 0);
	if (IS_ERR(dma_test_pdev))
		return PTR_ERR(dma_test_pdev);

	return platform_driver_probe(&dma_test_platform_driver, dma_test_probe);
}

static void __exit dma_test_exit(void)
{
	platform_driver_unregister(&dma_test_platform_driver);
	platform_device_unregister(dma_test_pdev);
}

module_init(dma_test_init);
module_exit(dma_test_exit);

/*
 * IPC library mailbox configuration
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "ipc.h"
#include "cvip.h"

struct mbox_client mbox_client_common = {
	.tx_block = false,
	.tx_tout = 500,
	.rx_callback = NULL,
	.tx_done = mbox_tx_done,
	.data_ack = 0,
};

#ifdef CONFIG_CVIP_IPC
mb_rx_callback_t isp_rx_cb[] = {
	isp_cam0_cntl_cb,
	isp_cam1_cntl_cb,
	isp_cam0_rgb_cb,
	isp_cam0_ir_cb,
	isp_cam1_rgb_cb,
	isp_cam1_ir_cb,
	isp_cam0_isp_input_cb,
	isp_cam1_isp_input_cb,
	isp_cam0_pre_buff_cb,
	isp_cam1_pre_buff_cb,
	isp_cam0_post_buff_cb,
	isp_cam1_post_buff_cb,
	isp_cam0_stream0_cb,
	isp_cam0_stream1_cb,
	isp_cam0_stream2_cb,
	isp_cam0_stream3_cb,
	isp_cam0_streamir_cb,
	isp_cam1_stream0_cb,
	isp_cam1_stream1_cb,
	isp_cam1_stream2_cb,
	isp_cam1_stream3_cb,
	isp_cam1_streamir_cb,
	isp_cam0_dma_cb,
	isp_cam1_dma_cb,
};

const char *isp_chan_name[] = {
	"cam0_cntl", "cam1_cntl", "cam0_rgb", "cam0_ir",
	"cam1_rgb", "cam1_ir", "cam0_isp_input", "cam1_isp_input",
	"cam0_pre_buff", "cam1_pre_buff", "cam0_post_buff", "came1_post_buff",
	"cam0_stream0", "cam0_stream1", "cam0_stream2", "cam0_stream3",
	"cam0_streamir", "cam1_stream0", "cam1_stream1", "cam1_stream2",
	"cam1_stream3", "cam1_streamir", "cam0_dma", "cam1_dma" };

int isp_chans_setup(struct device *dev, struct ipc_context *ipc)
{
	int idx = 0;
	int ret = 0;
	int count = 0;
	struct device_node *np = dev->of_node;
	struct isp_chan *pchan;

	/* read from device node for total number of isp channels */
	count = of_count_phandle_with_args(np, "mboxes", "#mbox-cells");
	if (count <= 0)
		return -ENODEV;
	ipc->num_isp_chans = count;

	ipc->isp_channels = kcalloc(count, sizeof(struct isp_chan), GFP_KERNEL);
	if (!ipc->isp_channels)
		return -ENOMEM;

	for (idx = 0; idx < count; idx++) {
		pchan = ipc->isp_channels + idx;
		pchan->cl = mbox_client_common;
		pchan->cl.dev = dev;
		pchan->cl.rx_callback = isp_rx_cb[idx];
		pchan->chan = mbox_request_channel_byname(
					&pchan->cl, isp_chan_name[idx]);
		if (IS_ERR(pchan->chan)) {
			pr_err("%s mbox request error\n", isp_chan_name[idx]);
			ret = PTR_ERR(pchan->chan);
			goto isp_out;
		}
	}

	return 0;
isp_out:
	kfree(ipc->isp_channels);
	return ret;
}
#endif

int ipc_chans_setup(struct device *dev, struct ipc_context *ipc)
{
	int idx = 0;
	int ret = 0;
	int count = 0;
	struct device_node *np = dev->of_node;
	struct ipc_chan *pchan;
	const char *chan_name;
	struct device *cvip_dev;
	struct device_node *cvip_np;

	if (IS_ENABLED(CONFIG_X86_IPC)) {

		cvip_dev = cvip_get_mb_device();
		if (IS_ERR_OR_NULL(cvip_dev))
			return -ENODEV;
		cvip_np = cvip_dev->of_node;
	}

	if (IS_ENABLED(CONFIG_X86_IPC))
		count = cvip_get_num_chans();
	else
		/* read from device node for total number of channels */
		count = of_count_phandle_with_args(np, "mboxes", "#mbox-cells");
	if (count <= 0)
		return -ENODEV;
	ipc->num_ipc_chans = count;

	ipc->channels = kcalloc(count, sizeof(struct ipc_chan), GFP_KERNEL);
	if (!ipc->channels)
		return -ENOMEM;

	for (idx = 0; idx < ipc->num_ipc_chans; idx++) {
		pchan = ipc->channels + idx;
		pchan->cl = mbox_client_common;
		pchan->cl.dev = dev;
		pchan->sync = false;
		INIT_LIST_HEAD(&pchan->rx_pending);
		INIT_LIST_HEAD(&pchan->msg_list);
		spin_lock_init(&pchan->rx_lock);
		mutex_init(&pchan->xfer_lock);
		ret = alloc_xfer_list(pchan);
		if (ret < 0)
			goto out;
	}

	/* specific channel configurations */
	for (idx = 0; idx < ipc->num_ipc_chans; idx++) {
		switch (idx) {
		case 0:
			pchan = ipc->channels + idx;
			if (IS_ENABLED(CONFIG_X86_IPC))
				pchan->cl.rx_callback = x86_rx_callback;
			/* sync channel with data acknowledge */
			pchan->sync = true;
			pchan->cl.data_ack = 1;
			break;
		case 1:
			pchan = ipc->channels + idx;   /* use mbox_2@ipc3 */
			if (IS_ENABLED(CONFIG_X86_IPC))
				pchan->cl.rx_callback = x86_mb2_rx_callback;
			if (IS_ENABLED(CONFIG_CVIP_IPC))
				pchan->cl.rx_callback = cvip_mb2_tx_callback;
			break;
		case 2:
			pchan = ipc->channels + idx;   /* use mbox_3@ipc3 */
			if (IS_ENABLED(CONFIG_X86_IPC))
				pchan->cl.rx_callback = x86_mb3_tx_callback;
			if (IS_ENABLED(CONFIG_CVIP_IPC))
				pchan->cl.rx_callback = cvip_mb3_rx_callback;
			break;
		case 3:
			pchan = ipc->channels + idx;   /* use mbox_1@ipc3 */
			if (IS_ENABLED(CONFIG_X86_IPC))
				pchan->cl.rx_callback = x86_mb1_tx_callback;
			if (IS_ENABLED(CONFIG_CVIP_IPC))
				pchan->cl.rx_callback = cvip_mb1_rx_callback;
			/* sync channel */
			pchan->sync = true;
			break;
		case 4:
			pchan = ipc->channels + idx;   /* use mbox_1@ipc0 */
			if (IS_ENABLED(CONFIG_CVIP_IPC))
				pchan->cl.rx_callback = smu_rx_callback;
			break;
		}
	}

	for (idx = 0; idx < ipc->num_ipc_chans; idx++) {
		pchan = ipc->channels + idx;

		if (IS_ENABLED(CONFIG_CVIP_IPC)) {
			chan_name = NULL;
			of_property_read_string_index(np, "mbox-names", idx, &chan_name);
			if (!chan_name) {
				pr_err("mbox-names (idx=%d) error\n", idx);
				goto out;
			}

			pchan->chan = mbox_request_channel_byname(&pchan->cl, chan_name);
			if (IS_ERR(pchan->chan)) {
				pr_err("%s mbox request error\n", chan_name);
				goto out;
			}

		} else if (IS_ENABLED(CONFIG_X86_IPC)) {
			pchan->chan = mbox_request_channel_ofcontroller(
					&pchan->cl, cvip_np, cvip_get_mbid(idx));
			if (IS_ERR(pchan->chan)) {
				pr_err("mbox %d request error\n", idx);
				goto out;
			}
		}
	}

	return 0;

out:
	for (idx = 0; idx < count; idx++) {
		pchan = ipc->channels + idx;
		dealloc_xfer_list(pchan);
	}
	kfree(ipc->channels);
	return ret;
}

int ipc_chans_cleanup(struct ipc_context *ipc)
{
	int idx = 0;
	int count = 0;
	struct ipc_chan *pchan;

	count = ipc->num_ipc_chans;
	for (idx = 0; idx < count; idx++) {
		pchan = ipc->channels + idx;
		dealloc_xfer_list(pchan);
	}
	kfree(ipc->channels);
	return 0;
}

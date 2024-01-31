/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <linux/clk.h>
#include <linux/cvip_event_logger.h>
#include <linux/dma-buf.h>
#include <linux/fdtable.h>
#include <linux/sched/task.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include "ipc.h"

static void imc_intr(struct isp_ipc_device *dev, enum isp_camera cam_id,
		     u32 type, u32 addr)
{
	/* read update write ? */
	type += cam_id * REG_IMC_CAM_SHIFT;
	writel_relaxed(BIT(type), dev->base + addr);
	smp_wmb(); /* memory barrier */
}

static void imc_ctrl_intr(struct isp_ipc_device *dev, enum isp_camera cam_id,
			  enum reg_imc_ctrl_type type)
{
	imc_intr(dev, cam_id, type, CVP_INT_CTRL_INFO);
}

static void imc_inbuf_intr(struct isp_ipc_device *dev, enum isp_camera cam_id,
			   enum reg_imc_inbuf_type type)
{
	imc_intr(dev, cam_id, type, CVP_INT_BUF_RELEASE_ACK);
}

static void imc_outbuf_intr(struct isp_ipc_device *dev, enum isp_camera cam_id,
			    enum reg_imc_outbuf_type type)
{
	imc_intr(dev, cam_id, type, CVP_INT_BUF_NOTIFY_ACK);
}

static void reset_cam(struct isp_ipc_device *dev, int cam_id)
{
	u32 ts = (USEC_PER_SEC / dev->camera[cam_id].cam_ctrl_info.fpks);

	mdelay(prandom_u32_max(ts));

	imc_ctrl_intr(dev, cam_id, REG_IMC_CTRL_RESET);
	CVIP_EVTLOG(dev->evlog_idx, "camera [1] reset", cam_id);
}

static void cam_ctrl_ack(struct isp_ipc_device *dev, int cam_id, u32 ack_id)
{
	void __iomem *reg;

	/* set CAM_CVP_CTRL_INFO_ACK_ID register with request id */
	reg = dev->base + CAM0_CVP_CTRL_INFO_ACK_ID +
	      cam_id * CAM_CTRL_INFO_REG_SIZE;
	writel_relaxed(ack_id, reg);
	smp_wmb(); /* memory barrier */

	imc_ctrl_intr(dev, cam_id, REG_IMC_CTRL_REQ_ACK);
	CVIP_EVTLOG(dev->evlog_idx,
		    "camera [1] control acknowledge with id [2]",
		    cam_id, ack_id);
}

static void cam_ctrl_response(struct isp_ipc_device *dev, int cam_id,
			      struct reg_cam_ctrl_resp *resp)
{
	void __iomem *reg;

	/* fill in response data */
	reg = dev->base + CAM0_CVP_CTRL_INFO_RESP_ID +
	      cam_id * CAM_CTRL_INFO_REG_SIZE;
	__iowrite32_copy(reg, resp, sizeof(*resp) / sizeof(u32));
	smp_wmb(); /* memory barrier */

	imc_ctrl_intr(dev, cam_id, REG_IMC_CTRL_RESP);
	CVIP_EVTLOG(dev->evlog_idx,
		    "camera [1] control response with id [2]",
		    cam_id, resp->resp_id);
}

static void
inbuf_ack(struct isp_ipc_device *dev, enum isp_camera cam_id, u32 fid)
{
	imc_inbuf_intr(dev, cam_id, REG_IMC_INBUF_REL_ACK);
	CVIP_EVTLOG(dev->evlog_idx,
		    "camera [1] input buffer ack [2]", cam_id, fid);
}

static void
prebuf_ack(struct isp_ipc_device *dev, enum isp_camera cam_id, u32 fid)
{
	imc_inbuf_intr(dev, cam_id, REG_IMC_PREBUF_REL_ACK);
	CVIP_EVTLOG(dev->evlog_idx,
		    "camera [1] pre buffer ack [2]", cam_id, fid);
}

static void
postbuf_ack(struct isp_ipc_device *dev, enum isp_camera cam_id, u32 fid)
{
	imc_inbuf_intr(dev, cam_id, REG_IMC_POSTBUF_REL_ACK);
	CVIP_EVTLOG(dev->evlog_idx,
		    "camera [1] post buffer ack [2]", cam_id, fid);
}

static void out_buff_release(struct isp_ipc_device *dev, enum isp_camera cam_id,
			     enum isp_stream_type stream, u32 frame_id)
{
	/* set CAM_CVP_YUV_STRAM0_OUTBUF_RELEASE register */
	writel_relaxed(frame_id, dev->base + CAM0_CVP_YUV_STRM0_OUTPUT_RELEASE
		       + stream * ISP_OUTPUT_BUFFER_RELEASE_SIZE
		       + cam_id * CAM_OUTBUFF_REG_SIZE);
	smp_wmb(); /* memory barrier */
	CVIP_EVTLOG(dev->evlog_idx,
		    "camera [1] stream [2] output buffer release with id [3]",
		    cam_id, stream, frame_id);
}

static void stats_buff_ack(struct isp_ipc_device *dev, enum isp_camera cam_id,
			   enum isp_stats_type type)
{
	enum reg_imc_outbuf_type imc_type;

	if (type == IRSTATS)
		imc_type = REG_IMC_STATS_IR;
	else if (type == RGBSTATS)
		imc_type = REG_IMC_STATS_RGB;
	else
		return;

	imc_outbuf_intr(dev, cam_id, imc_type);
	CVIP_EVTLOG(dev->evlog_idx,
		    "camera [1] type [2] stats buffer acknowledge",
		    cam_id, type);
}

static void
stats_buff_release(struct isp_ipc_device *dev, enum isp_camera cam_id,
		   enum isp_stats_type type, u32 frame_id)
{
	/* set CAM_CVP_IR_STATBUF_RELEASE register */
	writel_relaxed(frame_id, dev->base + CAM0_CVP_IR_STATBUF_RELEASE +
		       type * ISP_STATS_BUFFER_RELEASE_SIZE +
		       cam_id * CAM_OUTBUFF_REG_SIZE);
	smp_wmb(); /* memory barrier */
	CVIP_EVTLOG(dev->evlog_idx,
		    "camera [1] type [2] stats buffer release with id [3]",
		    cam_id, type, frame_id);
}

static void
cam_ctrl_cb(struct mbox_client *cl, enum isp_camera cam_id, void *message)
{
	struct platform_device *pdev;
	struct isp_ipc_device *isp_dev;
	struct cam_ctrl_node *msg;
	struct reg_cam_ctrl_info *info;
	struct reg_cam_ctrl_resp *resp;

	pdev = to_platform_device(cl->dev);
	isp_dev = platform_get_drvdata(pdev);

	dev_dbg(isp_dev->dev, "%s: cam_id=%d\n", __func__, cam_id);

	msg = &isp_dev->cam_ctrls;
	msg->camera_id = cam_id;
	/* copy payload */
	msg->data = *((struct reg_cam_ctrl_info *)message);

	/* acknowledge camera control */
	if (isp_dev->test_config == ISP_HW_MODE)
		cam_ctrl_ack(isp_dev, cam_id, msg->data.req_id);

	handle_isp_client_callback(isp_dev,
				   CAM0_CONTROL + cam_id,
				   &msg->data);

	info = &isp_dev->camera[cam_id].cam_ctrl_info;
	resp = &isp_dev->camera[cam_id].cam_ctrl_resp;
	if (isp_dev->test_config == ISP_HW_MODE &&
	    info->req.response == RESP_YES)
		cam_ctrl_response(isp_dev, cam_id, resp);
}

void isp_cam0_cntl_cb(struct mbox_client *cl, void *message)
{
	cam_ctrl_cb(cl, CAM0, message);
}
void isp_cam1_cntl_cb(struct mbox_client *cl, void *message)
{
	cam_ctrl_cb(cl, CAM1, message);
}

static void out_buff_cb(struct mbox_client *cl, enum isp_camera cam_id,
			enum isp_stream_type stream, void *message)
{
	struct platform_device *pdev;
	struct isp_ipc_device *isp_dev;
	struct out_buff_node *msg;
	int frame_id, slice_id, idx;
	struct isp_output_frame *frame;

	pdev = to_platform_device(cl->dev);
	isp_dev = platform_get_drvdata(pdev);

	dev_dbg(isp_dev->dev, "%s: stream=%u\n", __func__, stream);

	msg = &isp_dev->out_buffs;
	msg->camera_id = cam_id;
	msg->stream = stream;
	/* copy payload */
	msg->data = *((struct reg_outbuf *)message);

	/* acknowledge output buffer */
	/* no need for output buffer acknowledge */
	/* acknowledge is done through PL320 driver */

	handle_isp_client_callback(isp_dev,
				   CAM0_OUTPUT_YUV_0 +
				   stream * STREAM_TYPE_OFF + cam_id,
				   &msg->data);

	frame_id = msg->data.id.frame;
	slice_id = msg->data.id.field;
	idx = frame_id % MAX_OUTPUT_FRAME;
	frame = &isp_dev->camera[cam_id].output_frames[idx];
	if (isp_dev->test_config == ISP_HW_MODE &&
	    slice_id == (frame->total_slices - 1))
		out_buff_release(isp_dev, cam_id, stream, frame_id);
}

void isp_cam0_stream0_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM0, STREAM0, message);
}

void isp_cam0_stream1_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM0, STREAM1, message);
}

void isp_cam0_stream2_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM0, STREAM2, message);
}

void isp_cam0_stream3_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM0, STREAM3, message);
}

void isp_cam0_streamir_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM0, STREAMIR, message);
}

void isp_cam1_stream0_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM1, STREAM0, message);
}

void isp_cam1_stream1_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM1, STREAM1, message);
}

void isp_cam1_stream2_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM1, STREAM2, message);
}

void isp_cam1_stream3_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM1, STREAM3, message);
}

void isp_cam1_streamir_cb(struct mbox_client *client, void *message)
{
	out_buff_cb(client, CAM1, STREAMIR, message);
}

static void stats_buff_cb(struct mbox_client *cl, int cam_id,
			  enum isp_stats_type type, void *message)
{
	struct platform_device *pdev;
	struct isp_ipc_device *isp_dev;
	struct stats_buff_node *msg;

	pdev = to_platform_device(cl->dev);
	isp_dev = platform_get_drvdata(pdev);

	dev_dbg(isp_dev->dev, "%s: type=%u\n", __func__, type);

	msg = &isp_dev->stats_buffs;
	msg->camera_id = cam_id;
	msg->type = type;
	/* copy payload */
	msg->data = *((struct reg_stats_buf *)message);

	if (isp_dev->test_config == ISP_HW_MODE)
		stats_buff_ack(isp_dev, cam_id, type);

	handle_isp_client_callback(isp_dev,
				   CAM0_IR_STAT +
				   type * IR_RGB_STAT_OFF + cam_id,
				   &msg->data);

	if (isp_dev->test_config == ISP_HW_MODE)
		stats_buff_release(isp_dev, cam_id, type, msg->data.id.frame);
}

void isp_cam0_rgb_cb(struct mbox_client *client, void *message)
{
	stats_buff_cb(client, CAM0, RGBSTATS, message);
}

void isp_cam0_ir_cb(struct mbox_client *client, void *message)
{
	stats_buff_cb(client, CAM0, IRSTATS, message);
}

void isp_cam1_rgb_cb(struct mbox_client *client, void *message)
{
	stats_buff_cb(client, CAM1, RGBSTATS, message);
}

void isp_cam1_ir_cb(struct mbox_client *client, void *message)
{
	stats_buff_cb(client, CAM1, IRSTATS, message);
}

static void isp_msg_cb(struct mbox_client *cl, enum isp_camera cam_id,
		       enum isp_message_type type, void *message)
{
	struct platform_device *pdev;
	struct isp_ipc_device *isp_dev;
	struct isp_msg_node *msg;

	pdev = to_platform_device(cl->dev);
	isp_dev = platform_get_drvdata(pdev);

	dev_dbg(isp_dev->dev, "%s: type=%u\n", __func__, type);

	msg = &isp_dev->msg_nodes[type];
	msg->camera_id = cam_id;
	msg->type = type;

	switch (type) {
	case INPUT_BUFF_RELEASE:
		/* copy payload */
		msg->inbuf = *((struct reg_inbuf_release *)message);

		if (isp_dev->test_config == ISP_HW_MODE)
			inbuf_ack(isp_dev, cam_id, msg->inbuf.id.value);

		handle_isp_client_callback(isp_dev,
					   CAM0_INPUT_BUFF + cam_id,
					   &msg->inbuf);

		break;
	case PRE_BUFF_RELEASE:
		/* copy payload */
		msg->prepost = *((struct reg_prepost_release *)message);

		if (isp_dev->test_config == ISP_HW_MODE)
			prebuf_ack(isp_dev, cam_id, msg->inbuf.id.value);

		handle_isp_client_callback(isp_dev,
					   CAM0_PRE_DATA_BUFF + cam_id,
					   &msg->prepost);

		break;
	case POST_BUFF_RELEASE:
		/* copy payload */
		msg->prepost = *((struct reg_prepost_release *)message);

		if (isp_dev->test_config == ISP_HW_MODE)
			postbuf_ack(isp_dev, cam_id, msg->inbuf.id.value);

		handle_isp_client_callback(isp_dev,
					   CAM0_POST_DATA_BUFF + cam_id,
					   &msg->prepost);

		break;
	case WDMA_STATUS:
		/* copy payload */
		msg->wdma = *((struct reg_wdma_report *)message);

		/* acknowledge dma status */
		/* no need for dma status acknowledge */
		/* acknowledge is done through PL320 driver */

		handle_isp_client_callback(isp_dev,
					   CAM0_WDMA_STATUS + cam_id,
					   &msg->wdma);

		break;
	default:
		pr_warn("unsupported message type %d\n", type);
		break;
	}
}

void isp_cam0_isp_input_cb(struct mbox_client *client, void *message)
{
	isp_msg_cb(client, CAM0, INPUT_BUFF_RELEASE, message);
}

void isp_cam1_isp_input_cb(struct mbox_client *client, void *message)
{
	isp_msg_cb(client, CAM1, INPUT_BUFF_RELEASE, message);
}

void isp_cam0_pre_buff_cb(struct mbox_client *client, void *message)
{
	isp_msg_cb(client, CAM0, PRE_BUFF_RELEASE, message);
}

void isp_cam1_pre_buff_cb(struct mbox_client *client, void *message)
{
	isp_msg_cb(client, CAM1, PRE_BUFF_RELEASE, message);
}

void isp_cam0_post_buff_cb(struct mbox_client *client, void *message)
{
	isp_msg_cb(client, CAM0, POST_BUFF_RELEASE, message);
}

void isp_cam1_post_buff_cb(struct mbox_client *client, void *message)
{
	isp_msg_cb(client, CAM1, POST_BUFF_RELEASE, message);
}

void isp_cam0_dma_cb(struct mbox_client *client, void *message)
{
	isp_msg_cb(client, CAM0, WDMA_STATUS, message);
}

void isp_cam1_dma_cb(struct mbox_client *client, void *message)
{
	isp_msg_cb(client, CAM1, WDMA_STATUS, message);
}

static int isp_ipc_show(struct seq_file *f, void *offset)
{
	return 0;
}

static int isp_ipc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_ipc_show, inode->i_private);
}

static const struct file_operations isp_ipc_fops = {
	.owner		= THIS_MODULE,
	.open		= isp_ipc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int isp_ipc_remove(struct platform_device *pdev)
{
	struct isp_ipc_device *isp_ipc_dev = platform_get_drvdata(pdev);

	if (!isp_ipc_dev) {
		pr_err("NULL isp_ipc_dev!!\n");
		return -EINVAL;
	}

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(isp_ipc_dev->d_dir);
#endif

	return 0;
}

/* from isp fw AIDT */
int calculate_input_slice_info(struct camera *icam)
{
	u32 raw_height = icam->cam_ctrl_info.res.height;
	u32 raw_slice_num = icam->isp_ioimg_setting.raw_slice_num;
	u32 slice0_height = 0;
	u32 slicem_height = 0;
	u32 slicen_height = 0;

	if (!raw_height || !raw_slice_num) {
		pr_err("err: raw slice height or number is 0\n");
		return -1;
	}

	//1. sliceNum == 1
	if (raw_slice_num == 1) {
		slice0_height = raw_height;
		goto out;
	}

	//2. sliceNum >= 2
	//Slice 0 height
	//Assume 32 lines for the first output slice
	slice0_height = ISP_HW_PIPELINE_MAX_DELAY_LINE + 32;
	slice0_height = max((raw_height / raw_slice_num), slice0_height);

	//Slice 1 ~ N-2 height
	slicem_height = (raw_height - slice0_height) / (raw_slice_num - 1);
	if (!slicem_height) {
		pr_err("err: middle slice height is 0\n");
		return -1;
	}

	slicen_height = raw_height - slice0_height -
			slicem_height * (raw_slice_num - 2);

out:
	icam->inslice.slice_height[SLICE_FIRST] = slice0_height;
	icam->inslice.slice_height[SLICE_MID] = slicem_height;
	icam->inslice.slice_height[SLICE_LAST] = slicen_height;
	icam->inslice.slice_num = raw_slice_num;
	pr_debug("%s: s0_h=%u, sm_h=%u, sn_h=%u, s_num=%u\n", __func__,
		 slice0_height, slicem_height, slicen_height, raw_slice_num);

	return 0;
}

int calculate_output_slice_info(struct camera *icam)
{
	u32 out_height = icam->isp_ioimg_setting.output_height;
	u32 out_slice_num = icam->cam_ctrl_info.oslice_param.slice_num;

	if (!out_height || !out_slice_num) {
		pr_err("err: output slice height or number is 0\n");
		return -1;
	}

	icam->outslice.slice_height[SLICE_FIRST] =
		icam->cam_ctrl_info.oslice_param.height_s0;
	icam->outslice.slice_height[SLICE_MID] =
		icam->cam_ctrl_info.oslice_param.height_s;
	icam->outslice.slice_height[SLICE_LAST] =
		out_height - icam->outslice.slice_height[SLICE_FIRST] -
		icam->outslice.slice_height[SLICE_MID] * (out_slice_num - 2);
	icam->outslice.slice_num = out_slice_num;
	pr_debug("%s: s0_h=%u, sm_h=%u, sn_h=%u, s_num=%u\n", __func__,
		 icam->outslice.slice_height[SLICE_FIRST],
		 icam->outslice.slice_height[SLICE_MID],
		 icam->outslice.slice_height[SLICE_LAST],
		 icam->outslice.slice_num);

	return 0;
}

static const char *cond_to_string(int condition)
{
	switch (condition) {
	case ISP_COND_WAIT_STREAMON:
		return "stream on";
	case ISP_COND_WAIT_AGAIN0:
		return "again0";
	case ISP_COND_DQBUF_WAIT_FRAME:
		return "dqbuf frame";
	case ISP_COND_DQBUF_WAIT_STATS:
		return "dqbuf stats";
	case ISP_COND_QBUF_WAIT:
		return "qbuf";
	case ISP_COND_PREPOST_WAIT_PRE:
		return "predata";
	case ISP_COND_PREPOST_WAIT_POST:
		return "postdata";
	case ISP_COND_WAIT_WDMA:
		return "wdma";
	case ISP_COND_WAIT_STREAMOFF:
		return "stream off";
	case ISP_CERR_FW_TOUT_WAIT_DROP:
		return "err: fw timeout drop";
	case ISP_CERR_FW_TOUT_WAIT_RECOVER:
		return "err: fw timeout recover";
	case ISP_CERR_QOS_TOUT_WAIT_DROP:
		return "err: qos timeout drop";
	case ISP_CERR_QOS_TOUT_WAIT_RECOVER:
		return "err: qos timeout recover";
	case ISP_CERR_DROP_FRAME_WAIT:
		return "err: drop frame";
	case ISP_CERR_DISORDER_WAIT_SKIP:
		return "err: disorder frame skip";
	case ISP_CERR_DISORDER_WAIT_DROP:
		return "err: disorder frame drop";
	case ISP_CERR_DISORDER_WAIT_RECOVER:
		return "err: disorder frame recover";
	case ISP_CERR_OVERFLOW_WAIT:
		return "err: fw queue overflow";
	default:
		return "?";
	}
}

static int isp_wait_event_helper(struct camera *icam, unsigned long cond)
{
	int ret = -ETIMEDOUT, i;
	unsigned long missed;

	pr_debug("%s: cond=0x%lx(0x%lx)\n",
		 __func__, cond, icam->test_condition);

	ret = wait_event_interruptible_timeout(icam->test_wq,
					       !icam->test_condition,
					       TEST_TOUT);
	if (ret == 0) {
		missed = icam->test_condition & cond;
		for_each_set_bit(i, &missed, ISP_COND_MAX)
			pr_debug("wait event 0x%lx(%s) timeout\n",
				 cond, cond_to_string(i));
		ret = -ETIMEDOUT;
	} else if (ret < 0) {
		pr_debug("wait event 0x%lx(0x%lx) been signaled by %d\n",
			 cond, icam->test_condition, ret);
	} else {
		/* all events received */
		ret = 0;
	}

	return ret;
}

static inline struct isp_ipc_device *to_isp_dev(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct isp_ipc_device *idev = platform_get_drvdata(pdev);
	return idev;
}

static ssize_t testid_show(struct device *dev,
			   struct device_attribute *devattr,
			   char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;
	struct camera *icam = &idev->camera[cam_id];

	return sprintf(buf, "test_id:%d, cur_strm:0x%x\n", icam->isp_test_id,
		       icam->cam_ctrl_info.strm.stream);
}

/* input/output format is obtained from param dmabuf */
static void get_inout_fmt(struct camera *icam)
{
	struct isp_drv_to_cvip_ctl *conf = &icam->isp_ioimg_setting;

	if (!icam->out_param_base) {
		/* set default values */
		conf->type = TYPE_NONE;
		conf->raw_fmt = RAW_PKT_FMT_4;
		conf->raw_slice_num = 1;
		conf->output_slice_num = 1;
		conf->output_width = 1920;
		conf->output_height = 1080;
		conf->output_fmt = IMAGE_FORMAT_NV12;
		icam->outslice.slice_num = 1;
		icam->outslice.slice_height[SLICE_FIRST] = 1080;
		return;
	}

	if (icam->dmabuf_param)
		dma_buf_begin_cpu_access(icam->dmabuf_param, DMA_BIDIRECTIONAL);
	memcpy(conf, icam->out_param_base, sizeof(*conf));
	if (icam->dmabuf_param)
		dma_buf_end_cpu_access(icam->dmabuf_param, DMA_BIDIRECTIONAL);
	pr_debug("io_fmt=(%u,%u,%u,%u,%u,%u,%u)\n", conf->type,
		 conf->raw_fmt, conf->raw_slice_num, conf->output_slice_num,
		 conf->output_width, conf->output_height, conf->output_fmt);
}

void isp_setup_output_rings(struct camera *icam)
{
	enum _imageformat_t format;
	u32 width, height, pixel_size, pitch, chroma_pitch;
	u32 frame_size, yframe_size, uvframe_size, stats_size;
	u32 yslice_size[SLICE_MAX], uvslice_size[SLICE_MAX];
	u8 slice;
	int i;
	struct isp_output_frame *oframe;
	int *sheight = icam->outslice.slice_height;
	u32 tmp;

	width = icam->isp_ioimg_setting.output_width;
	pitch = round_up(width, ISP_STRIDE_ALIGN_SIZE);
	height = icam->isp_ioimg_setting.output_height;
	format = icam->isp_ioimg_setting.output_fmt;
	pixel_size = (format == IMAGE_FORMAT_P010) ? 2 : 1;
	tmp = pitch * pixel_size;
	yframe_size = tmp * height;
	yslice_size[SLICE_FIRST] = tmp * sheight[SLICE_FIRST];
	yslice_size[SLICE_MID] = tmp * sheight[SLICE_MID];
	yslice_size[SLICE_LAST] = tmp * sheight[SLICE_LAST];
	uvframe_size = 0;
	switch (format) {
	case IMAGE_FORMAT_I420:
	case IMAGE_FORMAT_YV12:
		chroma_pitch = round_up(width / 2, ISP_STRIDE_ALIGN_SIZE);
		tmp = chroma_pitch / 2 * pixel_size;
		uvframe_size = tmp * height;
		uvslice_size[SLICE_FIRST] = tmp * sheight[SLICE_FIRST];
		uvslice_size[SLICE_MID] = tmp * sheight[SLICE_MID];
		uvslice_size[SLICE_LAST] = tmp * sheight[SLICE_LAST];
		frame_size = yframe_size + (uvframe_size << 1);
		break;
	case IMAGE_FORMAT_NV12:
	case IMAGE_FORMAT_NV21:
	case IMAGE_FORMAT_P010:
		uvframe_size = yframe_size >> 1;
		uvslice_size[SLICE_FIRST] = yslice_size[SLICE_FIRST] / 2;
		uvslice_size[SLICE_MID] = yslice_size[SLICE_MID] / 2;
		uvslice_size[SLICE_LAST] = yslice_size[SLICE_LAST] / 2;
		frame_size = yframe_size + (yframe_size >> 1);
		break;
	case IMAGE_FORMAT_YUV422PLANAR:
	case IMAGE_FORMAT_YUV422INTERLEAVED:
		uvframe_size = yframe_size >> 1;
		uvslice_size[SLICE_FIRST] = yslice_size[SLICE_FIRST] / 2;
		uvslice_size[SLICE_MID] = yslice_size[SLICE_MID] / 2;
		uvslice_size[SLICE_LAST] = yslice_size[SLICE_LAST] / 2;
		frame_size = yframe_size << 1;
		break;
	default:
		pr_err("%s: unsupported output format %d\n", __func__, format);
	}
	slice = icam->outslice.slice_num;
	stats_size = STATS_SIZE;
	pr_debug("%s: ysize=%u, uvsize=%u, stats_size=%u\n",
		 __func__, yframe_size, uvframe_size, stats_size);
	for (i = 0; i < MAX_OUTPUT_FRAME; i++) {
		oframe = &icam->output_frames[i];
		oframe->data_addr = i * frame_size;
		oframe->y_frame_size = yframe_size;
		oframe->uv_frame_size = uvframe_size;
		oframe->ir_frame_size = yframe_size;
		oframe->width = width;
		oframe->height = height;
		oframe->frame_id = i;
		oframe->cur_slice = 0;
		oframe->total_slices = slice;
		memcpy(oframe->y_slice_size, yslice_size, sizeof(yslice_size));
		memcpy(oframe->uv_slice_size, uvslice_size,
		       sizeof(uvslice_size));
		memcpy(oframe->ir_slice_size, yslice_size, sizeof(yslice_size));
		oframe->stat_size = stats_size;
		oframe->stat_addr = i * stats_size;
		pr_debug("%s: frame = %d, data_addr = 0x%llx, slice_num=%u\n",
			 __func__, i, (u64)oframe->data_addr, slice);
		pr_debug("  y[0]= %u, y[m]= %u, y[n]= %u\n",
			 oframe->y_slice_size[SLICE_FIRST],
			 oframe->y_slice_size[SLICE_MID],
			 oframe->y_slice_size[SLICE_LAST]);
		pr_debug("  uv[0]=%u, uv[m]=%u, uv[n]=%u\n",
			 oframe->uv_slice_size[SLICE_FIRST],
			 oframe->uv_slice_size[SLICE_MID],
			 oframe->uv_slice_size[SLICE_LAST]);
	}
}

void isp_reset_output_rings(struct camera *icam)
{
	memset(icam->output_frames, 0, sizeof(icam->output_frames));
}

static ssize_t testid_store(struct device *dev,
			    struct device_attribute *devattr,
			    const char *buf, size_t n)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;
	struct camera *icam = &idev->camera[cam_id];
	int i;
	int ret = 0;
	int arg[2] = {0};
	unsigned long condition = 0;

	if (sscanf(buf, "%d %d", &arg[0], &arg[1]) == 0)
		return -EINVAL;

	dev_dbg(dev, "IPC test: new_test_id=%d(%d), cur_test_id=%d\n",
		arg[0], arg[1], icam->isp_test_id);

	/* buffer release op should be executed */
	if (icam->isp_test_id != ISP_TEST_NONE && arg[0] != ISP_TEST_STOP)
		return -EBUSY;

	icam->isp_test_id = arg[0];
	i = arg[0];
	if (i == ISP_TEST_PREPARE) {
		dev_dbg(dev, "IPC test:prepare test for %d\n", arg[1]);
		set_bit(ISP_COND_PREPARE, &icam->test_condition);
		i = arg[1];
	}

	switch (i) {
	case ISP_TEST_FMT:
		/* no preparation needed for fmt */
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition))
			break;

		dev_dbg(dev, "IPC test:setup output rings\n");

		get_inout_fmt(icam);
		break;
	case ISP_TEST_START:
		/* no preparation needed for start */
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition))
			break;

		dev_dbg(dev, "IPC test: dma_buf check\n");
		/* always succeed */
		break;
	case ISP_TEST_STREAMON:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			set_bit(ISP_COND_WAIT_STREAMON, &icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test: stream on\n");

		condition = BIT(ISP_COND_WAIT_STREAMON);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TEST_AGAIN0:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			set_bit(ISP_COND_WAIT_AGAIN0, &icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test: camera control\n");

		condition = BIT(ISP_COND_WAIT_AGAIN0);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TEST_DQBUF:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			icam->test_dqbuf_resp.lock_frame_id = FRAMEID_INVALID;
			icam->test_dqbuf_resp.lock_stats_id = FRAMEID_INVALID;
			set_bit(ISP_COND_DQBUF_WAIT_FRAME,
				&icam->test_condition);
			set_bit(ISP_COND_DQBUF_WAIT_STATS,
				&icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test:locking a frame\n");

		condition = BIT(ISP_COND_DQBUF_WAIT_FRAME) |
			    BIT(ISP_COND_DQBUF_WAIT_STATS);
		ret = isp_wait_event_helper(icam, condition);
		if (ret < 0) {
			icam->test_dqbuf_resp.lock_frame_id = FRAMEID_INVALID;
			icam->test_dqbuf_resp.lock_stats_id = FRAMEID_INVALID;
			break;
		}

		if (icam->dmabuf_param)
			dma_buf_begin_cpu_access(icam->dmabuf_param,
						 DMA_BIDIRECTIONAL);
		if (icam->out_param_base)
			memcpy(icam->out_param_base, &icam->test_dqbuf_resp,
			       sizeof(icam->test_dqbuf_resp));
		if (icam->dmabuf_param)
			dma_buf_end_cpu_access(icam->dmabuf_param,
					       DMA_BIDIRECTIONAL);

		break;
	case ISP_TEST_QBUF:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			set_bit(ISP_COND_QBUF_WAIT, &icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test: unlocking the frame\n");

		condition = BIT(ISP_COND_QBUF_WAIT);
		ret = isp_wait_event_helper(icam, condition);
		icam->test_dqbuf_resp.lock_frame_id = FRAMEID_INVALID;
		icam->test_dqbuf_resp.lock_stats_id = FRAMEID_INVALID;
		break;
	case ISP_TEST_PREPOST:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			set_bit(ISP_COND_PREPOST_WAIT_PRE,
				&icam->test_condition);
			set_bit(ISP_COND_PREPOST_WAIT_POST,
				&icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test: pre/post data\n");

		condition = BIT(ISP_COND_PREPOST_WAIT_PRE) |
			    BIT(ISP_COND_PREPOST_WAIT_POST);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TEST_WDMA:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			set_bit(ISP_COND_WAIT_WDMA, &icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test: write dma\n");

		condition = BIT(ISP_COND_WAIT_WDMA);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TERR_FW_TOUT:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			/* generate drop frame in sensor faking */
			icam->terr_base.drop_frame = FRAMEID_INVALID;
			icam->terr_base.recover_frame = FRAMEID_INVALID;
			set_bit(ISP_CERR_FW_TOUT_WAIT_DROP,
				&icam->test_condition);
			set_bit(ISP_CERR_FW_TOUT_WAIT_RECOVER,
				&icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test_err: fw timeout\n");

		condition = BIT(ISP_CERR_FW_TOUT_WAIT_DROP) |
			    BIT(ISP_CERR_FW_TOUT_WAIT_RECOVER);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TERR_QOS_TOUT:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			icam->terr_base.drop_frame = FRAMEID_INVALID;
			icam->terr_base.recover_frame = FRAMEID_INVALID;
			set_bit(ISP_CERR_QOS_TOUT_WAIT_DROP,
				&icam->test_condition);
			set_bit(ISP_CERR_QOS_TOUT_WAIT_RECOVER,
				&icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test_err: qos timeout\n");

		condition = BIT(ISP_CERR_QOS_TOUT_WAIT_DROP) |
			    BIT(ISP_CERR_QOS_TOUT_WAIT_RECOVER);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TERR_DROP_FRAME:
		/* no preparation needed for drop frame */
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition))
			break;

		dev_dbg(dev, "IPC test_err: drop frame\n");

		/* generate drop frame in sensor faking */
		icam->terr_base.drop_frame = FRAMEID_INVALID;
		icam->terr_base.recover_frame = FRAMEID_INVALID;
		set_bit(ISP_CERR_DROP_FRAME_WAIT, &icam->test_condition);

		condition = BIT(ISP_CERR_DROP_FRAME_WAIT);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TERR_DISORDER_FRAME:
		/* no preparation needed for disorder frame */
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition))
			break;

		dev_dbg(dev, "IPC test_err: disorder frame\n");

		icam->terr_base.drop_frame = FRAMEID_INVALID;
		icam->terr_base.recover_frame = FRAMEID_INVALID;

		/* generate out of order frame in sensor faking */
		set_bit(ISP_CERR_DISORDER_WAIT_SKIP, &icam->test_condition);
		set_bit(ISP_CERR_DISORDER_WAIT_DROP, &icam->test_condition);
		set_bit(ISP_CERR_DISORDER_WAIT_RECOVER, &icam->test_condition);

		condition = BIT(ISP_CERR_DISORDER_WAIT_SKIP);
		condition = BIT(ISP_CERR_DISORDER_WAIT_DROP);
		condition = BIT(ISP_CERR_DISORDER_WAIT_RECOVER);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TERR_OVERFLOW:
		/* no preparation needed for fw queue overflow */
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition))
			break;

		dev_dbg(dev, "IPC test_err: fw queue overflow\n");

		set_bit(ISP_CERR_OVERFLOW_WAIT, &icam->test_condition);

		condition = BIT(ISP_CERR_OVERFLOW_WAIT);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TERR_CAM_RESET:
		/* no preparation needed for camera reset */
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition))
			break;

		dev_dbg(dev, "IPC test_err: reset cam\n");
		reset_cam(idev, cam_id);
		break;
	case ISP_TEST_STREAMOFF:
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition)) {
			set_bit(ISP_COND_WAIT_STREAMOFF, &icam->test_condition);
			break;
		}

		dev_dbg(dev, "IPC test: stream off\n");

		condition = BIT(ISP_COND_WAIT_STREAMOFF);
		ret = isp_wait_event_helper(icam, condition);
		break;
	case ISP_TEST_STOP:
		/* clean up mailbox send register */
		ipc_mboxes_cleanup(idev->ipc, ISP_CHAN);

		/* no preparation needed for stop */
		if (test_bit(ISP_COND_PREPARE, &icam->test_condition))
			break;

		dev_dbg(dev, "IPC test: dma_buf release\n");
		if (icam->dmabuf_frame) {
			dma_buf_kunmap(icam->dmabuf_frame, 0,
				       icam->out_frame_base);
			dma_buf_put(icam->dmabuf_frame);
			icam->out_frame_base = NULL;
			icam->dmabuf_frame = NULL;
		}

		if (icam->dmabuf_stats) {
			dma_buf_kunmap(icam->dmabuf_stats, 0,
				       icam->out_stats_base);
			dma_buf_put(icam->dmabuf_stats);
			icam->out_stats_base = NULL;
			icam->dmabuf_stats = NULL;
		}

		if (icam->dmabuf_param) {
			dma_buf_kunmap(icam->dmabuf_param, 0,
				       icam->out_param_base);
			dma_buf_put(icam->dmabuf_param);
			icam->out_param_base = NULL;
			icam->dmabuf_param = NULL;
		}

		break;
	default:
		dev_err(dev, "ipc_default test\n");
		ret = -EINVAL;
	}

	icam->isp_test_id = ISP_TEST_NONE;
	if (!test_and_clear_bit(ISP_COND_PREPARE, &icam->test_condition))
		icam->test_condition = 0;

	return (ret < 0) ? ret : n;
}

static ssize_t sensor_frame_show(struct device *dev,
				 struct device_attribute *devattr,
				 char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n", idev->camera[cam_id].sensor_frame);
}

static ssize_t input_buffer_base_show(struct device *dev,
				      struct device_attribute *devattr,
				      char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%llx\n", idev->camera[cam_id].input_buf_offset);
}

static ssize_t input_buffer_size_show(struct device *dev,
				      struct device_attribute *devattr,
				      char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%lx\n", idev->camera[cam_id].input_buf_size);
}

static ssize_t output_buffer_base_show(struct device *dev,
				       struct device_attribute *devattr,
				       char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%llx\n", idev->camera[cam_id].output_buf_offset);
}

static ssize_t output_buffer_size_show(struct device *dev,
				       struct device_attribute *devattr,
				       char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%lx\n", idev->camera[cam_id].output_buf_size);
}

static ssize_t stats_buffer_base_show(struct device *dev,
				      struct device_attribute *devattr,
				      char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%llx\n", idev->camera[cam_id].stats_buf_offset);
}

static ssize_t stats_buffer_size_show(struct device *dev,
				      struct device_attribute *devattr,
				      char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%lx\n", idev->camera[cam_id].stats_buf_size);
}

static ssize_t data_buffer_base_show(struct device *dev,
				     struct device_attribute *devattr,
				     char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%llx\n", idev->camera[cam_id].data_buf_offset);
}

static ssize_t data_buffer_size_show(struct device *dev,
				     struct device_attribute *devattr,
				     char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%lx\n", idev->camera[cam_id].data_buf_size);
}

static ssize_t isp_input_frame_show(struct device *dev,
				    struct device_attribute *devattr,
				    char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n", idev->camera[cam_id].isp_input_frame);
}

static ssize_t isp_predata_frame_show(struct device *dev,
				      struct device_attribute *devattr,
				      char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n", idev->camera[cam_id].isp_predata_frame);
}

static ssize_t isp_postdata_frame_show(struct device *dev,
				       struct device_attribute *devattr,
				       char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n", idev->camera[cam_id].isp_postdata_frame);
}

static ssize_t isp_output_frame_show(struct device *dev,
				     struct device_attribute *devattr,
				     char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n", idev->camera[cam_id].isp_output_frame);
}

static ssize_t isp_stats_frame_show(struct device *dev,
				    struct device_attribute *devattr,
				    char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n", idev->camera[cam_id].isp_stat_frame);
}

static ssize_t sensor_width_show(struct device *dev,
				 struct device_attribute *devattr,
				 char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n",
		       (u32)idev->camera[cam_id].cam_ctrl_info.res.width);
}

static ssize_t sensor_width_store(struct device *dev,
				  struct device_attribute *devattr,
				  const char *buf, size_t n)
{
	u32 data;
	int ret;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	ret = kstrtou32(buf, 0, &data);
	if (ret)
		return ret;
	idev->camera[cam_id].cam_ctrl_info.res.width = (u32)data;
	return n;
}

static ssize_t sensor_height_show(struct device *dev,
				  struct device_attribute *devattr,
				  char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n",
		       (u32)idev->camera[cam_id].cam_ctrl_info.res.height);
}

static ssize_t sensor_height_store(struct device *dev,
				   struct device_attribute *devattr,
				   const char *buf, size_t n)
{
	u32 data;
	int ret;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	ret = kstrtou32(buf, 0, &data);
	if (ret)
		return ret;
	idev->camera[cam_id].cam_ctrl_info.res.height = (u32)data;
	return n;
}

static ssize_t sensor_bitdepth_show(struct device *dev,
				    struct device_attribute *devattr,
				    char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n",
		       (u32)idev->camera[cam_id].cam_ctrl_info.strm.bd);
}

static ssize_t sensor_bitdepth_store(struct device *dev,
				     struct device_attribute *devattr,
				     const char *buf, size_t n)
{
	u32 data;
	int ret;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	ret = kstrtou32(buf, 0, &data);
	if (ret)
		return ret;
	idev->camera[cam_id].cam_ctrl_info.strm.bd = data;
	return n;
}

static ssize_t output_format_show(struct device *dev,
				  struct device_attribute *devattr,
				  char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n",
		       idev->camera[cam_id].isp_ioimg_setting.output_fmt);
}

static ssize_t output_format_store(struct device *dev,
				   struct device_attribute *devattr,
				   const char *buf, size_t n)
{
	u32 data;
	int ret;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	ret = kstrtou32(buf, 0, &data);
	if (ret)
		return ret;
	idev->camera[cam_id].isp_ioimg_setting.output_fmt = data;
	return n;
}

static ssize_t output_width_show(struct device *dev,
				 struct device_attribute *devattr,
				 char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n",
		       idev->camera[cam_id].isp_ioimg_setting.output_width);
}

static ssize_t output_width_store(struct device *dev,
				  struct device_attribute *devattr,
				  const char *buf, size_t n)
{
	u32 data;
	int ret;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	ret = kstrtou32(buf, 0, &data);
	if (ret)
		return ret;
	idev->camera[cam_id].isp_ioimg_setting.output_width = data;
	return n;
}

static ssize_t output_height_show(struct device *dev,
				  struct device_attribute *devattr,
				  char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n",
		       idev->camera[cam_id].isp_ioimg_setting.output_height);
}

static ssize_t output_height_store(struct device *dev,
				   struct device_attribute *devattr,
				   const char *buf, size_t n)
{
	u32 data;
	int ret;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	ret = kstrtou32(buf, 0, &data);
	if (ret)
		return ret;
	idev->camera[cam_id].isp_ioimg_setting.output_height = data;
	return n;
}

static ssize_t output_slice_show(struct device *dev,
				 struct device_attribute *devattr,
				 char *buf)
{
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	return sprintf(buf, "0x%x\n",
		       idev->camera[cam_id].isp_ioimg_setting.output_slice_num);
}

static ssize_t output_slice_store(struct device *dev,
				  struct device_attribute *devattr,
				  const char *buf, size_t n)
{
	u32 data;
	int ret;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;

	ret = kstrtou32(buf, 0, &data);
	if (ret)
		return ret;
	idev->camera[cam_id].isp_ioimg_setting.output_slice_num = data;
	return n;
}

static int install_remote_fd(pid_t remote_pid, int remote_fd)
{
	int ret = 0;
	struct file *remote_file;
	struct files_struct *remote_files;
	struct task_struct *remote_task;
	int local_fd;

	remote_task = get_pid_task(find_get_pid(remote_pid), PIDTYPE_PID);
	if (!remote_task) {
		pr_err("%s: get remote task err\n", __func__);
		return -ENOENT;
	}

	remote_files = get_files_struct(remote_task);
	put_task_struct(remote_task);

	if (!remote_files) {
		pr_err("%s: get remote files err\n", __func__);
		return -ENOENT;
	}

	spin_lock(&remote_files->file_lock);
	remote_file = fcheck_files(remote_files, remote_fd);
	if (remote_file)
		get_file(remote_file);
	else
		ret = -ENOENT;
	spin_unlock(&remote_files->file_lock);
	put_files_struct(remote_files);

	if (ret) {
		pr_err("%s: get remote file err\n", __func__);
		return ret;
	}

	local_fd = get_unused_fd_flags(O_CLOEXEC);
	if (local_fd < 0) {
		pr_err("%s: get unused fd err %d\n", __func__, local_fd);
		return local_fd;
	}

	fd_install(local_fd, remote_file);

	pr_debug("%s: install to local_fd %d\n", __func__, local_fd);
	return local_fd;
}

static ssize_t
usrframefd_store(struct device *dev, struct device_attribute *devattr,
		 const char *buf, size_t count)
{
	pid_t remote_pid;
	int remote_fd, local_fd;
	void *vaddr;
	struct dma_buf *dmabuf;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;
	struct camera *icam = &idev->camera[cam_id];

	if (sscanf(buf, "%d:%d", &remote_pid, &remote_fd) != 2)
		return -EINVAL;

	dev_dbg(dev, "%s: rpid=%d, rfd=%d\n", __func__, remote_pid, remote_fd);

	local_fd = install_remote_fd(remote_pid, remote_fd);
	if (local_fd < 0)
		return local_fd;

	dmabuf = dma_buf_get(local_fd);
	if (IS_ERR(dmabuf)) {
		dev_err(dev, "Failed to get dma buffer\n");
		return -EINVAL;
	}

	vaddr = dma_buf_kmap(dmabuf, 0);
	if (IS_ERR_OR_NULL(vaddr)) {
		dev_err(dev, "Failed to map buffer\n");
		dma_buf_put(dmabuf);
		return -EINVAL;
	}

	icam->dmabuf_frame = dmabuf;
	icam->out_frame_base = vaddr;
	icam->out_frame_size = dmabuf->size;

	dev_dbg(dev, "get frame dmabuf va=[%p,%lx], *va=%x\n",
		vaddr, dmabuf->size, *(u32 *)vaddr);
	return count;
}

static ssize_t
usrstatsfd_store(struct device *dev, struct device_attribute *devattr,
		 const char *buf, size_t count)
{
	pid_t remote_pid;
	int remote_fd, local_fd;
	void *vaddr;
	struct dma_buf *dmabuf;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;
	struct camera *icam = &idev->camera[cam_id];

	if (sscanf(buf, "%d:%d", &remote_pid, &remote_fd) != 2)
		return -EINVAL;

	dev_dbg(dev, "%s: rpid=%d, rfd=%d\n", __func__, remote_pid, remote_fd);

	local_fd = install_remote_fd(remote_pid, remote_fd);
	if (local_fd < 0)
		return local_fd;

	dmabuf = dma_buf_get(local_fd);
	if (IS_ERR(dmabuf)) {
		dev_err(dev, "Failed to get dma buffer\n");
		return -EINVAL;
	}

	vaddr = dma_buf_kmap(dmabuf, 0);
	if (IS_ERR_OR_NULL(vaddr)) {
		dev_err(dev, "Failed to map buffer\n");
		dma_buf_put(dmabuf);
		return -EINVAL;
	}

	icam->dmabuf_stats = dmabuf;
	icam->out_stats_base = vaddr;
	icam->out_stats_size = dmabuf->size;

	dev_dbg(dev, "get stats dmabuf va=[%p,%lx], *va=%x\n",
		vaddr, dmabuf->size, *(u32 *)vaddr);
	return count;
}

static ssize_t
usrparamfd_store(struct device *dev, struct device_attribute *devattr,
		 const char *buf, size_t count)
{
	pid_t remote_pid;
	int remote_fd, local_fd;
	void *vaddr;
	struct dma_buf *dmabuf;
	struct isp_ipc_device *idev = to_isp_dev(dev);
	struct isp_device_attribute *attr = to_isp_dev_attr(devattr);
	int cam_id = attr->idx;
	struct camera *icam = &idev->camera[cam_id];

	if (sscanf(buf, "%d:%d", &remote_pid, &remote_fd) != 2)
		return -EINVAL;

	dev_dbg(dev, "%s: rpid=%d, rfd=%d\n", __func__, remote_pid, remote_fd);

	local_fd = install_remote_fd(remote_pid, remote_fd);
	if (local_fd < 0)
		return local_fd;

	dmabuf = dma_buf_get(local_fd);
	if (IS_ERR(dmabuf)) {
		dev_err(dev, "Failed to get dma buffer\n");
		return -EINVAL;
	}

	vaddr = dma_buf_kmap(dmabuf, 0);
	if (IS_ERR_OR_NULL(vaddr)) {
		dev_err(dev, "Failed to map buffer\n");
		dma_buf_put(dmabuf);
		return -EINVAL;
	}

	icam->dmabuf_param = dmabuf;
	icam->out_param_base = vaddr;
	icam->out_param_size = dmabuf->size;

	dev_dbg(dev, "get param dmabuf va=[%p,%lx], *va=%x\n",
		vaddr, dmabuf->size, *(u32 *)vaddr);
	return count;
}

static ISP_SYSFS_RW(testid, 0);
static ISP_SYSFS_RW(testid, 1);
static ISP_SYSFS_RO(sensor_frame, 0);
static ISP_SYSFS_RO(sensor_frame, 1);
static ISP_SYSFS_RO(output_buffer_base, 0);
static ISP_SYSFS_RO(output_buffer_base, 1);
static ISP_SYSFS_RO(output_buffer_size, 0);
static ISP_SYSFS_RO(output_buffer_size, 1);
static ISP_SYSFS_RO(input_buffer_base, 0);
static ISP_SYSFS_RO(input_buffer_base, 1);
static ISP_SYSFS_RO(input_buffer_size, 0);
static ISP_SYSFS_RO(input_buffer_size, 1);
static ISP_SYSFS_RO(stats_buffer_base, 0);
static ISP_SYSFS_RO(stats_buffer_base, 1);
static ISP_SYSFS_RO(stats_buffer_size, 0);
static ISP_SYSFS_RO(stats_buffer_size, 1);
static ISP_SYSFS_RO(data_buffer_base, 0);
static ISP_SYSFS_RO(data_buffer_base, 1);
static ISP_SYSFS_RO(data_buffer_size, 0);
static ISP_SYSFS_RO(data_buffer_size, 1);
static ISP_SYSFS_RO(isp_output_frame, 0);
static ISP_SYSFS_RO(isp_output_frame, 1);
static ISP_SYSFS_RO(isp_input_frame, 0);
static ISP_SYSFS_RO(isp_input_frame, 1);
static ISP_SYSFS_RO(isp_postdata_frame, 0);
static ISP_SYSFS_RO(isp_postdata_frame, 1);
static ISP_SYSFS_RO(isp_predata_frame, 0);
static ISP_SYSFS_RO(isp_predata_frame, 1);
static ISP_SYSFS_RO(isp_stats_frame, 0);
static ISP_SYSFS_RO(isp_stats_frame, 1);
static ISP_SYSFS_RW(sensor_width, 0);
static ISP_SYSFS_RW(sensor_width, 1);
static ISP_SYSFS_RW(sensor_height, 0);
static ISP_SYSFS_RW(sensor_height, 1);
static ISP_SYSFS_RW(sensor_bitdepth, 0);
static ISP_SYSFS_RW(sensor_bitdepth, 1);
static ISP_SYSFS_RW(output_format, 0);
static ISP_SYSFS_RW(output_format, 1);
static ISP_SYSFS_RW(output_width, 0);
static ISP_SYSFS_RW(output_width, 1);
static ISP_SYSFS_RW(output_height, 0);
static ISP_SYSFS_RW(output_height, 1);
static ISP_SYSFS_RW(output_slice, 0);
static ISP_SYSFS_RW(output_slice, 1);
static ISP_SYSFS_WO(usrframefd, 0);
static ISP_SYSFS_WO(usrframefd, 1);
static ISP_SYSFS_WO(usrstatsfd, 0);
static ISP_SYSFS_WO(usrstatsfd, 1);
static ISP_SYSFS_WO(usrparamfd, 0);
static ISP_SYSFS_WO(usrparamfd, 1);

struct attribute *sensor0_attrs[] = {
	&isp_attr_testid0.dev_attr.attr,
	&isp_attr_sensor_frame0.dev_attr.attr,
	&isp_attr_output_buffer_base0.dev_attr.attr,
	&isp_attr_output_buffer_size0.dev_attr.attr,
	&isp_attr_input_buffer_base0.dev_attr.attr,
	&isp_attr_input_buffer_size0.dev_attr.attr,
	&isp_attr_stats_buffer_base0.dev_attr.attr,
	&isp_attr_stats_buffer_size0.dev_attr.attr,
	&isp_attr_data_buffer_base0.dev_attr.attr,
	&isp_attr_data_buffer_size0.dev_attr.attr,
	&isp_attr_isp_output_frame0.dev_attr.attr,
	&isp_attr_isp_input_frame0.dev_attr.attr,
	&isp_attr_isp_postdata_frame0.dev_attr.attr,
	&isp_attr_isp_predata_frame0.dev_attr.attr,
	&isp_attr_isp_stats_frame0.dev_attr.attr,
	&isp_attr_sensor_width0.dev_attr.attr,
	&isp_attr_sensor_height0.dev_attr.attr,
	&isp_attr_sensor_bitdepth0.dev_attr.attr,
	&isp_attr_output_format0.dev_attr.attr,
	&isp_attr_output_width0.dev_attr.attr,
	&isp_attr_output_height0.dev_attr.attr,
	&isp_attr_output_slice0.dev_attr.attr,
	&isp_attr_usrframefd0.dev_attr.attr,
	&isp_attr_usrstatsfd0.dev_attr.attr,
	&isp_attr_usrparamfd0.dev_attr.attr,
	NULL,
};

struct attribute *sensor1_attrs[] = {
	&isp_attr_testid1.dev_attr.attr,
	&isp_attr_sensor_frame1.dev_attr.attr,
	&isp_attr_output_buffer_base1.dev_attr.attr,
	&isp_attr_output_buffer_size1.dev_attr.attr,
	&isp_attr_input_buffer_base1.dev_attr.attr,
	&isp_attr_input_buffer_size1.dev_attr.attr,
	&isp_attr_stats_buffer_base1.dev_attr.attr,
	&isp_attr_stats_buffer_size1.dev_attr.attr,
	&isp_attr_data_buffer_base1.dev_attr.attr,
	&isp_attr_data_buffer_size1.dev_attr.attr,
	&isp_attr_isp_output_frame1.dev_attr.attr,
	&isp_attr_isp_input_frame1.dev_attr.attr,
	&isp_attr_isp_postdata_frame1.dev_attr.attr,
	&isp_attr_isp_predata_frame1.dev_attr.attr,
	&isp_attr_isp_stats_frame1.dev_attr.attr,
	&isp_attr_sensor_width1.dev_attr.attr,
	&isp_attr_sensor_height1.dev_attr.attr,
	&isp_attr_sensor_bitdepth1.dev_attr.attr,
	&isp_attr_output_format1.dev_attr.attr,
	&isp_attr_output_width1.dev_attr.attr,
	&isp_attr_output_height1.dev_attr.attr,
	&isp_attr_output_slice1.dev_attr.attr,
	&isp_attr_usrframefd1.dev_attr.attr,
	&isp_attr_usrstatsfd1.dev_attr.attr,
	&isp_attr_usrparamfd1.dev_attr.attr,
	NULL,
};

struct attribute_group sensor0_attr_group = {
	.attrs = sensor0_attrs,
	.name = "sensor0"
};

struct attribute_group sensor1_attr_group = {
	.attrs = sensor1_attrs,
	.name = "sensor1"
};

static int __init ipc_isp_init(struct device *dev)
{
	if (sysfs_create_group(&dev->kobj, &sensor0_attr_group)) {
		pr_err("Cannot create sysfs file\n");
		return -1;
	}

	if (sysfs_create_group(&dev->kobj, &sensor1_attr_group)) {
		pr_err("Cannot create sysfs file\n");
		sysfs_remove_group(&dev->kobj, &sensor0_attr_group);
		return -1;
	}

	return 0;
}

struct isp_img isp_img_list[] = {
	{0, 0, "isp_test/3M_2048x1536_depth10_pkt4.raw"}, /* default */
	{352, 288, "isp_test/chess.raw"},
	{4096, 3072, "isp_test/12M_4096x3072_depth10_pkt4.raw"},
	{2048, 1536, "isp_test/3M_2048x1536_depth10_pkt4.raw"},
};

const char *isp_get_img_path(int width, int height)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(isp_img_list); i++)
		if (isp_img_list[i].width == width &&
		    isp_img_list[i].height == height)
			return isp_img_list[i].img_path;

	return NULL;
}

int
isp_load_image(struct isp_ipc_device *idev, int cam_id, const char *img_path)
{
	int ret;
	const struct firmware *fw;
	struct camera *cam = &idev->camera[cam_id];

	if (!img_path)
		return -EINVAL;

	ret = request_firmware_into_buf(&fw, img_path, idev->dev,
					cam->input_base, cam->input_buf_size);

	pr_debug("%s: %s to load %s for cam%d\n",
		 __func__, ret ? "fail" : "success", img_path, cam_id);

	ret = (ret < 0) ? ret : fw->size;
	release_firmware(fw);

	return ret;
}

static int setup_defaults(struct isp_ipc_device *idev)
{
	struct camera *cam;
	int i;

	for (i = 0; i < NUM_OF_CAMERA; i++) {
		cam = &idev->camera[i];
		get_inout_fmt(cam);
		isp_setup_output_rings(cam);
		isp_load_image(idev, i, isp_get_img_path(0, 0));
	}

	return 0;
}

static int isp_ipc_probe(struct platform_device *pdev)
{
	int ret;
	struct isp_ipc_device *isp_ipc_dev;
	struct camera *cam0, *cam1, *cam;
	struct resource *res, *res_sc;
	struct resource res_in, res_out;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *node;
	u8 *input_base, *output_base;
	int i;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("Error in get cvp_isp aperture\n");
		return -EINVAL;
	}

	res_sc = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res_sc) {
		pr_err("Error in get isp_scratch aperture\n");
		return -EINVAL;
	}

	node = of_parse_phandle(np, "memory-region", 0);
	if (!node) {
		pr_err("No input memory-region specified\n");
		return -EINVAL;
	}
	ret = of_address_to_resource(node, 0, &res_in);
	if (ret)
		return ret;

	node = of_parse_phandle(np, "memory-region", 1);
	if (!node) {
		pr_err("No output memory-region specified\n");
		return -EINVAL;
	}
	ret = of_address_to_resource(node, 0, &res_out);
	if (ret)
		return ret;

	isp_ipc_dev = devm_kzalloc(dev, sizeof(*isp_ipc_dev), GFP_KERNEL);
	if (!isp_ipc_dev)
		return -ENOMEM;

	isp_ipc_dev->base = devm_ioremap(dev, res->start, resource_size(res));
	if (isp_ipc_dev->base == NULL) {
		pr_err("ioremap failed\n");
		return -EADDRNOTAVAIL;
	}

	/* for the scatch register mapping */
	isp_ipc_dev->sc_base = devm_ioremap(dev, res_sc->start,
					    resource_size(res_sc));
	if (!isp_ipc_dev->sc_base) {
		pr_err("scratch ioremap failed\n");
		return -EADDRNOTAVAIL;
	}

	cam0 = &isp_ipc_dev->camera[0];
	cam1 = &isp_ipc_dev->camera[1];

	/* isp input buffer */
	input_base = devm_memremap(dev, res_in.start, resource_size(&res_in),
				   MEMREMAP_WB);
	if (IS_ERR(input_base)) {
		pr_err("input buffer memremap failed\n");
		return -EADDRNOTAVAIL;
	}
	cam0->input_buf_offset = res_in.start;
	cam0->input_buf_size = CAM0_INPUT_SIZE;
	cam1->input_buf_offset = res_in.start + CAM0_INPUT_SIZE;
	cam1->input_buf_size = resource_size(&res_in) - CAM0_INPUT_SIZE;
	cam0->input_base = input_base;
	cam1->input_base = input_base + CAM0_INPUT_SIZE;

	/* isp output buffer */
	output_base = devm_memremap(dev, res_out.start, resource_size(&res_out),
				    MEMREMAP_WB);
	if (IS_ERR(output_base)) {
		pr_err("output buffer memremap failed\n");
		return -EADDRNOTAVAIL;
	}
	cam0->output_buf_offset = res_out.start;
	cam0->output_buf_size = CAM0_OUTPUT_SIZE;
	cam1->output_buf_offset = res_out.start + CAM0_OUTPUT_SIZE;
	cam1->output_buf_size = resource_size(&res_out) - CAM0_OUTPUT_SIZE;
	cam0->output_base = output_base;
	cam1->output_base = output_base + CAM0_OUTPUT_SIZE;

	for (i = 0; i < NUM_OF_CAMERA; i++) {
		cam = &isp_ipc_dev->camera[i];
		cam->isp_test_id = ISP_TEST_NONE;
		cam->test_condition = 0;
		cam->test_dqbuf_resp.lock_frame_id = FRAMEID_INVALID;
		cam->test_dqbuf_resp.lock_stats_id = FRAMEID_INVALID;
		init_waitqueue_head(&cam->test_wq);
	}

	if (of_property_read_u32(np, "test-config", &isp_ipc_dev->test_config))
		isp_ipc_dev->test_config = ISP_SIMNOW_MODE;

	isp_ipc_dev->dev = &pdev->dev;
	ret = ipc_init(&(isp_ipc_dev->ipc));
	if (ret < 0) {
		pr_err("IPC init failure!!\n");
		goto err_init;
	}

	setup_defaults(isp_ipc_dev);

	isp_chans_setup(&pdev->dev, isp_ipc_dev->ipc);

	platform_set_drvdata(pdev, isp_ipc_dev);

#ifdef CONFIG_DEBUG_FS
	isp_ipc_dev->d_dir = debugfs_create_dir(dev_name(&pdev->dev), NULL);
	if (IS_ERR(isp_ipc_dev->d_dir)) {
		pr_warn("Failed to create debugfs: %ld\n",
			PTR_ERR(isp_ipc_dev->d_dir));
		isp_ipc_dev->d_dir = NULL;
	} else {
		debugfs_create_file("testing", 0400, isp_ipc_dev->d_dir,
				isp_ipc_dev, &isp_ipc_fops);
	}
#endif

	isp_ipc_dev->evlog_idx = CVIP_EVTLOGINIT(CVIP_ISP_EVENT_LOG_SIZE);
	dev_info(dev, "ISP test device initialized\n");
	ipc_isp_init(dev);

	return 0;

err_init:
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(isp_ipc_dev->d_dir);
#endif
	return ret;
}

static const struct of_device_id isp_ipc_of_match[] = {
	{ .compatible = "amd,isp-ipc", },
	{ },
};

static struct platform_driver isp_ipc_driver = {
	.probe = isp_ipc_probe,
	.remove = isp_ipc_remove,
	.driver = {
		.name = "isp_ipc",
		.of_match_table = isp_ipc_of_match,
	},
};
module_platform_driver(isp_ipc_driver);

MODULE_DESCRIPTION("MERO ISP TEST Driver");

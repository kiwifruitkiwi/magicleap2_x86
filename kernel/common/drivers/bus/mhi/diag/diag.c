// SPDX-License-Identifier: BSD-3-Clause-Clear
/**
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 */

#include <linux/mhi.h>
#include "diag_hdlc.h"
#include "diag.h"
#include "diag_mhi.h"
#include "trace.h"
#include "diag_msg.h"

#define MAX_STR_LENGTH    64

static int diag_encode_send(struct mhi_device *mhi_dev, void *buf, int len)
{
	int err = 0;
	int max_len = 0;
	struct diag_send_desc_type send = { NULL, NULL, DIAG_STATE_START, 0 };
	struct diag_hdlc_dest_type enc = { NULL, NULL, 0 };
	unsigned char *hdlc_encode_buf = NULL;
	int hdlc_encode_buf_len;

	if (!buf)
		return -EINVAL;

	if (len <= 0) {
		dev_err(&mhi_dev->dev,
			"diag: In %s, invalid len: %d", __func__, len);
		return -EINVAL;
	}

	hdlc_encode_buf = kzalloc(DIAG_MAX_HDLC_BUF_SIZE, GFP_KERNEL);
	if (!hdlc_encode_buf)
		return -ENOMEM;

	/**
	 * The worst case length will be twice as the incoming packet length.
	 * Add 3 bytes for CRC bytes (2 bytes) and delimiter (1 byte)
	 */
	max_len = (2 * len) + 3;
	if (max_len > DIAG_MAX_HDLC_BUF_SIZE) {
		dev_err(&mhi_dev->dev,
			"Current payload size %d\n",
			max_len);
		kfree(hdlc_encode_buf);
		return -EINVAL;
	}

	/* Perform HDLC encoding on incoming data */
	send.state = DIAG_STATE_START;
	send.pkt = (void *)(buf);
	send.last = (void *)(buf + len - 1);
	send.terminate = 1;

	enc.dest = hdlc_encode_buf;
	enc.dest_last = (void *)(hdlc_encode_buf + max_len - 1);
	diag_hdlc_encode(&send, &enc);
	hdlc_encode_buf_len = (int)(enc.dest -
					(void *)hdlc_encode_buf);
	err = diag_mhi_send(mhi_dev, hdlc_encode_buf, hdlc_encode_buf_len);
	if (err < 0)
		dev_err(&mhi_dev->dev,
			"diag: send_diag_cmd failed, error code %d\n",
			err);
	kfree(hdlc_encode_buf);
	return err;
}

static void diag_printbuf(struct device *dev,
			  unsigned char *payload,
			  int payload_len)
{
	int  i;
	char temp[MAX_STR_LENGTH];
	unsigned int pos = 0, chars_written = 0;
	static const char formatstr1[] = "[FWLOG]%02X ";
	static const char formatstr2[] = "%02X ";

	for (i = 0; i < payload_len; i++) {
		if (i % 16 == 0) {
			if (i) {
				trace_ath11k_diag_fwlog(dev,
							temp,
							MAX_STR_LENGTH);
				pos = 0;
			}
			chars_written = snprintf(&temp[pos],
						 MAX_STR_LENGTH - pos,
						 formatstr1,
						 payload[i]);
			pos += chars_written;
		} else if (pos < MAX_STR_LENGTH) {
			chars_written = snprintf(&temp[pos],
						 MAX_STR_LENGTH - pos,
						 formatstr2,
						 payload[i]);
			pos += chars_written;
		}
	}

	if (pos) {
		temp[pos] = 0;
		trace_ath11k_diag_fwlog(dev, temp, MAX_STR_LENGTH);
	}
}

int diag_local_write(struct device *dev, unsigned char *buf, int len)
{
	int ret = 0;
	u8 cmd;
	struct diag_hdlc_decode_type hdlc_decode;
	int iter = 0;
	unsigned char *hdlc_buf = NULL;
	u32 hdlc_buf_len = 0;

	if (len >= DIAG_MAX_RSP_SIZE) {
		dev_err(dev,
			"%s: Response length is more than supported len.\n",
			__func__);
		return -EINVAL;
	}
	hdlc_buf = kzalloc(DIAG_MAX_HDLC_BUF_SIZE, GFP_KERNEL);
	if (!hdlc_buf)
		return -ENOMEM;

	do {
		hdlc_decode.dest_ptr  = hdlc_buf + hdlc_buf_len;
		hdlc_decode.dest_size = DIAG_MAX_HDLC_BUF_SIZE - hdlc_buf_len;
		hdlc_decode.src_ptr   = buf;
		hdlc_decode.src_size  = len;
		hdlc_decode.src_idx   = iter;
		hdlc_decode.dest_idx  = 0;
		hdlc_decode.escaping  = 0;

		ret = diag_hdlc_decode(&hdlc_decode);
		/**
		 * hdlc_buf is of size DIAG_MAX_HDLC_BUF_SIZE.
		 * But the decoded packet should be within DIAG_MAX_REQ_SIZE.
		 */
		if (hdlc_buf_len + hdlc_decode.dest_idx <= DIAG_MAX_REQ_SIZE) {
			hdlc_buf_len += hdlc_decode.dest_idx;
		} else {
			dev_err(dev,
				"%s: Packet size is %d, max size is %d\n",
				__func__,
				hdlc_buf_len + hdlc_decode.dest_idx,
				DIAG_MAX_REQ_SIZE);
			ret = -EINVAL;
			break;
		}

		if (ret == HDLC_COMPLETE) {
			ret = crc_check(hdlc_buf, hdlc_buf_len);
			if (ret != 0) {
				dev_err(dev, "%s: Bad CRC\n", __func__);
				goto next;
			}
			hdlc_buf_len -= HDLC_FOOTER_LEN;

			if (hdlc_buf_len < 1) {
				dev_err(dev,
					"%s: len = %d, dest_len = %d\n",
					__func__,
					hdlc_buf_len,
					hdlc_decode.dest_idx);
				ret = -EINVAL;
				break;
			}

			cmd = hdlc_buf[DIAG_MSG_CMD_CODE_OFFSET];

				dev_dbg(dev, "%s: cmd_code %d\n",
					__func__, cmd);
			switch (cmd) {
			case DIAG_CMD_MSG_CONFIG:
				dev_dbg(dev, "received DIAG_CMD_MSG_CONFIG\n");
				break;
			case DIAG_CMD_EXT_MSG:
				diag_local_ext_msg_report_handler(hdlc_buf,
								  hdlc_buf_len);
				break;
			default:
				dev_dbg(dev, "%s: Unknown cmd_code %d\n",
					__func__, cmd);
				break;
			}
		} else {
			/*Check if all src buffer processed,while not
			 *complete
			 */
			if (hdlc_decode.src_idx == hdlc_decode.src_size)
				break;

			dev_err(dev, "%s: diag_hdlc_decode failed, ret = %d\n",
				__func__,
				ret);
		}
next:
		hdlc_buf_len = 0;
		while (iter < len) {
			if (buf[iter] != CONTROL_CHAR) {
				iter++;
			} else {
				iter++;
				break;
			}
		}

		if (iter == len)
			break;

	} while (true);

	kfree(hdlc_buf);
	hdlc_buf_len = 0;

	diag_printbuf(dev, buf, len);

	return ret;
}

int diag_loglevel_set(struct mhi_device *mhi_dev)
{
	struct msg_set_rt_mask_req_type *rt_mask_req = NULL;
	struct msg_set_toggle_config_req_type *toggle_config_req = NULL;
	struct msg_set_event_mask_req_type *event_mask_req = NULL;
	struct msg_set_log_mask_req_type *log_mask_req = NULL;
	struct diag_reg_param  diag_param[MAX_DIAG_PARAM];
	u32 diag_msg_lvl;
	/* todo: can be configured */
	u8 diag_event_enable = true;
	u8 diag_event_mask = DIAG_EVENT_MASK;
	u8 diag_log_mask = DIAG_LOG_MASK;
	int ret = 0;
	int req_len = 0;

	memset(diag_param, 0, sizeof(struct diag_reg_param) * MAX_DIAG_PARAM);

	diag_msg_lvl = MSG_LVL_LOW;
	rt_mask_req = kzalloc(DIAG_MAX_REQ_SIZE, GFP_KERNEL);
	ret = diag_create_msg_set_rt_mask_req(diag_param, rt_mask_req,
					      &req_len, MSG_SSID_WLAN,
					      MSG_SSID_WLAN_LAST, diag_msg_lvl);
	if (ret == 0)
		diag_encode_send(mhi_dev, rt_mask_req, req_len);
	else
		dev_err(&mhi_dev->dev,
			"%s: diag_create_msg_set_rt_mask_req failed\n",
			__func__);

	kfree(rt_mask_req);
	rt_mask_req = NULL;

	toggle_config_req = kzalloc(DIAG_MAX_REQ_SIZE, GFP_KERNEL);
	ret = diag_create_msg_event_toggle_req(toggle_config_req,
					       &req_len, diag_event_enable);
	if (ret == 0)
		diag_encode_send(mhi_dev, toggle_config_req, req_len);

	else
		dev_err(&mhi_dev->dev,
			"%s: diag_create_msg_event_toggle_config_req failed\n",
			__func__);

	kfree(toggle_config_req);
	toggle_config_req = NULL;

	event_mask_req = kzalloc(DIAG_MAX_REQ_SIZE, GFP_KERNEL);
	ret = diag_create_msg_set_event_mask_req(event_mask_req, &req_len,
						 diag_event_mask);
	if (ret == 0)
		diag_encode_send(mhi_dev, event_mask_req, req_len);

	else
		dev_err(&mhi_dev->dev,
			"%s: diag_create_msg_set_event_mask_req failed\n",
			__func__);

	kfree(event_mask_req);
	event_mask_req = NULL;

	log_mask_req = kzalloc(DIAG_MAX_REQ_SIZE, GFP_KERNEL);
	ret = diag_create_msg_set_log_mask_req(log_mask_req,
					       &req_len,
					       diag_log_mask);
	if (ret == 0)
		diag_encode_send(mhi_dev, log_mask_req, req_len);

	else
		dev_err(&mhi_dev->dev,
			"%s: diag_create_msg_set_log_mask_req failed\n",
			__func__);

	kfree(log_mask_req);
	log_mask_req = NULL;

	return 0;
}

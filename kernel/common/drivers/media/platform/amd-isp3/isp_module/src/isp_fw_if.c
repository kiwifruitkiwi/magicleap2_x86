/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "isp_fw_if.h"
#include "log.h"
#include "gamma_table.h"
#include "isp_soc_adpt.h"
#include "isp_para_capability.h"
#include "hal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][isp_fw_if]"

#define ISP_MOVNTDQA_SIZE      16
#define ISP_MOVNTDQA_OPS       4

static void memcpy_todev(void *dst, void *src, size_t count)
{
	size_t movcnt = count / ISP_MOVNTDQA_SIZE;

	/*
	 * if src / dst address is not 128bit aligned, then we cannot use
	 * the optimized movntdqa instruction.
	 */
	if (((unsigned long)(dst) & 0xF) || ((unsigned long)(src) & 0xF) || (count & 0xF)) {
		pr_err("%s: not aligned, src:0x%llx, dst:0x%llx, count:0x%llx\n",
		       __func__, (u64)src, (u64)dst, (u64)count);
		memcpy(dst, src, count);
		return;
	}

	kernel_fpu_begin();

	while (movcnt >= ISP_MOVNTDQA_OPS) {
		asm("movdqa (%0), %%xmm0\n"
		    "movdqa 16(%0), %%xmm1\n"
		    "movdqa 32(%0), %%xmm2\n"
		    "movdqa 48(%0), %%xmm3\n"
		    "movntdq %%xmm0, (%1)\n"
		    "movntdq %%xmm1, 16(%1)\n"
		    "movntdq %%xmm2, 32(%1)\n"
		    "movntdq %%xmm3, 48(%1)\n"
		    :: "r" (src), "r" (dst) : "memory");
		    src += (ISP_MOVNTDQA_SIZE * 4);
		    dst += (ISP_MOVNTDQA_SIZE * 4);
		    movcnt -= ISP_MOVNTDQA_OPS;
	}

	while (movcnt--) {
		asm("movdqa (%0), %%xmm0\n"
		    "movntdq %%xmm0, (%1)\n"
		    :: "r" (src), "r" (dst) : "memory");
		    src += ISP_MOVNTDQA_SIZE;
		    dst += ISP_MOVNTDQA_SIZE;
	}
	kernel_fpu_end();
}

static void memcpy_fromdev(void *dst, void *src, size_t count)
{
	size_t movcnt = count / ISP_MOVNTDQA_SIZE;

	/*
	 * if src / dst address is not 128bit aligned, then we cannot use
	 * the optimized movntdqa instruction.
	 */
	if (((unsigned long)(dst) & 0xF) || ((unsigned long)(src) & 0xF) || (count & 0xF)) {
		pr_err("%s: not aligned, src:0x%llx, dst:0x%llx, count:0x%llx\n",
		       __func__, (u64)src, (u64)dst, (u64)count);
		memcpy(dst, src, count);
		return;
	}

	kernel_fpu_begin();

	while (movcnt >= ISP_MOVNTDQA_OPS) {
		asm("movntdqa (%0), %%xmm0\n"
		    "movntdqa 16(%0), %%xmm1\n"
		    "movntdqa 32(%0), %%xmm2\n"
		    "movntdqa 48(%0), %%xmm3\n"
		    "movdqa %%xmm0, (%1)\n"
		    "movdqa %%xmm1, 16(%1)\n"
		    "movdqa %%xmm2, 32(%1)\n"
		    "movdqa %%xmm3, 48(%1)\n"
		    :: "r" (src), "r" (dst) : "memory");
		    src += (ISP_MOVNTDQA_SIZE * 4);
		    dst += (ISP_MOVNTDQA_SIZE * 4);
		    movcnt -= ISP_MOVNTDQA_OPS;
	}

	while (movcnt--) {
		asm("movntdqa (%0), %%xmm0\n"
		    "movdqa %%xmm0, (%1)\n"
		    :: "r" (src), "r" (dst) : "memory");
		    src += ISP_MOVNTDQA_SIZE;
		    dst += ISP_MOVNTDQA_SIZE;
	}
	kernel_fpu_end();
}

unsigned int get_nxt_cmd_seq_num(struct isp_context *isp)
{
	unsigned int seq_num;

	if (isp == NULL)
		return 1;

	seq_num = isp->host2fw_seq_num++;
	return seq_num;
}

unsigned char compute_check_sum(unsigned char *buffer,
		unsigned int buffer_size)
{
	unsigned int i;
	unsigned char checksum;

	checksum = 0;
	for (i = 0; i < buffer_size; i++)
		checksum += buffer[i];

	return checksum;
};

int no_fw_cmd_ringbuf_slot(struct isp_context *isp,
			enum fw_cmd_resp_stream_id cmd_buf_idx)
{
	unsigned int rreg;
	unsigned int wreg;
	unsigned int rd_ptr, wr_ptr;
	unsigned int new_wr_ptr;
	unsigned int len;

	isp_get_cmd_buf_regs(cmd_buf_idx, &rreg, &wreg, NULL, NULL, NULL);
	isp_fw_buf_get_cmd_base(isp->fw_work_buf_hdl, cmd_buf_idx,
				NULL, NULL, &len);

	rd_ptr = isp_hw_reg_read32(rreg);
	wr_ptr = isp_hw_reg_read32(wreg);

	new_wr_ptr = wr_ptr + sizeof(struct _Cmd_t);
	if (wr_ptr >= rd_ptr) {
		if (new_wr_ptr < len)
			return 0;
		else if (new_wr_ptr == len) {
			if (rd_ptr == 0)
				return 1;
			else
				return 0;
		} else {
			new_wr_ptr -= len;

			if (new_wr_ptr < rd_ptr)
				return 0;
			else
				return 1;
		}
	} else {
		if (new_wr_ptr < rd_ptr)
			return 0;
		else
			return 1;
	}
}

int insert_isp_fw_cmd(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream, struct _Cmd_t *cmd)
{
	unsigned long long mem_sys;
	unsigned long long mem_addr;
	unsigned int rreg;
	unsigned int wreg;
	unsigned int wr_ptr, old_wr_ptr, rd_ptr;
	int result = RET_SUCCESS;
	unsigned int len;

	if ((isp == NULL) || (cmd == NULL)) {
		ISP_PR_ERR("fail bad cmd 0x%p\n", cmd);
		return RET_FAILURE;
	}

	if (stream > FW_CMD_RESP_STREAM_ID_3) {
		ISP_PR_ERR("fail bad id %d\n", stream);
		return RET_FAILURE;
	}

	switch (cmd->cmdId) {
	case CMD_ID_SEND_BUFFER:
	case CMD_ID_SEND_FRAME_INFO:
	//case CMD_ID_SEND_MODE4_FRAME_INFO:
	//case CMD_ID_BIND_STREAM:
	//case CMD_ID_UNBIND_STREAM:
	case CMD_ID_GET_FW_VERSION:
	case CMD_ID_SET_LOG_MOD_EXT:
	case CMD_ID_SET_SENSOR_PROP:
	case CMD_ID_SET_SENSOR_CALIBDB:
	case CMD_ID_SET_DEVICE_SCRIPT:
	case CMD_ID_SET_DEVICE_CTRL_MODE:
	case CMD_ID_SET_SENSOR_HDR_PROP:
	case CMD_ID_SET_SENSOR_EMB_PROP:
	case CMD_ID_SET_CLK_INFO:
	case CMD_ID_LOG_SET_LEVEL:
	case CMD_ID_LOG_ENABLE_MOD:
	case CMD_ID_LOG_DISABLE_MOD:
	case CMD_ID_PROFILER_GET_RESULT:
	case CMD_ID_ENABLE_POWER_GATING:
	case CMD_ID_DISABLE_POWER_GATING:
	case CMD_ID_ENABLE_CLOCK_GATING:
	case CMD_ID_DISABLE_CLOCK_GATING:
/*
 *	case CMD_ID_SEND_FRAME_CTRL:
 *	case CMD_ID_SET_SENSOR_M2M_CALIBDB:
 *	case CMD_ID_READ_REG:
 *	case CMD_ID_WRITE_REG:
 *	case CMD_ID_SEND_I2C_MSG:
 *	case CMD_ID_PREPARE_CHANGE_ICLK:
 *	case CMD_ID_ACK_SENSOR_GROUP_HOLD:
 *	case CMD_ID_ACK_SENSOR_GROUP_RELEASE:
 *	case CMD_ID_ACK_SENSOR_SET_AGAIN:
 *	case CMD_ID_ACK_SENSOR_SET_DGAIN:
 *	case CMD_ID_ACK_SENSOR_SET_ITIME:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_HIGH_ITIME:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_LOW_ITIME:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_LOW_ITIME_RATIO:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_ITIME:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_HIGH_AGAIN:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_LOW_AGAIN:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_LOW_AGAIN_RATIO:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_AGAIN:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_HIGH_DGAIN:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_LOW_DGAIN:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_LOW_DGAIN_RATIO:
 *	case CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_DGAIN:
 *	case CMD_ID_ACK_LENS_SET_POS:
 *	case CMD_ID_ACK_FLASH_SET_POWER:
 *	case CMD_ID_ACK_FLASH_SET_ON:
 *	case CMD_ID_ACK_FLASH_SET_OFF:
 */
		stream = FW_CMD_RESP_STREAM_ID_GLOBAL;
		break;
	default:
		break;

	}

	isp_get_cmd_buf_regs(stream, &rreg, &wreg, NULL, NULL, NULL);
	isp_fw_buf_get_cmd_base(isp->fw_work_buf_hdl, stream,
				&mem_sys, &mem_addr, &len);

	if (no_fw_cmd_ringbuf_slot(isp, stream)) {
		ISP_PR_ERR("fail no cmdslot %s(%d)\n",
			isp_dbg_get_stream_str(stream), stream);
		return RET_FAILURE;
	}

	wr_ptr = isp_hw_reg_read32(wreg);
	rd_ptr = isp_hw_reg_read32(rreg);
	old_wr_ptr = wr_ptr;
	if (rd_ptr > len) {
		ISP_PR_ERR("fail %s(%u),rd_ptr %u(should<=%u),wr_ptr %u\n",
			isp_dbg_get_stream_str(stream),
			stream, rd_ptr, len, wr_ptr);
		return RET_FAILURE;
	};

	if (wr_ptr > len) {
		ISP_PR_ERR("fail %s(%u),wr_ptr %u(should<=%u), rd_ptr %u\n",
			isp_dbg_get_stream_str(stream),
			stream, wr_ptr, len, rd_ptr);
		return RET_FAILURE;
	};

	/* clear all pending read/write */
	mb();

	if (wr_ptr < rd_ptr) {
		mem_addr += wr_ptr;

		memcpy_todev((unsigned char *)(mem_sys + wr_ptr),
			     (unsigned char *)cmd, sizeof(struct _Cmd_t));
	} else {
		if ((len - wr_ptr) >= (sizeof(struct _Cmd_t))) {
			mem_addr += wr_ptr;

			memcpy_todev((unsigned char *)(mem_sys + wr_ptr),
				     (unsigned char *)cmd, sizeof(struct _Cmd_t));
		} else {
			unsigned int size;
			unsigned long long dst_addr;
			unsigned char *src;

			dst_addr = mem_addr + wr_ptr;
			src = (unsigned char *)cmd;
			size = len - wr_ptr;

			memcpy_todev((unsigned char *)(mem_sys + wr_ptr), src, size);

			src += size;
			size = sizeof(struct _Cmd_t) - size;
			dst_addr = mem_addr;

			memcpy_todev((unsigned char *)(mem_sys), src, size);
		}
	}

	/* make sure WC buffer are flushed */
	wmb();

	if (result != RET_SUCCESS)
		return result;

	wr_ptr += sizeof(struct _Cmd_t);
	if (wr_ptr >= len)
		wr_ptr -= len;

	isp_hw_reg_write32(wreg, wr_ptr);

	return result;
}

struct isp_cmd_element *isp_append_cmd_2_cmdq(
	struct isp_context *isp,
	struct isp_cmd_element *command)
{
	struct isp_cmd_element *tail_element = NULL;
	struct isp_cmd_element *copy_command = NULL;

	if (command == NULL) {
		ISP_PR_ERR("NULL cmd pointer\n");
		return NULL;
	}

	copy_command = isp_sys_mem_alloc(sizeof(struct isp_cmd_element));
	if (copy_command == NULL) {
		ISP_PR_ERR("memory allocate fail\n");
		return NULL;
	}

	memcpy(copy_command, command, sizeof(struct isp_cmd_element));
	copy_command->next = NULL;
	isp_mutex_lock(&isp->cmd_q_mtx);
	if (isp->cmd_q == NULL) {
		isp->cmd_q = copy_command;
		goto quit;
	}

	tail_element = isp->cmd_q;

	/*find the tail element */
	while (tail_element->next != NULL)
		tail_element = tail_element->next;

	/*insert current element after the tail element */
	tail_element->next = copy_command;
quit:
	isp_mutex_unlock(&isp->cmd_q_mtx);
	return copy_command;
};

struct isp_cmd_element *isp_rm_cmd_from_cmdq(
	struct isp_context *isp,
	unsigned int seq_num, unsigned int cmd_id,
	int signal_evt)
{
	struct isp_cmd_element *curr_element;
	struct isp_cmd_element *prev_element;

	isp_mutex_lock(&isp->cmd_q_mtx);

	curr_element = isp->cmd_q;
	if (curr_element == NULL) {
		ISP_PR_ERR("fail empty q\n");
		goto quit;
	}

	/*process the first element */
	if ((curr_element->seq_num == seq_num)
		&& (curr_element->cmd_id == cmd_id)) {
		isp->cmd_q = curr_element->next;
		curr_element->next = NULL;
		goto quit;
	}

	prev_element = curr_element;
	curr_element = curr_element->next;

	while (curr_element != NULL) {
		if ((curr_element->seq_num == seq_num)
		&& (curr_element->cmd_id == cmd_id)) {
			prev_element->next = curr_element->next;
			curr_element->next = NULL;
			goto quit;
		}

		prev_element = curr_element;
		curr_element = curr_element->next;
	}

	ISP_PR_ERR("cmd(0x%x,seq:%u) not found\n", cmd_id, seq_num);
 quit:
	if (curr_element && curr_element->evt && signal_evt) {
		ISP_PR_INFO("signal event %p\n", curr_element->evt);
		isp_event_signal(0, curr_element->evt);
	}
	isp_mutex_unlock(&isp->cmd_q_mtx);
	return curr_element;
};

struct isp_cmd_element *isp_rm_cmd_from_cmdq_by_stream(
	struct isp_context *isp,
	enum fw_cmd_resp_stream_id stream,
	int signal_evt)
{
	struct isp_cmd_element *curr_element;
	struct isp_cmd_element *prev_element;

	isp_mutex_lock(&isp->cmd_q_mtx);

	curr_element = isp->cmd_q;
	if (curr_element == NULL) {
		ISP_PR_WARN("fail empty stream:%u\n", stream);
		goto quit;
	}

	/*process the first element */
	if (curr_element->stream == stream) {
		isp->cmd_q = curr_element->next;
		curr_element->next = NULL;
		goto quit;
	}

	prev_element = curr_element;
	curr_element = curr_element->next;

	while (curr_element != NULL) {
		if (curr_element->stream == stream) {
			prev_element->next = curr_element->next;
			curr_element->next = NULL;
			goto quit;
		}

		prev_element = curr_element;
		curr_element = curr_element->next;
	}

	ISP_PR_ERR("stream %u no found\n", stream);
 quit:
	if (curr_element && curr_element->evt && signal_evt)
		isp_event_signal(0, curr_element->evt);
	isp_mutex_unlock(&isp->cmd_q_mtx);
	return curr_element;
}

int fw_if_send_prev_buf(struct isp_context __maybe_unused *isp,
	enum camera_id __maybe_unused cam_id,
	struct isp_img_buf_info __maybe_unused *buffer)
{
	return IMF_RET_FAIL;
	/*todo to be added later */
}

int fw_if_send_img_buf(struct isp_context *isp,
				struct isp_mapped_buf_info *buffer,
				enum camera_id cam_id, enum stream_id str_id)
{
	struct _CmdSendBuffer_t cmd;
	enum fw_cmd_resp_stream_id stream;
	int result;

	if (!is_para_legal(isp, cam_id)
		|| (buffer == NULL) || (str_id > STREAM_ID_ZSL)) {
		ISP_PR_ERR("fail para,isp %p,buf %p,cid %u,sid %u\n",
			isp, buffer, cam_id, str_id);
		return RET_FAILURE;
	};

	memset(&cmd, 0, sizeof(cmd));
	switch (str_id) {
	case STREAM_ID_PREVIEW:
		cmd.bufferType = BUFFER_TYPE_PREVIEW;
		break;
	case STREAM_ID_VIDEO:
		cmd.bufferType = BUFFER_TYPE_VIDEO;
		break;
	case STREAM_ID_ZSL:
		cmd.bufferType = BUFFER_TYPE_STILL;
		break;
	default:
		ISP_PR_ERR("fail bad sid %d\n", str_id);
		return RET_FAILURE;
	};
	stream = isp_get_stream_id_from_cid(isp, cam_id);
	cmd.buffer.vmidSpace.bit.vmid = 0;
	cmd.buffer.vmidSpace.bit.space = ADDR_SPACE_TYPE_GPU_VA;
	isp_split_addr64(buffer->y_map_info.mc_addr,
			 &cmd.buffer.bufBaseALo, &cmd.buffer.bufBaseAHi);
	cmd.buffer.bufSizeA = buffer->y_map_info.len;

	isp_split_addr64(buffer->u_map_info.mc_addr,
			 &cmd.buffer.bufBaseBLo, &cmd.buffer.bufBaseBHi);
	cmd.buffer.bufSizeB = buffer->u_map_info.len;

	isp_split_addr64(buffer->v_map_info.mc_addr,
			 &cmd.buffer.bufBaseCLo, &cmd.buffer.bufBaseCHi);
	cmd.buffer.bufSizeC = buffer->v_map_info.len;

	result = isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER,
				 stream,
				 FW_CMD_PARA_TYPE_DIRECT, &cmd, sizeof(cmd));

	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail send,buf %p,cid %u,sid %u\n",
			buffer, cam_id, str_id);
		return RET_FAILURE;
	}
	ISP_PR_DBG("suc,buf %p,cid %u,sid %u, y_addr:%llx, %llx, %llx\n",
		buffer, cam_id, str_id, buffer->y_map_info.sys_addr,
		buffer->u_map_info.sys_addr, buffer->v_map_info.sys_addr);
	return RET_SUCCESS;
}

int isp_send_fw_cmd_cid(struct isp_context *isp,
			enum camera_id cam_id,
			unsigned int cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size)
{
	return isp_send_fw_cmd_ex(isp, cam_id,
			cmd_id, stream, directcmd, package,
			package_size, NULL, NULL, NULL, NULL);
}

int isp_send_fw_cmd(struct isp_context *isp, unsigned int cmd_id,
			enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size)
{
	return isp_send_fw_cmd_ex(isp, CAMERA_ID_MAX,	/*don't care */
			cmd_id, stream, directcmd, package,
			package_size, NULL, NULL, NULL, NULL);
}

int isp_send_fw_cmd_sync(struct isp_context *isp,
			unsigned int cmd_id,
			enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size,
			unsigned int timeout /*in ms */,
			void *resp_pl, unsigned int *resp_pl_len)
{
	int ret;
	struct isp_event evt;
	unsigned int seq;

	isp_event_init(&evt, 1, 0);
	ret = isp_send_fw_cmd_ex(isp, CAMERA_ID_MAX,	/*don't care */
				 cmd_id, stream, directcmd, package,
				 package_size, &evt, &seq, resp_pl,
				 resp_pl_len);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("fail(%d) send cmd\n", ret);
		return ret;
	};

	ret = isp_event_wait(&evt, timeout);

	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("fail(%d) wait\n", ret);
	};

	if (ret == RET_TIMEOUT) {
		struct isp_cmd_element *ele;

		ele = isp_rm_cmd_from_cmdq(isp, seq, cmd_id, false);
		if (ele) {
			if (ele->mc_addr)
				isp_fw_ret_pl(isp->fw_work_buf_hdl,
					ele->mc_addr);
			if (ele->gpu_pkg)
				isp_gpu_mem_free(ele->gpu_pkg);

			isp_sys_mem_free(ele);
		}
	};
	return ret;
}

/*
 * Implementation for sending ISP FW command, used by online tuning.
 * This function will allocate a GPU buffer as a data exchange buffer between
 * ISP driver and ISP FW, and will copy data from the given buffer |package|.
 * This implementation waits until ISP FW ack, therefore, no any
 * synchronization needed.
 */
int isp_send_fw_cmd_online_tune(struct isp_context *isp, unsigned int cmd_id,
			enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size, unsigned int timeout,
			void *resp_pl, unsigned int *resp_pl_len)
{
	int ret;
	struct isp_event evt;
	unsigned int seq;
	struct isp_gpu_mem_info *gpu_mem;

	/* create a GPU memory to exhange data between ISP FW */
	gpu_mem = isp_gpu_mem_alloc(package_size, ISP_GPU_MEM_TYPE_NLFB);
	if (gpu_mem == NULL) {
		ISP_PR_ERR("%s: no gpu mem(%u), cid %d\n",
			__func__, package_size, cmd_id);
		return -ENOMEM;
	}

	/* copy user data to interal GPU memory */
	memcpy(gpu_mem->sys_addr, package, package_size);

	isp_event_init(&evt, 1, 0);
	ret = isp_send_fw_cmd_by_dma_buf(isp, CAMERA_ID_MAX,	/*don't care */
				 cmd_id, stream, directcmd, gpu_mem,
				 package_size, &evt, &seq, resp_pl,
				 resp_pl_len);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("%s: fail(%d) send cmd\n", __func__, ret);
		goto exit;
	};

	ISP_PR_INFO("%s: bef wait cmd:0x%x,evt:%p\n", __func__, cmd_id, &evt);
	ret = isp_event_wait(&evt, timeout);
	ISP_PR_INFO("%s: aft wait cmd:0x%x,evt:%p\n", __func__, cmd_id, &evt);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("%s: fail(%d) wait\n",
			__func__, ret);
	} else {
		/* copy back data from GPU mem to user buffer */
		memcpy(package, gpu_mem->sys_addr, package_size);
	}

	if (ret == RET_TIMEOUT) {
		struct isp_cmd_element *ele;

		ele = isp_rm_cmd_from_cmdq(isp, seq, cmd_id, false);
		if (ele) {
			if (ele->mc_addr)
				isp_fw_ret_pl(isp->fw_work_buf_hdl,
					ele->mc_addr);
			if (ele->gpu_pkg)
				isp_gpu_mem_free(ele->gpu_pkg);

			isp_sys_mem_free(ele);
		}
	};
exit:
	/* always release GPU memory */
	if (gpu_mem)
		isp_gpu_mem_free(gpu_mem);
	return ret;
}

int isp_send_fw_cmd_ex(struct isp_context *isp,
			enum camera_id cam_id,
			unsigned int cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size, struct isp_event *evt,
			unsigned int *seq, void *resp_pl,
			unsigned int *resp_pl_len)
{
	unsigned long long package_base = 0;
	unsigned long long pack_sys = 0;
	unsigned int pack_len;
	int result;
	unsigned int seq_num;
	struct isp_cmd_element command_element;
	struct isp_cmd_element *cmd_ele = NULL;
	int os_status;
	unsigned int sleep_count;
	struct _Cmd_t cmd;
	struct _CmdParamPackage_t *pkg;
	struct isp_gpu_mem_info *gpu_mem = NULL;

	if (directcmd && (package_size > sizeof(cmd.cmdParam))) {
		ISP_PR_ERR("fail pkgsize(%u) cid:%d,stream %d\n",
		package_size, cmd_id, stream);
		return RET_FAILURE;
	}

	if (package_size && (package == NULL)) {
		ISP_PR_ERR("fail null pkg cid:%d,stream %d\n",
			cmd_id, stream);
		return RET_FAILURE;
	};

	os_status = isp_mutex_lock(&isp->command_mutex);
	sleep_count = 0;
	while (1) {
		if (no_fw_cmd_ringbuf_slot(isp, stream)) {
			unsigned int rreg;
			unsigned int wreg;
			unsigned int len;
			unsigned int rd_ptr, wr_ptr;

			if (sleep_count < MAX_SLEEP_COUNT) {
				msleep(MAX_SLEEP_TIME);
				sleep_count++;
				continue;
			}
			isp_get_cmd_buf_regs(stream, &rreg, &wreg, NULL, NULL,
					     NULL);
			isp_fw_buf_get_cmd_base(isp->fw_work_buf_hdl, stream,
						NULL, NULL, &len);
			rd_ptr = isp_hw_reg_read32(rreg);
			wr_ptr = isp_hw_reg_read32(wreg);
			ISP_PR_ERR("fail no cmdslot cid:%d,stream %s(%d)",
				cmd_id, isp_dbg_get_stream_str(stream),
				stream);
			ISP_PR_ERR("rreg %u,wreg %u,len %u\n",
				rd_ptr, wr_ptr, len);
			goto busy_out;
		}
		break;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmdId = cmd_id;
	switch (stream) {
	case FW_CMD_RESP_STREAM_ID_1:
		cmd.cmdStreamId = STREAM_ID_1;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		cmd.cmdStreamId = STREAM_ID_2;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		cmd.cmdStreamId = STREAM_ID_3;
		break;
	default:
		cmd.cmdStreamId = (unsigned short) STREAM_ID_INVALID;
		break;
	}

	switch (cmd_id) {
	case CMD_ID_SET_CVIP_BUF_LAYOUT:
		msleep(300);
		break;
	}

	if (directcmd) {
		if (package_size)
			memcpy(cmd.cmdParam, package, package_size);
	} else if (package_size <= isp_get_cmd_pl_size()) {
		if (RET_SUCCESS !=
			isp_fw_get_nxt_cmd_pl(isp->fw_work_buf_hdl, &pack_sys,
				&package_base, &pack_len)) {
			ISP_PR_ERR("no enough pkg buf(0x%08x)\n", cmd_id);
			goto failure_out;
		};

		memcpy((void *)pack_sys,
			(unsigned char *)package, package_size);

		pkg = (struct _CmdParamPackage_t *) cmd.cmdParam;
		isp_split_addr64(package_base, &pkg->packageAddrLo,
				 &pkg->packageAddrHi);
		pkg->packageSize = package_size;
		pkg->packageCheckSum =
			compute_check_sum((unsigned char *) package,
							 package_size);
	} else {

		gpu_mem =
			isp_gpu_mem_alloc(package_size, ISP_GPU_MEM_TYPE_NLFB);

		if (gpu_mem == NULL) {
			ISP_PR_ERR("no gpu mem(%u), cid %d\n",
				package_size, cmd_id);
			goto failure_out;
		}
		memcpy(gpu_mem->sys_addr,
			(unsigned char *) package, package_size);
		pkg = (struct _CmdParamPackage_t *) cmd.cmdParam;
		isp_split_addr64(gpu_mem->gpu_mc_addr, &pkg->packageAddrLo,
				 &pkg->packageAddrHi);
		pkg->packageSize = package_size;
		pkg->packageCheckSum = compute_check_sum(
			(unsigned char *) package, package_size);
	}

	seq_num = get_nxt_cmd_seq_num(isp);
	cmd.cmdSeqNum = seq_num;
	cmd.cmdCheckSum =
		compute_check_sum((unsigned char *) &cmd, sizeof(cmd) - 1);

	if (seq)
		*seq = seq_num;
	command_element.seq_num = seq_num;
	command_element.cmd_id = cmd_id;
	command_element.mc_addr = package_base;
	command_element.evt = evt;
	command_element.gpu_pkg = gpu_mem;
	command_element.resp_payload = resp_pl;
	command_element.resp_payload_len = resp_pl_len;
	command_element.stream = stream;
	command_element.I2CRegAddr = (unsigned short) I2C_REGADDR_NULL;
	command_element.cam_id = cam_id;

	if (cmd_id == CMD_ID_SEND_I2C_MSG) {
		struct _CmdSendI2cMsg_t *i2c_msg;

		i2c_msg = (struct _CmdSendI2cMsg_t *) package;
		if (i2c_msg->msg.msgType == I2C_MSG_TYPE_READ)
			command_element.I2CRegAddr = i2c_msg->msg.regAddr;
	}

	cmd_ele = isp_append_cmd_2_cmdq(isp, &command_element);
	if (cmd_ele == NULL) {
		ISP_PR_ERR("fail for isp_append_cmd_2_cmdq\n");
		goto failure_out;
	}

	if (cmd_id == CMD_ID_SET_SENSOR_CALIBDB) {
		struct _CmdSetSensorCalibdb_t *p =
			(struct _CmdSetSensorCalibdb_t *) package;

		ISP_PR_DBG("%s(0x%08x:%s)%s,sn:%u,[%llx,%llx)(%u)\n",
			isp_dbg_get_cmd_str(cmd_id), cmd_id,
			isp_dbg_get_stream_str(stream),
			directcmd ? "direct" : "indirect", seq_num,
			isp_join_addr64(p->bufAddrLo, p->bufAddrHi),
			isp_join_addr64(p->bufAddrLo, p->bufAddrHi) +
			p->bufSize, p->bufSize);
	} else if (cmd_id == CMD_ID_SEND_BUFFER) {
		struct _CmdSendBuffer_t *p =
			(struct _CmdSendBuffer_t *) package;
		unsigned int total =
			p->buffer.bufSizeA + p->buffer.bufSizeB +
			p->buffer.bufSizeC;
		unsigned long long y =
		isp_join_addr64(p->buffer.bufBaseALo,
			p->buffer.bufBaseAHi);
		unsigned long long u =
			isp_join_addr64(p->buffer.bufBaseBLo,
				p->buffer.bufBaseBHi);
		unsigned long long v =
			isp_join_addr64(p->buffer.bufBaseCLo,
				p->buffer.bufBaseCHi);
		ISP_PR_DBG("%s(0x%08x:%s)%s,sn:%u,%s,[%llx,%llx,%llx](%u)\n",
			isp_dbg_get_cmd_str(cmd_id), cmd_id,
			isp_dbg_get_stream_str(stream),
			directcmd ? "direct" : "indirect", seq_num,
			isp_dbg_get_buf_type(p->bufferType),
			y, u, v,
			//y + total,
			total);
	} else if (cmd_id == CMD_ID_SEND_FRAME_INFO) {
		struct _FrameInfo_t *p =
			(struct _FrameInfo_t *)package;

		unsigned int total = p->rawBufferFrameInfo.buffer.bufSizeA
			+ p->rawBufferFrameInfo.buffer.bufSizeB
			+ p->rawBufferFrameInfo.buffer.bufSizeC;
		unsigned long long y =
		isp_join_addr64(p->rawBufferFrameInfo.buffer.bufBaseALo,
			p->rawBufferFrameInfo.buffer.bufBaseAHi);

		ISP_PR_DBG("%s(0x%08x:%s)%s,sn:%u,[%llx,%llx)(%u)\n",
			isp_dbg_get_cmd_str(cmd_id), cmd_id,
			isp_dbg_get_stream_str(stream),
			directcmd ? "direct" : "indirect", seq_num, y,
			y + total, total);
	} else {

		ISP_PR_DBG("%s(0x%08x:%s)%s,sn:%u\n",
			isp_dbg_get_cmd_str(cmd_id), cmd_id,
			isp_dbg_get_stream_str(stream),
			directcmd ? "direct" : "indirect", seq_num);
	}

	isp_get_cur_time_tick(&cmd_ele->send_time);
	result = insert_isp_fw_cmd(isp, stream, &cmd);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail for insert_isp_fw_cmd cid %u\n", cmd_id);
		isp_rm_cmd_from_cmdq(isp, cmd_ele->seq_num,
			cmd_ele->cmd_id, false);
		goto failure_out;
	}

	isp_mutex_unlock(&isp->command_mutex);
	return result;

 busy_out:
	isp_mutex_unlock(&isp->command_mutex);
	return RET_TIMEOUT;

failure_out:
	if (package_base)
		isp_fw_ret_pl(isp->fw_work_buf_hdl, package_base);

	if (gpu_mem)
		isp_gpu_mem_free(gpu_mem);

	if (cmd_ele)
		isp_sys_mem_free(cmd_ele);
	isp_mutex_unlock(&isp->command_mutex);
	return RET_FAILURE;
};

/*
 * This function acks just like `isp_send_fw_cmd_ex` but accepts a void*
 * argument |dma_buf| to represent an allocated DMA buffer. Caller has
 * responsibility to manage the lifecycle of the given DMA buffer handle.
 *
 * Notice, currently, argument |dma_buf| should be the pointer of
 * `struct isp_gpu_mem_ifno`.
 */
int isp_send_fw_cmd_by_dma_buf(struct isp_context *isp,
		enum camera_id cam_id,
		unsigned int cmd_id, enum fw_cmd_resp_stream_id stream,
		enum fw_cmd_para_type directcmd, void *dma_buf,
		unsigned int dma_buf_size, struct isp_event *evt,
		unsigned int *seq, void *resp_pl, unsigned int *resp_pl_len)
{
	void *package = NULL;  /* address which the current CPU can access */
	unsigned int package_size = 0;
	int result;
	unsigned int seq_num;
	struct isp_cmd_element command_element;
	struct isp_cmd_element *cmd_ele = NULL;
	int os_status;
	unsigned int sleep_count;
	struct _Cmd_t cmd;
	struct isp_gpu_mem_info *gpu_mem = NULL;

	if (directcmd && (dma_buf_size > sizeof(cmd.cmdParam))) {
		ISP_PR_ERR("%s: fail pkgsize(%u) cid:%d,stream %d\n",
		__func__, dma_buf_size, cmd_id, stream);
		return RET_FAILURE;
	}

	/* check if it's valid arguments */
	if (dma_buf_size == 0 || dma_buf == NULL) {
		ISP_PR_ERR("%s: fail null pkg cid:%d,stream %d\n",
			__func__, cmd_id, stream);
		return RET_FAILURE;
	};

	os_status = isp_mutex_lock(&isp->command_mutex);
	sleep_count = 0;
	while (1) {
		if (no_fw_cmd_ringbuf_slot(isp, stream)) {
			unsigned int rreg;
			unsigned int wreg;
			unsigned int len;
			unsigned int rd_ptr, wr_ptr;

			if (sleep_count < MAX_SLEEP_COUNT) {
				msleep(MAX_SLEEP_TIME);
				sleep_count++;
				continue;
			}
			isp_get_cmd_buf_regs(stream, &rreg, &wreg, NULL, NULL,
					     NULL);
			isp_fw_buf_get_cmd_base(isp->fw_work_buf_hdl, stream,
						NULL, NULL, &len);
			rd_ptr = isp_hw_reg_read32(rreg);
			wr_ptr = isp_hw_reg_read32(wreg);
			ISP_PR_ERR
	("fail no cmdslot cid:%d,stream %s(%d),rreg %u,wreg %u,len %u\n",
				cmd_id, isp_dbg_get_stream_str(stream), stream,
				rd_ptr, wr_ptr, len);
			goto busy_out;
		}
		break;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmdId = cmd_id;
	switch (stream) {
	case FW_CMD_RESP_STREAM_ID_1:
		cmd.cmdStreamId = STREAM_ID_1;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		cmd.cmdStreamId = STREAM_ID_2;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		cmd.cmdStreamId = STREAM_ID_3;
		break;
	default:
		cmd.cmdStreamId = (unsigned short) STREAM_ID_INVALID;
		break;
	}

	/* argument |dma_buf| should be type of `struct isp_gpu_mem_info *` */
	gpu_mem = (struct isp_gpu_mem_info *)dma_buf;

	/* assign CPU accessible address to variable |package| */
	package = (void *)gpu_mem->sys_addr;
	package_size = dma_buf_size;

	if (directcmd) {
		/*
		 * if it's direct command, no need to pass the GPU memory to
		 * ISP FW, but only using the ring buffer to send data
		 */
		memcpy(cmd.cmdParam, package, package_size);
	} else {
		struct _CmdParamPackage_t *pkg;

		/* compose cmdParam */
		pkg = (struct _CmdParamPackage_t *)cmd.cmdParam;
		isp_split_addr64(gpu_mem->gpu_mc_addr, &pkg->packageAddrLo,
				&pkg->packageAddrHi);
		pkg->packageSize = package_size;
		pkg->packageCheckSum = compute_check_sum(
				(unsigned char *) package,
				package_size);
	}

	seq_num = get_nxt_cmd_seq_num(isp);
	cmd.cmdSeqNum = seq_num;
	cmd.cmdCheckSum = compute_check_sum(
		(unsigned char *) &cmd, sizeof(cmd) - 1);

	if (seq)
		*seq = seq_num;

	command_element.seq_num = seq_num;
	command_element.cmd_id = cmd_id;
	command_element.mc_addr = 0; /* no need to clear while cmd done */
	command_element.evt = evt;
	command_element.gpu_pkg = 0; /* no need to clear while cmd done */
	command_element.resp_payload = resp_pl;
	command_element.resp_payload_len = resp_pl_len;
	command_element.stream = stream;
	command_element.I2CRegAddr = (unsigned short)I2C_REGADDR_NULL;
	command_element.cam_id = cam_id;

	cmd_ele = isp_append_cmd_2_cmdq(isp, &command_element);
	if (cmd_ele == NULL) {
		ISP_PR_ERR("-%s: fail for isp_append_cmd_2_cmdq\n", __func__);
		goto failure_out;
	}

	ISP_PR_INFO("%s: %s(0x%08x:%s)%s,sn:%u\n",
		__func__,
		isp_dbg_get_cmd_str(cmd_id), cmd_id,
		isp_dbg_get_stream_str(stream),
		directcmd ? "direct" : "indirect", seq_num);


	isp_get_cur_time_tick(&cmd_ele->send_time);
	result = insert_isp_fw_cmd(isp, stream, &cmd);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("%s: fail for insert_isp_fw_cmd cid %u\n",
			__func__, cmd_id);

		isp_rm_cmd_from_cmdq(isp, cmd_ele->seq_num,
			cmd_ele->cmd_id, false);
		goto failure_out;
	}

	isp_mutex_unlock(&isp->command_mutex);
	return result;

busy_out:
	isp_mutex_unlock(&isp->command_mutex);
	return RET_TIMEOUT;

failure_out:
	if (cmd_ele)
		isp_sys_mem_free(cmd_ele);
	isp_mutex_unlock(&isp->command_mutex);
	return RET_FAILURE;
}

int fw_if_set_brightness(struct isp_context *isp,
			unsigned int camera_id,
			int brightness)
{
	int ret;
	struct _CmdCprocSetBrightness_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.brightness = (unsigned char) brightness;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_CPROC_SET_BRIGHTNESS, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
	ISP_PR_DBG("set brightness to %d ret %d\n", brightness, ret);
	return ret;
}

int fw_if_set_contrast(struct isp_context *isp, unsigned int camera_id,
				int contrast)
{
	int ret;
	struct _CmdCprocSetContrast_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.contrast = (unsigned char) contrast;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_CPROC_SET_CONTRAST, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
	ISP_PR_DBG("set contrast to %u ret %d\n", contrast, ret);
	return ret;
}

int fw_if_set_hue(struct isp_context *isp, unsigned int camera_id,
		int hue)
{
	int ret;
	struct _CmdCprocSetHue_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.hue = (unsigned char) hue;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_CPROC_SET_HUE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
	ISP_PR_DBG("set hue %d ret %d\n", hue, ret);
	return ret;
};

int fw_if_set_satuation(struct isp_context *isp, unsigned int camera_id,
			int stauation)
{
	int ret;
	struct _CmdCprocSetSaturation_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.saturation = (unsigned char) stauation;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_CPROC_SET_SATURATION, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
	ISP_PR_DBG("set saturation to %d ret %d\n", stauation, ret);
	return ret;
}

int fw_if_set_color_enable(struct isp_context *isp,
			unsigned int camera_id, int color_enable)
{
	enum fw_cmd_resp_stream_id stream_id;
	unsigned int cmd_id = CMD_ID_CPROC_DISABLE;

	if (color_enable)
		cmd_id = CMD_ID_CPROC_ENABLE;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	return isp_send_fw_cmd(isp, cmd_id, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
}

int fw_if_set_flicker_mode(struct isp_context *isp,
			unsigned int camera_id,
			enum _AeFlickerType_t mode)
{
	struct _CmdAeSetFlicker_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.flickerType = mode;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	return isp_send_fw_cmd(isp, CMD_ID_AE_SET_FLICKER, stream_id,
		FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
};

int isp_fw_set_iso(struct isp_context *isp,
		unsigned int camera_id, unsigned int iso)
{

	struct _CmdAeSetIsoPriority_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.iso = iso;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	return isp_send_fw_cmd(isp, CMD_ID_AE_SET_ISO_PRIORITY, stream_id,
		FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));

	return 0;
}

int isp_fw_set_ae_flicker(struct isp_context *isp,
				 enum camera_id cam_id,
				 enum _AeFlickerType_t flickerType)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdAeSetFlicker_t flick;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	flick.flickerType = flickerType;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_FLICKER, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&flick, sizeof(flick)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set ae flicker(%u)\n", flickerType);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set ae flicker(%u) suc\n", flickerType);
		ret = RET_SUCCESS;
	};
	return ret;
}

int fw_if_set_ev_compensation(struct isp_context *isp,
			unsigned int camera_id, struct _AeEv_t ev)
{
	struct _CmdAeSetEvCompensation_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.ev = ev;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	return isp_send_fw_cmd(isp, CMD_ID_AE_SET_EV_COMPENSATION, stream_id,
		FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));

	return 0;
};

/*todo*/
int fw_if_set_scene_mode(struct isp_context __maybe_unused *isp,
	unsigned int __maybe_unused camera_id,
	enum pvt_scene_mode __maybe_unused mode)
{
	int result = RET_SUCCESS;

	return result;
};

#define hal_read_img_mem(PHY, VIRT, LEN) isp_hw_mem_read_(PHY, VIRT, LEN)

int isp_get_f2h_resp(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream,
			struct _Resp_t *response)
{
	unsigned int rd_ptr;
	unsigned int wr_ptr;
	unsigned int rd_ptr_dbg;
	unsigned int wr_ptr_dbg;
	unsigned long long mem_addr;
	unsigned int rreg;
	unsigned int wreg;
	unsigned int checksum;
	unsigned int len;
	void **mem_sys;

	isp_get_resp_buf_regs(stream, &rreg, &wreg, NULL, NULL, NULL);
	isp_fw_buf_get_resp_base(isp->fw_work_buf_hdl, stream,
				 (unsigned long long *)&mem_sys,
				 &mem_addr, &len);

	rd_ptr = isp_hw_reg_read32(rreg);
	wr_ptr = isp_hw_reg_read32(wreg);
	rd_ptr_dbg = rd_ptr;
	wr_ptr_dbg = wr_ptr;

	if (rd_ptr > len) {
		ISP_PR_ERR("fail %s(%u),rd_ptr %u(should<=%u),wr_ptr %u\n",
			isp_dbg_get_stream_str(stream),
			stream, rd_ptr, len, wr_ptr);
		return RET_FAILURE;
	};

	if (wr_ptr > len) {
		ISP_PR_ERR("fail %s(%u),wr_ptr %u(should<=%u), rd_ptr %u\n",
			isp_dbg_get_stream_str(stream),
			stream, wr_ptr, len, rd_ptr);
		return RET_FAILURE;
	}

	/* clear all pending read/write */
	mb();

	if (rd_ptr < wr_ptr) {
		if ((wr_ptr - rd_ptr) >= (sizeof(struct _Resp_t))) {
			memcpy_fromdev((unsigned char *)response,
				       (unsigned char *)mem_sys + rd_ptr,
				       sizeof(struct _Resp_t));

			rd_ptr += sizeof(struct _Resp_t);
			if (rd_ptr < len) {
				isp_hw_reg_write32(rreg, rd_ptr);
			} else {
				ISP_PR_ERR("%s(%u),rd %u(should<=%u),wr %u\n",
				isp_dbg_get_stream_str(stream),
				stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		} else {
			ISP_PR_ERR("sth wrong with wptr and rptr\n");
			return RET_FAILURE;
		}
	} else if (rd_ptr > wr_ptr) {
		unsigned int size;
		unsigned char *dst;
		unsigned long long src_addr;

		dst = (unsigned char *)response;

		src_addr = mem_addr + rd_ptr;
		size = len - rd_ptr;
		if (size > sizeof(struct _Resp_t)) {
			mem_addr += rd_ptr;
			memcpy_fromdev((unsigned char *)response,
				       (unsigned char *)(mem_sys) + rd_ptr,
				       sizeof(struct _Resp_t));
			rd_ptr += sizeof(struct _Resp_t);
			if (rd_ptr < len)
				isp_hw_reg_write32(rreg, rd_ptr);
			else {
				ISP_PR_ERR("%s(%u),rd %u(should<=%u),wr %u\n",
					isp_dbg_get_stream_str(stream),
					stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		} else {
			if ((size + wr_ptr) < (sizeof(struct _Resp_t))) {
				ISP_PR_ERR("sth wrong with wptr and rptr1\n");
				return RET_FAILURE;
			}

			memcpy_fromdev(dst, (unsigned char *)(mem_sys) + rd_ptr, size);

			dst += size;
			src_addr = mem_addr;
			size = sizeof(struct _Resp_t) - size;
			if (size)
				memcpy_fromdev(dst, (unsigned char *)(mem_sys), size);
			rd_ptr = size;
			if (rd_ptr < len) {
				isp_hw_reg_write32(rreg, rd_ptr);
			} else {
				ISP_PR_ERR("%s(%u),rd %u(should<=%u),wr %u\n",
					isp_dbg_get_stream_str(stream),
					stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		}
	} else/* if (rd_ptr == wr_ptr)*/
		return RET_TIMEOUT;

out:
	/* make sure WC buffer are flushed */
	mb();

	checksum = compute_check_sum((unsigned char *) response,
		(sizeof(struct _Resp_t) - 1));
	if (checksum != response->respCheckSum) {
		ISP_PR_ERR("resp checksum[0x%x],should 0x%x,rdptr %u,wrptr %u",
		checksum, response->respCheckSum, rd_ptr_dbg, wr_ptr_dbg);
		ISP_PR_ERR("%s(%u), seqNo %u, respId %s(0x%x)\n",
			isp_dbg_get_stream_str(stream), stream,
			response->respSeqNum,
			isp_dbg_get_resp_str(response->respId),
			response->respId);
		return RET_FAILURE;
	}

	return RET_SUCCESS;
};

void isp_get_cmd_buf_regs(enum fw_cmd_resp_stream_id idx,
			unsigned int *rreg, unsigned int *wreg,
			unsigned int *baselo_reg, unsigned int *basehi_reg,
			unsigned int *size_reg)
{
	switch (idx) {
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		if (rreg)
			*rreg = mmISP_RB_RPTR1;
		if (wreg)
			*wreg = mmISP_RB_WPTR1;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO1;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI1;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE1;
		break;
	case FW_CMD_RESP_STREAM_ID_1:
		if (rreg)
			*rreg = mmISP_RB_RPTR2;
		if (wreg)
			*wreg = mmISP_RB_WPTR2;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO2;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI2;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE2;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		if (rreg)
			*rreg = mmISP_RB_RPTR3;
		if (wreg)
			*wreg = mmISP_RB_WPTR3;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO3;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI3;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE3;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		if (rreg)
			*rreg = mmISP_RB_RPTR4;
		if (wreg)
			*wreg = mmISP_RB_WPTR4;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO4;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI4;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE4;
		break;
	default:
		ISP_PR_ERR("fail id[%d]\n", idx);
		break;
	};
};

void isp_get_resp_buf_regs(enum fw_cmd_resp_stream_id idx,
			unsigned int *rreg, unsigned int *wreg,
			unsigned int *baselo_reg, unsigned int *basehi_reg,
			unsigned int *size_reg)
{
	switch (idx) {
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		if (rreg)
			*rreg = mmISP_RB_RPTR9;
		if (wreg)
			*wreg = mmISP_RB_WPTR9;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO9;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI9;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE9;
		break;
	case FW_CMD_RESP_STREAM_ID_1:
		if (rreg)
			*rreg = mmISP_RB_RPTR10;
		if (wreg)
			*wreg = mmISP_RB_WPTR10;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO10;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI10;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE10;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		if (rreg)
			*rreg = mmISP_RB_RPTR11;
		if (wreg)
			*wreg = mmISP_RB_WPTR11;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO11;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI11;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE11;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		if (rreg)
			*rreg = mmISP_RB_RPTR12;
		if (wreg)
			*wreg = mmISP_RB_WPTR12;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO12;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI12;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE12;
		break;
	default:
		if (rreg)
			*rreg = 0;
		if (wreg)
			*wreg = 0;
		if (baselo_reg)
			*baselo_reg = 0;
		if (basehi_reg)
			*basehi_reg = 0;
		if (size_reg)
			*size_reg = 0;
		ISP_PR_ERR("fail idx (%u)\n", idx);
		break;
	};
}

void isp_init_fw_ring_buf(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id idx, unsigned int cmd)
{
	unsigned int rreg;
	unsigned int wreg;
	unsigned int baselo_reg;
	unsigned int basehi_reg;
	unsigned int size_reg;
	unsigned long long mc;
	unsigned int lo;
	unsigned int hi;
	unsigned int len;
	unsigned int pgfsm_value;

	if (cmd) {		/*command buffer */
		if ((isp == NULL) || (idx > FW_CMD_RESP_STREAM_ID_3)) {
			ISP_PR_ERR("(%u:cmd) fail,bad para\n", idx);
			return;
		};
		isp_get_cmd_buf_regs(idx, &rreg, &wreg,
			&baselo_reg, &basehi_reg, &size_reg);
		isp_fw_buf_get_cmd_base(isp->fw_work_buf_hdl, idx,
			NULL, &mc, &len);
	} else {		/*response buffer */
		if ((isp == NULL) || (idx > FW_CMD_RESP_STREAM_ID_3)) {
			ISP_PR_ERR("(%u:resp) fail,bad para\n", idx);
			return;
		};
		isp_get_resp_buf_regs(idx, &rreg, &wreg,
			&baselo_reg, &basehi_reg, &size_reg);
		isp_fw_buf_get_resp_base(isp->fw_work_buf_hdl, idx,
			NULL, &mc, &len);
	};
	isp_split_addr64(mc, &lo, &hi);

	isp_hw_reg_write32(rreg, 0);
	pgfsm_value = isp_hw_reg_read32(rreg);

	isp_hw_reg_write32(wreg, 0);
	pgfsm_value = isp_hw_reg_read32(wreg);


	isp_hw_reg_write32(baselo_reg, lo);
	pgfsm_value = isp_hw_reg_read32(baselo_reg);

	isp_hw_reg_write32(basehi_reg, hi);
	pgfsm_value = isp_hw_reg_read32(basehi_reg);

	isp_hw_reg_write32(size_reg, len);
	pgfsm_value = isp_hw_reg_read32(size_reg);
}

void isp_tmr0_gpuva_wa(void)
{
	ISP_PR_ERR("%s\n", __func__);
	isp_hw_reg_write32(mmISP_CCPU_ERR_DETECT_EN, 0x00000000);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_MIPI0, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_MIPI1, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_MIPI2, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD0, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD0_ALT, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD1, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD1_ALT, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD2, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD2_ALT, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR0, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR0_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR1, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR1_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR2, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR2_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR3, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR3_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_SI, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_SI_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_TNR, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_TNR_ALT, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_TNR, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_TNR_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CCPU_CACHE_SET0, 0x0E400004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CCPU_CACHE_SET1, 0x0E000004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CCPU_CACHE_SET2, 0x0DC00004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CCPU_CACHE_SET3, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CCPU_CACHE_SET4, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_CCPU_NONCACHE_SET0, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_CCPU_NONCACHE_SET1, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_CCPU_NONCACHE_SET2, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_CCPU_NONCACHE_SET3, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CCPU_CACHE_SET0, 0x03900004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CCPU_CACHE_SET1, 0x03800004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CCPU_CACHE_SET2, 0x03700004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CCPU_CACHE_SET3, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CCPU_CACHE_SET4, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_CCPU_NONCACHE_SET0, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_CCPU_NONCACHE_SET1, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_CCPU_NONCACHE_SET2, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_CCPU_NONCACHE_SET3, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_IDMA_CACHE_SET0, 0x0DC00004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_IDMA_CACHE_SET1, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_IDMA_CACHE_SET2, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_IDMA_CACHE_SET3, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_IDMA_NONCACHE_SET0, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_IDMA_NONCACHE_SET1, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_IDMA_NONCACHE_SET2, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_ARUSER_IDMA_NONCACHE_SET3, 0x000E0004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_IDMA_CACHE_SET0, 0x03700004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_IDMA_CACHE_SET1, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_IDMA_CACHE_SET2, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_IDMA_CACHE_SET3, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_IDMA_NONCACHE_SET0, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_IDMA_NONCACHE_SET1, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_IDMA_NONCACHE_SET2, 0x00038004);
	isp_hw_reg_write32(mmISP_MI_AWUSER_IDMA_NONCACHE_SET3, 0x00038004);
}

void isp_isppg_wa_for_none_cvip_sensor(void)
{
	ENTER();
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_MIPI0, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_MIPI1, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_MIPI2, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD0, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD0_ALT, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD1, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD1_ALT, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD2, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_CRD2_ALT, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR0, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR0_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR1, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR1_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR2, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR2_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR3, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_CWR3_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_SI, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_SI_ALT, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_TNR, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_ARUSER_TNR_ALT, 0x000E0004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_TNR, 0x00038004);
	isp_hw_reg_write32(mmISP_PSP_MI_AWUSER_TNR_ALT, 0x00038004);
}

void print_scratch(void)
{
	unsigned int r[16];
	unsigned int pc;
	unsigned int data;
	unsigned int sts;
	unsigned int lsu;
	unsigned int lsl;
	unsigned int lsad;

	pc = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_PC);
	ISP_PR_ERR("PC:0x%x\n", pc);
	data = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_DATA);
	sts = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_STATUS);
	ISP_PR_ERR("DATA:0x%x,STATUS:0x%x\n", data, sts);
	lsu = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_LS0_32UPPERBITS);
	lsl = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_LS0_32LOWERBITS);
	lsad = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_LS0_ADDR);
	ISP_PR_ERR("LS0 UP/LOW/ADDR:0x%x 0x%x 0x%x\n", lsu, lsl, lsad);
	pc = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_PC);
	ISP_PR_ERR("PC:0x%x\n", pc);
	r[0] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH0);
	r[1] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH1);
	r[2] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH2);
	r[3] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH3);
	r[4] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH4);
	r[5] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH5);
	r[6] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH6);
	r[7] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH7);
	r[8] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH8);
	r[9] = isp_hw_reg_read32(mmISP_CCPU_SCRATCH9);
	r[10] = isp_hw_reg_read32(mmISP_CCPU_SCRATCHA);
	r[11] = isp_hw_reg_read32(mmISP_CCPU_SCRATCHB);
	r[12] = isp_hw_reg_read32(mmISP_CCPU_SCRATCHC);
	r[13] = isp_hw_reg_read32(mmISP_CCPU_SCRATCHD);
	r[14] = isp_hw_reg_read32(mmISP_CCPU_SCRATCHE);
	r[15] = isp_hw_reg_read32(mmISP_CCPU_SCRATCHF);

	ISP_PR_ERR("SCRATCH:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
		   r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	ISP_PR_ERR("SCRATCH:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,\n",
		   r[8], r[9], r[10], r[11], r[12], r[13], r[14], r[15]);
	pc = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_PC);
	ISP_PR_ERR("PC:0x%x\n", pc);
}

/*refer to aidt_api_boot_firmware*/
int isp_fw_boot(struct isp_context *isp)
{
	unsigned long long fw_base_addr;
	unsigned long long fw_base_sys_addr;
	unsigned long long stack_base_addr_sys;
	unsigned long long stack_base_addr;
	unsigned long long heap_base_addr;
	unsigned long long fw_rb_log_base;
	unsigned int fw_trace_buf_size;
	/*unsigned int max_package_buffer_num; */
	/*unsigned int max_package_buffer_size; */
	unsigned int fw_buf_size;
	unsigned int stack_size;
	unsigned int heap_size;
	unsigned int reg_val;

	unsigned int fw_rb_log_base_lo;
	unsigned int fw_rb_log_base_hi;
	unsigned int i;

	int result = RET_SUCCESS;
	unsigned char copy_fw_needed = 1;

	long long start_time;
	long long end_time;

	//unsigned long long orig_codebase_sys = 0;
	//unsigned long long orig_codebase_mc = 0;
	//unsigned int orig_codebase_len = 0;

	if (ISP_GET_STATUS(isp) != ISP_STATUS_PWR_ON) {
		ISP_PR_ERR("fail bad status %d\n", ISP_GET_STATUS(isp));
		return RET_FAILURE;
	}

	isp_hw_ccpu_disable(isp);

	isp_hw_reg_write32(mmISP_SOFT_RESET, 0x0);	//Disable CCPU

	for (i = 0; i < ISP_FW_CMD_BUF_COUNT; i++)
		isp_fw_buf_get_cmd_base(isp->fw_work_buf_hdl, i,
					&isp->fw_cmd_buf_sys[i],
					&isp->fw_cmd_buf_mc[i],
					&isp->fw_cmd_buf_size[i]);
	for (i = 0; i < ISP_FW_RESP_BUF_COUNT; i++)
		isp_fw_buf_get_resp_base(isp->fw_work_buf_hdl, i,
					 &isp->fw_resp_buf_sys[i],
					 &isp->fw_resp_buf_mc[i],
					 &isp->fw_resp_buf_size[i]);

	isp_fw_buf_get_trace_base(isp->fw_work_buf_hdl, &isp->fw_log_sys,
				&isp->fw_log_mc, &isp->fw_log_size);
	fw_rb_log_base = isp->fw_log_mc;
	fw_trace_buf_size = isp->fw_log_size;
	fw_rb_log_base_lo = (unsigned int) (fw_rb_log_base & 0xffffffff);
	fw_rb_log_base_hi = (unsigned int) (fw_rb_log_base >> 32);

	for (i = 0; i < ISP_FW_CMD_BUF_COUNT; i++)
		isp_init_fw_ring_buf(isp, i, 1);
	for (i = 0; i < ISP_FW_RESP_BUF_COUNT; i++)
		isp_init_fw_ring_buf(isp, i, 0);
	/*
	 *isp_hw_ccpu_disable(isp);
	 */
	isp_init_fw_rb_log_buffer(isp, fw_rb_log_base_lo,
				fw_rb_log_base_hi, fw_trace_buf_size);

	isp_fw_buf_get_code_base(isp->fw_work_buf_hdl, &fw_base_sys_addr,
				 &fw_base_addr, &fw_buf_size);

	isp_fw_buf_get_stack_base(isp->fw_work_buf_hdl, &stack_base_addr_sys,
				&stack_base_addr, &stack_size);
	isp_fw_buf_get_heap_base(isp->fw_work_buf_hdl, NULL,
				&heap_base_addr, &heap_size);

	/*configure TEXT BASE ADDRESS */
#ifndef USING_PSP_TO_LOAD_ISP_FW
	if (copy_fw_needed && isp->fw_data)
		memcpy((void *)fw_base_sys_addr, isp->fw_data, isp->fw_len);
#endif

	if (1 && copy_fw_needed) {
		//unsigned int *ch = (unsigned int *)fw_base_sys_addr;

		ISP_PR_DBG("fw len %u,aft memcpy from %p to %llx, mc %llx\n",
			isp->fw_len, isp->fw_data,
			fw_base_sys_addr, fw_base_addr);
	}
	isp_hw_reg_write32(mmISP_CCPU_ERR_DETECT_EN, 0);
#ifdef USING_PSP_TO_LOAD_ISP_FW
//psp loading FW
	{
		unsigned long l_h;
		unsigned long l_l;
		unsigned long l_s;
		unsigned long long l_addr;

		l_l = isp_hw_reg_read32(mmISP_CCPU_CACHE_OFFSET0_LO);
		l_h = isp_hw_reg_read32(mmISP_CCPU_CACHE_OFFSET0_HI);
		l_s = isp_hw_reg_read32(mmISP_CCPU_CACHE_SIZE0);
		l_addr = isp_join_addr64(l_l, l_h);
		ISP_PR_PC("isp code 0x%llx, %u\n", l_addr, (unsigned int)l_s);
		if (l_addr == 0) {
			l_h = 0xf4;
			l_l = 0x1ec00000;
			l_addr = 0xf41ec00000;
			l_s = 2097152;
			ISP_PR_ERR("isp changecode to 0x%llx, %u\n",
				l_addr, (unsigned int)l_s);
			// Configure TEXT BASE ADDRESS
			isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET0_LO,
					(l_addr & 0xffffffff));
			isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET0_HI,
					((l_addr >> 32) & 0xffffffff));
			isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE0, l_s);
		}
		if (l_h == 0xf4)
			isp_tmr0_gpuva_wa();
		else if (g_prop->cvip_enable == CVIP_STATUS_DISABLE)
			isp_isppg_wa_for_none_cvip_sensor();
		// Configure TEXT BASE ADDRESS for IDMA,
		// some test code may need it
		isp_hw_reg_write32(mmISP_IDMA_CACHE_OFFSET0_LO,
			(l_addr & 0xffffffff));
		isp_hw_reg_write32(mmISP_IDMA_CACHE_OFFSET0_HI,
			((l_addr >> 32) & 0xffffffff));
		isp_hw_reg_write32(mmISP_IDMA_CACHE_SIZE0, l_s);

		/*configure STACK BASE ADDRESS */
		l_l = isp_hw_reg_read32(mmISP_CCPU_CACHE_OFFSET1_LO);
		l_h = isp_hw_reg_read32(mmISP_CCPU_CACHE_OFFSET1_HI);
		l_s = isp_hw_reg_read32(mmISP_CCPU_CACHE_SIZE1);
		l_addr = isp_join_addr64(l_l, l_h);
		ISP_PR_PC("isp stack 0x%llx, %u\n", l_addr, (unsigned int)l_s);
		if (l_addr == 0) {
			l_addr = 0xf41ee00000;
			l_s = 524288;
			ISP_PR_ERR("isp changestack to 0x%llx, %u\n",
					l_addr, (unsigned int)l_s);
			/*configure STACK BASE ADDRESS */
			isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET1_LO,
					(l_addr & 0xffffffff));
			isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET1_HI,
					((l_addr >> 32) & 0xffffffff));
			isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE1, l_s);
		}

		/*configure HEAP BASE ADDRESS */
		l_l = isp_hw_reg_read32(mmISP_CCPU_CACHE_OFFSET2_LO);
		l_h = isp_hw_reg_read32(mmISP_CCPU_CACHE_OFFSET2_HI);
		l_s = isp_hw_reg_read32(mmISP_CCPU_CACHE_SIZE2);
		l_addr = isp_join_addr64(l_l, l_h);
		ISP_PR_PC("isp heap 0x%llx, %u\n", l_addr, (unsigned int)l_s);
		if (l_addr == 0) {
			l_addr = 0xf41ee80000;
			l_s = 5767168;
			ISP_PR_ERR("isp changeheap to 0x%llx, %u\n",
				l_addr, (unsigned int)l_s);
			/*configure HEAP BASE ADDRESS */
			isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET2_LO,
					(l_addr & 0xffffffff));
			isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET2_HI,
					((l_addr >> 32) & 0xffffffff));
			isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE2,
					l_s/* + fw_trace_buf_size */);
		}

		// Configure HEAP BASE ADDRESS for IDMA, swReg is in heap
		isp_hw_reg_write32(mmISP_IDMA_CACHE_OFFSET2_HI,
			((l_addr >> 32)
			& 0xffffffff));
		isp_hw_reg_write32(mmISP_IDMA_CACHE_OFFSET2_LO,
			(l_addr & 0xffffffff));
		isp_hw_reg_write32(mmISP_IDMA_CACHE_SIZE2, l_s);

	}


#else
//driver load FW
	{
		unsigned long l_h;
		unsigned long l_l;
		unsigned long l_s;
		unsigned long long l_addr;

		l_l = isp_hw_reg_read32(mmISP_CCPU_CACHE_OFFSET0_LO);
		l_h = isp_hw_reg_read32(mmISP_CCPU_CACHE_OFFSET0_HI);
		l_s = isp_hw_reg_read32(mmISP_CCPU_CACHE_SIZE0);
		l_addr = isp_join_addr64(l_l, l_h);
		ISP_PR_ERR("isp origcode 0x%llx, %u\n",
			l_addr, (unsigned int)l_s);
	}
	isp_tmr0_gpuva_wa();
	// Configure TEXT BASE ADDRESS
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET0_LO,
			(fw_base_addr & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET0_HI,
			((fw_base_addr >> 32) & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE0, fw_buf_size);
	ISP_PR_ERR("isp code 0x%llx, %u\n", fw_base_addr,
		(unsigned int)fw_buf_size);
	// Configure TEXT BASE ADDRESS for IDMA, some test code may need it
	isp_hw_reg_write32(mmISP_IDMA_CACHE_OFFSET0_LO,
		(fw_base_addr & 0xffffffff));
	isp_hw_reg_write32(mmISP_IDMA_CACHE_OFFSET0_HI,
		((fw_base_addr >> 32) & 0xffffffff));
	isp_hw_reg_write32(mmISP_IDMA_CACHE_SIZE0, fw_buf_size);

	/*configure STACK BASE ADDRESS */
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET1_LO,
			(stack_base_addr & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET1_HI,
			((stack_base_addr >> 32) & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE1, stack_size);
	ISP_PR_ERR("isp stack 0x%llx, %u\n", stack_base_addr,
		(unsigned int)stack_size);

	/*configure HEAP BASE ADDRESS */
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET2_LO,
			(heap_base_addr & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET2_HI,
			((heap_base_addr >> 32) & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE2,
			heap_size /* + fw_trace_buf_size */);
	ISP_PR_ERR("isp heap 0x%llx, %u\n", heap_base_addr,
		(unsigned int)heap_size);

	// Configure HEAP BASE ADDRESS for IDMA, swReg is in heap
	isp_hw_reg_write32(mmISP_IDMA_CACHE_OFFSET2_HI,
		(((fw_base_addr + fw_buf_size + stack_size) >> 32)
		& 0xffffffff));
	isp_hw_reg_write32(mmISP_IDMA_CACHE_OFFSET2_LO,
		((fw_base_addr + fw_buf_size + stack_size) & 0xffffffff));
	isp_hw_reg_write32(mmISP_IDMA_CACHE_SIZE2, heap_size);
#endif

	/*Clear CCPU_STATUS */
	isp_hw_reg_write32(mmISP_STATUS, 0x0);
	i = isp_hw_reg_read32(mmISP_STATUS);

	isp_hw_ccpu_enable(isp);

	/*Wait Firmware Ready(); */
	isp_get_cur_time_tick(&start_time);
	while (1) {
#define CCPU_REPORT_SHIFT (1)
		reg_val = isp_hw_reg_read32(mmISP_STATUS);
		if (reg_val & (1 << CCPU_REPORT_SHIFT))
			break;

		isp_get_cur_time_tick(&end_time);
		if (isp_is_timeout(&start_time, &end_time, 10000)) {
			ISP_PR_ERR("timeout,reg 0x%x\n", reg_val);
			print_scratch();
			return RET_FAILURE;
		}
		usleep_range(1000, 2000);
	}
	//enable interrupt
	{
		unsigned int value = 0;
		//SYS_INT_RINGBUFFER_WPT9_EN
		value |= 1 << 16;
		//SYS_INT_RINGBUFFER_WPT10_EN
		value |= 1 << 18;
		//SYS_INT_RINGBUFFER_WPT11_EN
		value |= 1 << 20;
		//SYS_INT_RINGBUFFER_WPT12_EN
		value |= 1 << 22;
		isp_hw_reg_write32(mmISP_SYS_INT0_EN, value);
	}
	ISP_SET_STATUS(isp, ISP_STATUS_FW_RUNNING);
	ISP_PR_PC("ISP FW boot suc!");
	return result;
}

int isp_fw_log_module_set(struct isp_context *isp, enum _ModId_t id,
				bool enable)
{
	int ret;

	if (enable) {
		struct _CmdLogEnableMod_t mod;

		mod.mod = id;
		ret = isp_send_fw_cmd(isp, CMD_ID_LOG_ENABLE_MOD,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT, &mod,
				sizeof(struct _CmdLogEnableMod_t));
	} else {
		struct _CmdLogDisableMod_t mod;

		mod.mod = id;
		ret = isp_send_fw_cmd(isp, CMD_ID_LOG_DISABLE_MOD,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT, &mod,
				sizeof(struct _CmdLogDisableMod_t));
	}
	ISP_PR_DBG("mod:%d,en:%d,ret %d\n", id, enable, ret);
	return ret;
}

int isp_fw_log_level_set(struct isp_context *isp,
		enum _LogLevel_t level)
{
	struct _CmdLogSetLevel_t levl;
	int ret;

	levl.level = level;
	ret = isp_send_fw_cmd(isp, CMD_ID_LOG_SET_LEVEL,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_DIRECT, &levl,
			sizeof(struct _CmdLogSetLevel_t));
	ISP_PR_DBG("(level:%d),ret %d\n", level, ret);
	return ret;
}

int isp_fw_log_mod_ext_set(struct isp_context *isp,
				struct _CmdSetLogModExt_t *modext)
{
	int ret = RET_SUCCESS;

	ret = isp_send_fw_cmd(isp, CMD_ID_SET_LOG_MOD_EXT,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_DIRECT, modext,
			sizeof(struct _CmdSetLogModExt_t));
	ISP_PR_DBG("[0]:0x%x,[1]:0x%x,[2]:0x%x,[3]:0x%x,[4]:0x%x, ret %d\n",
		modext->level_bits[0], modext->level_bits[1],
		modext->level_bits[2], modext->level_bits[3],
		modext->level_bits[4], ret);
	return ret;
}

int isp_fw_get_iso_cap(struct isp_context *isp, enum camera_id cid)
{
#if	defined(PER_FRAME_CTRL)
	int ret;
	enum fw_cmd_resp_stream_id stream_id;

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	ret = isp_send_fw_cmd(isp, CMD_ID_AE_GET_ISO_CAPABILITY, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0);

	RET(ret);
	return ret;
#else
	return 0;
#endif
};

int isp_fw_get_ev_cap(struct isp_context *isp, enum camera_id cid)
{
#if	defined(PER_FRAME_CTRL)
	int ret;
	enum fw_cmd_resp_stream_id stream_id;

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	ret = isp_send_fw_cmd(isp, CMD_ID_AE_GET_EV_CAPABILITY, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
	RET(ret);
	return ret;
#else
	return 0;
#endif
};

/*synchronized version of isp_fw_get_ver*/
int isp_fw_get_ver(struct isp_context *isp, unsigned int __maybe_unused *ver)
{
	int ret;

	if ((isp == NULL) || (ver == NULL))
		ISP_PR_INFO("fail bad para\n");

	ret = isp_send_fw_cmd(isp, CMD_ID_GET_FW_VERSION,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("sent fw cmd failed\n");
		return ret;
	}

	return ret;
}

int isp_fw_get_cproc_status(struct isp_context *isp,
			enum camera_id cam_id, struct _CprocStatus_t *cproc)
{
	int ret;
	unsigned int len = sizeof(struct _CprocStatus_t);
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cam_id) || (cproc == NULL)) {
		//add this line to trick CP
		ISP_PR_INFO("fail bad para\n");
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	ret = isp_send_fw_cmd_sync(isp, CMD_ID_CPROC_GET_STATUS,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				NULL, 0, 1000 * 10, cproc, &len);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("cid %u fail\n", cam_id);
		return ret;
	}
	ISP_PR_DBG("cid %u,en %d,bright %d, contrast %u,hue %c,satur %u\n",
		cam_id, cproc->enabled, cproc->brightness,
		cproc->contrast, cproc->hue, cproc->saturation);

	return ret;
}

int isp_fw_set_sharpen_enable(struct isp_context *isp,
				 enum camera_id cam_id,
				 enum _SharpenId_t *sharpen_id)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdSharpenEnable_t payload;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	payload.id = *sharpen_id;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp,
				CMD_ID_SHARPEN_ENABLE,
				stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&payload,
				sizeof(payload)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set sharpen enable:%d\n", *sharpen_id);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set sharpen enable suc:%d\n", *sharpen_id);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_sharpen_config(struct isp_context *isp,
				 enum camera_id cam_id,
				 enum _SharpenId_t sharpen_id,
				 struct _TdbSharpRegByChannel_t param)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdSharpenConfig_t payload;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	payload.SharpenConfig = param;
	payload.id = sharpen_id;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp,
				CMD_ID_SHARPEN_CONFIG,
				stream_id,
				FW_CMD_PARA_TYPE_INDIRECT,
				&payload,
				sizeof(payload))
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail set sharpen config\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set sharpen config suc\n");
		ret = RET_SUCCESS;
	}

	return ret;
}


int isp_fw_set_sharpen_disable(struct isp_context *isp,
				 enum camera_id cam_id,
				 enum _SharpenId_t *sharpen_id)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdSharpenEnable_t payload;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	payload.id = *sharpen_id;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp,
				CMD_ID_SHARPEN_DISABLE,
				stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&payload,
				sizeof(payload)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set sharpen disable:%d\n", *sharpen_id);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set sharpen disable suc:%d\n", *sharpen_id);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_clk_gating_enable(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int enable)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, enable ? CMD_ID_ENABLE_CLOCK_GATING :
			CMD_ID_DISABLE_CLOCK_GATING, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail set clk gating enable:%d\n", enable);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set clk gating enable suc:%d\n", enable);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_power_gating_enable(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int enable)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, enable ? CMD_ID_ENABLE_POWER_GATING :
			CMD_ID_DISABLE_POWER_GATING, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail set power gating enable:%d\n", enable);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set power gating enable suc:%d\n", enable);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_image_effect_enable(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int enable)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, enable ? CMD_ID_IE_ENABLE :
			CMD_ID_IE_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail set ie enable:%d\n", enable);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set ie enable suc:%d\n", enable);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_image_effect_config(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _IeConfig_t *param, int enable)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdIeConfig_t payload;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	payload.param.status = *param;
	payload.param.enabled = enable;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp,
				CMD_ID_IE_CONFIG,
				stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&payload,
				sizeof(payload))
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail set ie config\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set ie config suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_snr_enable(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int enable)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, enable ? CMD_ID_SNR_ENABLE :
			CMD_ID_SNR_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail set snr enable:%d\n", enable);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set snr enable suc:%d\n", enable);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_gamma_enable(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int enable)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, enable ? CMD_ID_GAMMA_ENABLE :
			CMD_ID_GAMMA_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail set gamma enable:%d\n", enable);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set gamma enable suc:%d\n", enable);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_calib_enable(struct isp_context *isp,
				 enum camera_id cam_id,
				 unsigned char enable_mask)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdEnableSensorCalibdb_t  enableCalib;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	enableCalib.enableMask = enable_mask;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_ENABLE_CALIB,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&enableCalib,
			sizeof(struct _CmdEnableSensorCalibdb_t))
			!= RET_SUCCESS) {
		ISP_PR_ERR("set calib enable:%d\n", enable_mask);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set calib enable suc:%d\n", enable_mask);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_calib_disable(struct isp_context *isp,
				 enum camera_id cam_id,
				 unsigned int disable_mask)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdDisableSensorCalibdb_t  disableCalib;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	disableCalib.disableMask = disable_mask;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_DISABLE_CALIB,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&disableCalib,
			sizeof(struct _CmdDisableSensorCalibdb_t))
			!= RET_SUCCESS) {
		ISP_PR_ERR("set calib disable:%d\n", disable_mask);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set calib disable suc:%d\n", disable_mask);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_raw_fmt(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetRawPktFmt_t raw_fmt)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_RAW_PKT_FMT,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&raw_fmt,
				sizeof(struct _CmdSetRawPktFmt_t)) !=
				RET_SUCCESS) {
		ISP_PR_ERR("fail set raw fmt to %d", raw_fmt.rawPktFmt);
		ret = RET_FAILURE;
	} else {
		ISP_PR_PC("set raw fmt to %d suc", raw_fmt.rawPktFmt);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_aspect_ratio_window(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetAspectRatioWindow_t aprw)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_ASPECT_RATIO_WINDOW,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&aprw, sizeof(aprw)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set aspect ratio window\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set aspect ratio window suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_out_ch_prop(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetOutChProp_t cmdChProp)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_OUT_CH_PROP,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&cmdChProp, sizeof(cmdChProp)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set out ch prop\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set out ch prop suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_out_ch_fps_ratio(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetOutChFrameRateRatio_t cmdChRatio)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_OUT_CH_FRAME_RATE_RATIO,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&cmdChRatio, sizeof(cmdChRatio)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set out ch fps ratio\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set out ch fps ratio suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_out_ch_enable(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdEnableOutCh_t cmdChEn)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_ENABLE_OUT_CH,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&cmdChEn, sizeof(cmdChEn)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set out ch enable\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set out ch enable\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_tnr_enable(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int enable)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, enable ? CMD_ID_TNR_ENABLE :
			CMD_ID_TNR_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail set tnr enable:%d\n", enable);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set tnr enable suc:%d\n", enable);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_hdr_enable(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int enable)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, enable ? CMD_ID_AE_ENABLE_HDR :
			CMD_ID_AE_DISABLE_HDR, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail set hdr enable:%d\n", enable);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set hdr enable suc:%d\n", enable);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_snr_config(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _SinterParams_t *param)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdSnrConfig_t payload;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	payload.sinterParams = *param;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp,
				CMD_ID_SNR_CONFIG,
				stream_id,
				FW_CMD_PARA_TYPE_INDIRECT,
				&payload,
				sizeof(payload))
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail set snr config\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set snr config suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_tnr_config(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _TemperParams_t *param)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdTnrConfig_t payload;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	payload.temperParams = *param;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp,
				CMD_ID_TNR_CONFIG,
				stream_id,
				FW_CMD_PARA_TYPE_INDIRECT,
				&payload,
				sizeof(payload))
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail set tnr config\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set tnr config suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

void isp_set_mod_log_level(struct _CmdSetLogModExt_t *pmod,
			   enum _ModId_t mod_id, enum _LogLevel_t log_level)
{
	unsigned int idx = (mod_id - 1) / 8;
	unsigned int shift = 4 * ((mod_id - 1) % 8);
	unsigned int mask = 0xf << shift;
	unsigned int val;

	if (!pmod)
		return;
	val = pmod->level_bits[idx];
	pmod->level_bits[idx] = (val & ~mask) | (log_level << shift);
}

int isp_fw_start(struct isp_context *isp)
{
	int ret;
	unsigned int fw_ver;
	struct _CmdSetLogModExt_t modext;

	ENTER();

	if (ISP_GET_STATUS(isp) >= ISP_STATUS_FW_RUNNING) {
		ISP_PR_WARN("in status %d,do nothing\n", ISP_GET_STATUS(isp));
		return RET_SUCCESS;
	};

	if (ISP_GET_STATUS(isp) == ISP_STATUS_PWR_OFF) {
		ISP_PR_ERR("fail, no power up\n");
		return RET_FAILURE;
	}

	if (ISP_GET_STATUS(isp) == ISP_STATUS_PWR_ON) {
		ret = isp_fw_boot(isp);
		if (ret != RET_SUCCESS) {
			ISP_PR_ERR("fail for isp_fw_boot\n");
			return ret;
		}
	}

	isp->drv_settings.fw_log_cfg_en = g_prop->fw_log_enable;
	isp->drv_settings.fw_log_cfg_A = g_prop->fw_log_level;
	isp->drv_settings.fw_log_cfg_B = g_prop->fw_log_level;
	isp->drv_settings.fw_log_cfg_C = g_prop->fw_log_level;
	isp->drv_settings.fw_log_cfg_D = g_prop->fw_log_level;
	isp->drv_settings.fw_log_cfg_E = g_prop->fw_log_level;
	isp->drv_settings.fw_log_cfg_F = g_prop->fw_log_level;
	isp->drv_settings.fw_log_cfg_G = g_prop->fw_log_level;

	if (isp->drv_settings.fw_log_cfg_en != 0) {
		modext.level_bits[0] = isp->drv_settings.fw_log_cfg_A;/*1-8*/
		modext.level_bits[1] = isp->drv_settings.fw_log_cfg_B;/*9-16*/
		modext.level_bits[2] = isp->drv_settings.fw_log_cfg_C;/*17-24*/
		modext.level_bits[3] = isp->drv_settings.fw_log_cfg_D;/*25-32*/
		modext.level_bits[4] = isp->drv_settings.fw_log_cfg_E;/*33-40*/
		modext.level_bits[5] = isp->drv_settings.fw_log_cfg_F;/*41-48*/
		modext.level_bits[6] = isp->drv_settings.fw_log_cfg_G;/*49-56*/
		modext.level_bits[7] = g_prop->fw_log_level;/*57-64*/
	} else {
		// default, dsiable all log
		modext.level_bits[0] = 0x33333333;
		modext.level_bits[1] = 0x33333333;
		modext.level_bits[2] = 0x33333333;
		modext.level_bits[3] = 0x33333333;
		modext.level_bits[4] = 0x33333333;
		modext.level_bits[5] = 0x33333333;
		modext.level_bits[6] = 0x33333333;
		modext.level_bits[7] = 0x33333333;
	}
//	isp_set_mod_log_level(&modext, MOD_ID_LSC_MGR, LOG_LEVEL_WARN);
//	isp_set_mod_log_level(&modext, MOD_ID_HAL_ISP_HW, LOG_LEVEL_WARN);
//	isp_set_mod_log_level(&modext, MOD_ID_MMHUB, LOG_LEVEL_WARN);
//	isp_set_mod_log_level(&modext, MOD_ID_MODULE_SETUP, LOG_LEVEL_WARN);
	isp_fw_log_mod_ext_set(isp, &modext);
	isp_fw_get_ver(isp, &fw_ver);

	RET(RET_SUCCESS);
	return RET_SUCCESS;
};

void isp_fw_stop_all_stream(struct isp_context __maybe_unused *isp)
{
}

int isp_fw_set_focus_mode(struct isp_context *isp,
			enum camera_id cam_id,
			enum _AfMode_t mode)
{
	struct _CmdAfSetMode_t af;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	af.mode = mode;
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_MODE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&af, sizeof(af)) != RET_SUCCESS) {
		ISP_PR_ERR("fail send cmd af mode(%u)\n", af.mode);
		ret = RET_FAILURE;
	} else {
		sif->af_mode = mode;
		ISP_PR_INFO("set af mode(%u) suc\n", af.mode);
		ret = RET_SUCCESS;
	};
	return ret;
};

int isp_fw_start_af(struct isp_context *isp, enum camera_id cam_id)
{
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp_send_fw_cmd(isp, CMD_ID_AF_TRIG,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail send cmd mode\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_frame_rate_info(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int frame_rate)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdSetFrameRateInfo_t payload;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	payload.frameRate = frame_rate;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp,
				CMD_ID_SET_FRAME_RATE_INFO,
				stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&payload,
				sizeof(payload)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set frame rate info(%u)\n",
			frame_rate);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set frame rate info(%u) suc\n", frame_rate);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_3a_lock(struct isp_context *isp, enum camera_id cam_id,
			enum isp_3a_type type, enum _LockType_t lock_mode)
{
	unsigned int cmd;
	struct _CmdAeLock_t lock_type;
	char *str_type;
	char *str_lock;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	switch (type) {
	case ISP_3A_TYPE_AF:
		cmd = CMD_ID_AF_LOCK;
		str_type = "AF";
		break;
	case ISP_3A_TYPE_AE:
		cmd = CMD_ID_AE_LOCK;
		str_type = "AE";
		break;
	case ISP_3A_TYPE_AWB:
		cmd = CMD_ID_AWB_LOCK;
		str_type = "AWB";
		break;
	default:
		ISP_PR_INFO("fail 3a type %d\n", type);
		return RET_FAILURE;
	};
	switch (lock_mode) {
	case LOCK_TYPE_IMMEDIATELY:
		str_lock = "immediately";
		break;
	case LOCK_TYPE_CONVERGENCE:
		str_lock = "converge";
		break;
	default:
		ISP_PR_ERR("invalid lock type %d\n", lock_mode);
		return RET_FAILURE;
	};
	lock_type.lockType = lock_mode;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp_send_fw_cmd(isp, cmd,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&lock_type, sizeof(lock_type)) != RET_SUCCESS) {
		ISP_PR_ERR("cid %d %s %s fail send cmd\n",
			cam_id, str_type, str_lock);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cid %d %s %s,suc\n", cam_id, str_type, str_lock);
		ret = RET_SUCCESS;
	};
	return ret;
};

int isp_fw_3a_unlock(struct isp_context *isp, enum camera_id cam_id,
			enum isp_3a_type type)
{
	unsigned int cmd;
	char *str_type;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	switch (type) {
	case ISP_3A_TYPE_AF:
		cmd = CMD_ID_AF_UNLOCK;
		str_type = "AF";
		break;
	case ISP_3A_TYPE_AE:
		cmd = CMD_ID_AE_UNLOCK;
		str_type = "AE";
		break;
	case ISP_3A_TYPE_AWB:
		cmd = CMD_ID_AWB_UNLOCK;
		str_type = "AWB";
		break;
	default:
		ISP_PR_INFO("fail 3a type %d\n", type);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp_send_fw_cmd(isp, cmd,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("cid %d %s fail send cmd\n", cam_id, str_type);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cid %d %s ,suc\n", cam_id, str_type);
		ret = RET_SUCCESS;
	};
	return ret;
};

int isp_fw_set_lens_pos(struct isp_context *isp, enum camera_id cam_id,
			unsigned int pos)
{
	struct _CmdAfSetLensPos_t lens_pos;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	lens_pos.lensPos = pos;
	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_LENS_POS,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&lens_pos, sizeof(lens_pos)) != RET_SUCCESS) {
		ISP_PR_ERR("cid %d pos %d fail\n", cam_id, pos);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cid %d pos %d suc\n", cam_id, pos);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_lens_range(struct isp_context *isp, enum camera_id cid,
			struct _LensRange_t lens_range)
{
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_LENS_RANGE,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&lens_range, sizeof(lens_range)) != RET_SUCCESS) {
		ISP_PR_ERR("cid %d [%d %d] fail\n",
			cid, lens_range.minLens, lens_range.maxLens);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cid %d [%d %d] suc\n",
			cid, lens_range.minLens, lens_range.maxLens);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_exposure_mode(struct isp_context *isp,
				  enum camera_id cam_id, enum _AeMode_t mode)
{
	struct _CmdAeSetMode_t ae;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	ae.mode = mode;
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_MODE,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &ae, sizeof(ae)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set ae mode(%u)\n", ae.mode);
		ret = RET_FAILURE;
	} else {
		sif->aec_mode = mode;
		ISP_PR_INFO("set ae mode(%u) suc\n", ae.mode);
		ret = RET_SUCCESS;
	};
	return ret;
};

int isp_fw_set_wb_mode(struct isp_context *isp,
			enum camera_id cam_id, enum _AwbMode_t mode)
{
	//struct _CmdAwbSetMode_t awb;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	//awb.mode = mode;
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_MODE,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &mode, sizeof(mode)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set awb mode(%u)\n", mode);
		ret = RET_FAILURE;
	} else {
		sif->awb_mode = mode;
		ISP_PR_INFO("set awb mode(%u) suc\n", mode);
		ret = RET_SUCCESS;
	};
	return ret;
};

int isp_fw_set_wb_region(struct isp_context *isp,
			enum camera_id cam_id, struct _Window_t awb_window)
{
#if	defined(PER_FRAME_CTRL)
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_REGION, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &awb_window,
				sizeof(awb_window)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set wb window\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set wb window suc\n");
		ret = RET_SUCCESS;
	}

	return ret;
#endif
	return 0;
}

int isp_fw_set_ae_precapture(struct isp_context *isp,
			enum camera_id cam_id)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_PRECAPTURE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT, NULL,
				0) != RET_SUCCESS) {
		ISP_PR_ERR("fail set ae precapture\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set ae precapture suc\n");
		ret = RET_SUCCESS;
	}

	return ret;
}

int isp_fw_set_capture_yuv(struct isp_context *isp,
			enum camera_id cam_id, struct _ImageProp_t imageProp,
			struct _Buffer_t buffer)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdCaptureYuv_t cmd_para;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	cmd_para.buffer = buffer;
	cmd_para.imageProp = imageProp;
	if (isp_send_fw_cmd(isp, CMD_ID_CAPTURE_YUV,
				stream_id, FW_CMD_PARA_TYPE_INDIRECT,
				&cmd_para, sizeof(cmd_para)) != RET_SUCCESS) {
		ISP_PR_ERR("fail capture yuv\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("capture yuv suc\n");
		ret = RET_SUCCESS;
	}

	return ret;
}

int isp_fw_set_capture_raw(struct isp_context *isp,
			enum camera_id cam_id, struct _Buffer_t buffer)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdCaptureRaw_t cmd_para;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	cmd_para.buffer = buffer;
	if (isp_send_fw_cmd(isp, CMD_ID_CAPTURE_RAW,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&cmd_para, sizeof(cmd_para)) != RET_SUCCESS) {
		ISP_PR_ERR("fail capture raw\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("capture raw suc\n");
		ret = RET_SUCCESS;
	}

	return ret;
}

int isp_fw_set_acq_window(struct isp_context *isp,
			enum camera_id cam_id, struct _Window_t acq_window)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_ACQ_WINDOW, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&acq_window, sizeof(acq_window))
					!= RET_SUCCESS) {
		ISP_PR_ERR("fail set acq window\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set acq window suc\n");
		ret = RET_SUCCESS;
	}

	return ret;
}

int isp_fw_set_start_stream(struct isp_context *isp, enum camera_id cam_id)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdAeSetApplyMode_t aec_mode;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_START_STREAM, stream_id,
			    FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] set start stream fail\n", cam_id);
		ret = RET_FAILURE;
		goto fail;
	} else {
		ISP_PR_INFO("cam[%d] set start stream suc\n", cam_id);
		ret = RET_SUCCESS;
	}

	if (g_prop->aec_mode == AE_CONTROL_MODE_SYNC)
		aec_mode.mode = AE_APPLY_MODE_SYNC;
	else if (g_prop->aec_mode == AE_CONTROL_MODE_ASYNC)
		aec_mode.mode = AE_APPLY_MODE_ASYNC;
	else
		aec_mode.mode = (g_prop->cvip_enable == CVIP_STATUS_ENABLE) ?
				AE_APPLY_MODE_SYNC : AE_APPLY_MODE_ASYNC;

	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_APPLY_MODE, stream_id,
			    FW_CMD_PARA_TYPE_DIRECT, &aec_mode,
			    sizeof(aec_mode)) != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] set aec mode %d(0:async, 1:sync) fail\n",
			   cam_id, aec_mode.mode);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cam[%d] set aec mode %d(0:async, 1:sync) suc\n",
			    cam_id, aec_mode.mode);
		ret = RET_SUCCESS;
	}
fail:
	return ret;
}

int isp_fw_set_stop_stream(struct isp_context *isp, enum camera_id cam_id,
			   int destroyBuffers)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdStopStream_t stop_stream_para;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	stop_stream_para.destroyBuffers = destroyBuffers;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_STOP_STREAM, stream_id,
			    FW_CMD_PARA_TYPE_DIRECT, &stop_stream_para,
			    sizeof(stop_stream_para)) != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] fail stop stream(%u)", cam_id,
			   destroyBuffers);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cam[%d] stop stream(%u) suc", cam_id,
			    destroyBuffers);
		ret = RET_SUCCESS;
	}

	return ret;
}

int isp_fw_set_sensor_prop(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetSensorProp_t snr_prop)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_PROP,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_INDIRECT,
				&snr_prop,
				sizeof(snr_prop)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set sensor prop\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set sensor prop suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_hdr_prop(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetSensorHdrProp_t hdr_prop)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_HDR_PROP,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_INDIRECT,
			&hdr_prop,
			sizeof(hdr_prop)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set hdr prop\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set hdr prop suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_emb_prop(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetSensorEmbProp_t snr_embprop)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_EMB_PROP,
					FW_CMD_RESP_STREAM_ID_GLOBAL,
					FW_CMD_PARA_TYPE_INDIRECT,
					&snr_embprop,
					sizeof(snr_embprop)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set emb prop\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set emb prop suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_ae_region(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _Window_t ae_window)
{
#if	defined(PER_FRAME_CTRL)
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	CmdAeSetRegion_t payload;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	payload.window = ae_window;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_REGION, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&payload, sizeof(payload)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set ae region\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set ae region suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
#endif
	return 0;
}

int isp_fw_set_calib_data(struct isp_context *isp, enum camera_id cam_id,
			  struct _CmdSetSensorCalibdb_t calib)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	ISP_PR_PC("set calib[%u] %ux%u@%u expo:%u len:%u crc:0x%x\n",
		  calib.tdbIdx,
		  calib.width,
		  calib.height,
		  calib.fps,
		  calib.expoLimit,
		  calib.bufSize,
		  calib.checkSum);
	if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_CALIBDB,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT,
				&calib, sizeof(struct _CmdSetSensorCalibdb_t))
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail set calib data\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set calib data suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_stream_path(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetStreamPath_t stream_path_cmd)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SET_STREAM_PATH,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&stream_path_cmd, sizeof(stream_path_cmd))
			!= RET_SUCCESS) {
		ISP_PR_ERR("fail set stream path\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set stream path suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_zoom_window(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _Window_t ZoomWindow)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdSetZoomWindow_t zoom_cmd;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	zoom_cmd.window.h_offset = ZoomWindow.h_offset;
	zoom_cmd.window.v_offset = ZoomWindow.v_offset;
	zoom_cmd.window.h_size   = ZoomWindow.h_size;
	zoom_cmd.window.v_size   = ZoomWindow.v_size;

	if (isp_send_fw_cmd(isp, CMD_ID_SET_ZOOM_WINDOW, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &zoom_cmd,
					sizeof(zoom_cmd)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set zoom window\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set zoom window suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_send_frame_ctrl(struct isp_context *isp, enum camera_id cam_id,
			   struct _CmdFrameCtrl_t *frame_ctrl)
{
	int ret = RET_FAILURE;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cam_id) || !frame_ctrl)
		goto quit;

	sif = &isp->sensor_info[cam_id];
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_SEND_FRAME_CTRL, stream_id,
			      FW_CMD_PARA_TYPE_INDIRECT, frame_ctrl,
			      sizeof(struct _CmdFrameCtrl_t));

	if (ret != RET_SUCCESS)
		goto quit;

	sif->per_frame_ctrl_cnt++;
	/*
	 * Due to ISP/FW Per Frame Ctrl restrictions,
	 * should send at least 1 PFC cmds before start stream
	 */
	if (sif->per_frame_ctrl_cnt == PFC_CNT_BEFORE_START_STREAM) {
		//As required by FW the calib enable must be sent after the
		//first PFC and before start stream
		if (isp->sensor_info[cam_id].calib_enable &&
		    isp->sensor_info[cam_id].calib_set_suc) {
			if (isp_fw_set_calib_enable(isp, cam_id,
			TDB_IDX_0 | TDB_IDX_1 | TDB_IDX_2) != RET_SUCCESS) {
				ISP_PR_ERR("set calib enable failed\n");
			} else {
				ISP_PR_INFO("set calib enable suc\n");
			}
		} else {
			if (isp_fw_set_calib_disable(isp, cam_id, 0) !=
			    RET_SUCCESS) {
				ISP_PR_ERR("set calib disable failed\n");
			} else {
			//not fatal error, so don't return failure so that
			//camera can go on running
				ISP_PR_INFO("set calib disable suc\n");
			}
		}

		ret = isp_fw_set_start_stream(isp, cam_id);
	}
 quit:
	if (ret != RET_SUCCESS) {
		//add this line to trick CP
		ISP_PR_ERR("cam[%d] fail, fc %p", cam_id, frame_ctrl);
	} else {
		//add this line to trick CP
		ISP_PR_INFO("cam[%d] suc, fc %p", cam_id, frame_ctrl);
	}

	return ret;
}

int isp_fw_set_out_ch_disable(struct isp_context *isp,
				 enum camera_id cam_id,
				 enum _IspPipeOutCh_t ch)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdDisableOutCh_t cmdChDisable;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}

	cmdChDisable.ch = ch;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_DISABLE_OUT_CH,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&cmdChDisable, sizeof(cmdChDisable)) !=
				RET_SUCCESS) {
		ISP_PR_ERR("fail disable out ch\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("disable out ch suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_wb_gain(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _WbGain_t wb_gain)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_WB_GAIN, stream_id,
		FW_CMD_PARA_TYPE_DIRECT, &wb_gain, sizeof(wb_gain)) !=
		RET_SUCCESS) {
		ISP_PR_ERR("fail set wb gain\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set wb gain suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_send_buffer(struct isp_context *isp,
				 enum camera_id cam_id,
				 enum _BufferType_t bufferType,
				 struct _Buffer_t buffer)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdSendBuffer_t buf_type;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	}
	buf_type.buffer = buffer;
	buf_type.bufferType = bufferType;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &buf_type,
					sizeof(buf_type)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set send buffer\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set send buffer suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_wb_light(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int light)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_LIGHT_SOURCE,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &light, sizeof(light)) !=
					 RET_SUCCESS) {
		ISP_PR_ERR("fail set wb light(%u)\n", light);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set wb light(%u) suc\n", light);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_wb_colorT(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int colorT)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_COLOR_TEMPERATURE, stream_id,
		FW_CMD_PARA_TYPE_DIRECT, &colorT, sizeof(colorT)) !=
		RET_SUCCESS) {
		ISP_PR_ERR("fail set wb colorT(%u)\n", colorT);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set wb colorT(%u) suc\n", colorT);
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_wb_cc_matrix(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CcMatrix_t *matrix)
{
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_CC_MATRIX,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 matrix, sizeof(struct _CcMatrix_t)) !=
					 RET_SUCCESS) {
		ISP_PR_ERR("fail set cc matrix\n");
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set wb cc matrix suc\n");
		ret = RET_SUCCESS;
	};
	return ret;
}

int isp_fw_set_snr_ana_gain(struct isp_context *isp,
				 enum camera_id cam_id, unsigned int ana_gain)
{
	struct _CmdAeSetAnalogGain_t g;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	g.aGain = ana_gain;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_ANALOG_GAIN,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &g, sizeof(g)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set again(%u)\n", ana_gain);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set again(%u) suc\n", ana_gain);
		ret = RET_SUCCESS;
	};
	return ret;
};

int isp_fw_set_snr_dig_gain(struct isp_context __maybe_unused *isp,
	enum camera_id __maybe_unused cam_id,
	unsigned int __maybe_unused dig_gain)
{
	int ret = IMF_RET_SUCCESS;
	/*todo  to be added later */
	return ret;
};

int isp_fw_set_itime(struct isp_context *isp, enum camera_id cam_id,
			unsigned int itime)
{
	struct _CmdAeSetItime_t g;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cam_id:%d\n", cam_id);
		return RET_FAILURE;
	};
	g.itime = itime;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_ITIME,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &g, sizeof(g)) != RET_SUCCESS) {
		ISP_PR_ERR("fail set itime(%u)\n", itime);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("set itime(%u) suc\n", itime);
		ret = RET_SUCCESS;
	};
	return ret;
};

int isp_fw_send_meta_data_buf(struct isp_context __maybe_unused *isp,
	enum camera_id __maybe_unused cam_id,
	struct isp_meta_data_info __maybe_unused *info)
{
	int ret = IMF_RET_SUCCESS;
	/*todo  to be added later */
	return ret;
};

int isp_fw_send_all_meta_data(struct isp_context __maybe_unused *isp)
{
	int ret = IMF_RET_SUCCESS;
	/*todo  to be added later */
	return ret;
};

int isp_fw_set_script(struct isp_context *isp, enum camera_id cam_id,
			struct _CmdSetDeviceScript_t *script_cmd)
{
	int result;
	struct _CmdSetDeviceCtrlMode_t mode;

	if ((isp == NULL) || (script_cmd == NULL)) {
		ISP_PR_ERR("fail for bad para\n");
		return RET_FAILURE;
	};

	result = isp_send_fw_cmd(isp, CMD_ID_SET_DEVICE_SCRIPT,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_INDIRECT, script_cmd,
			sizeof(struct _CmdSetDeviceScript_t));
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail for send script cmd\n");
		return RET_FAILURE;
	}

	mode.sensorId = cam_id;
	//mode.mode = DEVICE_CONTROL_MODE_HOST;
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE)
		mode.mode = DEVICE_CONTROL_MODE_CVP;
	else
		mode.mode = DEVICE_CONTROL_MODE_SCRIPT;
	result = isp_send_fw_cmd(isp, CMD_ID_SET_DEVICE_CTRL_MODE,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_DIRECT, &mode, sizeof(mode));
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail for send enable cmd\n");
		return RET_FAILURE;
	}
	ISP_PR_DBG("suc, sensorId %d, mode %d\n", mode.sensorId, mode.mode);

	return RET_SUCCESS;
}

unsigned int l_rd_ptr;
unsigned int l_wr_ptr;
unsigned int isp_fw_get_fw_rb_log(struct isp_context *isp,
	unsigned char *buf)
{
	uint32_t rd_ptr, wr_ptr;
	uint32_t total_cnt;
	uint32_t cnt;
	uint64_t mem_addr;
	unsigned long long sys;
	unsigned int rb_size;
	unsigned int offset = 0;
	static int calculate_cpy_time;

	total_cnt = 0;
	cnt = 0;
	isp_fw_buf_get_trace_base(isp->fw_work_buf_hdl, &sys, &mem_addr,
				&rb_size);
	if (rb_size == 0)
		return 0;
	if (calculate_cpy_time) {
		long long bef;
		long long cur;
		unsigned int cal_cnt;

		calculate_cpy_time = 0;
		isp_get_cur_time_tick(&bef);
		for (cal_cnt = 0; cal_cnt < 10; cal_cnt++)
			memcpy(buf, (unsigned char *)(sys),
			ISP_FW_TRACE_BUF_SIZE);

		isp_get_cur_time_tick(&cur);
		cur -= bef;
		bef = (long long)((ISP_FW_TRACE_BUF_SIZE * 10) / 1024);
		cur = bef * 10000000 / cur;
		ISP_PR_ERR("memcpy speed %llxK/S\n", cur);
	}

	rd_ptr = isp_hw_reg_read32(FW_LOG_RB_RPTR_REG);
	wr_ptr = isp_hw_reg_read32(FW_LOG_RB_WPTR_REG);
goon:
	if (wr_ptr > rd_ptr)
		cnt = wr_ptr - rd_ptr;
	else if (wr_ptr < rd_ptr)
		cnt = rb_size - rd_ptr;
	else
		return 0;

	if (cnt > rb_size) {
		ISP_PR_ERR("fail fw log size %u\n", cnt);
		return 0;
	}

	memcpy(buf + offset, (unsigned char *) (sys + rd_ptr), cnt);

	offset += cnt;
	total_cnt += cnt;
	rd_ptr = (rd_ptr + cnt) % rb_size;
	if (rd_ptr < wr_ptr)
		goto goon;
	isp_hw_reg_write32(FW_LOG_RB_RPTR_REG, rd_ptr);

	return total_cnt;
}

int isp_fw_set_gamma(struct isp_context *isp,
		unsigned int cam_id, int gamma)
{
	int result = RET_SUCCESS;
#if	defined(PER_FRAME_CTRL)
	CmdGammaSetCurve_t curv;
	enum fw_cmd_resp_stream_id stream_id;

	if (is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("fail for para isp:%p,cam:%d\n", isp, cam_id);
		return RET_FAILURE;
	}

	if ((gamma < 0) || (gamma > 499)) {
		ISP_PR_ERR("fail for bad gamma %d\n", gamma);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	curv.segMode = GAMMA_SEG_MODE_LOG;
	memcpy(&curv.curve, g_gamma_table[gamma], sizeof(curv.curve));
	result =
		isp_send_fw_cmd(isp, CMD_ID_GAMMA_SET_CURVE,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&curv, sizeof(curv));

	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail for send gam cmd\n");
		return RET_FAILURE;
	}

#endif
	return result;
};

int isp_fw_send_metadata_buf(struct isp_context __maybe_unused *isp,
	enum camera_id __maybe_unused cam_id,
	struct isp_mapped_buf_info __maybe_unused *buffer)
{
	return IMF_RET_SUCCESS;
	/*todo  to be added later */
}

int isp_fw_config_cvip_sensor(struct isp_context *isp, enum camera_id cam_id,
			      struct _CmdConfigCvipSensor_t param)
{
	int ret;

	ret = isp_send_fw_cmd_sync(isp, CMD_ID_CONFIG_CVIP_SENSOR,
				   FW_CMD_RESP_STREAM_ID_GLOBAL,
				   FW_CMD_PARA_TYPE_DIRECT,
				   &param, sizeof(param), 6000, NULL, NULL);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] fail", cam_id);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cam[%d] suc", cam_id);
		ret = RET_SUCCESS;
	}

	return ret;
}

int isp_fw_set_cvip_buf(struct isp_context *isp, enum camera_id cam_id,
			struct _CmdSetCvipBufLayout_t param)
{
	int ret;

	if (isp_send_fw_cmd(isp, CMD_ID_SET_CVIP_BUF_LAYOUT,
			    FW_CMD_RESP_STREAM_ID_GLOBAL,
			    FW_CMD_PARA_TYPE_DIRECT, &param,
			    sizeof(param)) != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] fail", cam_id);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cam[%d] suc", cam_id);
		ret = RET_SUCCESS;
	}

	return ret;
}

int isp_fw_start_cvip_sensor(struct isp_context *isp, enum camera_id cam_id,
			     struct _CmdStartCvipSensor_t param)
{
	int ret;

	ret = isp_send_fw_cmd_sync(isp, CMD_ID_START_CVIP_SENSOR,
				   FW_CMD_RESP_STREAM_ID_GLOBAL,
				   FW_CMD_PARA_TYPE_DIRECT,
				   &param, sizeof(param), 300, NULL, NULL);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] fail", cam_id);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cam[%d] suc", cam_id);
		ret = RET_SUCCESS;
	}

	return ret;
}

int isp_fw_set_cvip_sensor_slice_num(struct isp_context *isp,
				     enum camera_id cam_id,
				     struct _CmdSetSensorSliceNum_t param)
{
	int ret;

	if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_SLICE_NUM,
			    FW_CMD_RESP_STREAM_ID_GLOBAL,
			    FW_CMD_PARA_TYPE_DIRECT, &param,
			    sizeof(param)) != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] fail", cam_id);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cam[%d] suc", cam_id);
		ret = RET_SUCCESS;
	}

	return ret;
}

// This function must be called before whenever there is a iclk change.
// change iclk message to SMU must be issued
// after we got the response of this FW command.
int isp_set_clk_info_2_fw(struct isp_context *isp)
{
	struct _CmdSetClkInfo_t clk_info;
	int ret;

	if (isp->clk_info_set_2_fw) {
		ISP_PR_INFO("suc do none\n");
		return RET_SUCCESS;
	}
	// FW interface is in kHz, SW is in mHz
	clk_info.sclk = isp->sclk * 1000;
	clk_info.iclk = isp->iclk * 1000;
	clk_info.refClk = isp->refclk * 1000;
	if (isp_send_fw_cmd(isp, CMD_ID_SET_CLK_INFO,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT, &clk_info,
				sizeof(clk_info)) != RET_SUCCESS) {
		ISP_PR_ERR("isp_send_fw_cmd failed\n");
		ret = RET_FAILURE;
	} else {
		isp->clk_info_set_2_fw = 1;
		ISP_PR_INFO("suc,s:%u,i:%u,ref:%u\n",
			clk_info.sclk, clk_info.iclk, clk_info.refClk);
		ret = RET_SUCCESS;
	};
	return ret;
}

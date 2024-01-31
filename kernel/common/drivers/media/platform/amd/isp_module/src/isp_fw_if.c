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

#include "isp_common.h"
#include "isp_fw_if.h"
#include "log.h"
#include "gamma_table.h"
#include "isp_soc_adpt.h"
#include "isp_para_capability.h"

uint32 get_nxt_cmd_seq_num(struct isp_context *isp)
{
	uint32 seq_num;

	if (isp == NULL)
		return 1;

	seq_num = isp->host2fw_seq_num++;
	return seq_num;
}

uint8 compute_check_sum(uint8 *buffer, uint32 buffer_size)
{
	uint32 i;
	uint8 checksum;

	checksum = 0;
	for (i = 0; i < buffer_size; i++)
		checksum += buffer[i];

	return checksum;
};

int32 no_fw_cmd_ringbuf_slot(struct isp_context *isp,
			enum fw_cmd_resp_stream_id cmd_buf_idx)
{
	uint32 rreg;
	uint32 wreg;
	uint32 rd_ptr, wr_ptr;
	uint32 new_wr_ptr;
	uint32 len;

	isp_get_cmd_buf_regs(cmd_buf_idx, &rreg, &wreg, NULL, NULL, NULL);
	isp_fw_buf_get_cmd_base(isp->fw_work_buf_hdl, cmd_buf_idx,
				NULL, NULL, &len);

	rd_ptr = isp_hw_reg_read32(rreg);
	wr_ptr = isp_hw_reg_read32(wreg);

	new_wr_ptr = wr_ptr + sizeof(Cmd_t);
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

result_t insert_isp_fw_cmd(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream, Cmd_t *cmd)
{
	uint64 mem_sys;
	uint64 mem_addr;
	uint32 rreg;
	uint32 wreg;
	uint32 wr_ptr, old_wr_ptr, rd_ptr;
	result_t result = RET_SUCCESS;
	uint32 len;
	Cmd_t nouse_cmd;

	if ((isp == NULL) || (cmd == NULL)) {
		isp_dbg_print_err
			("-><- %s, fail bad cmd 0x%p\n", __func__, cmd);
		return RET_FAILURE;
	}

	if (stream > FW_CMD_RESP_STREAM_ID_3) {
		isp_dbg_print_err("-><- %s, fail bad id %d\n",
				__func__, stream);
		return RET_FAILURE;
	}

	switch (cmd->cmdId) {
	case CMD_ID_SEND_BUFFER:
	case CMD_ID_SEND_MODE3_FRAME_INFO:
	case CMD_ID_SEND_MODE4_FRAME_INFO:
	case CMD_ID_BIND_STREAM:
	case CMD_ID_UNBIND_STREAM:
	case CMD_ID_GET_FW_VERSION:
	case CMD_ID_SET_LOG_MOD_EXT:
	case CMD_ID_SET_SENSOR_PROP:
	case CMD_ID_SET_SENSOR_CALIBDB:
	case CMD_ID_SET_DEVICE_SCRIPT:
	case CMD_ID_ENABLE_DEVICE_SCRIPT_CONTROL:
	case CMD_ID_DISABLE_DEVICE_SCRIPT_CONTROL:
	case CMD_ID_SET_SENSOR_HDR_PROP:
	case CMD_ID_SET_SENSOR_EMB_PROP:
	case CMD_ID_SET_CLK_INFO:
	case CMD_ID_LOG_SET_LEVEL:
	case CMD_ID_LOG_ENABLE_MOD:
	case CMD_ID_LOG_DISABLE_MOD:
	case CMD_ID_SEND_FRAME_CONTROL:
	case CMD_ID_PROFILER_GET_RESULT:
/*
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
		isp_dbg_print_err
			("-><- %s, fail no cmdslot %s(%d)\n",
			__func__, isp_dbg_get_stream_str(stream), stream);
		return RET_FAILURE;
	}

	wr_ptr = isp_hw_reg_read32(wreg);
	rd_ptr = isp_hw_reg_read32(rreg);
	old_wr_ptr = wr_ptr;
	if (rd_ptr > len) {
		isp_dbg_print_err
	("%s fail %s(%u),rd_ptr %u(should<=%u),wr_ptr %u\n",
			__func__, isp_dbg_get_stream_str(stream),
			stream, rd_ptr, len, wr_ptr);
		return RET_FAILURE;
	}

	if (wr_ptr > len) {
		isp_dbg_print_err
	("%s fail %s(%u),wr_ptr %u(should<=%u), rd_ptr %u\n",
			__func__, isp_dbg_get_stream_str(stream),
			stream, wr_ptr, len, rd_ptr);
		return RET_FAILURE;
	};

	if (wr_ptr < rd_ptr) {
		mem_addr += wr_ptr;

		memcpy((uint8 *) (mem_sys + wr_ptr),
			(uint8 *) cmd, sizeof(Cmd_t));
	} else {
		if ((len - wr_ptr) >= (sizeof(Cmd_t))) {
			mem_addr += wr_ptr;

			memcpy((uint8 *) (mem_sys + wr_ptr),
				(uint8 *) cmd, sizeof(Cmd_t));
		} else {
			uint32 size;
			uint64 dst_addr;
			uint8 *src;

			dst_addr = mem_addr + wr_ptr;
			src = (uint8 *) cmd;
			size = len - wr_ptr;

			memcpy((uint8 *) (mem_sys + wr_ptr), src, size);

			src += size;
			size = sizeof(Cmd_t) - size;
			dst_addr = mem_addr;

			memcpy((uint8 *) (mem_sys), src, size);
		}
	}

	if (result != RET_SUCCESS)
		return result;

	memcpy(&nouse_cmd, (uint8 *) (mem_sys + wr_ptr), sizeof(Cmd_t));

	wr_ptr += sizeof(Cmd_t);
	if (wr_ptr >= len)
		wr_ptr -= len;

	isp_hw_reg_write32(wreg, wr_ptr);

	return result;
}

struct isp_cmd_element *isp_append_cmd_2_cmdq(struct isp_context *isp,
				struct isp_cmd_element *command)
{
	struct isp_cmd_element *tail_element = NULL;
	struct isp_cmd_element *copy_command = NULL;

	if (command == NULL) {
		isp_dbg_print_err
			("NULL cmd pointer in %s\n", __func__);
		return NULL;
	}

	copy_command = isp_sys_mem_alloc(sizeof(struct isp_cmd_element));
	if (copy_command == NULL) {
		isp_dbg_print_err
			("memory allocate fail in %s\n", __func__);
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

struct isp_cmd_element *isp_rm_cmd_from_cmdq(struct isp_context *isp,
					uint32 seq_num, uint32 cmd_id,
					bool_t signal_evt)
{
	struct isp_cmd_element *curr_element;
	struct isp_cmd_element *prev_element;

	isp_mutex_lock(&isp->cmd_q_mtx);

	curr_element = isp->cmd_q;
	if (curr_element == NULL) {
		isp_dbg_print_err("-><- %s fail empty q\n", __func__);
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

	isp_dbg_print_err
		("-><- %s, cmd(0x%x,seq:%u) not found\n",
		__func__, cmd_id, seq_num);
 quit:
	if (curr_element && curr_element->evt && signal_evt) {
		isp_dbg_print_info
			("-><- %s, signal event %p\n",
			__func__, curr_element->evt);
		isp_event_signal(0, curr_element->evt);
	}
	isp_mutex_unlock(&isp->cmd_q_mtx);
	return curr_element;
};

struct isp_cmd_element *isp_rm_cmd_from_cmdq_by_stream(struct isp_context *isp,
						enum fw_cmd_resp_stream_id
						stream,
						bool_t signal_evt)
{
	struct isp_cmd_element *curr_element;
	struct isp_cmd_element *prev_element;

	isp_mutex_lock(&isp->cmd_q_mtx);

	curr_element = isp->cmd_q;
	if (curr_element == NULL) {
		isp_dbg_print_err
		("-><- %s fail empty stream:%u\n",
			__func__, stream);
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

	isp_dbg_print_err
		("-><- %s, stream %u no found\n",
		__func__, stream);
 quit:
	if (curr_element && curr_element->evt && signal_evt)
		isp_event_signal(0, curr_element->evt);
	isp_mutex_unlock(&isp->cmd_q_mtx);
	return curr_element;
}

result_t fw_if_send_prev_buf(struct isp_context *isp,
			enum camera_id cam_id,
			struct isp_img_buf_info *buffer)
{
	cam_id = cam_id;
	buffer = buffer;
	isp = isp;

	return IMF_RET_FAIL;
	/*todo to be added later */
};

result_t fw_if_send_img_buf(struct isp_context *isp,
				struct isp_mapped_buf_info *buffer,
				enum camera_id cam_id, enum stream_id str_id)
{
	CmdSendBuffer_t cmd;
	enum fw_cmd_resp_stream_id stream;
	result_t result;

	if (!is_para_legal(isp, cam_id)
		|| (buffer == NULL) || (str_id > STREAM_ID_ZSL)) {
		isp_dbg_print_err
	("-><- %s,fail para,isp %p,buf %p,cid %u,sid %u\n",
			__func__, isp, buffer, cam_id, str_id);
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
		cmd.bufferType = BUFFER_TYPE_ZSL;
		break;
	default:
		isp_dbg_print_err("-><- %s fail bad sid %d\n",
			__func__, str_id);
		return RET_FAILURE;
	};
	stream = isp_get_stream_id_from_cid(isp, cam_id);
	cmd.buffer.vmidSpace = 4;
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
		isp_dbg_print_err
		("-><- %s, fail send,buf %p,cid %u,sid %u\n",
			__func__, buffer, cam_id, str_id);
		return RET_FAILURE;
	}
	isp_dbg_print_verb
		("-><- %s, suc,buf %p,cid %u,sid %u\n",
		__func__, buffer, cam_id, str_id);
	return RET_SUCCESS;
}

result_t isp_send_fw_cmd_cid(struct isp_context *isp, enum camera_id cam_id,
			uint32 cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			uint32 package_size)
{
	return isp_send_fw_cmd_ex(isp, cam_id,
			cmd_id, stream, directcmd, package,
			package_size, NULL, NULL, NULL, NULL);
}

result_t isp_send_fw_cmd(struct isp_context *isp, uint32 cmd_id,
			enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			uint32 package_size)
{
	return isp_send_fw_cmd_ex(isp, CAMERA_ID_MAX,	/*don't care */
			cmd_id, stream, directcmd, package,
			package_size, NULL, NULL, NULL, NULL);
}

result_t isp_send_fw_cmd_sync(struct isp_context *isp, uint32 cmd_id,
			enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			uint32 package_size, uint32 timeout /*in ms */,
			void *resp_pl, uint32 *resp_pl_len)
{
	result_t ret;
	struct isp_event evt;
	uint32 seq;

	isp_event_init(&evt, 1, 0);
	ret = isp_send_fw_cmd_ex(isp, CAMERA_ID_MAX,	/*don't care */
				 cmd_id, stream, directcmd, package,
				 package_size, &evt, &seq, resp_pl,
				 resp_pl_len);
	if (ret != RET_SUCCESS) {
		isp_dbg_print_err
		("-><- %s,fail(%d) send cmd\n", __func__, ret);
		return ret;
	};
	isp_dbg_print_info
	("in %s bef wait cmd:0x%x,evt:%p\n", __func__, cmd_id,
		&evt);
	ret = isp_event_wait(&evt, timeout);
	isp_dbg_print_info
	("in %s aft wait cmd:0x%x,evt:%p\n", __func__, cmd_id,
		&evt);
	if (ret != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s fail(%d) wait\n", __func__, ret);
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

result_t isp_send_fw_cmd_ex(struct isp_context *isp, enum camera_id cam_id,
			uint32 cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			uint32 package_size, struct isp_event *evt,
			uint32 *seq, void *resp_pl, uint32 *resp_pl_len)
{
	uint64 package_base = 0;
	uint64 pack_sys = 0;
	uint32 pack_len;
	result_t result;
	uint32 seq_num;
	struct isp_cmd_element command_element;
	struct isp_cmd_element *cmd_ele = NULL;
	isp_ret_status_t os_status;
	uint32 sleep_count;
	Cmd_t cmd;
	CmdParamPackage_t *pkg;
	struct isp_gpu_mem_info *gpu_mem = NULL;

	if (directcmd && (package_size > sizeof(cmd.cmdParam))) {
		isp_dbg_print_err
	("-><- %s, fail pkgsize(%u) cmd id:0x%x,stream %d\n",
		__func__, package_size, cmd_id, stream);
		return RET_FAILURE;
	}

	if (package_size && (package == NULL)) {
		isp_dbg_print_err
		("-><- %s, fail null pkg cmd id:0x%x,stream %d\n",
			__func__, cmd_id, stream);
		return RET_FAILURE;
	};
	os_status = isp_mutex_lock(&isp->command_mutex);
	sleep_count = 0;
	while (1) {
		if (no_fw_cmd_ringbuf_slot(isp, stream)) {
			uint32 rreg;
			uint32 wreg;
			uint32 len;
			uint32 rd_ptr, wr_ptr;

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
			isp_dbg_print_err
	("fail no cmdslot cmd id:0x%x,stream %s(%d),rreg %u,wreg %u,len %u\n",
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
		cmd.cmdStreamId = (uint16) STREAM_ID_INVALID;
		break;
	}

	if (directcmd) {
		if (package_size)
			memcpy(cmd.cmdParam, package, package_size);
	} else if (package_size <= isp_get_cmd_pl_size()) {
		if (isp_fw_get_nxt_cmd_pl(isp->fw_work_buf_hdl, &pack_sys,
				&package_base, &pack_len) != RET_SUCCESS) {
			isp_dbg_print_err
			("-><- %s,no enough pkg buf(0x%08x)\n",
				__func__, cmd_id);
			goto failure_out;
		};

		memcpy((void *)pack_sys, (uint8 *) package, package_size);

		pkg = (CmdParamPackage_t *) cmd.cmdParam;
		isp_split_addr64(package_base, &pkg->packageAddrLo,
				 &pkg->packageAddrHi);
		pkg->packageSize = (uint16) package_size;
		pkg->packageCheckSum = compute_check_sum((uint8 *) package,
							 package_size);
	} else {

		gpu_mem =
			isp_gpu_mem_alloc(package_size, ISP_GPU_MEM_TYPE_NLFB);

		if (gpu_mem == NULL) {
			isp_dbg_print_err
			("-><- %s,no gpu mem(%u), cid %d\n",
				__func__, package_size, cmd_id);
			goto failure_out;
		}
		memcpy(gpu_mem->sys_addr, (uint8 *) package, package_size);
		pkg = (CmdParamPackage_t *) cmd.cmdParam;
		isp_split_addr64(gpu_mem->gpu_mc_addr, &pkg->packageAddrLo,
				 &pkg->packageAddrHi);
		pkg->packageSize = (uint16) package_size;
		pkg->packageCheckSum = compute_check_sum((uint8 *) package,
							 package_size);
	}

	seq_num = get_nxt_cmd_seq_num(isp);
	cmd.cmdSeqNum = seq_num;
	cmd.cmdCheckSum = compute_check_sum((uint8 *) &cmd, sizeof(cmd) - 1);

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
	command_element.I2CRegAddr = (uint16) I2C_REGADDR_NULL;
	command_element.cam_id = cam_id;

	if (cmd_id == CMD_ID_SEND_I2C_MSG) {
		CmdSendI2cMsg_t *i2c_msg;

		i2c_msg = (CmdSendI2cMsg_t *) package;
		if (i2c_msg->msg.msgType == I2C_MSG_TYPE_READ)
			command_element.I2CRegAddr = i2c_msg->msg.regAddr;
	}

	cmd_ele = isp_append_cmd_2_cmdq(isp, &command_element);
	if (cmd_ele == NULL) {
		isp_dbg_print_err
		("-><- %s, fail for isp_append_cmd_2_cmdq\n", __func__);
		goto failure_out;
	}

	if (cmd_id == CMD_ID_SET_SENSOR_CALIBDB) {
		CmdSetSensorCalibdb_t *p = (CmdSetSensorCalibdb_t *) package;

		isp_dbg_print_info
		("%s %s(0x%08x:%s)%s,sn:%u,[%llx,%llx)(%u)\n",
			__func__,
			isp_dbg_get_cmd_str(cmd_id), cmd_id,
			isp_dbg_get_stream_str(stream),
			directcmd ? "direct" : "indirect", seq_num,
			isp_join_addr64(p->bufAddrLo, p->bufAddrHi),
			isp_join_addr64(p->bufAddrLo, p->bufAddrHi) +
			p->bufSize, p->bufSize);
	} else if (cmd_id == CMD_ID_SEND_BUFFER) {
		CmdSendBuffer_t *p = (CmdSendBuffer_t *) package;
		uint32 total =
			p->buffer.bufSizeA + p->buffer.bufSizeB +
			p->buffer.bufSizeC;
		uint64 y =
		isp_join_addr64(p->buffer.bufBaseALo, p->buffer.bufBaseAHi);

		isp_dbg_print_info
		("%s %s(0x%08x:%s)%s, sn:%u,%s,[%llx,%llx)(%u)\n",
			__func__, isp_dbg_get_cmd_str(cmd_id), cmd_id,
			isp_dbg_get_stream_str(stream),
			directcmd ? "direct" : "indirect", seq_num,
			isp_dbg_get_buf_type(p->bufferType), y,
			y + total, total);
	} else if (cmd_id == CMD_ID_SEND_MODE4_FRAME_INFO) {
		Mode4FrameInfo_t *p = (Mode4FrameInfo_t *) package;

		uint32 total = p->rawBufferFrameInfo.buffer.bufSizeA
			+ p->rawBufferFrameInfo.buffer.bufSizeB
			+ p->rawBufferFrameInfo.buffer.bufSizeC;
		uint64 y =
		isp_join_addr64(p->rawBufferFrameInfo.buffer.bufBaseALo,
			p->rawBufferFrameInfo.buffer.bufBaseAHi);

		isp_dbg_print_info
		("%s %s(0x%08x:%s)%s,sn:%u,[%llx,%llx)(%u)\n",
			__func__,
			isp_dbg_get_cmd_str(cmd_id), cmd_id,
			isp_dbg_get_stream_str(stream),
			directcmd ? "direct" : "indirect", seq_num, y,
			y + total, total);
	} else {

		isp_dbg_print_info
			("%s %s(0x%08x:%s)%s,sn:%u\n",
			__func__,
			isp_dbg_get_cmd_str(cmd_id), cmd_id,
			isp_dbg_get_stream_str(stream),
			directcmd ? "direct" : "indirect", seq_num);
	}

	isp_get_cur_time_tick(&cmd_ele->send_time);
	result = insert_isp_fw_cmd(isp, stream, &cmd);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
	("-><- %s, fail for insert_isp_fw_cmd cmd id 0x%x\n",
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
	if (package_base)
		isp_fw_ret_pl(isp->fw_work_buf_hdl, package_base);

	if (gpu_mem)
		isp_gpu_mem_free(gpu_mem);

	if (cmd_ele)
		isp_sys_mem_free(cmd_ele);
	isp_mutex_unlock(&isp->command_mutex);
	return RET_FAILURE;
};

result_t fw_if_set_brightness(struct isp_context *isp, uint32 camera_id,
			int32 brightness)
{
	result_t ret;
	CmdCprocSetBrightness_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.brightness = (uint8) brightness;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_CPROC_SET_BRIGHTNESS, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
	isp_dbg_print_info("-><- %s to %d ret %d\n",
			__func__, brightness, ret);
	return ret;
}

result_t fw_if_set_contrast(struct isp_context *isp, uint32 camera_id,
				int32 contrast)
{
	result_t ret;
	CmdCprocSetContrast_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.contrast = (uint8) contrast;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_CPROC_SET_CONTRAST, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
	isp_dbg_print_info("-><- %s to %u ret %d\n",
			__func__, contrast, ret);
	return ret;
}

result_t fw_if_set_hue(struct isp_context *isp, uint32 camera_id, int32 hue)
{
	result_t ret;
	CmdCprocSetHue_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.hue = (uint8) hue;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_CPROC_SET_HUE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
	isp_dbg_print_info("-><- %s to %i ret %d\n",
			__func__, hue, ret);
	return ret;
};

result_t fw_if_set_satuation(struct isp_context *isp, uint32 camera_id,
			int32 stauation)
{
	result_t ret;
	CmdCprocSetSaturation_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.saturation = (uint8) stauation;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	ret = isp_send_fw_cmd(isp, CMD_ID_CPROC_SET_SATURATION, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
	isp_dbg_print_info
		("-><- %s to %d ret %d\n",
			__func__, stauation, ret);
	return ret;
};

result_t fw_if_set_cproc_enable(struct isp_context *isp,
				uint32 camera_id, int32 cproc_enable)
{
	isp = isp;
	camera_id = camera_id;
	cproc_enable = cproc_enable;
	return IMF_RET_FAIL;
};

result_t fw_if_set_color_enable(struct isp_context *isp,
			uint32 camera_id, int32 color_enable)
{
	enum fw_cmd_resp_stream_id stream_id;
	uint32 cmd_id = CMD_ID_CPROC_DISABLE;

	if (color_enable)
		cmd_id = CMD_ID_CPROC_ENABLE;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	return isp_send_fw_cmd(isp, cmd_id, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
};

result_t fw_if_set_anti_banding_mode(struct isp_context *isp,
			uint32 camera_id,
			enum anti_banding_mode mode)
{
	CmdAeSetFlicker_t value;
	enum fw_cmd_resp_stream_id stream_id;

	switch (mode) {
	case ANTI_BANDING_MODE_DISABLE:
		value.flickerType = AE_FLICKER_TYPE_OFF;
		break;
	case ANTI_BANDING_MODE_50HZ:
		value.flickerType = AE_FLICKER_TYPE_50HZ;
		break;
	case ANTI_BANDING_MODE_60HZ:
		value.flickerType = AE_FLICKER_TYPE_60HZ;
		break;
	default:
		isp_dbg_print_err("bad anti_banding value %d\n", mode);
		return RET_FAILURE;
	}
	value.flickerType = (uint8) mode;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	return isp_send_fw_cmd(isp, CMD_ID_AE_SET_FLICKER, stream_id,
		FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
};

/*todo*/
result_t fw_if_set_iso(struct isp_context *isp, uint32 camera_id, AeIso_t iso)
{
	CmdAeSetIso_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.iso = iso;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	return isp_send_fw_cmd(isp, CMD_ID_AE_SET_ISO, stream_id,
		FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
};

/*todo*/
result_t fw_if_set_ev_compensation(struct isp_context *isp,
			uint32 camera_id, AeEv_t *ev)
{
	CmdAeSetEv_t value;
	enum fw_cmd_resp_stream_id stream_id;

	value.ev = *ev;
	stream_id = isp_get_stream_id_from_cid(isp, camera_id);
	return isp_send_fw_cmd(isp, CMD_ID_AE_SET_EV, stream_id,
		FW_CMD_PARA_TYPE_DIRECT, &value, sizeof(value));
};

/*todo*/
result_t fw_if_set_scene_mode(struct isp_context *isp, uint32 camera_id,
			enum pvt_scene_mode mode)
{
	result_t result = RET_SUCCESS;

	unreferenced_parameter(isp);
	unreferenced_parameter(camera_id);
	unreferenced_parameter(mode);
	return result;
};

result_t fw_if_cproc_config(struct isp_context *isp, uint32 camera_id,
			int32 contrast, int32 brightness,
			int32 saturation, int32 hue)
{
	isp = isp;

	camera_id = camera_id;
	contrast = contrast;
	brightness = brightness;
	saturation = saturation;
	hue = hue;
	return IMF_RET_FAIL;
};

result_t fw_enable_cproc(struct isp_context *isp, uint32 camera_id,
			 int32 contrast, int32 brightness,
			 int32 saturation, int32 hue)
{
	fw_if_cproc_config(isp, camera_id, contrast, brightness,
			   saturation, hue);
	fw_if_set_cproc_enable(isp, camera_id, 1);

	return RET_SUCCESS;
}

#define hal_read_img_mem(PHY, VIRT, LEN) isp_hw_mem_read_(PHY, VIRT, LEN)

result_t isp_get_f2h_resp(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream,
			Resp_t *response)
{
	uint32 rd_ptr;
	uint32 wr_ptr;
	uint32 rd_ptr_dbg;
	uint32 wr_ptr_dbg;
	uint64 mem_addr;
	uint32 rreg;
	uint32 wreg;
	uint32 checksum;
	uint32 len;
	pvoid *mem_sys;

	isp_get_resp_buf_regs(stream, &rreg, &wreg, NULL, NULL, NULL);
	isp_fw_buf_get_resp_base(isp->fw_work_buf_hdl, stream,
				 (uint64 *)&mem_sys, &mem_addr, &len);

	rd_ptr = isp_hw_reg_read32(rreg);
	wr_ptr = isp_hw_reg_read32(wreg);
	rd_ptr_dbg = rd_ptr;
	wr_ptr_dbg = wr_ptr;

	if (rd_ptr > len) {
		isp_dbg_print_err
	("%s fail %s(%u),rd_ptr %u(should<=%u),wr_ptr %u\n",
			__func__, isp_dbg_get_stream_str(stream),
			stream, rd_ptr, len, wr_ptr);
		return RET_FAILURE;
	};

	if (wr_ptr > len) {
		isp_dbg_print_err
	("%s fail %s(%u),wr_ptr %u(should<=%u), rd_ptr %u\n",
			__func__, isp_dbg_get_stream_str(stream),
			stream, wr_ptr, len, rd_ptr);
		return RET_FAILURE;
	}

	if (rd_ptr < wr_ptr) {
		if ((wr_ptr - rd_ptr) >= (sizeof(Resp_t))) {
			memcpy((uint8 *) response,
				(uint8 *) mem_sys + rd_ptr, sizeof(Resp_t));

			rd_ptr += sizeof(Resp_t);
			if (rd_ptr < len) {
				isp_hw_reg_write32(rreg, rd_ptr);
			} else {
				isp_dbg_print_err
	("%s fail %s(%u),new rd_ptr %u(should<=%u),wr_ptr %u\n",
				__func__, isp_dbg_get_stream_str(stream),
				stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		} else {
			isp_dbg_print_err
	("fail,fw2host ring buffer has sth wrong with wptr and rptr\n");
			return RET_FAILURE;
		}
	} else if (rd_ptr > wr_ptr) {
		uint32 size;
		uint8 *dst;
		uint64 src_addr;

		dst = (uint8 *) response;

		src_addr = mem_addr + rd_ptr;
		size = len - rd_ptr;
		if (size > sizeof(Resp_t)) {
			mem_addr += rd_ptr;
			memcpy((uint8 *) response,
				(uint8 *) (mem_sys) + rd_ptr,
				sizeof(Resp_t));
			rd_ptr += sizeof(Resp_t);
			if (rd_ptr < len)
				isp_hw_reg_write32(rreg, rd_ptr);
			else {
				isp_dbg_print_err
	("%s fail1 %s(%u),new rd_ptr %u(should<=%u),wr_ptr %u\n",
			__func__, isp_dbg_get_stream_str(stream),
			stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		} else {
			if ((size + wr_ptr) < (sizeof(Resp_t))) {
				isp_dbg_print_err
	("fail,fw2host ring buffer has sth wrong with wptr and rptr1\n");
				return RET_FAILURE;
			}

			memcpy(dst, (uint8 *) (mem_sys) + rd_ptr, size);

			dst += size;
			src_addr = mem_addr;
			size = sizeof(Resp_t) - size;
			if (size)
				memcpy(dst, (uint8 *) (mem_sys), size);
			rd_ptr = size;
			if (rd_ptr < len) {
				isp_hw_reg_write32(rreg, rd_ptr);
			} else {
				isp_dbg_print_err
	("%s fail2 %s(%u),new rd_ptr %u(should<=%u),wr_ptr %u\n",
			__func__, isp_dbg_get_stream_str(stream),
			stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		}
	} else/* if (rd_ptr == wr_ptr)*/
		return RET_TIMEOUT;

out:
	checksum = compute_check_sum((uint8 *) response, (sizeof(Resp_t) - 1));
	if (checksum != response->respCheckSum) {
		isp_dbg_print_err
	("fail response checksum error 0x%x,should 0x%x,rdptr %u,wrptr %u\n",
		checksum, response->respCheckSum, rd_ptr_dbg, wr_ptr_dbg);
		isp_dbg_print_err
			("%s(%u), seqNo %u, respId %s(0x%x)\n",
			isp_dbg_get_stream_str(stream), stream,
			response->respSeqNum,
			isp_dbg_get_resp_str(response->respId),
			response->respId);
		return RET_FAILURE;
	}

	return RET_SUCCESS;
};

void isp_get_cmd_buf_regs(enum fw_cmd_resp_stream_id idx,
			uint32 *rreg, uint32 *wreg,
			uint32 *baselo_reg, uint32 *basehi_reg,
			uint32 *size_reg)
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
		isp_dbg_print_err("-><- %s, fail id[%d]\n",
			__func__, idx);
		break;
	};
};

void isp_get_resp_buf_regs(enum fw_cmd_resp_stream_id idx,
			uint32 *rreg, uint32 *wreg,
			uint32 *baselo_reg, uint32 *basehi_reg,
			uint32 *size_reg)
{
	switch (idx) {
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		if (rreg)
			*rreg = mmISP_RB_RPTR5;
		if (wreg)
			*wreg = mmISP_RB_WPTR5;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO5;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI5;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE5;
		break;
	case FW_CMD_RESP_STREAM_ID_1:
		if (rreg)
			*rreg = mmISP_RB_RPTR6;
		if (wreg)
			*wreg = mmISP_RB_WPTR6;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO6;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI6;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE6;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		if (rreg)
			*rreg = mmISP_RB_RPTR7;
		if (wreg)
			*wreg = mmISP_RB_WPTR7;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO7;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI7;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE7;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		if (rreg)
			*rreg = mmISP_RB_RPTR8;
		if (wreg)
			*wreg = mmISP_RB_WPTR8;
		if (baselo_reg)
			*baselo_reg = mmISP_RB_BASE_LO8;
		if (basehi_reg)
			*basehi_reg = mmISP_RB_BASE_HI8;
		if (size_reg)
			*size_reg = mmISP_RB_SIZE8;
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
		isp_dbg_print_err("-><- %s fail idx (%u)\n",
			__func__, idx);
		break;
	};
}

void isp_init_fw_ring_buf(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id idx, uint32 cmd)
{
	uint32 rreg;
	uint32 wreg;
	uint32 baselo_reg;
	uint32 basehi_reg;
	uint32 size_reg;
	uint64 mc;
	uint32 lo;
	uint32 hi;
	uint32 len;

	if (cmd) {		/*command buffer */
		if ((isp == NULL) || (idx > FW_CMD_RESP_STREAM_ID_3)) {
			isp_dbg_print_err
			("-><- %s(%u:cmd) fail,bad para\n",
				__func__, idx);
			return;
		};
		isp_get_cmd_buf_regs(idx, &rreg, &wreg,
				     &baselo_reg, &basehi_reg, &size_reg);
		isp_fw_buf_get_cmd_base(isp->fw_work_buf_hdl, idx,
					NULL, &mc, &len);
	} else {		/*response buffer */
		if ((isp == NULL) || (idx > FW_CMD_RESP_STREAM_ID_3)) {
			isp_dbg_print_err
			("-><- %s(%u:resp) fail,bad para\n",
				__func__, idx);
			return;
		};
		isp_get_resp_buf_regs(idx, &rreg, &wreg,
				&baselo_reg, &basehi_reg, &size_reg);
		isp_fw_buf_get_resp_base(isp->fw_work_buf_hdl, idx,
				NULL, &mc, &len);
	};
	isp_split_addr64(mc, &lo, &hi);
	isp_hw_reg_write32(rreg, 0);
	isp_hw_reg_write32(wreg, 0);
	isp_hw_reg_write32(baselo_reg, lo);
	isp_hw_reg_write32(basehi_reg, hi);
	isp_hw_reg_write32(size_reg, len);
}

/*refer to aidt_api_boot_firmware*/
result_t isp_fw_boot(struct isp_context *isp)
{
	uint64 fw_base_addr;
	uint64 fw_base_sys_addr;
	uint64 stack_base_addr_sys;
	uint64 stack_base_addr;
	uint64 heap_base_addr;
	uint64 fw_rb_log_base;
	uint32 fw_trace_buf_size;
	/*uint32 max_package_buffer_num; */
	/*uint32 max_package_buffer_size; */
	uint32 fw_buf_size;
	uint32 stack_size;
	uint32 heap_size;
	uint32 reg_val;

	uint32 fw_rb_log_base_lo;
	uint32 fw_rb_log_base_hi;
	uint32 i;

	result_t result = RET_SUCCESS;
	uint8 copy_fw_needed = 1;

	isp_time_tick_t start_time;
	isp_time_tick_t end_time;

//	uint64 orig_codebase_sys = 0;
//	uint64 orig_codebase_mc = 0;
//	uint32 orig_codebase_len = 0;

	if (ISP_GET_STATUS(isp) != ISP_STATUS_PWR_ON) {
		isp_dbg_print_err("-><- %s fail bad status %d\n",
				__func__, ISP_GET_STATUS(isp));
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
	fw_rb_log_base_lo = (uint32) (fw_rb_log_base & 0xffffffff);
	fw_rb_log_base_hi = (uint32) (fw_rb_log_base >> 32);

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
	if (copy_fw_needed) {

		memcpy((void *)fw_base_sys_addr, isp->fw_data,
							isp->fw_len);
	}

	if (1 && copy_fw_needed) {
		isp_dbg_print_info
		("fw len %u,aft memcpy from %p to %llx, mc %llx\n",
		isp->fw_len, isp->fw_data, fw_base_sys_addr, fw_base_addr);
	/*isp_dbg_print_info("0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
	 *ch[0], ch[1], ch[2], ch[3], ch[4], ch[5], ch[6], ch[7]);
	 *isp_dbg_print_info("0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
	 *ch[ts - 8], ch[ts - 7], ch[ts - 6], ch[ts - 5],
	 *ch[ts - 4], ch[ts - 3], ch[ts - 2], ch[ts - 1]);
	 */
	};
	// Configure TEXT BASE ADDRESS
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET0_LO,
			(fw_base_addr & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET0_HI,
			((fw_base_addr >> 32) & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE0, fw_buf_size);

	/*configure STACK BASE ADDRESS */
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET1_LO,
			(stack_base_addr & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET1_HI,
			((stack_base_addr >> 32) & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE1, stack_size);

	/*configure HEAP BASE ADDRESS */
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET2_LO,
			(heap_base_addr & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_OFFSET2_HI,
			((heap_base_addr >> 32) & 0xffffffff));
	isp_hw_reg_write32(mmISP_CCPU_CACHE_SIZE2,
			heap_size /* + fw_trace_buf_size */);

	/*Clear CCPU_STATUS */
	isp_hw_reg_write32(mmISP_STATUS, 0x0);

	isp_hw_ccpu_enable(isp);

	/*Wait Firmware Ready(); */
	isp_get_cur_time_tick(&start_time);
	while (1) {
#define CCPU_REPORT_SHIFT (1)
		reg_val = isp_hw_reg_read32(mmISP_STATUS);
		if (reg_val & (1 << CCPU_REPORT_SHIFT))
			break;

		{
			isp_get_cur_time_tick(&end_time);
			if (isp_is_timeout(&start_time, &end_time, 1000)) {
				isp_dbg_print_err
				("-><- %s fail timeout,reg 0x%x\n",
					__func__, reg_val);
				return RET_FAILURE;
			};
			usleep_range(1000, 2000);
		}
	}
	ISP_SET_STATUS(isp, ISP_STATUS_FW_RUNNING);
	isp_dbg_print_err("-><- %s suc\n", __func__);
	return result;
}

result_t isp_fw_log_module_set(struct isp_context *isp, ModId_t id,
				bool enable)
{
	result_t ret;

	if (enable) {
		CmdLogEnableMod_t mod;

		mod.mod = id;
		ret = isp_send_fw_cmd(isp, CMD_ID_LOG_ENABLE_MOD,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT, &mod,
				sizeof(CmdLogEnableMod_t));
	} else {
		CmdLogDisableMod_t mod;

		mod.mod = id;
		ret = isp_send_fw_cmd(isp, CMD_ID_LOG_DISABLE_MOD,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT, &mod,
				sizeof(CmdLogDisableMod_t));
	}
	isp_dbg_print_info("-><- %s,mod:%d,en:%d,ret %d\n",
			__func__, id, enable, ret);
	return ret;
}

result_t isp_fw_log_level_set(struct isp_context *isp, LogLevel_t level)
{
	CmdLogSetLevel_t levl;
	result_t ret;

	levl.level = level;
	ret = isp_send_fw_cmd(isp, CMD_ID_LOG_SET_LEVEL,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_DIRECT, &levl,
			sizeof(CmdLogSetLevel_t));
	isp_dbg_print_info("-><- %s(level:%d),ret %d\n",
			__func__, level, ret);
	return ret;
}

result_t isp_fw_log_mod_ext_set(struct isp_context *isp,
				CmdSetLogModExt_t *modext)
{
	result_t ret = RET_SUCCESS;

	ret = isp_send_fw_cmd(isp, CMD_ID_SET_LOG_MOD_EXT,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_DIRECT, modext,
			sizeof(CmdSetLogModExt_t));
	isp_dbg_print_info
	("-><- %s, A = 0x%x, B = 0x%x, C = 0x%x, D = 0x%x,ret %d\n",
	__func__, modext->level_bits_A, modext->level_bits_B,
	modext->level_bits_C, modext->level_bits_D, ret);
	return ret;
}

result_t isp_fw_get_iso_cap(struct isp_context *isp, enum camera_id cid)
{
	result_t ret;
	enum fw_cmd_resp_stream_id stream_id;

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	ret = isp_send_fw_cmd(isp, CMD_ID_AE_GET_ISO_CAPABILITY, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
	isp_dbg_print_info("-><- %s, ret %d\n", __func__, ret);
	return ret;
};

result_t isp_fw_get_ev_cap(struct isp_context *isp, enum camera_id cid)
{
	result_t ret;
	enum fw_cmd_resp_stream_id stream_id;

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	ret = isp_send_fw_cmd(isp, CMD_ID_AE_GET_EV_CAPABILITY, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
	isp_dbg_print_info("-><- %s, ret %d\n", __func__, ret);
	return ret;
};

/*synchronized version of isp_fw_get_ver*/
result_t isp_fw_get_ver(struct isp_context *isp, uint32 *ver)
{
	result_t ret;

	ver = ver;

	if ((isp == NULL) || (ver == NULL))
		isp_dbg_print_info("-><- %s fail bad para\n", __func__);

	ret = isp_send_fw_cmd(isp, CMD_ID_GET_FW_VERSION,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
	if (ret != RET_SUCCESS) {
		isp_dbg_print_err("-><- %s, fail\n", __func__);
		return ret;
	}

	isp_dbg_print_info("-><- %s suc\n", __func__);

	return ret;
}

result_t isp_fw_get_cproc_status(struct isp_context *isp,
			enum camera_id cam_id, CprocStatus_t *cproc)
{
	result_t ret;
	uint32 len = sizeof(CprocStatus_t);
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cam_id) || (cproc == NULL)) {
		isp_dbg_print_info
			("-><- %s fail bad para\n", __func__);
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	ret = isp_send_fw_cmd_sync(isp, CMD_ID_CPROC_GET_STATUS,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				NULL, 0, 1000 * 10, cproc, &len);
	if (ret != RET_SUCCESS) {
		isp_dbg_print_err
		("-><- %s, cid %u fail\n", __func__, cam_id);
		return ret;
	}
	isp_dbg_print_info
	("-><- %s suc, cid %u,en %i,bright %i, contrast %i,hue %i,satur %i\n",
	__func__, cam_id, cproc->enabled,
	cproc->brightness, cproc->contrast,
	cproc->hue, cproc->saturation);
	return ret;
}

result_t isp_fw_start(struct isp_context *isp)
{
	result_t ret;
	uint32 fw_ver;
	CmdSetLogModExt_t modext;

	isp_dbg_print_info("-> %s\n", __func__);

	if (ISP_GET_STATUS(isp) >= ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_info
			("<- %s, in status %d,do nothing\n",
			__func__, ISP_GET_STATUS(isp));
		return RET_SUCCESS;
	};

	if (ISP_GET_STATUS(isp) == ISP_STATUS_PWR_OFF) {
		isp_dbg_print_err("<- %s fail, no power up\n", __func__);
		return RET_FAILURE;
	}

	if (ISP_GET_STATUS(isp) == ISP_STATUS_PWR_ON) {
		ret = isp_fw_boot(isp);
		if (ret != RET_SUCCESS) {
			isp_dbg_print_err
				("<- %s fail for isp_fw_boot\n", __func__);
			return ret;
		}
	}

	if (isp->drv_settings.fw_log_cfg_en != 0) {
		/*1 - 8 */
		modext.level_bits_A = isp->drv_settings.fw_log_cfg_A;
		/*9 - 16 */
		modext.level_bits_B = isp->drv_settings.fw_log_cfg_B;
		/*17 - 24 */
		modext.level_bits_C = isp->drv_settings.fw_log_cfg_C;
		/*25 - 32 */
		modext.level_bits_D = isp->drv_settings.fw_log_cfg_D;
		/*33 */
		modext.level_bits_E = isp->drv_settings.fw_log_cfg_E;
	} else {
		// default, dsiable all log
		modext.level_bits_A = 0x33333333;	/* 1 - 8 */
		modext.level_bits_B = 0x33333333;	/* 9 - 16 */
		modext.level_bits_C = 0x33333333;	/* 17 - 24 */
		modext.level_bits_D = 0x33333333;	/* 25 - 32 */
		modext.level_bits_E = 0x3;		/* 33 */
	}
	isp_fw_log_mod_ext_set(isp, &modext);
	isp_fw_get_ver(isp, &fw_ver);
	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
};

void isp_fw_stop_all_stream(struct isp_context *isp)
{
	unreferenced_parameter(isp);
}

result_t isp_fw_set_focus_mode(struct isp_context *isp, enum camera_id cam_id,
			       AfMode_t mode)
{
	CmdAfSetMode_t af;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err("-><- %s fail, bad para,cam_id:%d\n",
			__func__, cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	af.mode = mode;
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_MODE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&af, sizeof(af)) != RET_SUCCESS) {
		isp_dbg_print_err
		("-><- %s fail send cmd af mode(%u)\n",
			__func__, af.mode);
		ret = RET_FAILURE;
	} else {
		sif->af_mode = mode;
		isp_dbg_print_info
			("-><- %s set af mode(%u) suc\n",
			__func__, af.mode);
		ret = RET_SUCCESS;
	};
	return ret;
};

result_t isp_fw_start_af(struct isp_context *isp, enum camera_id cam_id)
{
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
		("-><- %s fail, bad para,cam_id:%d\n", __func__, cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp_send_fw_cmd(isp, CMD_ID_AF_TRIG,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err("-><- %s fail send cmd mode\n", __func__);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info("-><- %s set suc\n", __func__);
		ret = RET_SUCCESS;
	};
	return ret;
}

result_t isp_fw_3a_lock(struct isp_context *isp, enum camera_id cam_id,
			enum isp_3a_type type, LockType_t lock_mode)
{
	uint32 cmd;
	CmdAeLock_t lock_type;
	char *str_type;
	char *str_lock;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

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
		isp_dbg_print_info
			("-><- %s fail 3a type %d\n", __func__, type);
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
		isp_dbg_print_info
		("-><- %s fail lock type %d\n", __func__, lock_mode);
		return RET_FAILURE;
	};
	lock_type.lockType = lock_mode;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp_send_fw_cmd(isp, cmd,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&lock_type, sizeof(lock_type)) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s,cid %d %s %s fail send cmd\n",
			__func__, cam_id, str_type, str_lock);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info
			("-><- %s,cid %d %s %s,suc\n",
			__func__, cam_id, str_type, str_lock);
		ret = RET_SUCCESS;
	};
	return ret;
};

result_t isp_fw_3a_unlock(struct isp_context *isp, enum camera_id cam_id,
			enum isp_3a_type type)
{
	uint32 cmd;
	char *str_type;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

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
		isp_dbg_print_info
			("-><- %s fail 3a type %d\n", __func__, type);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp_send_fw_cmd(isp, cmd,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s,cid %d %s fail send cmd\n",
			__func__, cam_id, str_type);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info
			("-><- %s,cid %d %s ,suc\n",
			__func__, cam_id, str_type);
		ret = RET_SUCCESS;
	};
	return ret;
};

result_t isp_fw_set_lens_pos(struct isp_context *isp, enum camera_id cam_id,
			uint32 pos)
{
	CmdAfSetLensPos_t lens_pos;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	lens_pos.lensPos = pos;
	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_LENS_POS,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&lens_pos, sizeof(lens_pos)) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s,cid %d pos %d fail\n",
			__func__, cam_id, pos);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info
			("-><- %s,cid %d pos %d suc\n",
			__func__, cam_id, pos);
		ret = RET_SUCCESS;
	};
	return ret;
}

result_t isp_fw_set_iso(struct isp_context *isp, enum camera_id cam_id,
			AeIso_t iso)
{
	enum fw_cmd_resp_stream_id stream_id;
	CmdAeSetIso_t iso_val;
	result_t ret;

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	iso_val.iso = iso;
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_ISO,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&iso_val, sizeof(iso_val)) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s,cid %d,iso %u fail\n",
			__func__, cam_id, iso.iso);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info
			("-><- %s,cid %d,iso %u suc\n",
			__func__, cam_id, iso.iso);
		ret = RET_SUCCESS;
	};
	return ret;
}

result_t isp_fw_set_lens_range(struct isp_context *isp, enum camera_id cid,
			       uint32 near_value, uint32 far_value)
{
	CmdAfSetLensRange_t lens_range;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	lens_range.lensRange.minLens = near_value;
	lens_range.lensRange.maxLens = far_value;
	lens_range.lensRange.stepLens = FOCUS_STEP_VALUE;
	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_LENS_RANGE,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&lens_range, sizeof(lens_range)) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s,cid %d [%d %d] fail\n",
			__func__, cid, near_value, far_value);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info
			("-><- %s,cid %d [%d %d] suc\n",
			__func__, cid, near_value, far_value);
		ret = RET_SUCCESS;
	};
	return ret;
}

result_t isp_fw_set_exposure_mode(struct isp_context *isp,
				  enum camera_id cam_id, AeMode_t mode)
{
	CmdAeSetMode_t ae;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
		("-><- %s fail, bad para,cam_id:%d\n",
			__func__, cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	ae.mode = mode;
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_MODE,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &ae, sizeof(ae)) != RET_SUCCESS) {
		isp_dbg_print_err
		("-><- %s fail set ae mode(%u)\n",
			__func__, ae.mode);
		ret = RET_FAILURE;
	} else {
		sif->aec_mode = mode;
		isp_dbg_print_info
		("-><- %s set ae mode(%u) suc\n",
			__func__, ae.mode);
		ret = RET_SUCCESS;
	};
	return ret;
};

result_t isp_fw_set_wb_mode(struct isp_context *isp, enum camera_id cam_id,
			AwbMode_t mode)
{
	CmdAwbSetMode_t awb;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail, bad para,cam_id:%d\n",
			__func__, cam_id);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	awb.mode = mode;
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_MODE,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &awb, sizeof(awb)) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s fail set awb mode(%u)\n",
			__func__, awb.mode);
		ret = RET_FAILURE;
	} else {
		sif->awb_mode = mode;
		isp_dbg_print_info
			("-><- %s set awb mode(%u) suc\n",
			__func__, awb.mode);
		ret = RET_SUCCESS;
	};
	return ret;
};

result_t isp_fw_set_snr_ana_gain(struct isp_context *isp,
				 enum camera_id cam_id, uint32 ana_gain)
{
	CmdAeSetAnalogGain_t g;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
		("-><- %s fail, bad para,cam_id:%d\n",
			__func__, cam_id);
		return RET_FAILURE;
	};
	g.aGain = ana_gain;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_ANALOG_GAIN,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &g, sizeof(g)) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s fail set again(%u)\n",
			__func__, ana_gain);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info
			("-><- %s set again(%u) suc\n",
			__func__, ana_gain);
		ret = RET_SUCCESS;
	};
	return ret;
};

result_t isp_fw_set_snr_dig_gain(struct isp_context *isp,
				 enum camera_id cam_id, uint32 dig_gain)
{
	unreferenced_parameter(isp);
	unreferenced_parameter(cam_id);
	unreferenced_parameter(dig_gain);
	return IMF_RET_FAIL;
	/*todo  to be added later */
};

result_t isp_fw_set_itime(struct isp_context *isp, enum camera_id cam_id,
			uint32 itime)
{
	CmdAeSetItime_t g;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail, bad para,cam_id:%d\n",
			__func__, cam_id);
		return RET_FAILURE;
	};
	g.itime = itime;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_ITIME,
					 stream_id, FW_CMD_PARA_TYPE_DIRECT,
					 &g, sizeof(g)) != RET_SUCCESS) {
		isp_dbg_print_err("-><- %s fail set itime(%u)\n",
				  __func__, itime);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info
		    ("-><- %s set itime(%u) suc\n", __func__, itime);
		ret = RET_SUCCESS;
	};
	return ret;
};

result_t isp_fw_send_meta_data_buf(struct isp_context *isp,
				   enum camera_id cam_id,
				   struct isp_meta_data_info *info)
{
	unreferenced_parameter(isp);
	unreferenced_parameter(cam_id);
	unreferenced_parameter(info);
	return IMF_RET_FAIL;
	/*todo  to be added later */
};

result_t isp_fw_send_all_meta_data(struct isp_context *isp)
{
	unreferenced_parameter(isp);

	return IMF_RET_FAIL;
	/*todo  to be added later */
};

result_t isp_fw_set_script(struct isp_context *isp,
			CmdSetDeviceScript_t *script_cmd)
{
	result_t result;

	if ((isp == NULL) || (script_cmd == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for bad para\n", __func__);
		return RET_FAILURE;
	};

	result = isp_send_fw_cmd(isp, CMD_ID_SET_DEVICE_SCRIPT,
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			FW_CMD_PARA_TYPE_INDIRECT, script_cmd,
			sizeof(CmdSetDeviceScript_t));
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s, fail for send script cmd\n", __func__);
		return RET_FAILURE;
	}

	result = isp_send_fw_cmd(isp, CMD_ID_ENABLE_DEVICE_SCRIPT_CONTROL,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s, fail for send enable cmd\n", __func__);
		return RET_FAILURE;
	}

	return RET_SUCCESS;
}

uint32 l_rd_ptr;
uint32 l_wr_ptr;
uint32 isp_fw_get_fw_rb_log(struct isp_context *isp, uint8 *buf)
{
	uint32_t rd_ptr, wr_ptr;
	uint32_t total_cnt;
	uint32_t cnt;
	uint64_t mem_addr;
	uint64 sys;
	uint32 rb_size;
	uint32 offset = 0;
	static int calculate_cpy_time;

	total_cnt = 0;
	cnt = 0;
	isp_fw_buf_get_trace_base(isp->fw_work_buf_hdl, &sys, &mem_addr,
				&rb_size);
	if (rb_size == 0)
		return 0;
	if (calculate_cpy_time) {
		isp_time_tick_t bef;
		isp_time_tick_t cur;
		uint32 cal_cnt;

		calculate_cpy_time = 0;
		isp_get_cur_time_tick(&bef);
		for (cal_cnt = 0; cal_cnt < 10; cal_cnt++)
			memcpy(buf, (uint8 *)(sys), ISP_FW_TRACE_BUF_SIZE);

		isp_get_cur_time_tick(&cur);
		cur -= bef;
		bef = (isp_time_tick_t)((ISP_FW_TRACE_BUF_SIZE * 10) / 1024);
		cur = bef * 10000000 / cur;
		isp_dbg_print_err("memcpy speed %lluK/S\n", cur);
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
		isp_dbg_print_err("fail fw log size %u\n", cnt);
		return 0;
	}

	memcpy(buf + offset, (uint8 *) (sys + rd_ptr), cnt);

	offset += cnt;
	total_cnt += cnt;
	rd_ptr = (rd_ptr + cnt) % rb_size;
	if (rd_ptr < wr_ptr)
		goto goon;
	isp_hw_reg_write32(FW_LOG_RB_RPTR_REG, rd_ptr);

	return total_cnt;
}

result_t isp_fw_set_gamma(struct isp_context *isp, uint32 cam_id, int32 gamma)
{
	result_t result = RET_SUCCESS;
	CmdGammaSetCurve_t curv;
	enum fw_cmd_resp_stream_id stream_id;

	if (is_para_legal(isp, cam_id)) {
		isp_dbg_print_err("-><- %s fail for para isp:%p,cam:%d\n",
			__func__, isp, cam_id);
		return RET_FAILURE;
	}

	if ((gamma < 0) || (gamma > 499)) {
		isp_dbg_print_err
		("-><- %s fail for bad gamma %d\n", __func__, gamma);
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
		isp_dbg_print_err
			("-><- %s fail for send gam cmd\n", __func__);
		return RET_FAILURE;
	}
	isp_dbg_print_info("-><- %s suc\n", __func__);
	return result;
};

result_t isp_fw_send_metadata_buf(struct isp_context *isp,
			enum camera_id cam_id,
			struct isp_mapped_buf_info *buffer)
{
	unreferenced_parameter(isp);
	unreferenced_parameter(cam_id);
	unreferenced_parameter(buffer);
	return IMF_RET_SUCCESS;//jjj TBD
	/*todo  to be added later */
}

// This function must be called before whenever there is a iclk change.
// change iclk message to SMU must be issued
// after we got the response of this FW command.
result_t isp_set_clk_info_2_fw(struct isp_context *isp)
{
	CmdSetClkInfo_t clk_info;
	result_t ret;

	if (isp->clk_info_set_2_fw) {
		isp_dbg_print_info
			("-><- %s, suc do none\n", __func__);
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
		isp_dbg_print_err("-><- %s fail\n", __func__);
		ret = RET_FAILURE;
	} else {
		isp->clk_info_set_2_fw = 1;
		isp_dbg_print_info
			("-><- %s suc,s:%u,i:%u,ref:%u\n",
			__func__, isp->sclk, isp->iclk, isp->refclk);
		ret = RET_SUCCESS;
	};
	return ret;
};

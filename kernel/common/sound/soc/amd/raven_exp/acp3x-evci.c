/*
 * EV command interace file for ACP PCI Driver
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "acp3x-evci.h"

void acp_copy_to_scratch_memory(unsigned char *ring_buffer,
				u32  offset,
				void *write_buffer,
				u32 bytes_to_write)
{
	int index;
	unsigned int *src_buf = (unsigned int *)write_buffer;
	unsigned int *dest_buf = (unsigned int *)ring_buffer;

	for (index = 0; index < (bytes_to_write / 4); index++)
		writel(*(src_buf + index), (u32 *)(dest_buf + (offset / 4) +
		      index));
}

void acp_copy_from_scratch_memory(unsigned char *ring_buffer,
				  u32  offset,
				  void *read_buffer,
				  u32 bytes_to_write)
{
	int index;
	int val = 0;
	unsigned int *src_buf = (unsigned int *)(ring_buffer + offset);
	unsigned int *dst_buf = (unsigned int *)read_buffer;

	for (index = 0; index < (bytes_to_write / 4); index++) {
		val = readl((u32 *)(src_buf + index));
		*(dst_buf + index) = val;
	}
}

static unsigned int get_data_size_avail(struct acp_ring_buffer_info *buf_info)
{
	unsigned int available_buffer_length = 0;
	unsigned int read_index = 0;
	unsigned int  write_index = 0;
	unsigned int buffer_size;

	if (buf_info) {
		read_index = readl((void __iomem *)&buf_info->read_index);
		write_index = readl((void __iomem *)&buf_info->write_index);
		buffer_size = readl((void __iomem *)&buf_info->size);
		if (write_index >= read_index)
			available_buffer_length = write_index - read_index;
		else if (write_index < read_index)
			available_buffer_length = buffer_size -
						  (read_index - write_index);
	}
	return available_buffer_length;
}

static unsigned int peek_data(struct acp_ring_buffer_info *resp_buff_info,
			      unsigned char *resp_ringbuf,
			      unsigned int bytes_to_read,
			      void *read_buffer)
{
	/* Read the data from the Address in BufferDesc into ReadBuffer of
	 * size bytes_to_read
	 */
	unsigned int  bytes_available_to_read = 0;
	unsigned int end_offset = 0;
	unsigned int temp_length = 0;
	unsigned int read_index = 0;
	unsigned int write_index = 0;
	unsigned int size = 0;

	if ((resp_buff_info) && (resp_ringbuf) && (bytes_to_read) &&
	    (read_buffer)) {
		read_index = readl((unsigned int *)&resp_buff_info->read_index);
		write_index = readl((unsigned int *)
				    &resp_buff_info->write_index);
		size = readl((unsigned int *)&resp_buff_info->size);
		if (read_index > size || write_index > size)
			return 0;
		bytes_available_to_read = get_data_size_avail(resp_buff_info);
		if (bytes_available_to_read >= bytes_to_read) {
			bytes_available_to_read = bytes_to_read;
			end_offset = size - 1;
			if ((read_index + bytes_to_read) <= end_offset) {
				acp_copy_from_scratch_memory((unsigned char *)
							     resp_ringbuf,
							     read_index,
							     read_buffer,
							     bytes_to_read);
			} else {
				temp_length = end_offset - read_index + 1;
				acp_copy_from_scratch_memory((unsigned char *)
							     resp_ringbuf,
							     read_index,
							     read_buffer,
							     temp_length);
				read_index = 0;
				acp_copy_from_scratch_memory((unsigned char  *)
							     resp_ringbuf,
							     read_index,
							     ((unsigned char *)
							     read_buffer +
							     temp_length),
							     (bytes_to_read -
							     temp_length));
			}
		} else {
			bytes_available_to_read = 0;
		}
	}
	return bytes_available_to_read;
}

static unsigned int get_free_space_avil(struct acp_ring_buffer_info *buf_info)
{
	unsigned int available_buffer_length = 0;
	unsigned int read_index = 0;
	unsigned int write_index = 0;
	unsigned int buffer_size;

	if (buf_info) {
		read_index = readl((void __iomem *)&buf_info->read_index);
		write_index = readl((void __iomem *)&buf_info->write_index);
		buffer_size = readl((void __iomem *)&buf_info->size);
		if (write_index >= read_index)
			available_buffer_length = buffer_size -
						  (write_index - read_index +
						  AOS_RING_OFFSET_DIFFERENCE);
		else if (write_index < read_index)
			available_buffer_length = read_index -
						  (write_index +
						  AOS_RING_OFFSET_DIFFERENCE);
	}
	return available_buffer_length;
}

unsigned int get_next_command_size(struct acp_ring_buffer_info *resp_buf_info,
				   unsigned char *resp_ring_buf)
{
	unsigned int command_size = 0;
	struct acpev_command acp_ev_cmd = { 0 };

	if ((resp_buf_info) && (resp_ring_buf)) {
		command_size = peek_data(resp_buf_info, resp_ring_buf,
					 sizeof(struct acpev_command),
						&acp_ev_cmd);
		if (command_size) {
			command_size = acp_ev_cmd.cmd_size;
			command_size += sizeof(struct acpev_command);
		} else {
			command_size = 0;
		}
	}
	return command_size;
}

unsigned int read_buffer(struct acp_ring_buffer_info *buf_info,
			 unsigned char *ring_buffer,
			 unsigned int bytes_to_read, void *read_buffer)
{
	/* Read the data from the Address in BufferDesc into read_buffer of size
	 * bytes_to_read
	 */
	unsigned int bytes_avil = 0;
	u32 end_offset = 0;
	u32 temp_length = 0;
	u32 read_index = 0;

	if ((buf_info) && (ring_buffer) && (bytes_to_read) &&
	    (read_buffer)) {
		bytes_avil = get_data_size_avail(buf_info);
		if (bytes_avil > 0) {
			bytes_avil = (bytes_avil <
						  bytes_to_read) ?
						  bytes_avil :
						  bytes_to_read;
			end_offset = readl((void __iomem *)&buf_info->size);
			end_offset = end_offset - 1;
			read_index = readl((void __iomem *)
					   &buf_info->read_index);
			if ((read_index + bytes_avil) <= end_offset) {
				acp_copy_from_scratch_memory((unsigned char  *)
							     ring_buffer,
							     read_index,
							     read_buffer,
							     bytes_avil);
				read_index += bytes_avil;
			} else  {
				temp_length = end_offset - read_index + 1;
				acp_copy_from_scratch_memory((unsigned char  *)
							     ring_buffer,
							     read_index,
							     read_buffer,
							     temp_length);
				read_index = 0;
				acp_copy_from_scratch_memory((unsigned char  *)
							     ring_buffer,
							     read_index,
							     ((unsigned char *)
							     read_buffer
							     + temp_length),
							     (bytes_avil -
							     temp_length));
				read_index += (bytes_avil -
					       temp_length);
			}
		} else {
			bytes_avil = 0;
		}
		writel(read_index, (void __iomem *)&buf_info->read_index);
	}
	return bytes_avil;
}

unsigned int write_buffer(struct acp_ring_buffer_info *buffer_info,
			  unsigned char *ring_buffer,
			  unsigned int bytes_to_write,
			  void *write_buffer)
{
	/* Write the data from the write_buffer to the Address in BufferDesc of
	 * size bytes_to_write
	 */
	u32 available_bytes = 0;
	u32 end_offset = 0;
	u32 current_write_index = 0;
	u32 remain_bytes = 0;

	if ((buffer_info) && (ring_buffer) && (write_buffer) &&
	    (bytes_to_write)) {
		available_bytes = get_free_space_avil(buffer_info);
		if (available_bytes && available_bytes >= bytes_to_write) {
			end_offset  = readl((void __iomem *)&buffer_info->size);
			end_offset = end_offset - 1;

			current_write_index = readl((void __iomem *)
					    &buffer_info->current_write_index);
			if ((current_write_index + bytes_to_write) <=
			    end_offset) {
				acp_copy_to_scratch_memory((unsigned char *)
							   ring_buffer,
							   current_write_index,
							   write_buffer,
							   bytes_to_write);
				current_write_index += bytes_to_write;
			} else {
				remain_bytes = (end_offset -
						 current_write_index + 1);
				acp_copy_to_scratch_memory((unsigned char  *)
							   ring_buffer,
							   current_write_index,
							   write_buffer,
							   remain_bytes);
				current_write_index = 0;
				acp_copy_to_scratch_memory((unsigned char  *)
							   ring_buffer,
							   current_write_index,
							   ((unsigned char *)
							   write_buffer +
							   remain_bytes),
							   (bytes_to_write -
							   remain_bytes));
				current_write_index += (bytes_to_write -
							remain_bytes);
			}
			writel(current_write_index, (void __iomem *)
			       &buffer_info->current_write_index);
			available_bytes = bytes_to_write;
		} else {
			available_bytes = 0;
		}
	}
	return available_bytes;
}

unsigned int read_response(struct acp_ring_buffer_info *resp_buffer_info,
			   unsigned char *resp_ring_buffer,
			   void *readbuffer, unsigned int buffer_size)
{
	unsigned int  cmd_resp_length = 0;

	if ((resp_buffer_info) && (resp_ring_buffer) && (readbuffer) &&
	    (buffer_size)) {
		cmd_resp_length = get_next_command_size(resp_buffer_info,
							resp_ring_buffer);
		if (buffer_size >= cmd_resp_length)
			cmd_resp_length = read_buffer(resp_buffer_info,
						      resp_ring_buffer,
						      cmd_resp_length,
						      readbuffer);
		else
			cmd_resp_length = 0;
	} else {
		cmd_resp_length = 0;
	}

	return cmd_resp_length;
}

void update_write_index(struct acp_ring_buffer_info *buffer_info)
{
	u32 current_write_index = 0;

	current_write_index = readl((void __iomem *)
				    &buffer_info->current_write_index);
	writel(current_write_index, (void __iomem *)&buffer_info->write_index);
}

int acp_get_cmd_number(struct acp3x_dev_data *adata)
{
	int count = 0;

	if (adata->cmd_number ==  0xffffffff)
		adata->cmd_number = 0;
	else
		adata->cmd_number += 1;
	count  = adata->cmd_number;
	return count;
}

int acp_get_next_cmd_number(struct acp3x_dev_data *adata)
{
	int count = 0;

	if (adata->cmd_number != 0xffffffff)
		count = adata->cmd_number + 1;
	return count;
}

bool acp_remove_responses(struct acp3x_dev_data *adata,
			  bool remove_all,
			  struct acp_response *err_response,
			  bool *pending_responses)
{
	u32     index = 0;
	bool    error_found = false;
	int resp_stat;
	struct acp_response *acp_resp;

	if (pending_responses)
		*pending_responses = false;

	for (index = 0; index < ACP_RESPONSE_LIST_SIZE; index++) {
		acp_resp = &adata->resp_list.resp[index];
		if (acp_resp->command_id) {
			resp_stat = acp_resp->response_status;
			if (!error_found && resp_stat > 0) {
				error_found = true;
				pr_err("%s error resp found\n", __func__);
				if (err_response) {
					*err_response =
					adata->resp_list.resp[index];
					pr_err("err_response is set\n");
				}
			}
			if (resp_stat == -1) {
				/* response is still pending */
				if (pending_responses)
					*pending_responses = true;
				if (remove_all)
					acp_resp->command_id = 0;
			} else {
				pr_debug("%s response removed successfully\n",
					 __func__);
				acp_resp->command_id = 0;
			}
		}
	}
	return error_found;
}

int acp_add_response(unsigned int cmd_id, unsigned int cmd_number,
		     struct acp3x_dev_data *adata, unsigned int *p_event)
{
	unsigned int      index;
	unsigned int      initial_list_index;
	int      status = -1;
	struct acp_response *acp_resp;

	index = adata->resp_list.last_resp_write_index + 1;
	if (index >= ACP_RESPONSE_LIST_SIZE)
		index = 0;
	initial_list_index = index;

	for (;;) {
		acp_resp = &adata->resp_list.resp[index];
		if (acp_resp->command_id == 0) {
			acp_resp->command_number = cmd_number;
			acp_resp->response_status = -1;
			acp_resp->command_id = cmd_id;
			acp_resp->p_event = p_event;
			adata->resp_list.last_resp_write_index = index;
			status = 0;
			break;
		}
		index++;
		if (index >= ACP_RESPONSE_LIST_SIZE)
			index = 0;
		if (index == initial_list_index)
			break;
	}
	return status;
}

bool acp_record_response(struct acp3x_dev_data *adata,
			 unsigned int command_number,
			 unsigned int response_status)
{
	unsigned int      list_index;
	unsigned int      initial_list_index;
	bool         found = false;
	struct acp_response *acp_resp;

	list_index = adata->resp_list.last_resp_update_index + 1;
	if (list_index >= ACP_RESPONSE_LIST_SIZE)
		list_index = 0;
	initial_list_index = list_index;

	for (;;) {
		acp_resp = &adata->resp_list.resp[list_index];
		if (acp_resp->command_id) {
			if (acp_resp->command_number == command_number) {
				if (acp_resp->p_event) {
					pr_debug("p_event set\n");
					adata->evresp_event = 0x01;
					wake_up_interruptible(&adata->ev_event);
				}
				acp_resp->response_status = response_status;
				adata->resp_list.last_resp_update_index =
								list_index;
				found = true;
				break;
			}
		}
		list_index++;
		if (list_index >= ACP_RESPONSE_LIST_SIZE)
			list_index = 0;
		if (list_index == initial_list_index)
			break;
	}
	return found;
}

int  ev_wait_for_buffer_empty(struct acp_ring_buffer_info *cmd_buffer_info,
			      unsigned int cmd_length,
			      struct acp3x_dev_data *adata)
{
	unsigned int       bytes_avail;
	unsigned int       delay_count = 0;
	unsigned int       delay_limit;
	int                 status = 0;
	unsigned long jiffies = msecs_to_jiffies(5);
	u32 *resp_event;

	delay_limit = adata->ev_command_wait_time;
	for (;;) {
		resp_event = &adata->response_wait_event;
		bytes_avail = get_free_space_avil(cmd_buffer_info);
		if (bytes_avail < cmd_length) {
			status =
			wait_event_interruptible_timeout(adata->evresp,
							 *resp_event != 0,
							 jiffies);
			if (status < 0)
				pr_debug("error -ERESTARTSYS occurred\n");
			*resp_event  = 0;
			if (!status) {
				pr_err("%s timeout occurred\n", __func__);
				delay_count++;
				if (delay_count > delay_limit) {
					pr_err("Command buffer empty timeout\n");
					break;
				}
				mdelay(5);
			}
		} else {
			status  = 0;
			break;
		}
	}
	return status;
}

bool frame_command(struct acp_ring_buffer_info *cmd_ring_buffer_info,
		   unsigned char *cmd_ring_buffer,
		   void *write_cmd_buffer,
		   struct acp3x_dev_data *adata)
{
	struct acpev_command *acp_ev_command_id = (struct acpev_command *)
						   write_cmd_buffer;
	unsigned int  acpev_cmd_id  = acp_ev_command_id->cmd_id;
	void *actual_command_buffer = (void *)((unsigned char *)
				       write_cmd_buffer +
				       sizeof(struct acpev_command));
	unsigned short size = 0;
	unsigned int cmd_num = 0;

	switch (acpev_cmd_id) {
	case ACP_EV_CMD_TEST_EV1:
	{
		struct acpev_cmd_test_ev1 *cmd_buffer =
						(struct acpev_cmd_test_ev1 *)
						 actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size = sizeof(struct acpev_cmd_test_ev1);
		size = acp_ev_command_id->cmd_size +
		       sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_GET_VER_INFO:
	{
		struct acpev_cmd_get_ver_info *cmd_buffer =
					(struct acpev_cmd_get_ver_info *)
					 actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
					sizeof(struct acpev_cmd_get_ver_info);
		size = acp_ev_command_id->cmd_size +
		       sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_SET_LOG_BUFFER:
	{
		struct acpev_cmd_set_log_buf *cmd_buffer =
					(struct acpev_cmd_set_log_buf *)
					 actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
					sizeof(struct acpev_cmd_set_log_buf);
		size = acp_ev_command_id->cmd_size +
		       sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_SET_CLOCK_FREQUENCY:
	{
		struct acpev_cmd_set_clock_frequency *cmd_buffer =
				(struct acpev_cmd_set_clock_frequency *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_set_clock_frequency);
		size = acp_ev_command_id->cmd_size +
		       sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_START_DMA_FRM_SYS_MEM:
	{
		struct acpev_cmd_start_dma_frm_sys_mem *cmd_buffer =
				(struct acpev_cmd_start_dma_frm_sys_mem  *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_start_dma_frm_sys_mem);
		size = acp_ev_command_id->cmd_size +
				sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_STOP_DMA_FRM_SYS_MEM:
	{
		struct acpev_cmd_stop_dma_frm_sys_mem *cmd_buffer =
				(struct acpev_cmd_stop_dma_frm_sys_mem *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_stop_dma_frm_sys_mem);
		size = acp_ev_command_id->cmd_size +
			sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_START_DMA_TO_SYS_MEM:
	{
		struct acpev_cmd_start_dma_to_sys_mem *cmd_buffer =
				(struct acpev_cmd_start_dma_to_sys_mem *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_start_dma_to_sys_mem);
		size = acp_ev_command_id->cmd_size +
			sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_STOP_DMA_TO_SYS_MEM:
	{
		struct acpev_cmd_stop_dma_to_sys_mem *cmd_buffer =
				(struct acpev_cmd_stop_dma_to_sys_mem *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_stop_dma_to_sys_mem);
		size = acp_ev_command_id->cmd_size +
			sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_I2S_DMA_START:
	{
		struct acpev_cmd_i2s_dma_start *cmd_buffer =
				(struct acpev_cmd_i2s_dma_start *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_i2s_dma_start);
		size = acp_ev_command_id->cmd_size +
			sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_I2S_DMA_STOP:
	{
		struct acpev_cmd_i2s_dma_stop *cmd_buffer =
				(struct acpev_cmd_i2s_dma_stop *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_i2s_dma_stop);
		size = acp_ev_command_id->cmd_size +
			sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_START_TIMER:
	{
		struct acpev_cmd_start_timer *cmd_buffer =
				(struct acpev_cmd_start_timer *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_start_timer);
		size = acp_ev_command_id->cmd_size +
			sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_STOP_TIMER:
	{
		struct acpev_cmd_stop_timer *cmd_buffer =
				(struct acpev_cmd_stop_timer *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_stop_timer);
		size = acp_ev_command_id->cmd_size +
				sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_MEMORY_PG_SHUTDOWN_ENABLE:
	{
		struct acpev_cmd_memory_pg_shutdown *cmd_buffer =
				(struct acpev_cmd_memory_pg_shutdown *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_memory_pg_shutdown);
		size = acp_ev_command_id->cmd_size +
				sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_MEMORY_PG_SHUTDOWN_DISABLE:
	{
		struct acpev_cmd_memory_pg_shutdown *cmd_buffer =
				(struct acpev_cmd_memory_pg_shutdown *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_memory_pg_shutdown);
		size = acp_ev_command_id->cmd_size +
				sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_MEMORY_PG_DEEPSLEEP_ON:
	{
		struct acpev_cmd_memory_pg_deepsleep *cmd_buffer =
				(struct acpev_cmd_memory_pg_deepsleep *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_memory_pg_deepsleep);
		size = acp_ev_command_id->cmd_size +
				sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	case ACP_EV_CMD_MEMORY_PG_DEEPSLEEP_OFF:
	{
		struct acpev_cmd_memory_pg_deepsleep *cmd_buffer =
				(struct acpev_cmd_memory_pg_deepsleep *)
				actual_command_buffer;
		cmd_buffer->cmd_number = acp_get_cmd_number(adata);
		cmd_num = cmd_buffer->cmd_number;
		acp_ev_command_id->cmd_size =
				sizeof(struct acpev_cmd_memory_pg_deepsleep);
		size = acp_ev_command_id->cmd_size +
				sizeof(struct acpev_command);
		write_buffer(cmd_ring_buffer_info, cmd_ring_buffer, size,
			     write_cmd_buffer);
	}
	break;
	default:
	break;
	}
	update_write_index(cmd_ring_buffer_info);
	return true;
}

int  ev_send_cmd(void *ev_cmd, u32 ev_cmd_length, struct acp3x_dev_data *adata,
		 unsigned int *p_event)
{
	u32  bytes_available = 0;
	struct acp_ring_buffer_info     *cmd_buf_info;
	struct acp_ring_buffer_info     *resp_buf_info;
	struct acpev_command         *acpev_cmd;
	u32 next_cmd = 0;
	u32 cmd_id = 0;
	int  ret = 0;
	u32  delay_count = 0;
	bool first = false;
	u32  poll_count = 0;
	unsigned long jiffies = msecs_to_jiffies(5);
	unsigned int *ev;
	wait_queue_head_t *res;

	if (!adata->is_acp_active)
		return ret = STATUS_DEVICE_NOT_READY;
	mutex_lock(&adata->acp_ev_cmd_mutex);
	acpev_cmd = (struct acpev_command *)ev_cmd;
	next_cmd = acp_get_next_cmd_number(adata);
	cmd_id = acpev_cmd->cmd_id;
	pr_debug("%s cmd_number:0x%x cmd_id:0x%x cmd_length:%d\n", __func__,
		 next_cmd, cmd_id, ev_cmd_length);

	if (ret != STATUS_DEVICE_NOT_READY) {
		cmd_buf_info = &adata->acp_sregmap->acp_cmd_ring_buf_info;
		resp_buf_info = &adata->acp_sregmap->acp_resp_ring_buf_info;
		bytes_available = get_free_space_avil(cmd_buf_info);
		if (ev_cmd_length > bytes_available) {
			/* Not enough space is available.
			 * Wait for buffer to drain
			 */
			ret = ev_wait_for_buffer_empty(cmd_buf_info,
						       ev_cmd_length,
						       adata);
		}
		if (ret == STATUS_SUCCESS) {
			acpev_cmd = (struct acpev_command *)ev_cmd;
			delay_count = adata->ev_command_wait_time;
			first = true;
			while (1) {
				ret =
				acp_add_response(acpev_cmd->cmd_id,
						 acp_get_next_cmd_number(adata),
						 adata, p_event);
				if (ret == 0)
					break;
				if (first) {
					pr_debug("waiting for space in response list\n");
					first = false;
				}
				ev = &adata->response_wait_event;
				res = &adata->evresp;
				ret = wait_event_interruptible_timeout(*res,
								       *ev != 0,
								       jiffies);
				if (ret < 0)
					pr_err("%s - error from waitqueue\n",
					       __func__);
				adata->response_wait_event  = 0;

				/* Response list is full, check status from
				 * current responses remove them. Only remove
				 * from list if response has been received
				 */
				acp_remove_responses(adata, false,
						     &adata->command_response,
						     NULL);
				if (!ret) {
					poll_count++;
					if (poll_count > delay_count) {
						pr_err("timeout while waiting for space in resp list\n");
						break;
					}
				}
			}
			/* Write this command into the current packet */
			frame_command(cmd_buf_info,
				      adata->acp_sregmap->acp_cmd_buf,
				      ev_cmd, adata);
			acpbus_trigger_host_to_dsp_swintr(adata);
		}
	}
	mutex_unlock(&adata->acp_ev_cmd_mutex);
	return ret;
}

void ev_record_version_info(struct acpev_resp_version *version,
			    struct acp3x_dev_data *adata)
{
	pr_debug("%s- version info %04x\n", __func__, version->version_id);
}

void ev_process_response(unsigned char *packet, unsigned long packet_size,
			 struct acp3x_dev_data *adata)
{
	struct acpev_command  *acp_ev_command;

	acp_ev_command = (struct acpev_command *)packet;
	if (acp_ev_command) {
		switch (acp_ev_command->cmd_id) {
		case ACP_EV_CMD_RESP_STATUS:
		{
			struct acpev_resp_status *response_status =
						(struct acpev_resp_status *)
						(packet +
						 sizeof(struct acpev_command));

			acp_record_response(adata, response_status->cmd_number,
					    response_status->status);
			pr_debug("%s cmd Number:0x%x, Resp Id:%d, status:%d",
				 __func__,
				response_status->cmd_number,
				acp_ev_command->cmd_id,
				response_status->status);
			if (response_status->status)
				pr_debug("resp error, cmd Id 0x%x, status %d\n",
					 response_status->cmd_number,
					 response_status->status);
		}
		break;
		case ACP_EV_CMD_RESP_VERSION:
		{
			struct acpev_resp_version *version =
						(struct acpev_resp_version *)
						(packet +
						 sizeof(struct acpev_command));
			acp_record_response(adata, version->cmd_number, 0);
			pr_debug("%s cmd Number:0x%x, Resp Id:%d\n",
				 __func__, version->cmd_number,
				 acp_ev_command->cmd_id);
			ev_record_version_info(version, adata);
		}
		break;
		}
	}
}

void acp_get_ev_cmd_info(void  *ev_cmd, unsigned int *cmd_num,
			 unsigned short *acp_ev_cmd_id)
{
	struct acpev_command *acp_ev_command_id;
	void *actual_cmd_buf;

	if (ev_cmd) {
		acp_ev_command_id = (struct acpev_command *)ev_cmd;
		actual_cmd_buf = (void *)((unsigned char *)ev_cmd +
					  sizeof(struct acpev_command));
		*acp_ev_cmd_id = acp_ev_command_id->cmd_id;
	} else {
		return;
	}

	pr_debug("%s cmd_id:0x%x\n", __func__, *acp_ev_cmd_id);
	switch (*acp_ev_cmd_id) {
	case ACP_EV_CMD_TEST_EV1:
	{
		struct acpev_cmd_test_ev1 *p_cmd_buf =
				(struct acpev_cmd_test_ev1 *)actual_cmd_buf;
		*cmd_num = p_cmd_buf->cmd_number;
	}
	break;
	case ACP_EV_CMD_GET_VER_INFO:
	{
		struct acpev_cmd_get_ver_info *p_cmd_buf =
			(struct acpev_cmd_get_ver_info *)actual_cmd_buf;
		*cmd_num = p_cmd_buf->cmd_number;
	}
	break;
	case ACP_EV_CMD_SET_CLOCK_FREQUENCY:
	{
		struct acpev_cmd_set_clock_frequency *p_cmd_buf =
			(struct acpev_cmd_set_clock_frequency *)actual_cmd_buf;
		*cmd_num = p_cmd_buf->cmd_number;
	}
	break;
	default:
	break;
	}
}

int acp_ev_send_cmd_and_wait_for_response(struct acp3x_dev_data *adata,
					  void *p_ev_cmd,
					  unsigned int cmd_length)
{
	int   ret = 0;
	unsigned int    cmd_number = (unsigned int)-1;
	unsigned short  cmd_id = (unsigned short)-1;
	unsigned int    idx = 0;
	unsigned long jiffies = msecs_to_jiffies(30);
	unsigned int  ev_response_event = 0;

	ret = ev_send_cmd(p_ev_cmd, cmd_length, adata, &ev_response_event);
	if (ret == 0) {
		acp_get_ev_cmd_info(p_ev_cmd, &cmd_number, &cmd_id);
		pr_debug("cmd_number:0x%x cmd_id: 0x%x\n", cmd_number, cmd_id);
		ret = wait_event_interruptible_timeout(adata->ev_event,
						       adata->evresp_event != 0,
						       jiffies);
		if (ret == 0) {
			pr_err("Failed To Get Response Event ret:0x%x\n", ret);
			for (idx = 0; idx < ACP_RESPONSE_LIST_SIZE; idx++) {
				if (adata->resp_list.resp[idx].command_number ==
				    cmd_number) {
					memset(&adata->resp_list.resp[idx], 0,
					       sizeof(struct acp_response));
					break;
				}
			}
		} else if (ret > 0) {
			pr_debug("%s Received cmd_number 0x%x cmd_id 0x%x",
				 __func__, cmd_number, cmd_id);
		} else {
			pr_err("ev_event wait queue error:%d\n", ret);
			return ret;
		}
		adata->evresp_event = 0x00;
	} else {
		pr_err("%s ev_send_command Failed ret : 0x%x\n", __func__, ret);
	}
	return ret;
}


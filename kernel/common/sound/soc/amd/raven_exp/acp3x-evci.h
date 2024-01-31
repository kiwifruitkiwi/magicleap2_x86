/*
 * EV command interace header file for ACP PCI Driver
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __EVCI_H__
#define __EVCI_H__

#include "acp3x.h"

/* Ring buffer offset difference */
#define AOS_RING_OFFSET_DIFFERENCE      0x00000004

unsigned int get_next_command_size(struct acp_ring_buffer_info *res_buffer_info,
				   unsigned char *res_ring_buffer);

unsigned int read_buffer(struct acp_ring_buffer_info *buffer_info,
			 unsigned char *ring_buffer,
			 unsigned int bytes_to_read, void *read_buffer);

unsigned int write_buffer(struct acp_ring_buffer_info *buffer_info,
			  unsigned char *ring_buffer,
			  unsigned int bytes_to_write, void *write_buffer);

unsigned int read_response(struct acp_ring_buffer_info *res_buffer_info,
			   unsigned char *res_ring_buffer,
			   void *read_buffer, unsigned int buffer_size);

void update_write_index(struct acp_ring_buffer_info *bufer_info);

int acp_get_cmd_number(struct acp3x_dev_data *adata);

int acp_get_next_cmd_number(struct acp3x_dev_data *adata);

bool acp_remove_responses(struct acp3x_dev_data *adata, bool remove_all,
			  struct acp_response *error_response,
			  bool *pending_responses);

int acp_add_response(unsigned int cmd_id, unsigned int cmd_number,
		     struct acp3x_dev_data *adata, unsigned int *p_event);

bool acp_record_response(struct acp3x_dev_data *adata,
			 unsigned int command_number,
			 unsigned int response_status);

int  ev_wait_for_buffer_empty(struct acp_ring_buffer_info *cmd_buffer_info,
			      unsigned int cmd_length,
			      struct acp3x_dev_data *adata);

bool frame_command(struct acp_ring_buffer_info *cmd_ring_buffer_info,
		   unsigned char *cmd_ring_buffer,
		   void *write_cmd_buffer,
		   struct acp3x_dev_data *adata);

int  ev_send_cmd(void *ev_cmd, u32 ev_cmd_length,
		 struct acp3x_dev_data *adata,
		 unsigned int *p_event);

void ev_record_version_info(struct acpev_resp_version *version,
			    struct acp3x_dev_data *adata);

void ev_process_response(unsigned char *packet, unsigned long packet_size,
			 struct acp3x_dev_data *adata);

void acp_get_ev_cmd_info(void  *ev_cmd, unsigned int *cmd_num,
			 unsigned short *acp_ev_cmd_id);

int acp_ev_send_cmd_and_wait_for_response(struct acp3x_dev_data *adata,
					  void *p_ev_cmd,
					  unsigned int cmd_length);

#endif

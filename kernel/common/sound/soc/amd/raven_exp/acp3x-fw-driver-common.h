/*
 * Common header file for ACP PCI Driver and ACP firmware
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __EXT_SCRATCH_MEM_H__
#define __EXT_SCRATCH_MEM_H__

#include "acp3x-common.h"

#pragma pack(push, 4)

/* MAX number of DMA descripotos */
#define   MAX_NUM_DMA_DESC_DSCR                 64
#define   MAX_COMMAND_BUFFER_SIZE               2560
#define   MAX_RESPONSE_BUFFER_SIZE              2560

struct  acp_atu_grp_pte {
	u32 addr_low;
	u32 addr_high;
};

union acp_config_dma_trnsfer_cnt {
	struct {
		unsigned int  count : 19;
		unsigned int  reserved : 12;
		unsigned  ioc : 1;
	} bitfields, bits;
	unsigned int    u32_all;
	signed int  i32_all;
};

struct acp_config_dma_descriptor {
	unsigned int    src_addr;
	unsigned int    dest_addr;
	union acp_config_dma_trnsfer_cnt  dma_transfer_count;
	unsigned int                     reserved;
};

struct acpev_cmd_test_ev1 {
	unsigned int cmd_number;
	unsigned int data;
};

struct acpev_cmd_get_ver_info {
	u32 cmd_number;
};

struct acpev_cmd_set_log_buf {
	unsigned int cmd_number;
	unsigned int flags;
	unsigned int log_buffer_offset;
	unsigned int log_size;
};

struct acpev_cmd_start_dma_frm_sys_mem {
	unsigned int cmd_number;
	unsigned int flags;
	unsigned int dma_buf_offset;
	unsigned int dma_buf_size;
};

struct acpev_cmd_stop_dma_frm_sys_mem {
	unsigned int cmd_number;
};

struct acpev_cmd_start_dma_to_sys_mem {
	unsigned int cmd_number;
	unsigned int flags;
	unsigned int dma_buf_offset;
	unsigned int dma_buf_size;
};

struct acpev_cmd_stop_dma_to_sys_mem {
	unsigned int cmd_number;
};

/* This union is used to pass the value in register
 * mmACP_FUTURE_REG_ACLK_3 for S/R,and how much iRAM
 * is required
 * iram_fence_size - IRAM fence size
 * is_resume - this flag is set by host driver to resume
 * enable_disable_fw_logs - fw logs flag
 * 0x00 : default value
 * 0x01 : Logs Enable
 * 0x10 : Disable Logs
 */
union acp_sr_fence_update {
	struct {
		unsigned int  iram_fence_size : 12;
		unsigned int  is_resume : 4;
		unsigned int  enable_disable_fw_logs : 2;
		unsigned int: 16;
	} bits;
	unsigned int u32_all;
};

enum acp_clock_type {
	ACP_ACLK_CLOCK,
	ACP_SCLK_CLOCK,
	ACP_CLOCK_TYPE_MAX,
	ACP_CLOCK_TYPE_FORCE = 0xFF
};

struct acpev_cmd_set_clock_frequency {
	unsigned int cmd_number;
	unsigned int clock_type;
	unsigned int clock_freq;
};

struct acpev_cmd_i2s_dma_start {
	unsigned int cmd_number;
	unsigned int sample_rate;
	unsigned int bit_depth;
	unsigned int ch_count;
	unsigned int i2s_instance;
};

struct acpev_cmd_i2s_dma_stop {
	unsigned int cmd_number;
	unsigned int i2s_instance;
};

struct acpev_cmd_start_timer {
	unsigned int cmd_number;
};

struct acpev_cmd_stop_timer {
	unsigned int cmd_number;
};

struct acpev_cmd_memory_pg_shutdown {
	unsigned int cmd_number;
	unsigned int mem_bank_number;
};

struct acpev_cmd_memory_pg_deepsleep {
	unsigned int cmd_number;
	unsigned int mem_bank_number;
};

enum acpev_cmd {
	ACP_EV_CMD_RESERVED,
	ACP_EV_CMD_RESP_STATUS = 0x01,
	ACP_EV_CMD_RESP_VERSION,
	ACP_EV_CMD_GET_VER_INFO,
	ACP_EV_CMD_SET_LOG_BUFFER,
	ACP_EV_CMD_TEST_EV1,
	ACP_EV_CMD_SET_CLOCK_FREQUENCY,
	ACP_EV_CMD_START_DMA_FRM_SYS_MEM,
	ACP_EV_CMD_STOP_DMA_FRM_SYS_MEM,
	ACP_EV_CMD_START_DMA_TO_SYS_MEM,
	ACP_EV_CMD_STOP_DMA_TO_SYS_MEM,
	ACP_EV_CMD_I2S_DMA_START,
	ACP_EV_CMD_I2S_DMA_STOP,
	ACP_EV_CMD_START_TIMER,
	ACP_EV_CMD_STOP_TIMER,
	ACP_EV_CMD_MEMORY_PG_SHUTDOWN_ENABLE,
	ACP_EV_CMD_MEMORY_PG_SHUTDOWN_DISABLE,
	ACP_EV_CMD_MEMORY_PG_DEEPSLEEP_ON,
	ACP_EV_CMD_MEMORY_PG_DEEPSLEEP_OFF
};

/* acp_scratch_register_config structure used to map scratch memory
 * acp_cmd_ring_buf_info - this structure hold information about
 * acp command ring buffer like write_index, current_write_index
 * acp_cmd_buf - acp command ring buffer
 * acp_resp_ring_buf_info - acp response ring buffer information
 * such as read index, write index
 * acp_resp_buf - acp response ring buffer
 */
struct  acp_scratch_register_config {
	struct acp_ring_buffer_info             acp_cmd_ring_buf_info;
	unsigned char               acp_cmd_buf[MAX_COMMAND_BUFFER_SIZE];
	struct acp_ring_buffer_info             acp_resp_ring_buf_info;
	unsigned char           acp_resp_buf[MAX_RESPONSE_BUFFER_SIZE];
	unsigned int                       reserve[];
};

#pragma pack(pop)

#endif // __EXT_SCRATCH_MEM_H__


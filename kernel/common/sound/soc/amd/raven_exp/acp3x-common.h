/*
 * Common header file for ACP PCI Driver and ACP firmware
 * Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __ACP_COMMON_H__
#define __ACP_COMMON_H__

#pragma pack(push, 4)

#define ACP_AXI2AXIATU_BASE_ADDR_GRP_AXI2AXIATUGrpEnable          (0x1UL << 31)
#define ACP_AXI2AXIATU_PAGE_SIZE_2MB                              0x200000
#define ACP_AXI2AXIATU_PAGE_SIZE_64KB                             0x10000
#define ACP_AXI2AXIATU_PAGE_SIZE_4KB                              0x1000
#define ACP_AXI2AXIATU_GRP_MAX_GROUP_SIZE                         0x800000
#define ACP_AXI2AXIATU_PAGE_SIZE_Mask4KB     0x2
#define ACP_AXI2AXIATU_PAGE_SIZE_Mask64KB    0x1
#define ACP_AXI2AXIATU_PAGE_SIZE_Mask2MB     0x0

struct ev_uint64 {
	unsigned int    low_part;
	int             high_part;
};

struct acp_ring_buffer_info {
	unsigned int read_index;
	unsigned int write_index;
	unsigned int size;
	unsigned int current_write_index;
};

#pragma pack(pop)
#endif /* __ACP_COMMON_H__ */

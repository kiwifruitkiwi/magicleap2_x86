/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#if	!defined(ISP3_SILICON)
#pragma once

enum _DmaDirection_t {
	DMA_DIRECTION_SYS_TO_DEV,   // system memory to device
	DMA_DIRECTION_SYS_FROM_DEV, // device to system memory
	DMA_DIRECTION_DEV_TO_DEV,   // device to device
	DMA_DIRECTION_SYS_TO_SYS,   // system to system
};

enum _IOCTL_RET_STATUS {
	IOCTL_RET_STATUS_SUCCESS          = 0,
	IOCTL_RET_STATUS_WRONG_PARAM      = 1,
	IOCTL_RET_STATUS_NO_MEM           = 2,
	IOCTL_RET_STATUS_OUT_OF_RANGE     = 3,
	IOCTL_RET_STATUS_NOT_EXIST        = 4,
	IOCTL_RET_STATUS_NOT_SUPPORT      = 5,
	IOCTL_RET_STATUS_FAIL             = 6,
	IOCTL_RET_STATUS_DMA_BUF_MAP_ERR  = 7,
	IOCTL_RET_STATUS_DMA_TIMEOUT      = 8,
	IOCTL_RET_STATUS_DMA_INTERNAL_ERR = 9,
};

void cdma_init(void *pDrvData);
void cdma_reset(const void *pDrvData);
int cdma_is_idle(const void *pDrvData);
int cdma_has_error(const void *pDrvData);
int cdma_start(const void *pDrvData, uint64_t srcAddr,
	uint64_t dstAddr, uint32_t size, enum _DmaDirection_t direction);

enum _IOCTL_RET_STATUS do_dma_once(void *pDrvData,
			uint64_t dmaSrcAddr,
			uint64_t dmaDstAddr,
			unsigned int length,
			enum _DmaDirection_t direct);
enum _IOCTL_RET_STATUS do_dma_sg(void *pDrvData,
			uint64_t devAddr,
			struct physical_address *phy_sg_tbl,
			unsigned int sg_tbl_cnt,
			enum _DmaDirection_t direct,
			unsigned int lenInBytes);
#endif

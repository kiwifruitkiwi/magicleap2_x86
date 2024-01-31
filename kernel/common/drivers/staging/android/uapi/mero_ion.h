/*
 * ION driver for AMD Mero SoC CVIP Subsystem.
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _UAPI_LINUX_MERO_ION_H
#define _UAPI_LINUX_MERO_ION_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define CVIP_ADDR_APU_DIRECT_DRAM	0x800000000
#define CVIP_ADDR_APU_DIRECT_DRAM_ALT0	0x3000000000
#define CVIP_ADDR_APU_DIRECT_DRAM_ALT1	0x3800000000
#define CVIP_ADDR_SYSTEMCACHE_0		0x1000000000
#define CVIP_ADDR_SYSTEMCACHE_1		0x1400000000
#define CVIP_ADDR_SYSTEMCACHE_2		0x1800000000
#define CVIP_ADDR_SYSTEMCACHE_3		0x1C00000000
#define CVIP_ADDR_16GB			0x400000000

#define ION_BIT(bit) (1UL << (bit))

enum ion_heap_ids {
	INVALID_HEAP_ID = -1,
	ION_HEAP_X86_CMA_ID  = 0,
	ION_HEAP_X86_SYSTEM_ID  = 7,
	ION_HEAP_X86_CVIP_SHARED_ID  = 8,
	ION_HEAP_CVIP_DDR_PRIVATE_ID = 9,
	ION_HEAP_CVIP_SRAM_Q6_S0_ID = 10,
	ION_HEAP_CVIP_SRAM_C5_S0_ID = 11,
	ION_HEAP_CVIP_SRAM_C5_S1_ID = 12,
	ION_HEAP_CVIP_SRAM_Q6_M0_ID = 13,
	ION_HEAP_CVIP_SRAM_Q6_M1_ID = 14,
	ION_HEAP_CVIP_SRAM_Q6_M2_ID = 15,
	ION_HEAP_CVIP_SRAM_Q6_M3_ID = 16,
	ION_HEAP_CVIP_SRAM_Q6_M4_ID = 17,
	ION_HEAP_CVIP_SRAM_Q6_M5_ID = 18,
	ION_HEAP_CVIP_SRAM_C5_M0_ID = 19,
	ION_HEAP_CVIP_SRAM_C5_M1_ID = 20,
	ION_HEAP_CVIP_SRAM_A55_ID = 21,
	ION_HEAP_CVIP_ISPIN_FMR5_ID = 22,
	ION_HEAP_CVIP_HYDRA_FMR2_ID = 23,
	ION_HEAP_CVIP_SUSB_FMR3_ID = 24,
	ION_HEAP_CVIP_ISPOUT_FMR8_ID = 25,
	ION_HEAP_CVIP_X86_SPCIE_FMR2_10_ID = 26,
	ION_HEAP_CVIP_X86_READONLY_FMR10_ID = 27,
	ION_HEAP_CVIP_READONLY_FMR4_ID = 28,
	MAX_ION_HEAP_NUM = 32,
};

#define ION_DSP_HEAP_NAME	"dsp"
#define ION_SYSTEM_HEAP_NAME	"system"
#define ION_XPORT_HEAP_NAME	"xport"
#define ION_FMR2_HEAP_NAME	"fmr2"
#define ION_FMR3_HEAP_NAME	"fmr3"
#define ION_FMR4_HEAP_NAME	"fmr4"
#define ION_FMR5_HEAP_NAME	"fmr5"
#define ION_FMR8_HEAP_NAME	"fmr8"
#define ION_FMR10_HEAP_NAME	"fmr10"
#define ION_FMR2_10_HEAP_NAME	"fmr2/10"

#define MERO_CVIP_ADDR_XPORT_BASE	0x4000000000
#define MERO_CVIP_XPORT_APERTURE_SIZE	0x800000000

/*
 * flags to select system cache path in cvip
 */
#define ION_FLAG_CVIP_SLC_CACHED_0		ION_BIT(17)
#define ION_FLAG_CVIP_SLC_CACHED_1		ION_BIT(18)
#define ION_FLAG_CVIP_SLC_CACHED_2		ION_BIT(19)
#define ION_FLAG_CVIP_SLC_CACHED_3		ION_BIT(20)

/*
 * flag to indicate this buffer should be ARM CPU inner cached
 */
#define ION_FLAG_CVIP_ARM_CACHED		ION_BIT(21)

/*
 * flags to select system direct dram alt_0/alt_1 path in cvip
 */
#define ION_FLAG_CVIP_DRAM_ALT0			ION_BIT(22)
#define ION_FLAG_CVIP_DRAM_ALT1			ION_BIT(23)

/**
 * heap flags - flag for not clearing memory region to zero when heap is created
 */
#define ION_HEAP_FLAG_INIT_NO_MEMORY_CLEAR	ION_BIT(1)

/**
 * heap flags - flag for not clearing memory region to zero when buffer of the
 *              heap is freed
 */
#define ION_BUFFER_FLAG_NO_MEMORY_CLEAR		ION_BIT(2)

/**
 * heap flags - flag for indicating there is syscache support
 */
#define ION_HEAP_FLAG_HAS_SYSCACHE		ION_BIT(3)

/**
 * heap flags - flag for indicating there is syscache support with shift setup
 */
#define ION_HEAP_FLAG_HAS_SYSCACHE_SHIFT	ION_BIT(4)

/**
 * struct ion_heap_xport_data - data for x-interface heap creation
 * @paddr - physical address of memory region for x-interface heap
 * @size - heap size
 */
struct ion_heap_xport_data {
	__u64 paddr;
	__u64 size;
	__u32 reserved0;
	__u32 reserved1;
};

/**
 * struct ion_buf_physdata - data to obtain phy_addr for ion buffer
 * @fd - ion buffer fd
 * @paddr - physical address of memory region associated with the given fd
 */
struct ion_buf_physdata {
	__u64 fd;
	__u64 paddr;
	__u32 reserved0;
	__u32 reserved1;
};

/**
 * struct ion_buf_mapping_data - data to obtain iommu mapping for ion buffer
 * @fd - ion buffer fd
 * @map_addr - iommu mapped address of memory region with the given fd
 * @map_size - iommu mapped size of memory region with the given fd
 * @map_attach - iommu mapping attachment associated data with the given fd,
 *		 output returned parameter on mapping IOCTL to be stored
 *		 input parameter on unmapping IOCTL
 * @map_table - iommu mapping sg table associated data with the given fd
 *		 output returned parameter on mapping IOCTL to be stored
 *		 input parameter on unmapping IOCTL
 */
struct ion_buf_mapping_data {
	int fd;
	void *map_addr;
	unsigned int map_size;
	void *map_attach;
	void *map_table;
};

#define ION_IOC_MERO_MAGIC 'M'

/**
 * DOC: ION_IOC_MERO_REGISTER_XPORT_HEAP - create heap at CVIP x-interface
 *
 * Take an ion_heap_xport_data struct and create a carveout heap at CVIP
 * x-interface shared memory region
 */
#define ION_IOC_MERO_REGISTER_XPORT_HEAP   _IOWR(ION_IOC_MERO_MAGIC, 1, \
						 struct ion_heap_xport_data)

/**
 * DOC: ION_IOC_MERO_UNREGISTER_XPORT_HEAP - destroy heap at CVIP x-interface
 *
 * Destroy the heap created at CVIP x-interface shared memory region
 */
#define ION_IOC_MERO_UNREGISTER_XPORT_HEAP   _IO(ION_IOC_MERO_MAGIC, 2)

/**
 * DOC: ION_IOC_MERO_GET_BUFFER_PADDR - get physical address for an ION buffer
 *
 * Take an ion_buf_physdata struct and return physical address of ion buffer fd
 */
#define ION_IOC_MERO_GET_BUFFER_PADDR   _IOWR(ION_IOC_MERO_MAGIC, 3, \
						 struct ion_buf_physdata)

/**
 * DOC: ION_IOC_MERO_IOMMU_MAP - iommu map for an ION buffer
 *
 * Take an ion_buf_mapping_data struct, return iommu mapped address of buffer fd
 */
#define ION_IOC_MERO_IOMMU_MAP   _IOWR(ION_IOC_MERO_MAGIC, 4, \
						 struct ion_buf_mapping_data)

/**
 * DOC: ION_IOC_MERO_IOMMU_UNMAP - iommu unmap for an ION buffer
 *
 * Take an ion_buf_mapping_data struct, iommu unmap for ion buffer fd
 */
#define ION_IOC_MERO_IOMMU_UNMAP   _IOWR(ION_IOC_MERO_MAGIC, 5, \
						 struct ion_buf_mapping_data)

/**
 * DOC: ION_IOC_MERO_MMAP_CVIP_SINTF - iommu map indicate for CVIP S-INTF
 *
 */
#define ION_IOC_MERO_MMAP_CVIP_SINTF   _IO(ION_IOC_MERO_MAGIC, 6)

#endif /* _UAPI_LINUX_MERO_ION_H */

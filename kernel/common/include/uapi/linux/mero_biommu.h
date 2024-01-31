/*
 * Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _UAPI_LINUX_MERO_BIOMMU_H_
#define _UAPI_LINUX_MERO_BIOMMU_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * struct map_data - biommu map data for ioctl caller
 * @input_addr: input address of biommu mapping
 * @map_size: size of biommu mapping
 * @output_addr: output_address of biommu mapping
 * @map_prot: permission of biommu mapping
 */
struct map_data {
	__u64 input_addr;
	__u64 map_size;
	__u64 output_addr;
	__u64 map_prot;
};

/**
 * struct unmap_data - biommu unmap data for ioctl caller
 * @input_addr: input address of biommu mapping
 * @map_size: size of biommu mapping
 */
struct unmap_data {
	__u64 input_addr;
	__u64 map_size;
};

#define IOCTL_BIOMMU_ADD_MAPPING        _IOW('m', 0x0, struct map_data)
#define IOCTL_BIOMMU_REMOVE_MAPPING     _IOW('m', 0x1, struct unmap_data)

#if defined(__cplusplus)
}
#endif

#endif // _UAPI_LINUX_MERO_BIOMMU_H_

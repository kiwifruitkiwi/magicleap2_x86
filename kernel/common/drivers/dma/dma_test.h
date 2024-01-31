/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _UAPI_LINUX_DMA_TEST_H
#define _UAPI_LINUX_DMA_TEST_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define MAX_DMADEV 2
#define MAX_CHANS_PER_DMADEV 8
#define MAX_CHANS (MAX_DMADEV * MAX_CHANS_PER_DMADEV)
#define MAX_CHARS 20

/**
 * struct dma_test_rw_data - metadata passed to the kernel
 * @buf_size: ion buffer size to allocate
 * @ptr:	a pointer to an area at least as large as size
 * @offset:	offset into the ion buffer to start reading
 * @size:	size to read or write
 * @write:	1 to write, 0 to read
 * @dma_channel: DMA channel to request
 * @src_fd:	ion alloc fd for DMA src
 * @dst_fd:	ion alloc fd for DMA dst
 * @runtime:	time used for DMA transfer
 */
struct dma_test_rw_data {
	size_t buf_size;
	__u64 ptr;
	__u64 offset;
	__u64 size;
	int write;
	char dma_channel[MAX_CHARS];
	int src_fd;
	int dst_fd;
	__u64 runtime_ns;
};

struct dma_test_multi_chan_rw_data {
	size_t buf_size[MAX_CHANS];
	__u64 ptr;
	__u64 offset;
	__u64 size;
	int write;
	char dma_channel[MAX_CHANS][MAX_CHARS];
	int src_fd[MAX_CHANS];
	int dst_fd[MAX_CHANS];
	int total_cases;
	__u64 runtime_ns[MAX_CHANS];
};

#define DMA_IOC_MAGIC		'I'

#define DMA_IOC_TEST_SET_CHANNEL \
			_IO(DMA_IOC_MAGIC, 0xf0)

#define DMA_IOC_TEST_RUN_DMA \
			_IOW(DMA_IOC_MAGIC, 0xf1, struct dma_test_rw_data)

#define DMA_IOC_TEST_DMA_CHANS_SEQ \
			_IOWR(DMA_IOC_MAGIC, 0xf2, \
			struct dma_test_multi_chan_rw_data)

#define DMA_IOC_TEST_DMA_CHANS_PARA \
			_IOWR(DMA_IOC_MAGIC, 0xf3, \
			struct dma_test_multi_chan_rw_data)

#define DMA_IOC_TEST_IMPORT_FD \
	_IOW(DMA_IOC_MAGIC, 0xf4, struct dma_test_rw_data)

#define DMA_IOC_TEST_IMPORT_MULTI_FD \
	_IOW(DMA_IOC_MAGIC, 0xf5, struct dma_test_multi_chan_rw_data)

#define DMA_IOC_TEST_CLEANUP \
	_IO(DMA_IOC_MAGIC, 0xf6)

#endif /* _UAPI_LINUX_DMA_TEST_H */

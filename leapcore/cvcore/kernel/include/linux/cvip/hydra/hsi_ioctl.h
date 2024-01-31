/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include <hsi.h>

#define HSI_TS_SUBMIT    (0)
#define HSI_TS_INTR      (1)
#define HSI_TS_WAKE      (2)
#define HSI_TS_WAIT_WAKE (3)
#define HSI_TS_LAST (HSI_TS_WAIT_WAKE)
#define HSI_TS_COUNT     (HSI_TS_LAST + 1)

struct hsi_ioctl {
    uint32_t substream;

    // Inputs to kernel
    uint64_t sh_name_data;
    uint64_t sh_name_meta;
    uint32_t rd_timeout_ms;

    // Returned from ioctl(WAIT)
    int32_t  status;
        #define HSI_STATUS_DCPRS_ERR            (1 << 0)
    uint64_t flags;
    uint32_t slice;
    uint32_t spills;
    uint32_t size_data;
    uint32_t size_meta;
    uint32_t size_meta_header;
    uint32_t size_meta_footer;
    uint32_t ss_data_length[HSI_MAX_SUBSTREAMS];
    uint32_t ss_meta_length[HSI_MAX_SUBSTREAMS];

    // Input & Output
    uint64_t addr_data;
    uint64_t addr_meta;
    uint64_t ref;

    struct hsi_properties {
        // Per device properties.
        uint32_t substream_count;

        // Per substream properties.
        struct {
            uint32_t slices;
            uint32_t max_buf_count;
            uint32_t size_data_buf;
            uint32_t size_meta_buf;

        } substreams[HSI_MAX_SUBSTREAMS];
    } props;

    struct hsi_stats {
        // Timestamps
        uint64_t ts[HSI_TS_COUNT];
    } stats;
};


#define HSI_EVFLAG_DATA           (1 << 0)
#define HSI_EVFLAG_META_HEADER    (1 << 1)
#define HSI_EVFLAG_META_FOOTER    (1 << 2)
#define HSI_EVFLAG_LAST_SLICE     (1 << 3)
#define HSI_EVFLAG_SLICE          (1 << 4)

//
// HSI ioctl commands
//
#define HSI_IOC_MAGIC 'H'
#define HSI_SUBMIT_BUFS     _IOWR(HSI_IOC_MAGIC, 55, struct hsi_ioctl)
#define HSI_WAIT            _IOWR(HSI_IOC_MAGIC, 56, struct hsi_ioctl)
#define HSI_GET_DEV_PROPS   _IOWR(HSI_IOC_MAGIC, 57, struct hsi_ioctl)
#define HSI_DEV_IS_BUSY     _IOWR(HSI_IOC_MAGIC, 58, struct hsi_ioctl)
#define HSI_UTRACE_0        _IOWR(HSI_IOC_MAGIC, 70, struct hsi_ioctl)
#define HSI_UTRACE_1        _IOWR(HSI_IOC_MAGIC, 71, struct hsi_ioctl)
#define HSI_UTRACE_2        _IOWR(HSI_IOC_MAGIC, 72, struct hsi_ioctl)
#define HSI_UTRACE_3        _IOWR(HSI_IOC_MAGIC, 73, struct hsi_ioctl)


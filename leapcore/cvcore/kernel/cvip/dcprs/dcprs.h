/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#define DCPRS_MAX_SRC_BUFS      (2)
#define DCPRS_BUF_IX_G          (0)
#define DCPRS_BUF_IX_RB         (1)

struct dcprs_ioctl {
    uint32_t flags;
        #define DCPRS_F_SECT_MODE    (1 << 0)   // must be set when operating in sectioning mode
        #define DCPRS_F_SECT_LAST_G  (1 << 1)   // must be set when passing last section of compressed G data
        #define DCPRS_F_SECT_LAST_RB (1 << 2)   // must be set when passing last section of compressed RB data
        #define DCPRS_F_EOF          (1 << 3)   // set by driver on last slice or full-frame eof
        #define DCPRS_F_GRAYSCALE    (1 << 4)   // must be set when decompressing a single grayscale image
        #define DCPRS_F_ION          (1 << 5)   // treat addresses as ion descriptors
        #define DCPRS_F_NOMAP        (1 << 6)   // don't do iommu mapping

    uint64_t dst_shname;
    uint64_t dst_addr;
    uint32_t dst_length;

    // Uncompressed data buffers.
    // The length reflects the size of the complete compressed frame buffer.
    // In sectioning mode, the addr and length fields must be the same on every call passing in another section,
    // with only the sect_length field changing to indicate the size of the newest section.
    struct {
        uint64_t shname;        // Shregion name.
        uint64_t addr;          // Base of first (or only) section of compressed frame.
        uint32_t length;        // Length of complete compressed frame.
        uint32_t sect_length;   // Current compressed section length.
    } src[DCPRS_MAX_SRC_BUFS];

    uint32_t cur_slice;     // Set by driver. Will be zero for full frame mode.

    // These fields are used for the DCPRS_CONFIG command.
    uint32_t uncomp_length;   // Set to expected uncompressed frame size. Must be <= dst_length.
    uint32_t slice_length;    // Leave as zero to disable slicing. Must be divisible by 16 bytes.
};


//
// DCPRS ioctl commands
//
#define DCPRS_IOC_MAGIC 'D'
#define DCPRS_DECOMPRESS           _IOWR(DCPRS_IOC_MAGIC, 55, struct dcprs_ioctl)
#define DCPRS_WAIT                 _IOWR(DCPRS_IOC_MAGIC, 56, struct dcprs_ioctl)
#define DCPRS_CONFIG               _IOWR(DCPRS_IOC_MAGIC, 57, struct dcprs_ioctl)
#define DCPRS_TEST                 _IOWR(DCPRS_IOC_MAGIC, 58, struct dcprs_ioctl)


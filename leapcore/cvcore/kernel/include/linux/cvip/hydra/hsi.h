/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

// This file defines all the structures that comprise the HSI (Hydra Streaming Interface) between
// the Linux-based driver and the Hydra-based streaming endpoint device, which could be an X11, HiFi4, or MLDMA core.
//
// The key interface structure contains a pair of queues called the pending and completion queues.
// The host driver writes to the pending queue and reads from the completion queue.
// The device reads from the pending queue and writes to the completion queue.
// There is guaranteed never to be more than a single reader or a single writer for each queue.
//
// Depending on the packet transfer rate required and the packet lengths, the transfers may or may not be DMA driven.
//
// All the structures are physically located in a portion of SRAM that's visible to both Hydra and the PCIe host.
//
// Unless stated otherwise, all fields are considered read-only by the Linux host, and are writable by Hydra.
//

#include "cv_version.h"

// Maximum number of unidirectional streaming endpoint devices supported. MUST be a power of 2.
#define HSI_MAX_STREAMS     (16)

// The maximum number of entries on either a pending or completion queue. The actual number used may be smaller.
#define HSI_MAX_QELEM       (32)

// The maximum length of the readable name of the streaming endpoint device.
#define HSI_DEV_NAME_LEN    (16)

// Indicates whether data is flowing to or from the host. In almost all Hydra cases, it's egress data.
#define HSI_DIR_EGRESS      (0)
#define HSI_DIR_INGRESS     (1)

// Total number of substreams per stream.
#define HSI_MAX_SUBSTREAMS  (2)

// The index into the substream array for the combined stream
#define HSI_COMBINED_SUBSTREAM_INDEX (0)

// Magic number for HSI header
#define HSI_MAGIC           (0x48534020)

// The maximum number of slices allowed for a stream
#define HSI_MAX_SLICES      (8)

// Max number of virtual streams per stream.
#define HSI_MAX_VSTREAMS    (6)


// The pending queue contains the following element:
// NB: The handle indicated in this field must match the one
struct hsi_pend_qelem {
    uint64_t addr_data;     // pci bus address of buffer
    uint64_t addr_meta;     // pci bus address of metadata
    uint16_t user_handle;   // handle to associate with this buffer
    uint16_t rsvd;          // reserved
} __attribute__((packed));


//
// The completion queue consists of elements matching the the following element,
// and is identical to the NWL completion queue status element.
//
struct hsi_compl_qelem {
    // -----------------------------------------------------------------
    // Hardware specific entries - Do not change or reorder
    // -----------------------------------------------------------------
    union {
        uint32_t register_value;          // Status Flags - 32 Bits
        struct {
            uint32_t COMPLETED:1;         // Bit 0: DMA Completed
            uint32_t SOURCE_ERROR:1;      // Bit 1: Source Error Detected during DMA
            uint32_t DESTINATION_ERROR:1; // Bit 2: Destination Error Detected during DMA
            uint32_t INTERNAL_ERROR:1;    // Bit 3: Internal Error Detected during DMA
            uint32_t COMP_BYTE_COUNT:24;  // Bits 4 - 27: Completed Byte Count
            uint32_t FRAME_COUNT:4;       // Bits 28 - 31: Frame counter (modulo 16)
        } bitfield;
    } status;
    uint16_t user_handle; // from pendq
    uint16_t user_id;     // info
#define HSI_UID_SLICE_ID        (0x000f)
#define HSI_UID_HEADER          (0x0010)
#define HSI_UID_FOOTER          (0x0020)
#define HSI_UID_LAST_SLICE      (0x0040)
#define HSI_UID_SPILLWAY        (0x0080)
#define HSI_UID_VSTREAM_MASK    (0x0700)
#define HSI_UID_VSTREAM_MASK_S  (8)
#define HSI_UID_MASK            (0x007f)

} __attribute__((packed));


// The key interface structure is defined below:
// It contains a pair of queues called the pending and completion queues.
// The host driver writes to the pending queue and reads from the completion queue.
// The streaming device reads from the pending queue and writes to the completion queue.

// The X11 application creates an SRAM-based table of the structure at startup time and
// signals the host upon its completion. The host then populates Linux's /dev directory with
// entries for each device that the X11 has marked as valid.
//
struct hsi_stream {
    // This stream name will appear as Linux device /dev/hydra/name
    char name[HSI_DEV_NAME_LEN];

    // Set by X11 to indicate a stream that's present.
    uint32_t            present;

    // Must be HSI_DIR_EGRESS
    uint32_t direction;

    // Substream count
    uint32_t subs_count;

    // Each substream gets its own pending/completion queue.
    struct substream {
        uint32_t chan_id;       // indicates MSI irq/DMA channel id
        uint32_t compressed;    // indicates a compressed substream
        uint32_t buf_size;      // bytes per buffer
        uint32_t buf_count_max; // max number of pendq buffers
        uint32_t meta_size;     // bytes per metadata buffer
        uint32_t slices;        // max slices per payload
        uint32_t vstream_count; // number of virtual streams in this stream.

        // Table of virtual stream names.
        char vstream_name[HSI_MAX_VSTREAMS][HSI_DEV_NAME_LEN];

        // PCIe buffer address to which any egress device may write data when the pending queue is empty or
        // the "enabled" field is zero. It is effectively a null sink for when a device can't just drop data.
        // This field is written only by the host driver, and read by the device.
        // If it is zero, the device MAY  NOT initiate any PCIe transfers.
        uint64_t            spillway_buf_addr[HSI_MAX_VSTREAMS];
    
        // Queue headers.
        struct {
            uint32_t head;
            uint32_t tail;
        } pendq_hdr[HSI_MAX_VSTREAMS], cplq_hdr;

        // The pending queue is populated by the host with buffers. If this is an egress device,
        // the entries contain empty buffers to write data into. In the case of an ingress device,
        // they contain valid data for the device to read from.
        struct hsi_pend_qelem pendq[HSI_MAX_VSTREAMS][HSI_MAX_QELEM];

        // The completion queue is populated by the device with buffers. Depending again on whether
        // this is an egress or ingress device, they contain buffers that have either been written to or read from.
        struct hsi_compl_qelem cmplq[HSI_MAX_QELEM] __attribute__ ((aligned (64)));

    } sub[HSI_MAX_SUBSTREAMS];
};


// HSI root structure.
struct hsi_root {
    uint32_t          hsi_magic;
    CvVersion         vers;
    uint32_t          host_proc_complete;
    struct hsi_stream tab[HSI_MAX_STREAMS];
};


//
// SRAM-based device table.
//
extern volatile struct hsi_root g_hsi_root;


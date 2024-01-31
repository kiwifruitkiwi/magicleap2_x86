/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

typedef enum { XLAT_INGRESS, XLAT_EGRESS } xlat_dir_t;

typedef struct _egress_xlat {
    // Word #1
    union {
        struct {
            uint32_t egress_size_max : 8;
            uint32_t egress_size_offset : 8;
            uint32_t rsvd : 15;
            uint32_t egress_present : 1;
        } FIELDS;
        uint32_t WORD;
    } caps;

    // Word #2
    union {
        struct {
            uint32_t rsvd1 : 7;
            uint32_t wr_pending_ctr : 9;
            uint32_t rsvd2 : 7;
            uint32_t rr_pending_ctr : 9;
        } FIELDS;
        uint32_t WORD;
    } status;

    // Word #3
    union {
        struct {
            uint32_t egress_enable : 1;
            uint32_t rsvd3 : 1;
            uint32_t egress_sec_enable : 1;
            uint32_t egress_invalid : 1;
            uint32_t rsvd2 : 12;
            uint32_t egress_size : 5;
            uint32_t rsvd1 : 2;
            uint32_t egress_attr_enable : 1;
            uint32_t egress_attr_r : 4;
            uint32_t egress_attr_w : 4;
        } FIELDS;
        uint32_t WORD;
    } control;

    // Word #4
    uint32_t rsvd;

    // Word #5
    uint32_t src_base_lo;

    // Word #6
    uint32_t src_base_hi;

    // Word #7
    uint32_t dst_base_lo;

    // Word #8
    uint32_t dst_base_hi;

} __attribute__((packed)) egress_xlat_t;

typedef struct _ingress_xlat {
    // Word #1
    union {
        struct {
            uint32_t ingress_size_max : 8;
            uint32_t ingress_size_offset : 8;
            uint32_t rsvd : 15;
            uint32_t ingress_present : 1;
        } FIELDS;
        uint32_t WORD;
    } caps;

    // Word #2
    union {
        struct {
            uint32_t rsvd1 : 7;
            uint32_t wr_pending_ctr : 9;
            uint32_t rsvd2 : 7;
            uint32_t rr_pending_ctr : 9;
        } FIELDS;
        uint32_t WORD;
    } status;

    // Word #3
    union {
        struct {
            uint32_t ingress_enable : 1;
            uint32_t rsvd3 : 1;
            uint32_t ingress_sec_enable : 1;
            uint32_t ingress_invalid : 1;
            uint32_t rsvd2 : 12;
            uint32_t ingress_size : 5;
            uint32_t rsvd1 : 2;
            uint32_t ingress_attr_enable : 1;
            uint32_t ingress_attr_r : 4;
            uint32_t ingress_attr_w : 4;
        } FIELDS;
        uint32_t WORD;
    } control;

    // Word #4
    uint32_t rsvd;

    // Word #5
    uint32_t src_base_lo;

    // Word #6
    uint32_t src_base_hi;

    // Word #7
    uint32_t dst_base_lo;

    // Word #8
    uint32_t dst_base_hi;

} __attribute__((packed)) ingress_xlat_t;

typedef enum {
    XLAT_WIN_SIZE_4K = 0,
    XLAT_WIN_SIZE_8K = 1,
    XLAT_WIN_SIZE_16K = 2,
    XLAT_WIN_SIZE_32K = 3,
    XLAT_WIN_SIZE_64K = 4,
    XLAT_WIN_SIZE_128K = 5,
    XLAT_WIN_SIZE_256K = 6,
    XLAT_WIN_SIZE_512K = 7,
    XLAT_WIN_SIZE_1MB = 8,
    XLAT_WIN_SIZE_2MB = 9,
    XLAT_WIN_SIZE_4MB = 10,
    XLAT_WIN_SIZE_8MB = 11,
    XLAT_WIN_SIZE_16MB = 12,
    XLAT_WIN_SIZE_32MB = 13,
    XLAT_WIN_SIZE_64MB = 14,
} xlat_win_size_t;


int pci_xlat_cfg(uint8_t *base, uint8_t win_num, xlat_dir_t dir, xlat_win_size_t win_size,
                 uint64_t src, uint64_t dst);

int pci_xlat_dump(uint8_t *base, uint8_t win_num, xlat_dir_t dir);

uint32_t pci_xlat_min_region_size(uint32_t buf_len);

void pci_xlat_clear(uint8_t *base_egress, uint8_t *base_ingress);

void pci_msi_send(uint8_t *base, uint8_t vec);

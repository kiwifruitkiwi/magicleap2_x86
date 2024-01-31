/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

//////////////////////////////////////////////////////////////////////////////////
// File : dcprs_desc.h
// Created on : Thursday Sep 12, 2019 [11:46:34 pm]
//////////////////////////////////////////////////////////////////////////////////

#define DCPRS_DESC_BASE_ADDRESS                            (0x00000000)

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int : 1;
        //
        // First Row Padding Size - Number of padded rows
        unsigned int frps     : 10;
        //
        // Last Column Padding Option: 0 - Pad with zeros. 1 - Duplicate the peripheral pixels
        unsigned int lcpo     : 1;
        //
        // First Column Padding Option: 0 - Pad with zeros. 1 - Duplicate the peripheral pixels
        unsigned int fcpo     : 1;
        //
        // Last Row Padding Option: 0 - Pad with zeros. 1 - Duplicate the peripheral pixels
        unsigned int lrpo     : 1;
        //
        // First Row Padding Option: 0 - Pad with zeros. 1 - Duplicate the peripheral pixels
        unsigned int frpo     : 1;
        //
        // CDP Enable
        unsigned int cdpe     : 1;
        //
        // Pad Only - Bypass the De-Compression engine.
        unsigned int padonly  : 1;
        //
        // Split Mode:
        // 0 - RGB Merge - Merges the Green and Blue / Red streams into the original Bayer pattern (Configured by register according to the camera type)
        // 1 - Grayscale Merge - Merges the frame horizontally: Beginning of the line from engine 0 and the rest from engine 1
        // 2 - Source to Destination - Each engine outputs its de-compressed image separately
        // 3-7 - Reserved
        unsigned int mergemod : 3;
        //
        // Frame Type
        unsigned int ftype    : 8;
        // Just for padding
        unsigned int : 3;
        //
        // Extended Descriptor
        unsigned int ed       : 1;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word0_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int : 2;
        //
        // Last Column Padding Size in pixels
        unsigned int lcps : 10;
        //
        // First Column Padding Size in pixels
        unsigned int fcps : 10;
        //
        // Last Row Padding Size - Number of padded rows
        unsigned int lrps : 10;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word1_t;

typedef union {
    //
    //
    struct {
        //
        // Number of Rows
        unsigned int nrow : 16;
        //
        // Number of Columns
        unsigned int ncol : 16;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word2_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int : 6;
        //
        // AXI_WR Channel 0 Frame Auto Increment Enable - Auto Increment the base address at the end of each frame
        unsigned int axiwrch0aie     : 1;
        //
        // AXI_WR Channel 0 Frame Vertical Flip
        unsigned int axiwrch0vflip   : 1;
        //
        // AXI_WR Channel 0 Data Packing on AXI bus: 1 - According to output BPP, 0 - Each pixel is represented by 16 bits regardless of its output BPP
        unsigned int axiwrch0dpck    : 1;
        //
        // AXI_WR Channel 0 FIFO Enable:
        // 0 - Address to AXI Master is incremented each burst (On the AXI bus, address is incremented each transaction)
        // 1 - Address to AXI Master is constant according to the base address (On the AXI bus, address is incremented inside the burst)
        unsigned int axiwrch0fifoen  : 1;
        //
        // AXI_WR Channel 0 DMA Mode:
        // 0 - Frame mode - lines are 128 bits aligned,
        // 1 - Consecutive data mode
        unsigned int axiwrch0mod     : 1;
        //
        // AXI_RD Channel 0 Output Bit Per Pixel (Data Format):
        // 00 - 8 BPP
        // 01 - 10 BPP
        // 10 - 12 BPP
        // 11 - 16 BPP
        unsigned int axirdch0obpp    : 2;
        //
        // AXI_RD Channel 0 Input Bit Per Pixel (Data Format):
        // 00 - 8 BPP
        // 01 - 10 BPP
        // 10 - 12 BPP
        // 11 - 16 BPP
        unsigned int axirdch0ibpp    : 2;
        //
        // AXI_RD Channel 0 Frame Auto Increment Enable - Auto Increment the base address at the end of each frame
        unsigned int axirdch0aie     : 1;
        //
        // AXI_RD Channel 0 BPP Converter Shift Left: Shift the pixel to the left 0-15 bits.
        unsigned int axirdch0bppcsfl : 4;
        //
        // AXI_RD Channel 0 BPP Converter Shift Right: Round up and shift the pixel to the right 0-15 bits.
        unsigned int axirdch0bppcsfr : 4;
        //
        // AXI_RD Channel 0 Data Packing on AXI bus: 1 - According to output BPP, 0 - Each pixel is represented by 16 bits regardless of its output BPP
        unsigned int axirdch0dpck    : 1;
        //
        // AXI_RD Channel 0 FIFO Enable:
        // 0 - Address to AXI Master is incremented each burst (On the AXI bus, address is incremented each transaction)
        // 1 - Address to AXI Master is constant according to the base address (On the AXI bus, address is incremented inside the burst)
        unsigned int axirdch0fifoen  : 1;
        //
        // AXI_RD Channel 0 DMA Mode:
        // 0 - Frame mode - lines are 128 bits aligned,
        // 1 - Consecutive data mode
        unsigned int axirdch0mod     : 1;
        //
        // CDP Descriptor Pointer
        unsigned int cdpdescptr      : 5;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word3_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int : 6;
        //
        // AXI_RD Channel 0 Row Size - In Bytes
        unsigned int axirdch0rs : 26;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word4_t;

typedef union {
    //
    //
    struct {
        //
        // AXI_RD Channel 0 Auto Increment Size in bytes
        unsigned int axirdch0ais  : 16;
        //
        // AXI_RD Channel 0 Row Gap: Gap between two consecutive ROIs rows address in  system memory - In Bytes
        unsigned int axirdch0rgap : 16;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word5_t;

typedef union {
    //
    //
    struct {
        //
        // AXI_RD Channel 0 Base Address - Low
        unsigned int axirdch0ba_l : 32;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word6_t;

typedef union {
    //
    //
    struct {
        //
        // AXI_RD Channel 0 Base Address - High
        unsigned int axirdch0ba_h : 32;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word7_t;

typedef union {
    //
    //
    struct {
        // Just for padding
        unsigned int : 16;
        //
        // AXI_WR Channel 0 Auto Increment Size in bytes
        unsigned int axiwrch0ais : 16;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word8_t;

typedef union {
    //
    //
    struct {
        //
        // AXI_WR Channel 0 Base Address - Low
        unsigned int axiwrch0ba_l : 32;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word9_t;

typedef union {
    //
    //
    struct {
        //
        // AXI_WR Channel 0 Base Address - High
        unsigned int axiwrch0ba_h : 32;
    } bitfield;
    unsigned int register_value;
} dcprs_desc_word10_t;

typedef struct {
    //
    //
    //
    // Offset - 0x0
    dcprs_desc_word0_t  word0;

    //
    //
    //
    // Offset - 0x4
    dcprs_desc_word1_t  word1;

    //
    //
    //
    // Offset - 0x8
    dcprs_desc_word2_t  word2;

    //
    //
    //
    // Offset - 0xc
    dcprs_desc_word3_t  word3;

    //
    //
    //
    // Offset - 0x10
    dcprs_desc_word4_t  word4;

    //
    //
    //
    // Offset - 0x14
    dcprs_desc_word5_t  word5;

    //
    //
    //
    // Offset - 0x18
    dcprs_desc_word6_t  word6;

    //
    //
    //
    // Offset - 0x1c
    dcprs_desc_word7_t  word7;

    //
    //
    //
    // Offset - 0x20
    dcprs_desc_word8_t  word8;

    //
    //
    //
    // Offset - 0x24
    dcprs_desc_word9_t  word9;

    //
    //
    //
    // Offset - 0x28
    dcprs_desc_word10_t word10;

} dcprs_desc_t;

extern volatile dcprs_desc_t *const dcprs_desc;

/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

typedef volatile struct {
    uint32_t mode;
        #define CAST_MODE_ENABLE   (0x01)
    uint32_t error;
    uint32_t unkn_mrk_loc;    // unknown marker error location
    uint32_t unex_mrk_loc;    // unexpected marker error location
    uint32_t scan_hdr_err_loc;// scan header error location
    uint32_t dnl_err_loc;     // DNL error location
    uint32_t ecs_err_num;     // ESC number of error
    uint32_t ecs_err_loc;     // Location of ESC error
    uint32_t version;         // core version
        #define CAST_VER           (0x00010300)
    uint32_t output_dwidth;   // output data width
    uint32_t input_bus_len;   // input bus length
    uint32_t input_buf_size;  // input buffer size
    uint32_t num_impl_pipes;  //  number of implemented pipes
    uint32_t max_rst_interval;//  maximum optimal restart interval
    uint32_t max_near_param;  //  maximum near parameter value supported
} cast_t;

/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include "cdp_top.h"
#include "cdp_common.h"

#define FAST_MAX_WINS   (200)

typedef enum {
    CDP_PYRAMID_RESIZE_1_1 = 0,
    CDP_PYRAMID_RESIZE_4_5,
    CDP_PYRAMID_RESIZE_5_7,
    CDP_PYRAMID_RESIZE_2_3,
    CDP_PYRAMID_RESIZE_5_8,
    CDP_PYRAMID_RESIZE_4_7,
    CDP_PYRAMID_RESIZE_1_2,
    CDP_PYRAMID_RESIZE_5_6,
} cdp_pyramid_scale_t;


// Threshold window set to apply to image.
struct cdp_windows
{
    cdp_rgf_fast_thold_mem_t    thresholds[FAST_MAX_WINS];
    uint16_t n_thresholds;
    uint16_t grid_size_x;
    uint16_t grid_size_y;
};


// The FAST output buffers are a table of these.
struct cdp_score {
    uint16_t    x_ix;
    uint16_t    y_ix;
    uint32_t    score;
};


// Shregion buffer from user space.
struct cdp_buffer {
    uint64_t shname;
    uint64_t addr;
    uint32_t length;
};


// Gaussian blur coefficients.
struct gauss_config {
    uint16_t enable;
    uint16_t shift;
    uint16_t bbb;
    uint16_t center;
    uint16_t cross_in;
    uint16_t cross_out;
    uint16_t corner_in;
    uint16_t corner_out;
};


// Resizing params.
struct cdp_pyramid_cfg {
    uint32_t            x_in, y_in;
    uint32_t            x_out, y_out;
    cdp_pyramid_scale_t scale;
};


// Region of interest configuration.
struct cdp_roi_cfg {
    struct {
        uint16_t enable;
        uint16_t center_x;
        uint16_t center_y;
        uint16_t radius;
    } cir;
    struct {
        uint16_t enable;
        uint16_t border;
    } rec;
};


// Output by driver.
struct cdp_hw_err {
    uint32_t err[CDP_N_STAGES];
    uint32_t global;
    uint32_t master;
};


// NMS grid sizes.
enum nms_type { WIN_3x3, WIN_5x5 };


struct cdp_ioctl {

    // DDR buffer addresses.
    struct cdp_stage_bufs {
        struct cdp_buffer input;      // valid only for stage 0: input image buffer address
        struct cdp_buffer out_axi;    // valid only for stage 2 and 3: blurred and resized output image buffer address
        struct cdp_buffer out_fast;   // valid for all stages: NMS output address for list of points.
    } stage_bufs[CDP_N_STAGES];

    // For one-time configuration at start-up.
    struct {
        struct cdp_pyramid_cfg pyr[CDP_N_STAGES];
    } cfg;

    // For per-image config and running the algorithms.
    struct {
        struct gauss_config       gauss[CDP_N_STAGES];                     // Gaussian coefficients for each stage.
        uint32_t                  pyr_gauss_dis_fast[CDP_N_STAGES];        // Disable downsizing and blur for FAST pipe.
        uint32_t                  fast_seq_len[CDP_N_STAGES];              // Must be between 9 and 15
        uint16_t                  fast_thresh_glob[CDP_N_STAGES];          // set if single threshold is used for whole image, i.e., not using windows (below)
        struct cdp_windows        thresh_wins[CDP_N_STAGES];
        uint32_t                  nms_max_results_per_win[CDP_N_STAGES];
        uint32_t                  nms_results_win_size_x[CDP_N_STAGES];
        uint32_t                  nms_results_win_size_y[CDP_N_STAGES];
        enum nms_type             nms_slide_win[CDP_N_STAGES];
        struct cdp_roi_cfg        roi[CDP_N_STAGES];
    } run;

    struct {
        uint32_t threshold_offset;
        uint32_t map_size;
    } mmap;

    // This is the only section of the cdp_ioctl structure that's modified by the driver.
    // Everything else is guranteed to not change across ioctl calls.
    struct {
        uint32_t fast_count[CDP_N_STAGES];
        struct {
            uint32_t err[CDP_N_STAGES];
            uint32_t err_global;
            uint32_t err_master;
        } hw_err;
    } out;

};


//
// CDP ioctl commands
//
#define CDP_IOC_MAGIC 'C'
#define CDP_CONFIG              _IOWR(CDP_IOC_MAGIC, 55, struct cdp_ioctl)
#define CDP_MMAP                _IOWR(CDP_IOC_MAGIC, 56, struct cdp_ioctl)
#define CDP_RUN                 _IOWR(CDP_IOC_MAGIC, 57, struct cdp_ioctl)

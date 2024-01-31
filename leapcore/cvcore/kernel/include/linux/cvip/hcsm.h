/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include <linux/ioctl.h>
#include <linux/types.h>

struct hcsm_ioctl {
    uint32_t hydra_id;
    uint32_t component_id;
    uint32_t timeout_ms;
    uint64_t time_sent_us;
    uint64_t time_received_us;
    char     queue_holder[25];
    char     op_name[64];
};

#define HCSM_IOC_MAGIC   ('!')
#define HCSM_RUN_REQUEST (_IOWR(HCSM_IOC_MAGIC, 0, struct hcsm_ioctl))


//
// used internally by the hcsm driver
//

int hcsm_attributes_register(struct device *dev);
void hcsm_attributes_remove(struct device *dev);

// must be power of 2
#define HCSM_STAT_SAMPLES (32)

struct hcsm_stat_tracker {
    uint64_t min_read_time_us;
    uint64_t max_read_time_us;
    uint64_t read_samples[HCSM_STAT_SAMPLES];
    uint8_t read_index;

    uint64_t min_write_time_us;
    uint64_t max_write_time_us;
    uint64_t write_samples[HCSM_STAT_SAMPLES];
    uint8_t write_index;

    uint64_t min_sync_time_us;
    uint64_t max_sync_time_us;
    uint64_t sync_samples[HCSM_STAT_SAMPLES];
    uint8_t sync_index;

    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t total_syncs;

    uint64_t avg_read_time_us;
    uint64_t avg_write_time_us;
    uint64_t avg_sync_time_us;

    uint16_t warning_wa;
    uint16_t warning_full_all;
    uint16_t warning_full_queue;
    uint16_t warning_full_fifo;

    uint64_t _start_time_us;
};

void hcsm_stats_init(struct hcsm_stat_tracker *stats);
void hcsm_stats_start(struct hcsm_stat_tracker *stats);
void hcsm_stats_log_read(struct hcsm_stat_tracker *stats);
void hcsm_stats_log_write(struct hcsm_stat_tracker *stats);
void hcsm_stats_log_sync(struct hcsm_stat_tracker *stats);
void hcsm_stats_average(struct hcsm_stat_tracker *stats, struct hcsm_stat_tracker *computed);

void hcsm_stats_log_warning_wrap_around(struct hcsm_stat_tracker *stats);
void hcsm_stats_log_warning_all_full(struct hcsm_stat_tracker *stats);
void hcsm_stats_log_warning_queue_full(struct hcsm_stat_tracker *stats);
void hcsm_stats_log_warning_fifo_full(struct hcsm_stat_tracker *stats);

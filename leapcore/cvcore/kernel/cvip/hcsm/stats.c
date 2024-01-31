// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2023 Magic Leap, Inc. All rights reserved.
//

#include <linux/device.h>

#include "gsm_jrtc.h"
#include "hcsm.h"

void hcsm_stats_init(struct hcsm_stat_tracker *stats) {

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    memset(stats, 0, sizeof(struct hcsm_stat_tracker));

    // preload with unreasonable values, so the logic cleans them up over time
    stats->min_read_time_us = 10000;
    stats->max_read_time_us = 0;
    stats->min_write_time_us = 10000;
    stats->max_write_time_us = 0;
}

void hcsm_stats_start(struct hcsm_stat_tracker *stats) {

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    stats->_start_time_us = gsm_jrtc_get_time_us();
}

void hcsm_stats_log_read(struct hcsm_stat_tracker *stats) {

    uint64_t delta_us;

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    delta_us = gsm_jrtc_get_time_us() - stats->_start_time_us;

    if (stats->min_read_time_us > delta_us) {
        stats->min_read_time_us = delta_us;
    }

    if(stats->max_read_time_us < delta_us) {
        stats->max_read_time_us = delta_us;
    }

    stats->total_reads += 1;
    stats->read_samples[stats->read_index] = delta_us;
    stats->read_index = (stats->read_index + 1) & (HCSM_STAT_SAMPLES-1);
}

void hcsm_stats_log_write(struct hcsm_stat_tracker *stats) {

    uint64_t delta_us;

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    delta_us = gsm_jrtc_get_time_us() - stats->_start_time_us;

    if (stats->min_write_time_us > delta_us) {
        stats->min_write_time_us = delta_us;
    }

    if(stats->max_write_time_us < delta_us) {
        stats->max_write_time_us = delta_us;
    }

    stats->total_writes += 1;
    stats->write_samples[stats->write_index] = delta_us;
    stats->write_index = (stats->write_index + 1) & (HCSM_STAT_SAMPLES-1);
}

void hcsm_stats_log_sync(struct hcsm_stat_tracker *stats) {

    uint64_t delta_us;

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    delta_us = gsm_jrtc_get_time_us() - stats->_start_time_us;

    if (stats->min_sync_time_us > delta_us) {
        stats->min_sync_time_us = delta_us;
    }

    if(stats->max_sync_time_us < delta_us) {
        stats->max_sync_time_us = delta_us;
    }

    stats->total_syncs += 1;
    stats->sync_samples[stats->sync_index] = delta_us;
    stats->sync_index = (stats->sync_index + 1) & (HCSM_STAT_SAMPLES-1);
}

void hcsm_stats_average(struct hcsm_stat_tracker *stats, struct hcsm_stat_tracker *computed) {

    int i;

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    if (NULL == computed) {
        pr_err("computed-ptr is null\n");
        return;
    }

    // make sure all writes completed before we copy over
    // small chance of a overwrite/corruption but we're ok w/that for now
    smp_wmb();
    memcpy(computed, stats, sizeof(struct hcsm_stat_tracker));

    computed->avg_read_time_us = 0;
    computed->avg_write_time_us = 0;
    computed->avg_sync_time_us = 0;

    // operate on the computed structure only; stats can change at any time
    for(i=0; i<HCSM_STAT_SAMPLES; i++) {
        computed->avg_read_time_us += computed->read_samples[i];
        computed->avg_write_time_us += computed->write_samples[i];
        computed->avg_sync_time_us += computed->sync_samples[i];
    }

    computed->avg_read_time_us  /= HCSM_STAT_SAMPLES;
    computed->avg_write_time_us /= HCSM_STAT_SAMPLES;
    computed->avg_sync_time_us  /= HCSM_STAT_SAMPLES;
}

void hcsm_stats_log_warning_wrap_around(struct hcsm_stat_tracker *stats) {

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    stats->warning_wa += 1;
}

void hcsm_stats_log_warning_all_full(struct hcsm_stat_tracker *stats) {

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    stats->warning_full_all += 1;
}

void hcsm_stats_log_warning_queue_full(struct hcsm_stat_tracker *stats) {

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    stats->warning_full_queue += 1;
}

void hcsm_stats_log_warning_fifo_full(struct hcsm_stat_tracker *stats) {

    if (NULL == stats) {
        pr_err("stats-ptr is null\n");
        return;
    }

    stats->warning_full_fifo += 1;
}

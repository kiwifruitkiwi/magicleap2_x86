// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2023 Magic Leap, Inc. All rights reserved.
//

#include <linux/device.h>

#include "gsm_jrtc.h"
#include "hcsm.h"

// stats attribute
static ssize_t stats_show(struct device *dev, struct device_attribute *attr, char *buf) {

    struct hcsm_stat_tracker pri, sec;
    struct hcsm_stat_tracker *stats_array = dev_get_drvdata(dev);
    char *bbuf = buf;

    memset(&pri, 0, sizeof(struct hcsm_stat_tracker));
    memset(&sec, 0, sizeof(struct hcsm_stat_tracker));

    // collect stats
    hcsm_stats_average(&(stats_array[0]), &pri);
    hcsm_stats_average(&(stats_array[1]), &sec);

    buf += snprintf(buf, PAGE_SIZE, "Primary Stats (us)\n");
    buf += snprintf(buf, PAGE_SIZE, "  r: min-avg-max [%04llu]-[%04llu]-[%04llu]\n",
                    pri.min_read_time_us, pri.avg_read_time_us, pri.max_read_time_us);
    buf += snprintf(buf, PAGE_SIZE, "  w: min-avg-max [%04llu]-[%04llu]-[%04llu]\n",
                    pri.min_write_time_us, pri.avg_write_time_us, pri.max_write_time_us);
    buf += snprintf(buf, PAGE_SIZE, "  s: min-avg-max [%04llu]-[%04llu]-[%04llu]\n",
                    pri.min_sync_time_us, pri.avg_sync_time_us, pri.max_sync_time_us);

    buf += snprintf(buf, PAGE_SIZE, "Secondary Stats (us)\n");
    buf += snprintf(buf, PAGE_SIZE, "  r: min-avg-max [%04llu]-[%04llu]-[%04llu]\n",
                    sec.min_read_time_us, sec.avg_read_time_us, sec.max_read_time_us);
    buf += snprintf(buf, PAGE_SIZE, "  w: min-avg-max [%04llu]-[%04llu]-[%04llu]\n",
                    sec.min_write_time_us, sec.avg_write_time_us, sec.max_write_time_us);
    buf += snprintf(buf, PAGE_SIZE, "  s: min-avg-max [%04llu]-[%04llu]-[%04llu]\n",
                    sec.min_sync_time_us, sec.avg_sync_time_us, sec.max_sync_time_us);

    buf += snprintf(buf, PAGE_SIZE, "Total Counts\n");
    buf += snprintf(buf, PAGE_SIZE, "  p: r-w-s       [%07llu]-[%07llu]-[%07llu]\n",
                    pri.total_reads, pri.total_writes, pri.total_syncs);
    buf += snprintf(buf, PAGE_SIZE, "  s: r-w-s       [%07llu]-[%07llu]-[%07llu]\n",
                    sec.total_reads, sec.total_writes, sec.total_syncs);

    buf += snprintf(buf, PAGE_SIZE, "Total Warnings\n");
    buf += snprintf(buf, PAGE_SIZE, "  p: wa-fa-fq-ff [%02u]-[%02u]-[%02u]-[%02u]\n",
                    pri.warning_wa, pri.warning_full_all, pri.warning_full_queue, pri.warning_full_fifo);
    buf += snprintf(buf, PAGE_SIZE, "  s: wa-fa-fq-ff [%02u]-[%02u]-[%02u]-[%02u]\n",
                    sec.warning_wa, sec.warning_full_all, sec.warning_full_queue, sec.warning_full_fifo);
    buf += snprintf(buf, PAGE_SIZE, "\n");

    return buf - bbuf;
}
static DEVICE_ATTR_RO(stats);

// the ocd-organizer in me wants to see something like
//  .../stats/all
//  .../stats/primary/
//  .../stats/primary/avgs/...
//
// but this is apparently a mighty hard thing to do so we'll mark that as a nice-to-have
//
static struct attribute *hcsm_attributes[] = {
    &dev_attr_stats.attr,
    NULL
};

static const struct attribute_group hcsm_attr_group = {
    .attrs = hcsm_attributes,
};

int hcsm_attributes_register(struct device *dev) {

    int result;

    result = sysfs_create_group(&(dev->kobj), &hcsm_attr_group);
    if (result) {
        pr_err("bad sysfs_create_group [%d]\n", result);
        return -ENXIO;
    }

    return 0;
}

void hcsm_attributes_remove(struct device *dev) {
    sysfs_remove_group(&(dev->kobj), &hcsm_attr_group);
}

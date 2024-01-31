// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

// pr_fmt will insert our module name into the pr_info() macro.
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/console.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/hrtimer.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/version.h>
#include "hsi.h"
#include "hsi_priv.h"

// if we're not on cvip, don't do _any_ of this
#ifdef __aarch64__

#include "gsm_jrtc.h"

#ifndef CONFIG_GPIOLIB
#error THIS IS A PROBLEM
#endif

#define GPIO_FIRST              (320)
#define GPIO_TIME_SYNC          (327 - GPIO_FIRST)

static int wearable_open(struct inode *, struct file *);
static int wearable_release(struct inode *, struct file *);
static long wearable_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations const fops_wearable = {
    .owner          = THIS_MODULE,
    .open           = wearable_open,
    .release        = wearable_release,
    .unlocked_ioctl = wearable_ioctl,
};

static struct {
    struct class      *class;
    struct cdev       cdev;
    struct device     *dev;
    dev_t             devno;

    int64_t           start;
    int64_t           end;
    wait_queue_head_t waitq;
    int               triggered;
} g_dev;


static enum hrtimer_restart tmr_cb(struct hrtimer *tmr)
{
    g_dev.end = ktime_to_ns(ktime_get());

    // Tell the hydras to sync.
    gpio_set_value(GPIO_TIME_SYNC, 1);
    udelay(500);
    gpio_set_value(GPIO_TIME_SYNC, 0);
    pr_info("sent hydra sync signal!\n");

    g_dev.triggered = 1;
    wake_up_interruptible(&g_dev.waitq);

    return HRTIMER_NORESTART;
}


static int wearable_open(struct inode *pInode, struct file *pFile)
{
    return 0;
}


static int wearable_release(struct inode *pInode, struct file *pFile)
{
    return 0;
}


static long wearable_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
    static struct hrtimer tmr;
    static ktime_t ktime_interval;
    uint64_t trig, cur = 0;

    if (copy_from_user(&trig, (void __user *)arg, sizeof(trig))) {
        pr_err("mem fault!\n");
        return -EFAULT;
    }

    cur = gsm_jrtc_get_time_ns();

    //pr_info("%s: trig_ns=0x%llx, cur_ns=0x%llx, diff_ns=%llx, res=%d ns\n", __func__, trig, cur, trig - cur, hrtimer_resolution);

    // Make sure trigger in in the future.
    if (cur > trig) {
        pr_err("trigger is in the past!\n");
        return -EIO;
    }

    // Only support timesync operation for now, so ignore command.
    if (cmd) {
        ktime_interval = ktime_set( 0, trig - cur );
        hrtimer_init(&tmr, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        tmr.function = tmr_cb;
        g_dev.triggered = 0;
        g_dev.start = ktime_to_ns(ktime_get());
        hrtimer_start(&tmr, ktime_interval, HRTIMER_MODE_REL);

        wait_event_interruptible(g_dev.waitq, g_dev.triggered);

        //pr_info("k_start=%lld, k_end=%lld, diff_ns=%lld, diff_ms=%lld\n", g_dev.start, g_dev.end, g_dev.end - g_dev.start, (g_dev.end - g_dev.start) / 1000 / 1000);
    }

    return 0;
}


void wearable_create(void)
{
    int ret;

    memset(&g_dev, 0, sizeof(g_dev));

    // Acquire timesync gpio.
    if ((ret = gpio_request_one(GPIO_TIME_SYNC, GPIOF_OUT_INIT_LOW, "time_sync")) < 0) {
        pr_err("can't get TIMESYNC gpio %d, ret=%d!\n", GPIO_TIME_SYNC, ret);
    }

    g_dev.class = class_create(THIS_MODULE, "wearable");
    if (IS_ERR(g_dev.class)) {
        pr_err("class_create failed\n");
        gpio_free(GPIO_TIME_SYNC);
        return;
    }

    // Allocate a major device number and a range of minor device numbers.
    ret = alloc_chrdev_region(&g_dev.devno, 0, 1, "wearable");
    if (ret < 0) {
        pr_err("can't allocate hsi major dev num. (%d)\n", ret);
        class_destroy(g_dev.class);
        gpio_free(GPIO_TIME_SYNC);
        return;
    }

    // Register device.
    cdev_init(&g_dev.cdev, &fops_wearable);
    g_dev.cdev.owner = THIS_MODULE;
    ret = cdev_add(&g_dev.cdev, g_dev.devno, 1);
    if (ret) {
        pr_err("bad cdev_add (%d)\n", ret);
        unregister_chrdev_region(g_dev.devno, 1);
        class_destroy(g_dev.class);
        gpio_free(GPIO_TIME_SYNC);
        return;
    }

    g_dev.dev = device_create(g_dev.class, NULL, g_dev.devno, NULL, "wearable");
    if (IS_ERR(g_dev.dev)) {
        pr_err("bad dev create.\n");
        unregister_chrdev_region(g_dev.devno, 1);
        cdev_del(&g_dev.cdev);
        class_destroy(g_dev.class);
        gpio_free(GPIO_TIME_SYNC);
        return;
    }

    init_waitqueue_head(&g_dev.waitq);
}


void wearable_destroy(void)
{
    device_destroy(g_dev.class, g_dev.devno);
    cdev_del(&g_dev.cdev);
    unregister_chrdev_region(g_dev.devno, 1);
    class_destroy(g_dev.class);
    gpio_free(GPIO_TIME_SYNC);
}
#else
void wearable_create(void) { }
void wearable_destroy(void) { }
#endif

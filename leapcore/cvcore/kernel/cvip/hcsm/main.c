// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

// pr_fmt will insert our module name into the pr_info() macro.
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/tty_flip.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/circ_buf.h>
#include <linux/kfifo.h>

#include "hcsm.h"
#include "hydra.h"
#include "gsm_jrtc.h"
#include "shmem_map.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_AUTHOR("cmatthews@magicleap.com");
MODULE_DESCRIPTION("Hydra Control Service Module");

#define HCSM_MODULE_NAME      ("hcsm")
#define HCSM_CLASS_NAME       ("magicleap-hcsm")

#define HCSM_MAX_HYDRAS       (2)
#define HCSM_MAX_COMPONENTS   (31)

// Quite a bit to talk about here so... some notes
//
// 1) IRQ handling
//      - we use a tasklet to handle processing of responses due to a surprisingly variable
//        amount of time when accessing hydra-sram space.
//        min-avg-max times seem to fall in the 0-20-60us range for an arbitrary read. This is
//        probably influenced in some way by cvip/pci loading but its not obvious how.
//        writes on the other hand are are almost always <1us but that's due to pci
//        not waiting for the writes to get to their destination (reads have no choice)
//      - to ease the variability of the reads, the queue structure, size and driver
//        behavior is architected to be as consistent as possible, leaving out changing/fluid
//        read/write behavior.
// 2) Queue Design
//      - taking the above into consideration, we use a circular queue on both
//        inbound/outbound activity, with a slightly modified approach to handling
//        the full vs. empty scenario - this is done to ensure maximum utilization of
//        available space while keeping the total reads/writes to a minimum.
//      - originally, the queue len was 4 deep on both sides and we saw ~2 elements at any given time.
//        with increased control activity, we're now seeing upwards of 4 if not more, so the total len
//        is now set to 8 on each side with an additional fifo in cvip side; if we still find issues
//        where we're backed up and unable to respond fast enough, we could make the response queue deeper
//        and rely more on the fifo to hold pending requests before sending them to hydra.
// 3) Control Flow handled through one tasklet
//      - originally, we used spinlocks to control queue access between the tasklet and the ioctls
//        but as it turns out, tasklet consumes an entire cpu when run (like an isr) and spinlocks will also
//        cause the running thread to spin/consume a cpu with no chance of preemption. while that's ok
//        for the tasklet itself, that's not-so-great for ioctls, of which we'll have many. While this still
//        should be quick on the ioctl side, the tasklet will end up being the bottle-neck with slow-ish
//        pci-access and a potential for multiple ioctls to consume multiple cpus and not letting anyone
//        do anything until the tasklet itself is done. one might describe that as 'horribly inefficient' so
//        its been changed to not rely on spinlocks.
//      - these are important things to understand for the following implementation:
//        -- a tasklet will only ever run one instance of itself at any given time, on a single cpu, across
//           the entire system, no matter how many cpus there are.
//        -- when a tasklet is scheduled, but not running
//           --- if another tasklet schedule event comes in, this is quietly discarded
//        -- when a tasklet is running
//           --- if another tasklet schedule event comes in, this IS queued into the list of things to run
//      - to remove the spinlock on the ioctl (requester) side, we switch to kfifo and a mutex; the tasklet
//        sits on the other end of the queue, consuming as much as it can at all times

struct hcsm_queues {
  uint8_t hm_head_resp;
  uint8_t cm_tail_resp;
  uint8_t cm_head_req;
  uint8_t hm_tail_req;
  uint8_t req_queue[8];
  uint8_t resp_queue[8];
} __attribute__((__packed__));
// must be evenly divisible by 32

#define HCSM_SET_QUEUE_FULL(head)           ( head |= 0x80 )
#define HCSM_INC_QUEUE_OBJ(head_tail)       ( (1 + head_tail) & 0x07 )
#define HCSM_WILL_QUEUE_BE_FULL(toadd, ref) ( ((toadd + 1) & 0x07) == ref )
#define HCSM_IS_QUEUE_FULL(head)            ( (head & 0x80) )
#define HCSM_CLR_QUEUE_FULL(head)           ( (head &= 0x7F) )

#define HCSM_REQUEST_FIFO_LEN               (8)

struct hcsm_component {
    uint8_t data_received;
    int error_code;
    uint64_t time_sent_us;
    wait_queue_head_t waitq;
    char queue_holder[25];
    char op_name[64];
};

struct hcsm_isr_context {
    struct tasklet_struct tasklet;
    volatile uint8_t hydra_present;
    uint64_t expected_replies;
};

int hydra_event_handler(struct notifier_block *self,
                          unsigned long val, void *data);
static struct notifier_block hydra_event_notifier = {
   .notifier_call = hydra_event_handler,
};

// Device-global data.
static struct hcsm_device {
    dev_t                        major_num;
    struct class                 *class;
    struct device                *dev;
    struct device                *sys_dev;
    struct cdev                  cdev;
    struct hcsm_component        components[HCSM_MAX_HYDRAS][HCSM_MAX_COMPONENTS];
    volatile struct hcsm_queues *hcp_queues[HCSM_MAX_HYDRAS];
    struct hcsm_isr_context      isrc[HCSM_MAX_HYDRAS];
    struct hcsm_stat_tracker     stats[HCSM_MAX_HYDRAS];
    struct mutex                 fifo_locks[HCSM_MAX_HYDRAS];
    DECLARE_KFIFO(fifo_requests, uint8_t, HCSM_REQUEST_FIFO_LEN)[HCSM_MAX_HYDRAS];
} hcsm_dev;

static int hcsm_open(struct inode *pInode, struct file *pFile) {
    (void) pInode;
    (void) pFile;
    return 0;
}

static int hcsm_release(struct inode *pInode, struct file *pFile) {
    (void) pInode;
    (void) pFile;
    return 0;
}

static void hcsm_hcp_callback(uint32_t hydra_id) {
    tasklet_hi_schedule(&(hcsm_dev.isrc[hydra_id].tasklet));
}

static void hcsm_tasklet_controller(unsigned long id) {

    int i;
    uint8_t hydra_id;
    uint32_t raw32;
    uint8_t component_id;
    struct hcsm_component *c_data;
    struct hcsm_queues cache;
    volatile struct hcsm_queues *remote_data;
    uint32_t *src, *dest;
    struct hcsm_isr_context *isrc;
    struct hcsm_stat_tracker *stats;

    // this has got to be the one of strangest c-syntactical-capable things I've ever seen
    // its not possible, outside of this, to get a pointer to a kfifo obj because its anonymous
    typeof(hcsm_dev.fifo_requests[0]) *fifo_ptr;

    uint8_t flag_requested = 0;
    uint8_t flag_consumed = 0;
    uint8_t flag_write_resp_head = 0;

    hydra_id = (uint8_t) (id & 0xFF);

    if(hydra_id >= HCSM_MAX_HYDRAS) {
        pr_err("hydra-id in controller is out of range [%d]\n", hydra_id);
        return;
    }

    remote_data = hcsm_dev.hcp_queues[hydra_id];
    isrc = &(hcsm_dev.isrc[hydra_id]);
    stats = &(hcsm_dev.stats[hydra_id]);
    fifo_ptr = &(hcsm_dev.fifo_requests[hydra_id]);

    smp_rmb();
    if(0 == isrc->hydra_present) {
        pr_err("hydra-[%d]: no longer present but notification previously sent\n", hydra_id);
        pr_err("hydra-[%d]: alerting any pending requestors [0x%llX]\n", hydra_id, isrc->expected_replies);
        // alert anyone who was expecting something; the error-code shouldn't be necessary but is a precaution
        if(isrc->expected_replies) {
            for(i=0; i<HCSM_MAX_COMPONENTS; i++) {
                if(isrc->expected_replies & (uint64_t)(1 << i)) {
                    c_data = &(hcsm_dev.components[hydra_id][i]);
                    c_data->data_received = 1;
                    c_data->error_code = -ENODEV;
                    smp_wmb();
                    wake_up_interruptible(&(c_data->waitq));
                    isrc->expected_replies &= ~((uint64_t)(1 << i));
                }
            }
        }
        return;
    }

    // potentially one downside to all this is we will always have to read from hydra
    // instead of knowing to read or not to read, now that we can be scheduled from
    // two different sources; the up-side is if this does happen, we can process everything
    // in one iteration - both read and write, more efficiently
    memset(&cache, 0, sizeof(struct hcsm_queues));

    hcsm_stats_start(stats);

    // read in the entire queue structure; can only read 4 bytes at a time
    // total right now is 5 reads
    src = (uint32_t *) remote_data;
    dest = (uint32_t *) &cache;
    for(i=0; i<(sizeof(struct hcsm_queues) / 4); i++) {
        raw32 = ioread32(&(src[i]));
        memcpy(&(dest[i]), &raw32, 4);
    }

    hcsm_stats_log_read(stats);

    // response queue first; will be ignored if there's nothing to do
    while(cache.hm_head_resp != cache.cm_tail_resp)  {
        flag_consumed = 1;

        component_id = cache.resp_queue[cache.cm_tail_resp];
        c_data = &(hcsm_dev.components[hydra_id][component_id]);
        cache.cm_tail_resp = HCSM_INC_QUEUE_OBJ(cache.cm_tail_resp);

        if(c_data->data_received) {
            // this shouldn't happen and if it does, there's a likelyhood hydra had to drop the
            // response due to full response queue. regardless, print out some debug info when
            // this occurs.
            pr_warn("hydra-[%d]: notifying component [%d] complete but previous notification wasn't processed!\n", hydra_id, component_id);
            pr_warn("hydra-[%d]: [%s]-[%s] may be impacted\n", hydra_id, c_data->queue_holder, c_data->op_name);
            pr_warn("hydra-[%d]: time since req sent [%llu]\n", hydra_id, (gsm_jrtc_get_time_us() - c_data->time_sent_us));
            hcsm_stats_log_warning_wrap_around(stats);
        }

        c_data->data_received = 1;
        smp_wmb();
        wake_up_interruptible(&(c_data->waitq));

        // if the queue was full, mark it not full
        if(HCSM_IS_QUEUE_FULL(cache.hm_head_resp)) {
            HCSM_CLR_QUEUE_FULL(cache.hm_head_resp);
            flag_write_resp_head = 1;
        }

        isrc->expected_replies &= ~((uint64_t)(1 << component_id));
    }

    if(kfifo_is_full(fifo_ptr) && HCSM_IS_QUEUE_FULL(cache.cm_head_req)) {
        pr_crit("hydra-[%d]: pci-request and fifo are full\n", hydra_id);
        hcsm_stats_log_warning_all_full(stats);
    } else {
        if(HCSM_IS_QUEUE_FULL(cache.cm_head_req)) {
            pr_warn("hydra-[%d]: pci-request queue is full\n", hydra_id);
            hcsm_stats_log_warning_queue_full(stats);
        }
        if(kfifo_is_full(fifo_ptr) && HCSM_IS_QUEUE_FULL(cache.cm_head_req)) {
            pr_warn("hydra-[%d]: fifo is full\n", hydra_id);
            hcsm_stats_log_warning_fifo_full(stats);
        }
    }

    // then process outbound requests; will be skipped if nothing to do
    while((0 == kfifo_is_empty(fifo_ptr)) && (0 == HCSM_IS_QUEUE_FULL(cache.cm_head_req))) {
        uint8_t val;
        flag_requested = 1;

        // drain from our queue and place into hydra's queue
        // use the return val otherwise compiler complains
        val = kfifo_get(fifo_ptr, &(cache.req_queue[cache.cm_head_req]));
        (void) val;
        smp_wmb();

        isrc->expected_replies |= (1 << cache.req_queue[cache.cm_head_req]);

        // queue will be full means our head has caught up to the tail,
        // we have to ensure the full bit is set
        if(HCSM_WILL_QUEUE_BE_FULL(cache.cm_head_req, cache.hm_tail_req)) {
            cache.cm_head_req = HCSM_INC_QUEUE_OBJ(cache.cm_head_req);
            HCSM_SET_QUEUE_FULL(cache.cm_head_req);
        } else {
            cache.cm_head_req = HCSM_INC_QUEUE_OBJ(cache.cm_head_req);
        }
    }

    if(flag_consumed || flag_requested) {
        hcsm_stats_start(stats);
    }

    // only write back what we have to
    if(flag_consumed) {
        if(flag_write_resp_head) {
            iowrite16(cache.hm_head_resp, (void *)&(remote_data->hm_head_resp));
        } else {
            iowrite8(cache.cm_tail_resp, (void *)&(remote_data->cm_tail_resp));
        }
    }

    if(flag_requested) {
        iowrite8(cache.cm_head_req, (void *)&(remote_data->cm_head_req));
        iowrite32(*((uint32_t *)&(cache.req_queue[0])), (void *)&(remote_data->req_queue[0]));
        iowrite32(*((uint32_t *)&(cache.req_queue[4])), (void *)&(remote_data->req_queue[4]));
    }

    //
    // writes to pci are asynchronous (i.e. scheduled) and their order is not guaranteed.
    // forcing a read after any writes however, should flush and commit them all. this will ensure
    // hydra has the data before we signal them. unfortunately, this can be quite costly at times
    // and there's no control or apparent rhythm to it; during testing, noted a max delay of
    // nearly 1ms (~950us) on the high end and sub-us (0us) on the low.
    //
    if(flag_consumed || flag_requested) {
        hcsm_stats_log_write(stats);
        hcsm_stats_start(stats);
        ioread32((void *)remote_data);
        hcsm_stats_log_sync(stats);
    }

    if(flag_requested) {
        hydra_hcp_irq_post(hydra_id);
    }
}

static long hcsm_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg) {

    long retval;
    struct mutex *lock;
    typeof(hcsm_dev.fifo_requests[0]) *fifo_ptr;
    uint32_t fifo_count;
    struct hcsm_component *c_data;
    struct hcsm_ioctl ioctl_arg;

    // Make sure we got a valid ioctl control struct.
    if(!access_ok((void *)arg, sizeof(ioctl_arg))) {
        pr_err("ioctl: bad ioctl_arg address\n");
        return -EFAULT;
    }

    // Get user's ioctl info.
    if (copy_from_user(&ioctl_arg, (void __user *)arg, sizeof(ioctl_arg))) {
        pr_err("ioctl: copy_from_user failed\n");
        return -EBADE;
    }

    if(ioctl_arg.hydra_id > HCSM_MAX_HYDRAS) {
        pr_err("ioctl: supplied hydra-id is out of bounds [%d > %d]\n", ioctl_arg.hydra_id, HCSM_MAX_HYDRAS);
        return -EOVERFLOW;
    }

    if(ioctl_arg.component_id > HCSM_MAX_COMPONENTS) {
        pr_err("ioctl: supplied component-id is out of bounds [%d > %d]\n", ioctl_arg.component_id, HCSM_MAX_COMPONENTS);
        return -EOVERFLOW;
    }

    // grab ptrs to hydra of interest
    lock = &(hcsm_dev.fifo_locks[ioctl_arg.hydra_id]);

    // in the future, if locking control is pushed into kernel; we'll need to lock before this point
    c_data = &(hcsm_dev.components[ioctl_arg.hydra_id][ioctl_arg.component_id]);
    fifo_ptr = &(hcsm_dev.fifo_requests[ioctl_arg.hydra_id]);

    switch(cmd) {
        case HCSM_RUN_REQUEST:

            smp_rmb();
            if (!hcsm_dev.isrc[ioctl_arg.hydra_id].hydra_present) {
                pr_err("ioctl: hydra-[%d] no longer present\n", ioctl_arg.hydra_id);
                pr_err("ioctl: [%s]-[%s] impacted\n", ioctl_arg.queue_holder, ioctl_arg.op_name);
                retval = -ENODEV;
                break;
            }

            // copy some debug strings and mark data not received
            memcpy(c_data->queue_holder, ioctl_arg.queue_holder, sizeof(ioctl_arg.queue_holder));
            memcpy(c_data->op_name, ioctl_arg.op_name, sizeof(ioctl_arg.op_name));

            // many-producers, one consumer of requests; gotta lock the queue on this end
            mutex_lock(lock);
            fifo_count = kfifo_put(fifo_ptr, ioctl_arg.component_id);

            // we're full, error time!
            if(0 == fifo_count) {
                mutex_unlock(lock);
                retval = -EXFULL;
                break;
            }

            if(fifo_count+1 == HCSM_REQUEST_FIFO_LEN) {
                pr_warn("ioctl: hydra[%d] request queue has 1 slot left\n", ioctl_arg.hydra_id);
            }

            c_data->data_received = 0;
            mutex_unlock(lock);
            smp_wmb();

            ioctl_arg.time_sent_us = gsm_jrtc_get_time_us();
            c_data->time_sent_us = ioctl_arg.time_sent_us;

            smp_rmb();
            tasklet_hi_schedule(&(hcsm_dev.isrc[ioctl_arg.hydra_id].tasklet));

            retval = wait_event_interruptible_timeout((c_data->waitq), (c_data->data_received == 1), (ioctl_arg.timeout_ms * HZ / 1000));

            ioctl_arg.time_received_us = gsm_jrtc_get_time_us();
            smp_rmb();
            if ((!hcsm_dev.isrc[ioctl_arg.hydra_id].hydra_present) || (-ENODEV == c_data->error_code)) {
                pr_err("ioctl: hydra-[%d] no longer present\n", ioctl_arg.hydra_id);
                pr_err("ioctl: [%s]-[%s] impacted\n", ioctl_arg.queue_holder, ioctl_arg.op_name);
                retval = -ENODEV;
                break;
            }

            // timeout case - we _really_ timed out
            if (retval == 0) {
                pr_err("ioctl: hydra[%d] did not respond for component-request [%d]\n", ioctl_arg.hydra_id, ioctl_arg.component_id);
                pr_err("ioctl: [%s]-[%s] impacted\n", ioctl_arg.queue_holder, ioctl_arg.op_name);
                retval = -ETIME;
                break;
            }

            // unknown error situation
            if (retval < 0) {
                pr_err("ioctl: unexpected return value for wait_event [%d] [%d]\n", ioctl_arg.hydra_id, ioctl_arg.component_id);
                pr_err("ioctl: [%s]-[%s] impacted\n", ioctl_arg.queue_holder, ioctl_arg.op_name);
                retval = -EPROTO;
                break;
            }

            retval = 0;
            break;
        default:
            pr_err("ioctl: illegal command [0x%x]\n", cmd);
            retval = -EILSEQ;
            break;
    }

    // success or failure, copy ioctl info back.
    if (copy_to_user((void __user *)arg, &ioctl_arg, sizeof(ioctl_arg))) {
        pr_err("ioctl: copy_to_user failed\n");
        return -EBADE;
    }

    return retval;
}

static struct file_operations const hcsm_file_ops = {
    .owner          = THIS_MODULE,
    .open           = hcsm_open,
    .release        = hcsm_release,
    .unlocked_ioctl = hcsm_ioctl
};

static char *hcsm_devnode(struct device *dev, umode_t *mode) {
    if (!mode) {
        return NULL;
    }
    *mode = 0666;
    return NULL;
}

static void hcsm_setup_hcp_mem(void) {

    void *ptr;

    hydra_get_sram_hcp_ptr(HIX_HYDRA_PRI, &ptr);
    hcsm_dev.hcp_queues[HIX_HYDRA_PRI] = (struct hcsm_queues *)ptr;

    hydra_get_sram_hcp_ptr(HIX_HYDRA_SEC, &ptr);
    hcsm_dev.hcp_queues[HIX_HYDRA_SEC] = (struct hcsm_queues *)ptr;
}

int hydra_event_handler(struct notifier_block *self,
                          unsigned long hix, void *data)
{

    if(hix >= HCSM_MAX_HYDRAS) {
        pr_err("event-handler hix out of range [%ld]\n", hix);
        return NOTIFY_OK;
    }

    hcsm_dev.isrc[hix].hydra_present = (0 != data);
    smp_wmb();

    if (hcsm_dev.isrc[hix].hydra_present) {
        hcsm_setup_hcp_mem();
        pr_info("hydra[%ld] present\n", hix);
    } else {
        pr_info("hydra[%ld] NOT present\n", hix);
    }

    return NOTIFY_OK;
}

static int __init hcsm_module_init(void) {

    int result, i, j;

    memset(&(hcsm_dev), 0, sizeof(hcsm_dev));

    // initialize all the wait queues
    for(i=0; i<HCSM_MAX_HYDRAS; i++) {
        for(j=0; j<HCSM_MAX_COMPONENTS; j++) {
            init_waitqueue_head(&(hcsm_dev.components[i][j].waitq));
        }
        hcsm_stats_init(&(hcsm_dev.stats[i]));
        tasklet_init(&(hcsm_dev.isrc[i].tasklet), hcsm_tasklet_controller, i);
        mutex_init(&(hcsm_dev.fifo_locks[i]));
        INIT_KFIFO(hcsm_dev.fifo_requests[i]);

        // start off by assuming hydra is present (this will have to change)
        hcsm_dev.isrc[i].hydra_present = 1;
    }

    // Create a single ML class for all the devices we're going to expose.
    hcsm_dev.class = class_create(THIS_MODULE, HCSM_CLASS_NAME);
    if (IS_ERR(hcsm_dev.class)) {
        pr_err("class_create failed\n");
        return -ENXIO;
    }
    hcsm_dev.class->devnode = hcsm_devnode;

    // Create root in sysfs.
    hcsm_dev.sys_dev = root_device_register(HCSM_MODULE_NAME);
    if (IS_ERR(hcsm_dev.sys_dev)) {
        pr_err("bad sys_dev create.\n");
        class_destroy(hcsm_dev.class);
        return -ENXIO;
    }

    result = hcsm_attributes_register(hcsm_dev.sys_dev);
    if(result) {
        root_device_unregister(hcsm_dev.sys_dev);
        class_destroy(hcsm_dev.class);
        return -ENXIO;
    }

    // should follow this behavior for everything
    dev_set_drvdata(hcsm_dev.sys_dev, hcsm_dev.stats);

    result = alloc_chrdev_region(&hcsm_dev.major_num, 0, 1, "hcsm-mgmt");
    if (result) {
        pr_err("can't allocate major dev num. (%d)\n", result);
        root_device_unregister(hcsm_dev.sys_dev);
        class_destroy(hcsm_dev.class);
        return -ENXIO;
    }

    // Create a single character device.
    cdev_init(&hcsm_dev.cdev, &hcsm_file_ops);
    hcsm_dev.cdev.owner = THIS_MODULE;
    result = cdev_add(&hcsm_dev.cdev, MKDEV(MAJOR(hcsm_dev.major_num), 0), 1);
    if (result) {
        pr_err("bad cdev_add (%d)\n", result);
        unregister_chrdev(MAJOR(hcsm_dev.major_num), HCSM_MODULE_NAME);
        root_device_unregister(hcsm_dev.sys_dev);
        class_destroy(hcsm_dev.class);
        return -ENXIO;
    }

    // Announce it to sysfs.
    hcsm_dev.dev = device_create(hcsm_dev.class, NULL, hcsm_dev.major_num, &hcsm_dev, HCSM_MODULE_NAME);
    if (IS_ERR(hcsm_dev.dev)) {
        pr_err("bad dev create.\n");
        unregister_chrdev(MAJOR(hcsm_dev.major_num), HCSM_MODULE_NAME);
        cdev_del(&hcsm_dev.cdev);
        root_device_unregister(hcsm_dev.sys_dev);
        class_destroy(hcsm_dev.class);
        return -ENXIO;
    }

    // Register / interface with hydra.ko
    register_hydra_notifier(&hydra_event_notifier);
    hcsm_setup_hcp_mem();
    hydra_register_hcp_resp_handler(hcsm_hcp_callback);

    pr_info("setup is complete complete\n");

    return 0;
}

static void __exit hcsm_module_exit(void) {

    // Don't care about hydra events any longer.
    unregister_hydra_notifier(&hydra_event_notifier);
    hydra_register_hcp_resp_handler(0);
    device_destroy(hcsm_dev.class, hcsm_dev.major_num);
    cdev_del(&hcsm_dev.cdev);
    unregister_chrdev_region(hcsm_dev.major_num, 1);
    hcsm_attributes_remove(hcsm_dev.sys_dev);
    root_device_unregister(hcsm_dev.sys_dev);
    class_destroy(hcsm_dev.class);
    memset(&(hcsm_dev.hcp_queues), 0, sizeof(hcsm_dev.hcp_queues));
    pr_info("teardown is complete\n");
}

module_init(hcsm_module_init);
module_exit(hcsm_module_exit);

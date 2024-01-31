// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2019, The Linux Foundation. All rights reserved. */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/mod_devicetable.h>
#include <linux/mhi.h>
#include "diag_mhi.h"
//#include "../mhi.h"
#include "diag.h"

struct diag_mhi_device {
	struct mhi_device *mhi_dev;
	struct device *dev;
	spinlock_t ul_lock;	/* lock to protect ul_pkts */
	struct list_head ul_pkts;
	struct mutex dl_mutex;	/* mutex to protect firmware log packet */
};

struct diag_mhi_pkt {
	struct list_head node;
	struct kref refcount;
	struct completion done;
	unsigned char *buf;
};

static void diag_mhi_pkt_release(struct kref *ref)
{
	struct diag_mhi_pkt *pkt = container_of(ref, struct diag_mhi_pkt,
						refcount);
	kfree(pkt);
}

int diag_mhi_send(struct mhi_device *mhi_dev, unsigned char *buf, int len)
{
	int rc;
	struct diag_mhi_device *diag_dev = dev_get_drvdata(&mhi_dev->dev);
	struct diag_mhi_pkt *pkt;

	pkt = kzalloc(sizeof(*pkt), GFP_KERNEL);
	if (!pkt)
		return -1;

	init_completion(&pkt->done);
	kref_init(&pkt->refcount);
	kref_get(&pkt->refcount);
	pkt->buf = buf;

	spin_lock_bh(&diag_dev->ul_lock);
	list_add_tail(&pkt->node, &diag_dev->ul_pkts);
	dev_info(&mhi_dev->dev, "%s mhi_dev:%pK, pkt: %pK buf:%pK, len:%d\n",
		 __func__, mhi_dev, pkt, buf, len);
	rc = mhi_queue_buf(mhi_dev, DMA_TO_DEVICE, buf, len,
			   MHI_EOT);
	if (rc) {
		list_del(&pkt->node);
		kfree(pkt);
		spin_unlock_bh(&diag_dev->ul_lock);
		return rc;
	}
	spin_unlock_bh(&diag_dev->ul_lock);

	rc = wait_for_completion_interruptible_timeout(&pkt->done, HZ * 5);
	if (rc > 0) {
		dev_dbg(&mhi_dev->dev, "%s remainder time: %d\n", __func__, rc);
		rc = 0;
	} else if (rc == 0) {
		dev_dbg(&mhi_dev->dev, "%s timeout\n", __func__);
		rc = -1;
	}
	kref_put(&pkt->refcount, diag_mhi_pkt_release);
	return rc;
}

static void diag_mhi_dl_callback(struct mhi_device *mhi_dev,
				 struct mhi_result *mhi_res)
{
	struct mhi_controller *mhi_ctrl;
	struct diag_mhi_device *diag_dev = dev_get_drvdata(&mhi_dev->dev);

	if (mhi_res->transaction_status) {
		dev_err(&mhi_dev->dev,
			"%s invalid transaction_status\n", __func__);
		return;
	}

	dev_dbg(&mhi_dev->dev, "%s buf addr:%p, bytes:%lu\n",
		__func__, mhi_res->buf_addr, mhi_res->bytes_xferd);

	mhi_ctrl = mhi_dev->mhi_cntrl;

	mutex_lock(&diag_dev->dl_mutex);
	diag_local_write(mhi_ctrl->cntrl_dev,
			 mhi_res->buf_addr,
			 mhi_res->bytes_xferd);
	mutex_unlock(&diag_dev->dl_mutex);
}

static void diag_mhi_ul_callback(struct mhi_device *mhi_dev,
				 struct mhi_result *mhi_res)
{
	struct diag_mhi_device *diag_dev = dev_get_drvdata(&mhi_dev->dev);
	struct diag_mhi_pkt *pkt;

	spin_lock_bh(&diag_dev->ul_lock);
	pkt = list_first_entry(&diag_dev->ul_pkts, struct diag_mhi_pkt, node);
	dev_dbg(&mhi_dev->dev, "%s diag_mhi_pkt=0x%p\n", __func__, pkt);
	list_del(&pkt->node);
	complete_all(&pkt->done);

	kref_put(&pkt->refcount, diag_mhi_pkt_release);
	spin_unlock_bh(&diag_dev->ul_lock);
}

static int diag_mhi_probe(struct mhi_device *mhi_dev,
			  const struct mhi_device_id *id)
{
	struct diag_mhi_device *diag_dev;

	diag_dev = devm_kzalloc(&mhi_dev->dev, sizeof(*diag_dev), GFP_KERNEL);
	if (!diag_dev)
		return -ENOMEM;
	diag_dev->mhi_dev = mhi_dev;
	diag_dev->dev = &mhi_dev->dev;

	INIT_LIST_HEAD(&diag_dev->ul_pkts);

	mutex_init(&diag_dev->dl_mutex);

	dev_set_drvdata(&mhi_dev->dev, diag_dev);

	dev_info(&mhi_dev->dev, "MHI diag driver probed\n");

	diag_loglevel_set(mhi_dev);

	return 0;
}

static void diag_mhi_remove(struct mhi_device *mhi_dev)
{
	dev_info(&mhi_dev->dev, "%s\n", __func__);
	dev_set_drvdata(&mhi_dev->dev, NULL);
}

static const struct mhi_device_id diag_mhi_match[] = {
	{ .chan = "DIAG" },
	{}
};

static struct mhi_driver diag_mhi_driver = {
	.probe = diag_mhi_probe,
	.remove = diag_mhi_remove,
	.dl_xfer_cb = diag_mhi_dl_callback,
	.ul_xfer_cb = diag_mhi_ul_callback,
	.id_table = diag_mhi_match,
	.driver = {
		.name = "diag_mhi",
		.owner = THIS_MODULE,
	},
};

module_driver(diag_mhi_driver, mhi_driver_register,
	      mhi_driver_unregister);

MODULE_DESCRIPTION("DIAG MHI interface driver");
MODULE_LICENSE("GPL v2");

/*
 * xport example driver for AMD SoC CVIP Subsystem.
 *
 * Copyright (C) 2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/iommu.h>
#include <linux/syscalls.h>
#include "ion.h"
#include "mero_ion.h"
#include "xport.h"

#define CDEV_NAME		"xport"
#define XPORT_BUF_SIZE	(1024 * 1024)

struct xport_device {
	struct device *dev;
	int res_len;
	int fd;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *dma_attach;
	struct sg_table *sg_table;
	void *vaddr;
};

static struct xport_device *xdev_p;

static int xport_intf_open(struct inode *inode, struct file *file)
{
	int fd;

	file->private_data = xdev_p;

	/*
	 * since there is no IOMMU device on the x-port interface, this is
	 * a hack to share the dma330 IOMMU device, just to use the proper
	 * dma_buf ops when performing dma_buf_attach and
	 * dma_buf_map_attachment
	 */
	if (share_dma_device(xdev_p->dev))
		return -ENOMEM;

	xdev_p->fd = -1;
	fd = ion_alloc(XPORT_BUF_SIZE, 1 << ION_HEAP_X86_CVIP_SHARED_ID, ION_FLAG_CACHED);
	if (fd < 0) {
		dev_err(xdev_p->dev, "Fail to allocate ION xport shared mem\n");
		return -ENOMEM;
	}

	xdev_p->fd = fd;
	xdev_p->res_len = XPORT_BUF_SIZE;
	xdev_p->dmabuf = NULL;
	xdev_p->dma_attach = NULL;
	xdev_p->sg_table = NULL;
	xdev_p->vaddr = NULL;

	xdev_p->dmabuf = dma_buf_get(fd);
	if (IS_ERR(xdev_p->dmabuf)) {
		dev_err(xdev_p->dev, "Fail to get dmabuf from fd:%d\n", fd);
		xdev_p->dmabuf = NULL;
		return -EINVAL;
	}

	xdev_p->dma_attach = dma_buf_attach(xdev_p->dmabuf, xdev_p->dev);
	if (IS_ERR(xdev_p->dma_attach)) {
		dev_err(xdev_p->dev, "Fail to attach to dmabuf\n");
		xdev_p->dma_attach = NULL;
		return -EINVAL;
	}

	xdev_p->sg_table = dma_buf_map_attachment(xdev_p->dma_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(xdev_p->sg_table)) {
		dev_err(xdev_p->dev, "Fail to get sgtable for dmabuf\n");
		xdev_p->sg_table = NULL;
		return -EINVAL;
	}

	xdev_p->vaddr = dma_buf_kmap(xdev_p->dmabuf, 0);
	return 0;
}

static int xport_intf_release(struct inode *inode, struct file *file)
{
	if (xdev_p->vaddr)
		dma_buf_kunmap(xdev_p->dmabuf, 0, xdev_p->vaddr);

	if (xdev_p->sg_table)
		dma_buf_unmap_attachment(xdev_p->dma_attach, xdev_p->sg_table,
					 DMA_BIDIRECTIONAL);

	if (xdev_p->dma_attach)
		dma_buf_detach(xdev_p->dmabuf, xdev_p->dma_attach);

	if (xdev_p->dmabuf)
		dma_buf_put(xdev_p->dmabuf);

	if (xdev_p->fd >= 0)
		ksys_close(xdev_p->fd);
	return 0;
}

static ssize_t xport_intf_read(struct file *file, char __user *buf,
			       size_t count, loff_t *ppos)
{
	struct xport_device *xdev;
	int ret;

	xdev = (struct xport_device *)file->private_data;
	if (!xdev)
		return -ENODEV;

	if (*ppos > xdev->res_len || count > xdev->res_len)
		return -EINVAL;

	dma_buf_begin_cpu_access(xdev->dmabuf, DMA_FROM_DEVICE);
	ret = simple_read_from_buffer(buf, count, ppos, xdev->vaddr, count);
	dma_buf_end_cpu_access(xdev->dmabuf, DMA_FROM_DEVICE);

	return ret;
}

static ssize_t xport_intf_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct xport_device *xdev;
	int ret;

	xdev = (struct xport_device *)file->private_data;
	if (!xdev)
		return -ENODEV;

	if (*ppos > xdev->res_len || count > xdev->res_len)
		return -EINVAL;

	dma_buf_begin_cpu_access(xdev->dmabuf, DMA_TO_DEVICE);
	ret = simple_write_to_buffer(xdev->vaddr, count, ppos, buf, count);
	dma_buf_end_cpu_access(xdev->dmabuf, DMA_TO_DEVICE);

	return ret;
}

static long xport_intf_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOIOCTLCMD;
}

static const struct file_operations xport_intf_fops = {
	.owner = THIS_MODULE,
	.open = xport_intf_open,
	.read = xport_intf_read,
	.write = xport_intf_write,
	.unlocked_ioctl = xport_intf_ioctl,
	.release = xport_intf_release,
	.llseek = default_llseek,
};

static struct miscdevice xport_intf_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = CDEV_NAME,
	.fops = &xport_intf_fops,
};

static int xport_add_device_node(struct xport_device *xdev)
{
	int ret;
	struct device *dev;

	dev = xdev->dev;

	ret = misc_register(&xport_intf_dev);
	if (ret) {
		dev_err(dev, "misc_register failed, err = %d\n", ret);
		return ret;
	}

	dev_info(dev, "Device node %s has initialized successfully.\n", CDEV_NAME);
	return 0;
}

static void xport_remove_node(void)
{
	misc_deregister(&xport_intf_dev);
}

static int xport_probe(struct platform_device *pdev)
{
	int ret;
	struct xport_device *xdev;

	xdev = devm_kzalloc(&pdev->dev, sizeof(*xdev), GFP_KERNEL);
	if (!xdev)
		return -ENOMEM;

	xdev->dev = &pdev->dev;

	ret = xport_add_device_node(xdev);
	if (ret) {
		dev_err(xdev->dev, "Fail add xport device node\n");
		return ret;
	}

	xdev_p = xdev;
	dev_info(xdev->dev, "Successfully probed XPORT device\n");

	return 0;
}

static int xport_remove(struct platform_device *pdev)
{
	xport_remove_node();
	return 0;
}

static const struct of_device_id xport_of_match[] = {
	{ .compatible = "amd,cvip-xport", },
	{ },
};

static struct platform_driver xport_driver = {
	.probe = xport_probe,
	.remove = xport_remove,
	.driver = {
		.name = "xport",
		.of_match_table = xport_of_match,
	},
};

static int __init xport_init(void)
{
	int ret;

	ret = platform_driver_register(&xport_driver);
	if (ret)
		return ret;

	return 0;
}

static void __exit xport_exit(void)
{
	platform_driver_unregister(&xport_driver);
}

module_init(xport_init);
module_exit(xport_exit);

MODULE_DESCRIPTION("xport test device");

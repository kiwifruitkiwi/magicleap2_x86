/*
 * ACP PCI Ioctl Driver
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "acp5x-evci.h"

#define ACP5X_MAGIC 'x'
#define ACP5X_IO_LOAD_FIRMWARE _IO(ACP5X_MAGIC, 1)
#define ACP5X_REG_WRITE _IOWR(ACP5X_MAGIC, 2, unsigned int)
#define ACP5X_REG_READ _IOWR(ACP5X_MAGIC, 3, unsigned int)
#define ACP5X_IO_EV_CMD _IOW(ACP5X_MAGIC, 4, unsigned int)
#define ACP5X_IO_SET_FW_LOGGING _IO(ACP5X_MAGIC, 5)
#define ACP5X_IO_I2S_DMA_EV_CMD _IOW(ACP5X_MAGIC, 6, struct stream_param)
#define ACP5X_IO_I2S_DMA_STOP_EVCMD _IOW(ACP5X_MAGIC, 7, unsigned int)
#define ACP5X_I0_ACP_ACLK_SET_FREQUENCY _IOW(ACP5X_MAGIC, 8, unsigned int)
#define ACP5X_IO_MEMORY_PG_SHUTDOWN_ENABLE _IOW(ACP5X_MAGIC, 9, unsigned int)
#define ACP5X_IO_MEMORY_PG_SHUTDOWN_DISABLE _IOW(ACP5X_MAGIC, 10, unsigned int)
#define ACP5X_IO_MEMORY_PG_DEEPSLEEP_ON _IOW(ACP5X_MAGIC, 11, unsigned int)
#define ACP5X_IO_MEMORY_PG_DEEPSLEEP_OFF _IOW(ACP5X_MAGIC, 12, unsigned int)
#define ACP5X_I0_I2S_MCLK_SET_FREQUENCY _IOW(ACP5X_MAGIC, 13, unsigned int)

#define ACP5X_IO_EV_CMD_ML _IOWR(ACP5X_MAGIC, 20, unsigned int)
#define ACP5X_IO_RESET_CMD_ML _IOWR(ACP5X_MAGIC, 21, unsigned int)

struct reg_struct {
	unsigned int cmd_offset;
	unsigned int val;
};

struct ml_io_struct {
	unsigned int val;
	void *buffer;
};

dev_t dev;
static struct class *dev_class;
static struct cdev acp5x_cdev;
static int mmap_device = MMAP_DEV_LOGGER;

static int acp5x_proxy_open(struct inode *inode, struct file *file);
static int acp5x_proxy_release(struct inode *inode, struct file *file);
static ssize_t acp5x_proxy_read(struct file *filp, char __user *buf,
				size_t len, loff_t *off);
static ssize_t acp5x_proxy_write(struct file *filp, const char *buf,
				 size_t len, loff_t *off);
static struct acp5x_dev_data *get_acp5x_device_drv_data(void);
static long acp5x_proxy_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg);
static int acp5x_proxy_mmap(struct file *filp, struct vm_area_struct *vma);

static int acp5x_proxy_open(struct inode *inode, struct file *file)
{
	pr_debug("Device File Opened...!!!\n");
	return 0;
}

static int acp5x_proxy_release(struct inode *inode, struct file *file)
{
	pr_debug("Device File Closed...!!!\n");
	return 0;
}

static ssize_t acp5x_proxy_read(struct file *filp, char __user *buf,
				size_t len, loff_t *off)
{
	pr_debug("Read Function\n");
	return 0;
}

static ssize_t acp5x_proxy_write(struct file *filp, const char __user *buf,
				 size_t len, loff_t *off)
{
	pr_debug("Write function\n");
	return 0;
}

static struct acp5x_dev_data *get_acp5x_device_drv_data()
{
	struct pci_dev *p_acpdev;

	p_acpdev = pci_get_device(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID, NULL);
	return pci_get_drvdata(p_acpdev);
}

static long acp5x_do_ioctl(struct file *file, unsigned int cmd,
			   void __user *argp)
{
	struct reg_struct acp5x_reg;
	struct ml_io_struct acp5x_ml_io;
	unsigned int val = 0;
	unsigned int i2s_instance;
	unsigned int clock_val;
	int ret = 0;
	struct stream_param param;
	struct pci_dev *p_acpdev;
	struct acp5x_dev_data *acp5x_data = get_acp5x_device_drv_data();

	p_acpdev = pci_get_device(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID, NULL);
	if (!acp5x_data->disable_runtime_pm) {
		if (p_acpdev) {
			ret = pm_runtime_get_sync(&p_acpdev->dev);
			if (ret < 0)
				return ret;
		}
	}
	switch (cmd) {
	case ACP5X_IO_LOAD_FIRMWARE:
		/* currently fw is loaded during acp pci driver probe */
		pr_debug("loading acp5x firmware\n");
		break;
	case ACP5X_REG_WRITE:
		if (copy_from_user(&acp5x_reg, argp, sizeof(struct reg_struct)))
			return -EFAULT;
		if (acp5x_data->acp5x_base && acp5x_reg.cmd_offset <=
		    ACP5x_REG_END) {
			acp_writel(acp5x_reg.val, acp5x_data->acp5x_base +
				  acp5x_reg.cmd_offset);
			pr_debug("acp5x reg write cmd_offset: 0x%x val:0x%x\n",
				 acp5x_reg.cmd_offset, acp5x_reg.val);
		}
		break;
	case ACP5X_REG_READ:
		if (copy_from_user(&acp5x_reg, argp, sizeof(struct reg_struct)))
			return -EFAULT;
		if (acp5x_data->acp5x_base && acp5x_reg.cmd_offset <=
		    ACP5x_REG_END) {
			acp5x_reg.val = acp_readl(acp5x_data->acp5x_base +
						 acp5x_reg.cmd_offset);
		}
		if (copy_to_user(argp, &acp5x_reg, sizeof(struct reg_struct)))
			return -EFAULT;
		pr_debug("acp5x reg read cmd_offset:0x%x val:0x%x\n",
			 acp5x_reg.cmd_offset, acp5x_reg.val);
		break;
	case ACP5X_IO_EV_CMD:
		if (copy_from_user(&val, (unsigned int *)argp, sizeof(val)))
			return -EFAULT;
		ret = send_ev_command(acp5x_data, val);
		pr_debug("send_ev_command Command_ID = %d ret = %d\n",
			 val, ret);
		break;
	case ACP5X_IO_EV_CMD_ML:
		if (copy_from_user(&acp5x_ml_io, (struct ml_io_struct *)argp,
		    sizeof(struct ml_io_struct)))
			return -EFAULT;
		ret = send_ev_command(acp5x_data, acp5x_ml_io.val);
		acp5x_ml_io.buffer = acp5x_data->acp_dma_virt_rx_addr;
		if (copy_to_user((struct ml_io_struct *)argp, &acp5x_ml_io,
		    sizeof(struct ml_io_struct)))
			return -EFAULT;
		pr_debug("send_ev_command Command_ID = %d ret = %d\n",
			 acp5x_ml_io.val, ret);
		break;
	case ACP5X_IO_RESET_CMD_ML:
		if (copy_from_user(&acp5x_ml_io, (struct ml_io_struct *)argp,
		    sizeof(struct ml_io_struct)))
			return -EFAULT;
		mmap_device = acp5x_ml_io.val;
		pr_debug("send_ev_command Command_ID = %d\n",
			 acp5x_ml_io.val);
		break;
	case ACP5X_IO_SET_FW_LOGGING:
		mmap_device = MMAP_DEV_LOGGER;
		ret = acp5x_set_fw_logging(acp5x_data);
		pr_debug("acp5x_set_fw_logging status:%d\n", ret);
		break;
	case ACP5X_IO_I2S_DMA_EV_CMD:
		if (copy_from_user(&param, (struct stream_param *)argp,
				   sizeof(struct stream_param)))
			return -EFAULT;
		ret = ev_acp_i2s_dma_start(acp5x_data, &param);
		pr_debug("send_ev_command Command_ID = %d ret = %d\n", val,
			 ret);
		break;
	case ACP5X_IO_I2S_DMA_STOP_EVCMD:
		if (copy_from_user(&i2s_instance, (unsigned int *)argp,
				   sizeof(i2s_instance)))
			return -EFAULT;
		ret = ev_acp_i2s_dma_stop(acp5x_data, i2s_instance);
		break;
	case ACP5X_I0_ACP_ACLK_SET_FREQUENCY:
		if (copy_from_user(&clock_val, (unsigned int *)argp,
				   sizeof(clock_val)))
			return -EFAULT;
		/* this is an example to set the aclk frequency after resume
		 * set acp clk frequency ev command is a blocking ev command
		 * i.e until this ev command execution is completed and
		 * response is received no other ev commands will be triggered.
		 * These are special ev commands can't be combined with other
		 * ev commands.
		 * example use case will be programming aclk frequency
		 * depending upon type of plugin going to be executed.
		 * In case of Deep Sleep, BIOS should apply SMU changes to lower
		 * the clock when ACP device enters in to D3 state.
		 */
		ret = ev_set_acp_clk_frequency(ACP_ACLK_CLOCK,
					       clock_val,
					       acp5x_data);
		break;
	case ACP5X_IO_MEMORY_PG_SHUTDOWN_ENABLE:
		if (copy_from_user(&val, (unsigned int *)argp, sizeof(val)))
			return -EFAULT;
		ret = ev_acp_memory_pg_shutdown_enable(acp5x_data, val);
		break;
	case ACP5X_IO_MEMORY_PG_SHUTDOWN_DISABLE:
		if (copy_from_user(&val, (unsigned int *)argp, sizeof(val)))
			return -EFAULT;
		ret = ev_acp_memory_pg_shutdown_disable(acp5x_data, val);
		break;
	case ACP5X_IO_MEMORY_PG_DEEPSLEEP_ON:
		if (copy_from_user(&val, (unsigned int *)argp, sizeof(val)))
			return -EFAULT;
		ret = ev_acp_memory_pg_deepsleep_on(acp5x_data, val);
		break;
	case ACP5X_IO_MEMORY_PG_DEEPSLEEP_OFF:
		if (copy_from_user(&val, (unsigned int *)argp, sizeof(val)))
			return -EFAULT;
		ret = ev_acp_memory_pg_deepsleep_off(acp5x_data, val);
		break;
	case ACP5X_I0_I2S_MCLK_SET_FREQUENCY:
		if (copy_from_user(&clock_val, (unsigned int *)argp,
				   sizeof(clock_val)))
			return -EFAULT;
		ret = ev_set_i2s_mclk_frequency(clock_val,
						acp5x_data);
		break;
	default:
		break;
	}

	if (!acp5x_data->disable_runtime_pm) {
		if (p_acpdev) {
			pm_runtime_mark_last_busy(&p_acpdev->dev);
			pm_runtime_put_autosuspend(&p_acpdev->dev);
		}
	}
	return 0;
}

static long acp5x_proxy_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	return acp5x_do_ioctl(file, cmd, (void __user *)arg);
}

/* driver mmap routine */
static int acp5x_proxy_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct pci_dev *p_acpdev;
	struct acp5x_dev_data *acp5x_data = get_acp5x_device_drv_data();
	void *virt_addr;
	dma_addr_t dma_addr;

	if (mmap_device == MMAP_DEV_TX) {
		virt_addr = acp5x_data->acp_dma_virt_addr;
		dma_addr = acp5x_data->acp_dma_addr;
	} else if (mmap_device == MMAP_DEV_RX) {
		virt_addr = acp5x_data->acp_dma_virt_rx_addr;
		dma_addr = acp5x_data->acp_dma_rx_addr;
	} else { /* default to LOG buffer */
		virt_addr = acp5x_data->acp_log_virt_addr;
		dma_addr = acp5x_data->acp_log_dma_addr;
	}

	p_acpdev = pci_get_device(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID, NULL);
	vma->vm_flags |= VM_IO |  VM_DONTEXPAND | VM_DONTDUMP |
			 VM_DONTCOPY | VM_DONTEXPAND | VM_NORESERVE |
			 VM_PFNMAP;

	dma_mmap_coherent(&p_acpdev->dev,
			  vma,
			  virt_addr,
			  dma_addr,
			  vma->vm_end - vma->vm_start);
	return 0;
}

static const struct file_operations fops = {
	.owner          = THIS_MODULE,
	.read           = acp5x_proxy_read,
	.write          = acp5x_proxy_write,
	.open           = acp5x_proxy_open,
	.mmap           = acp5x_proxy_mmap,
	.unlocked_ioctl = acp5x_proxy_ioctl,
	.release        = acp5x_proxy_release,
};

int acp5x_proxy_driver_init(void)
{
	if ((alloc_chrdev_region(&dev, 0, 1, "acp5x_proxy_Dev")) < 0) {
		pr_err("Cannot allocate major number\n");
		return -1;
	}
	pr_debug("Major = %d Minor = %d\n", MAJOR(dev), MINOR(dev));
	cdev_init(&acp5x_cdev, &fops);
	if ((cdev_add(&acp5x_cdev, dev, 1)) < 0) {
		pr_err("Cannot add the device to the system\n");
		goto r_class;
	}
	dev_class = class_create(THIS_MODULE, "acp5x_class");
	if (!dev_class) {
		pr_err("Cannot create the struct class\n");
		goto r_class;
	}
	if (!(device_create(dev_class, NULL, dev, NULL,
			    "acp5x_proxy_device"))) {
		pr_err("Cannot create the Device 1\n");
		goto r_device;
	}
	return 0;
r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

void acp5x_proxy_driver_exit(void)
{
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&acp5x_cdev);
	unregister_chrdev_region(dev, 1);
}


MODULE_DESCRIPTION("AMD acp5x proxy driver");
MODULE_LICENSE("GPL v2");

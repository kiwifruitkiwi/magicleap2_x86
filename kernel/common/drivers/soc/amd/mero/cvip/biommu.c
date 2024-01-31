/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/iommu.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <uapi/linux/mero_biommu.h>

#define CLASS_NAME "biommu"
#define CDEV_NUM 255

/**
 * struct biommu_mapping - biommu mapping information
 * @list: list node for list of biommu mappings
 * @output_addr: output address of biommu mapping
 * @map_size: size of biommu mapping
 * @input_addr: input_address of biommu mapping
 * @map_prot: permission of biommu mapping
 */
struct biommu_mapping {
	struct list_head list;
	phys_addr_t output_addr;
	u64 map_size;
	unsigned long input_addr;
	u64 map_prot;
};

/**
 * struct biommu_dev - biommu device information
 * @list: list node for list of biommu_device
 * @biommu_mappings: mappings of biommu
 * @tbu_irq: pointer to TBU IRQ resource
 * @domain: iommu domain
 * @struct_mutex: mutex lock for biommu_device
 * @dev: struct device for biommu
 * @minor : Minor device number of character device.
 * @cdev: The character device associated with the biommu device
 * @no_suspend_unmap: special handling to bypass iommu unmap during suspend
 */
struct biommu_device {
	struct list_head list;
	struct resource *tbu_irq;
	struct iommu_domain *domain;
	struct list_head biommu_mappings;
	int mapping_num;
	struct mutex struct_mutex; /*protect biommu_device*/
#ifdef CONFIG_DEBUG_FS
	struct dentry *d_dir;
#endif
	struct device *dev;
	dev_t minor;
	struct cdev cdev;
	bool no_suspend_unmap;
};

/*
 * struct biommu_list - List of BIOMMU device
 * @mutex: mutex lock for biommu list
 * @list: biommu device list
 */
struct biommu_list {
	struct mutex mutex; /*protect biommu list*/
	struct list_head list;
};

static struct biommu_list biommus;
static int major;
static struct dentry *biommu_dir;
static struct class *biommu_class;

static int biommu_match_child(struct device *dev, void *name)
{
	if (!dev || !name)
		return 0;

	if (!strcmp(dev_name(dev), (char *)name)) {
		pr_debug("biommu dev name matched:%s\n", dev_name(dev));
		return 1;
	}

	return 0;
}

struct iommu_domain *biommu_get_domain_byname(char *name)
{
	struct biommu_device *bdev;
	struct device *dev;
	struct iommu_domain *domain = NULL;

	mutex_lock(&biommus.mutex);
	list_for_each_entry(bdev, &biommus.list, list) {
		dev = device_find_child(bdev->dev, name,
					biommu_match_child);
		if (dev) {
			put_device(dev);
			domain = bdev->domain;
			break;
		}
	}
	mutex_unlock(&biommus.mutex);

	return domain;
}
EXPORT_SYMBOL_GPL(biommu_get_domain_byname);

struct device *biommu_get_domain_dev_byname(char *name)
{
	struct biommu_device *bdev;
	struct device *dev;

	mutex_lock(&biommus.mutex);
	list_for_each_entry(bdev, &biommus.list, list) {
		dev = device_find_child(bdev->dev, name,
					biommu_match_child);
		if (dev) {
			put_device(dev);
			break;
		}
	}
	mutex_unlock(&biommus.mutex);

	return dev;
}
EXPORT_SYMBOL_GPL(biommu_get_domain_dev_byname);

/**
 * biommu_check_overlap_no_lock() - check if mapping is overlap with exiting
 * mappings. If try to map an overlap mapping, arm smmu driver will throw a
 * kernel WARNING.
 * @biommu_dev: biommu device
 * @maping: mapping to be mapped
 *
 * Return value:
 * 0: not overlap
 * other: overlap
 *
 * Held struct_mutex before function call.
 */
static int biommu_check_overlap_nolock(struct biommu_device *biommu_dev,
				       struct biommu_mapping *mapping)
{
	struct biommu_mapping *temp;

	list_for_each_entry(temp, &biommu_dev->biommu_mappings, list) {
		if ((mapping->input_addr >= temp->input_addr &&
		     mapping->input_addr < temp->input_addr + temp->map_size) ||
		    (temp->input_addr >= mapping->input_addr &&
		       temp->input_addr < mapping->input_addr +
					   mapping->map_size)){
			pr_err("overlap mapping: 0x%lx/0x%llx : 0x%llx\n",
			       mapping->input_addr, mapping->map_size,
			       mapping->output_addr);
			return -EINVAL;
		}
	}

	return 0;
}

static void biommu_check_mapping_alignment(struct biommu_device *biommu_dev,
					   struct biommu_mapping *mapping)
{
	u64 pg_size;
	u64 pg_mask;

	/* find out the minimum page size supported */
	pg_size = 1UL << __ffs(biommu_dev->domain->pgsize_bitmap);
	pg_mask = ~(pg_size - 1);

	if (mapping->input_addr != (mapping->input_addr & pg_mask)) {
		pr_warn("biommu:input addr not aligned, 0x%lx\n",
			mapping->input_addr);
		mapping->input_addr &= pg_mask;
	}

	if (mapping->map_size != (mapping->map_size & pg_mask)) {
		pr_warn("biommu:map size not aligned, 0x%llx\n",
			mapping->map_size);
		mapping->map_size &= pg_mask;
		mapping->map_size += pg_size;
	}

	if (mapping->output_addr != (mapping->output_addr & pg_mask)) {
		pr_warn("biommu:output addr not aligned, 0x%llx\n",
			mapping->output_addr);
		mapping->output_addr &= pg_mask;
	}
}

static int biommu_map(struct biommu_device *biommu_dev,
		      phys_addr_t output_addr, size_t map_size,
		      unsigned long input_addr, uint64_t map_prot)
{
	int ret;
	struct biommu_mapping *mapping;

	if (!biommu_dev || !biommu_dev->domain) {
		pr_err("Invalid biommu_dev!!\n");
		return -EINVAL;
	}

	mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
	if (!mapping)
		return -ENOMEM;

	mapping->output_addr = output_addr;
	mapping->map_size = map_size;
	mapping->input_addr = input_addr;
	mapping->map_prot = map_prot;
	biommu_check_mapping_alignment(biommu_dev, mapping);

	mutex_lock(&biommu_dev->struct_mutex);
	ret = biommu_check_overlap_nolock(biommu_dev, mapping);
	if (ret) {
		kfree(mapping);
		mutex_unlock(&biommu_dev->struct_mutex);
		return -EINVAL;
	}

	ret = iommu_map(biommu_dev->domain, mapping->input_addr,
			mapping->output_addr, mapping->map_size,
			mapping->map_prot);
	if (ret) {
		pr_err("Error in iommu_map: 0x%lx/0x%llx : 0x%llx  err:%d\n",
		       mapping->input_addr, mapping->map_size,
		       mapping->output_addr, ret);
		mutex_unlock(&biommu_dev->struct_mutex);
		kfree(mapping);
		return ret;
	}

	list_add_tail(&mapping->list, &biommu_dev->biommu_mappings);
	biommu_dev->mapping_num++;
	pr_debug("biommu mapping: 0x%lx/0x%llx : 0x%llx\n",
		 mapping->input_addr, mapping->map_size,
		 mapping->output_addr);
	mutex_unlock(&biommu_dev->struct_mutex);
	return 0;
}

static int biommu_unmap(struct biommu_device *biommu_dev,
			unsigned long input_addr, size_t map_size)
{
	int ret = -EINVAL;
	struct biommu_mapping *mapping;

	mutex_lock(&biommu_dev->struct_mutex);
	list_for_each_entry(mapping, &biommu_dev->biommu_mappings, list) {
		if (mapping->input_addr == input_addr &&
		    mapping->map_size == map_size) {
			iommu_unmap(biommu_dev->domain, input_addr, map_size);
			list_del(&mapping->list);
			biommu_dev->mapping_num--;
			kfree(mapping);
			ret = 0;
			break;
		}
	}
	if (ret)
		pr_err("no mapping can be unmaped\n");
	mutex_unlock(&biommu_dev->struct_mutex);
	return ret;
}

static irqreturn_t biommu_irq_handler(int irq, void *dev)
{
	dev_err(dev, "Unhandled interrupt %d\n", irq);
	return IRQ_HANDLED;
}

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>

/**
 * biommu_mapping_show() - "/sys/kernel/debug/xxx/mapping" read call_back.
 * user space can get mappings by reading this file node.
 *
 * use case:
 *	cat /sys/kernel/debug/xxx/mapping
 *
 *	out_put:
 *	mapping[1/6]: 0x60000/0x1000 : 0x300000(0x3)
 *	mapping[2/6]: 0xdb800000/0x1000 : 0x400000(0x3)
 *	mapping[3/6]: 0xda000000/0x1000 : 0x420000(0x3)
 *	mapping[4/6]: 0x50000/0x1000 : 0x300000(0x3)
 *	mapping[5/6]: 0x800000/0x1000 : 0x800000(0x3)
 *	mapping[6/6]: 0x0/0x10000 : 0x0(0x3)
 *
 *	output format:
 *	input address/map size/ : output address(permision)
 *
 * This function will be called by reading "/sys/kernel/debug/xxx/mapping".
 */
static int biommu_mapping_show(struct seq_file *f, void *offset)
{
	int ret;
	int i = 0;
	struct biommu_mapping *mapping;
	struct biommu_device *biommu_dev = f->private;

	if (!biommu_dev)
		return 0;

	ret = mutex_lock_interruptible(&biommu_dev->struct_mutex);
	if (ret) {
		pr_err("Error in mutex_lock: %d\n", ret);
		return -ERESTART;
	}

	list_for_each_entry(mapping, &biommu_dev->biommu_mappings, list) {
		seq_printf(f, "mapping[%d/%d]: 0x%lx/0x%llx : 0x%llx(0x%llx)\n",
			   ++i, biommu_dev->mapping_num, mapping->input_addr,
			   mapping->map_size, mapping->output_addr,
			   mapping->map_prot);
	}

	mutex_unlock(&biommu_dev->struct_mutex);

	return 0;
}

static int biommu_add_mapping(struct biommu_device *biommu_dev, const char *buf)
{
	int ret;
	char cmd;
	struct biommu_mapping mapping = {0};

	ret = sscanf(buf, "%c:%lx:%llx:%llx:%llx", &cmd, &mapping.input_addr,
		     &mapping.map_size, &mapping.output_addr,
		     &mapping.map_prot);

	if (ret != 5)
		return -EINVAL;

	if (cmd != 'a')
		return -EINVAL;

	ret = biommu_map(biommu_dev, mapping.output_addr, mapping.map_size,
			 mapping.input_addr, mapping.map_prot);
	return ret;
}

static int biommu_remove_mapping(struct biommu_device *biommu_dev,
				 const char *buf)
{
	int ret;
	char cmd;
	struct biommu_mapping mapping;

	ret = sscanf(buf, "%c:%lx:%llx", &cmd, &mapping.input_addr,
		     &mapping.map_size);

	if (ret != 3)
		return -EINVAL;

	if (cmd != 'r')
		return -EINVAL;

	ret = biommu_unmap(biommu_dev, mapping.input_addr,
			   mapping.map_size);
	return ret;
}

 /**
  * biommu_mapping_write() - "/sys/kernel/debug/xxx/mapping" write call_back.
  * user space can add/remove by writing new mapping to this file node.
  *
  * usage:
  *	 echo a:0x5000:0x1000:0x3000:0x3 > /sys/kernel/debug/xxx/mapping
  *	 cmd = 'add', input address = 0x5000, map size = 0x1000,
  *	 output address = 0x3000 permision = 0x3;
  *
  *	 echo r:0x3:0x5000:0x1000 > /sys/kernel/debug/xxx/mapping
  *	 cmd = 'remove', input address = 0x5000, map size = 0x1000;
  *
  * permission:
  *	IOMMU_READ	(1 << 0)
  *	IOMMU_WRITE	(1 << 1)
  *	IOMMU_CACHE	(1 << 2)
  *	IOMMU_NOEXEC	(1 << 3)
  *	IOMMU_MMIO	(1 << 4)
  *
  * This function will be called by writing to "/sys/kernel/debug/xxx/mapping".
  */
static ssize_t biommu_mapping_write(struct file *file,
				    const char __user *user_buf,
				    size_t size, loff_t *ppos)
{
	int ret = 0;
	char buf[256];
	size_t buf_size;
	struct biommu_device *biommu_dev;

	buf_size = min(size, sizeof(buf) - 1);
	if (strncpy_from_user(buf, user_buf, buf_size) < 0)
		return -EFAULT;

	buf[buf_size] = 0;

	biommu_dev = ((struct seq_file *)file->private_data)->private;
	if (!biommu_dev)
		return -EFAULT;

	switch (buf[0]) {
	case 'a':
		ret = biommu_add_mapping(biommu_dev, buf);
		if (ret)
			return ret;
		break;
	case 'r':
		ret = biommu_remove_mapping(biommu_dev, buf);
		if (ret)
			return ret;
		break;
	default:
		pr_err("NOT support command: %c\n", buf[0]);
		return -EFAULT;
	}

	return size;
}

static int biommu_mapping_open(struct inode *inode, struct file *file)
{
	return single_open(file, biommu_mapping_show, inode->i_private);
}

static const struct file_operations biommu_mapping_fops = {
	.owner		= THIS_MODULE,
	.open		= biommu_mapping_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= biommu_mapping_write,
};

/**
 * biommu_sid_show() - "/sys/kernel/debug/xxx/stream_id" read call_back.
 * user space can get stream ids by reading this file node.
 *
 * use case:
 *	cat /sys/kernel/debug/xxx/stream_id
 *
 *	out_put:
 *	stream_id[0] = 0x1000
 *	stream_id[1] = 0x2000
 *	stream_id[2] = 0x3000
 *
 * This function will be called by reading "/sys/kernel/debug/xxx/stream_id".
 */
static int biommu_sid_show(struct seq_file *f, void *offset)
{
	int i;
	int ret = 0;
	int sid;
	struct biommu_device *biommu_dev;

	biommu_dev = (struct biommu_device *)((struct seq_file *)f->private);
	if (!biommu_dev || !biommu_dev->dev || !biommu_dev->dev->iommu_fwspec)
		return 0;

	ret = mutex_lock_interruptible(&biommu_dev->struct_mutex);
	if (ret) {
		pr_err("Error in mutex_lock: %d\n", ret);
		return -ERESTART;
	}
	for (i = 0; i < biommu_dev->dev->iommu_fwspec->num_ids; i++) {
		ret = iommu_fwspec_get_id_nolock(biommu_dev->dev, &sid, i);
		if (ret) {
			pr_err("Error: failed to get sid\n");
			mutex_unlock(&biommu_dev->struct_mutex);
			return 0;
		}
		seq_printf(f, "stream id[%d] = 0x%x\n", i, sid);
	}
	mutex_unlock(&biommu_dev->struct_mutex);
	return 0;
}

static int biommu_dev_map(struct biommu_device *biommu_dev)
{
	int ret = 0;
	struct biommu_mapping *mapping;

	list_for_each_entry(mapping, &biommu_dev->biommu_mappings, list) {
		ret = iommu_map(biommu_dev->domain, mapping->input_addr,
				mapping->output_addr, mapping->map_size,
				mapping->map_prot);
		if (ret) {
			dev_err(biommu_dev->dev, "failed iommu_map in %s\n", __func__);
			break;
		}
		dev_dbg(biommu_dev->dev, "map: 0x%lx/0x%llx : 0x%llx\n",
			mapping->input_addr, mapping->map_size,
			mapping->output_addr);
	}

	return ret;
}

static void biommu_dev_unmap(struct biommu_device *biommu_dev)
{
	struct biommu_mapping *mapping;

	list_for_each_entry(mapping, &biommu_dev->biommu_mappings, list) {
		iommu_unmap(biommu_dev->domain, mapping->input_addr,
			    mapping->map_size);
		dev_dbg(biommu_dev->dev, "unmap: 0x%lx/0x%llx : 0x%llx\n",
			mapping->input_addr, mapping->map_size,
			mapping->output_addr);
	}
}

static void biommu_dev_unmap_suspend(struct biommu_device *biommu_dev)
{
	struct biommu_mapping *mapping;

	list_for_each_entry(mapping, &biommu_dev->biommu_mappings, list) {
		if (biommu_dev->no_suspend_unmap) {
			dev_info(biommu_dev->dev, "skip iommu unmap 0x%lx/0x%llx : 0x%llx\n",
				 mapping->input_addr, mapping->map_size,
				 mapping->output_addr);
		} else {
			iommu_unmap(biommu_dev->domain, mapping->input_addr,
				    mapping->map_size);
			dev_dbg(biommu_dev->dev, "unmap: 0x%lx/0x%llx : 0x%llx\n",
				mapping->input_addr, mapping->map_size,
				mapping->output_addr);
		}
	}
}

static int biommu_dev_map_resume(struct biommu_device *biommu_dev)
{
	int ret = 0;
	struct biommu_mapping *mapping;

	if (biommu_dev->no_suspend_unmap) {
		list_for_each_entry(mapping, &biommu_dev->biommu_mappings, list) {
			dev_info(biommu_dev->dev, "resume remap 0x%lx/0x%llx : 0x%llx\n",
				 mapping->input_addr, mapping->map_size,
				 mapping->output_addr);
			iommu_unmap(biommu_dev->domain, mapping->input_addr,
				    mapping->map_size);
		}
	}

	/* must re-attach iommu device due to SMMU HW is reset on resume */
	ret = iommu_attach_device(biommu_dev->domain, biommu_dev->dev);
	if (ret) {
		dev_err(biommu_dev->dev, "iommu_attach_device ERROR:%d\n", ret);
		return ret;
	}

	list_for_each_entry(mapping, &biommu_dev->biommu_mappings, list) {
		ret = iommu_map(biommu_dev->domain, mapping->input_addr,
				mapping->output_addr, mapping->map_size,
				mapping->map_prot);
		if (ret) {
			dev_err(biommu_dev->dev, "failed iommu_map in %s\n", __func__);
			break;
		}
		dev_dbg(biommu_dev->dev, "map: 0x%lx/0x%llx : 0x%llx\n",
			mapping->input_addr, mapping->map_size,
			mapping->output_addr);
	}

	return ret;
}

/**
 * biommu_sid_write() - "/sys/kernel/debug/xxx/stream_id" write call_back.
 * user space can set stream id by wite new sid to this file node.
 *
 * use case:
 *	echo 0x1000 > /sys/kernel/debug/xxx/stream_id
 *	set stream id to 0x1000
 *
 * This function will be called by writing to "/sys/kernel/debug/xxx/stream_id".
 * So far, only support setting stream_id[0]
 */
static ssize_t biommu_sid_write(struct file *file, const char __user *user_buf,
				size_t size, loff_t *ppos)
{
	u32 sid;
	u32 old_sid;
	char buf[64];
	size_t buf_size;
	int ret = 0;
	struct biommu_device *biommu_dev;

	if (!access_ok(user_buf, size))
		return -EFAULT;

	buf_size = (size > sizeof(buf) - 1) ? (sizeof(buf) - 1) : size;
	if (strncpy_from_user(buf, user_buf, buf_size) < 0)
		return -EFAULT;
	buf[buf_size] = 0;
	ret = kstrtou32(buf, 0, &sid);
	if (ret)
		return -EFAULT;

	biommu_dev = ((struct seq_file *)file->private_data)->private;
	if (!biommu_dev)
		return -EFAULT;

	ret = mutex_lock_interruptible(&biommu_dev->struct_mutex);
	if (ret) {
		pr_err("Error in mutex_lock: %d\n", ret);
		return -ERESTART;
	}
	ret = iommu_fwspec_get_id_nolock(biommu_dev->dev, &old_sid, 0);
	if (ret) {
		pr_err("biommu: failed to get sid\n");
		ret = -EBUSY;
		goto err;
	}
	if (sid == old_sid) {
		pr_err("biommu: Error, sid = old_sid\n");
		ret = -EBUSY;
		goto err;
	}

	biommu_dev_unmap(biommu_dev);

	ret = iommu_set_sid_nolock(biommu_dev->dev, sid, 0);
	if (ret) {
		pr_err("biommu: failed to set stream id\n");
		ret = -EBUSY;
	}

	biommu_dev->domain = iommu_get_domain_for_dev(biommu_dev->dev);
	biommu_dev_map(biommu_dev);
err:
	mutex_unlock(&biommu_dev->struct_mutex);
	return (ret == 0) ? size : ret;
}

static int biommu_sid_open(struct inode *inode, struct file *file)
{
	return single_open(file, biommu_sid_show, inode->i_private);
}

static const struct file_operations biommu_sid_fops = {
	.owner		= THIS_MODULE,
	.open		= biommu_sid_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= biommu_sid_write,
};

#endif

static int biommu_chrdev_open(struct inode *inode, struct file *file)
{
	struct biommu_device *biommu_dev;

	biommu_dev = container_of(inode->i_cdev,
				  struct biommu_device, cdev);
	file->private_data = biommu_dev;
	return 0;
}

/**
 * biommu_ioctl() - "/dev/biommu/xxxx" ioctl callback.
 */
static long biommu_ioctl(struct file *file, unsigned int ioctl_cmd,
			 unsigned long ioctl_param)
{
	struct biommu_device *biommu_dev;
	struct map_data map;
	struct unmap_data unmap;
	unsigned int size = _IOC_SIZE(ioctl_cmd);

	biommu_dev = (struct biommu_device *)file->private_data;

	switch (ioctl_cmd) {
	case IOCTL_BIOMMU_ADD_MAPPING:
		if (size != sizeof(struct map_data))
			return -EINVAL;

		if (copy_from_user(&map, (void __user *)ioctl_param, size))
			return -EINVAL;

		return biommu_map(biommu_dev, map.output_addr,
				  map.map_size, map.input_addr,
				  map.map_prot);

	case IOCTL_BIOMMU_REMOVE_MAPPING:
		if (size != sizeof(struct unmap_data))
			return -EINVAL;

		if (copy_from_user(&unmap, (void __user *)ioctl_param, size))
			return -EINVAL;

		return biommu_unmap(biommu_dev, unmap.input_addr,
				    unmap.map_size);
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations biommu_chrdev_fops = {
	.open = biommu_chrdev_open,
	.unlocked_ioctl = biommu_ioctl,
};

static int biommu_add_node(struct biommu_device *biommu_dev,
			   const struct file_operations *fops)
{
	int ret;
	struct cdev *cdev;
	static dev_t index;
	struct device *device;

	cdev = &biommu_dev->cdev;
	biommu_dev->minor = index++;
	cdev_init(cdev, &biommu_chrdev_fops);
	cdev->owner = THIS_MODULE;
	ret = cdev_add(cdev, MKDEV(major, biommu_dev->minor), 1);
	if (ret) {
		pr_err("biommu: Failed to add cdev.\n");
		return ret;
	}

	device = device_create(biommu_class, biommu_dev->dev,
			       MKDEV(major, biommu_dev->minor),
			       NULL, "biommu!%s",
			       dev_name(biommu_dev->dev));
	if (IS_ERR(device)) {
		cdev_del(cdev);
		pr_err("biommu: Failed to create the device.\n");
		return PTR_ERR(device);
	}

	pr_debug("biommu: Device node biommu!%s has initialized successfully.\n",
		 dev_name(biommu_dev->dev));
	return 0;
}

static void biommu_remove_node(struct biommu_device *biommu_dev)
{
	device_destroy(biommu_class, MKDEV(major, biommu_dev->minor));
	cdev_del(&biommu_dev->cdev);
}

static int biommu_remove(struct platform_device *pdev)
{
	struct biommu_mapping *mapping, *tmp;
	struct biommu_device *biommu_dev = platform_get_drvdata(pdev);

	if (!biommu_dev) {
		pr_err("NULL biommu_dev!!\n");
		return -EINVAL;
	}

	list_del(&biommu_dev->list);
	biommu_remove_node(biommu_dev);

	list_for_each_entry_safe(mapping, tmp, &biommu_dev->biommu_mappings,
				 list) {
		biommu_unmap(biommu_dev, mapping->input_addr,
			     mapping->map_size);
	}

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(biommu_dev->d_dir);
#endif

	if (biommu_dev->tbu_irq)
		free_irq(biommu_dev->tbu_irq->start, biommu_dev);
	mutex_destroy(&biommu_dev->struct_mutex);
	kfree(biommu_dev);
	return 0;
}

static int biommu_fault_handler(struct iommu_domain *domain,
				struct device *dev,
				unsigned long paddr, int flags,
				void *data)
{
	dev_err(dev, "paddr = 0x%lx  operation = %s\n", paddr,
		flags ? "write" : "read");
	return 0;
}

static void biommu_parse_mappings(struct biommu_device *biommu_dev)
{
	int i = 0;
	int ret = 0;
	phys_addr_t output_addr;
	u64 map_size;
	phys_addr_t input_addr;
	u64 map_prot;
	struct device_node *np;

	np = biommu_dev->dev->of_node;

	if (of_property_read_bool(np, "no-suspend-unmap"))
		biommu_dev->no_suspend_unmap = true;
	while (!ret) {
		ret = of_property_read_u64_index(np, "output_addr", i,
						 &output_addr);
		if (ret)
			continue;
		ret = of_property_read_u64_index(np, "map_size", i,
						 &map_size);
		if (ret)
			continue;
		ret = of_property_read_u64_index(np, "input_addr", i,
						 &input_addr);
		if (ret)
			continue;
		ret = of_property_read_u64_index(np, "map_prot", i,
						 &map_prot);
		if (ret)
			continue;

		ret = biommu_map(biommu_dev, output_addr, map_size, input_addr,
				 map_prot);

		if (ret)
			continue;
		i++;
	}
}

static int biommu_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *tbu_irq;
	struct biommu_device *biommu_dev;
	struct iommu_domain *domain;

	tbu_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	biommu_dev = kzalloc(sizeof(*biommu_dev), GFP_KERNEL);
	if (!biommu_dev)
		return -ENOMEM;

	biommu_dev->dev = &pdev->dev;
	mutex_init(&biommu_dev->struct_mutex);
	INIT_LIST_HEAD(&biommu_dev->biommu_mappings);

	if (tbu_irq) {
		ret = request_irq(tbu_irq->start, biommu_irq_handler, 0,
				  "biommu_tbu", biommu_dev);
		if (ret) {
			pr_err("Error in request_irq: %d\n", ret);
			goto err;
		}

		biommu_dev->tbu_irq = tbu_irq;
	}

	domain = iommu_get_domain_for_dev(&pdev->dev);
	if (!domain) {
		pr_err("NULL iommu_domain\n");
		ret = -ENOENT;
		goto err;
	}

	biommu_dev->domain = domain;

	iommu_set_fault_handler(domain, biommu_fault_handler, biommu_dev);

	biommu_parse_mappings(biommu_dev);

	platform_set_drvdata(pdev, biommu_dev);

#ifdef CONFIG_DEBUG_FS
	biommu_dev->d_dir = debugfs_create_dir(dev_name(&pdev->dev),
					       biommu_dir);
	if (IS_ERR(biommu_dev->d_dir)) {
		pr_warn("Failed to create debugfs: %ld\n",
			PTR_ERR(biommu_dev->d_dir));
		biommu_dev->d_dir = NULL;
	} else {
		debugfs_create_file("mapping", 0600, biommu_dev->d_dir,
				    biommu_dev, &biommu_mapping_fops);
		debugfs_create_file("stream_id", 0600, biommu_dev->d_dir,
				    biommu_dev, &biommu_sid_fops);
	}
#endif
	ret = biommu_add_node(biommu_dev, &biommu_chrdev_fops);
	if (ret)
		goto err;

	mutex_lock(&biommus.mutex);
	list_add(&biommu_dev->list, &biommus.list);
	mutex_unlock(&biommus.mutex);

	pr_debug("Bus IOMMU device initialized\n");

	return 0;

err:
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(biommu_dev->d_dir);
#endif
	if (biommu_dev->tbu_irq)
		free_irq(biommu_dev->tbu_irq->start, biommu_dev);
	mutex_destroy(&biommu_dev->struct_mutex);
	kfree(biommu_dev);

	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int biommu_pm_suspend(struct device *dev)
{
	int ret = 0;
	struct biommu_device *biommu_dev = dev_get_drvdata(dev);

	if (!biommu_dev) {
		pr_err("NULL biommu_dev!!\n");
		return -EINVAL;
	}

	ret = mutex_lock_interruptible(&biommu_dev->struct_mutex);
	if (ret) {
		dev_err(biommu_dev->dev, "Error in mutex_lock: %d\n", ret);
		return ret;
	}

	if (list_empty(&biommu_dev->biommu_mappings))
		goto suspend_unlock;

	biommu_dev_unmap_suspend(biommu_dev);

suspend_unlock:
	mutex_unlock(&biommu_dev->struct_mutex);
	return 0;
}

static int biommu_pm_resume(struct device *dev)
{
	int ret = 0;
	struct biommu_device *biommu_dev = dev_get_drvdata(dev);

	if (!biommu_dev) {
		pr_err("NULL biommu_dev!!\n");
		return -EINVAL;
	}

	ret = mutex_lock_interruptible(&biommu_dev->struct_mutex);
	if (ret) {
		dev_err(biommu_dev->dev, "Error in mutex_lock: %d\n", ret);
		return ret;
	}

	if (list_empty(&biommu_dev->biommu_mappings))
		goto resume_unlock;

	biommu_dev_map_resume(biommu_dev);

resume_unlock:
	mutex_unlock(&biommu_dev->struct_mutex);
	return 0;
}

static const struct dev_pm_ops biommu_dev_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(biommu_pm_suspend, biommu_pm_resume)
};

#define BIOMMU_PM_OPS	(&biommu_dev_pm_ops)
#else
#define BIOMMU_PM_OPS	NULL
#endif

static const struct of_device_id biommu_of_match[] = {
	{ .compatible = "amd,cvip-biommu", },
	{ },
};

static struct platform_driver biommu_driver = {
	.probe = biommu_probe,
	.remove = biommu_remove,
	.driver = {
		.name = "biommu",
		.of_match_table = biommu_of_match,
		.pm = BIOMMU_PM_OPS,
	},
};

static int __init biommu_init(void)
{
	int ret;
	dev_t devt;

	mutex_init(&biommus.mutex);
	INIT_LIST_HEAD(&biommus.list);

	/* Register the device class */
	biommu_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(biommu_class)) {
		pr_err("biommu: Failed to register device class.\n");
		return PTR_ERR(biommu_class);
	}

	ret = alloc_chrdev_region(&devt, 0, CDEV_NUM, "biommu");
	if (ret) {
		class_destroy(biommu_class);
		pr_err("biommu: failed to alloc_chrdev_region\n");
		return -EBUSY;
	}
	major = MAJOR(devt);

#ifdef CONFIG_DEBUG_FS
	biommu_dir = debugfs_create_dir("biommu", NULL);
	if (!biommu_dir) {
		unregister_chrdev_region(devt, CDEV_NUM);
		class_destroy(biommu_class);
		return -ENODEV;
	}
#endif
	ret = platform_driver_register(&biommu_driver);
	if (ret) {
#ifdef CONFIG_DEBUG_FS
		debugfs_remove_recursive(biommu_dir);
#endif
		unregister_chrdev_region(devt, CDEV_NUM);
		class_destroy(biommu_class);
	}

	return ret;
}

static void __exit biommu_exit(void)
{
	platform_driver_unregister(&biommu_driver);
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(biommu_dir);
#endif
	unregister_chrdev_region(major, CDEV_NUM);
	class_destroy(biommu_class);
}

core_initcall(biommu_init);
module_exit(biommu_exit);

MODULE_DESCRIPTION("MERO BUS IOMMU Driver");

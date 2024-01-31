// SPDX-License-Identifier: GPL-2.0-only
/*
 * AMD Platform Security Processor (PSP) interface
 *
 * Copyright (C) 2016,2020 Advanced Micro Devices, Inc.
 *
 * Author: Brijesh Singh <brijesh.singh@amd.com>
 */

#include <linux/kernel.h>
#include <linux/irqreturn.h>

#include "sp-dev.h"
#include "psp-dev.h"
#include "sev-dev.h"
#include "tee-dev.h"

#define WARM_RESET_EVENT   0x10680
#define PSP_CVIP_CAPABILITY_DEFAULT 2

struct psp_device *psp_master;
static uint32_t cvip_wd_counter;

struct atomic_notifier_head cvip_wdt_notifier_list;

void cvip_wdt_notify(void)
{
	atomic_notifier_call_chain(&cvip_wdt_notifier_list, 0,
							   &cvip_wd_counter);
}

static struct psp_device *psp_alloc_struct(struct sp_device *sp)
{
	struct device *dev = sp->dev;
	struct psp_device *psp;

	psp = devm_kzalloc(dev, sizeof(*psp), GFP_KERNEL);
	if (!psp)
		return NULL;

	psp->dev = dev;
	psp->sp = sp;

	snprintf(psp->name, sizeof(psp->name), "psp-%u", sp->ord);

	return psp;
}

#ifdef CONFIG_X86_64
static irqreturn_t psp_irq_handler(int irq, void *data)
{
	struct psp_device *psp = data;
	unsigned int status;
	uint32_t counter;

	/* Read the interrupt status: */
	status = ioread32(psp->io_regs + psp->vdata->intsts_reg);

	/* invoke subdevice interrupt handlers */
	if (status) {
		if (psp->sev_irq_handler)
			psp->sev_irq_handler(irq, psp->sev_irq_data, status);

		if (psp->tee_irq_handler)
			psp->tee_irq_handler(irq, psp->tee_irq_data, status);
	}

	/* counter is the number of CVIP watchdog reset */
	counter = ioread32(psp->io_regs + WARM_RESET_EVENT);
	if (cvip_wd_counter < counter) {
		cvip_wd_counter = counter;
		/* notify that we hit a warm reset on cvip */
		cvip_wdt_notify();
	}

	/* Clear the interrupt status by writing the same value we read. */
	iowrite32(status, psp->io_regs + psp->vdata->intsts_reg);

	return IRQ_HANDLED;
}

static unsigned int psp_get_capability(struct psp_device *psp)
{
	unsigned int val = ioread32(psp->io_regs + psp->vdata->feature_reg);


	if ( -EPERM == val) {
		//Current platform not dont yet support PSP Feature register feature
		//Override to enable tee interface, by setting tee enable bit 0x2
		val = 0x2;
		printk("tee: updated psp-tee capability %x\n", val);
	}

	/*
	 * Check for a access to the registers.  If this read returns
	 * 0xffffffff, it's likely that the system is running a broken
	 * BIOS which disallows access to the device. Stop here and
	 * fail the PSP initialization (but not the load, as the CCP
	 * could get properly initialized).
	 */
	if (val == 0xffffffff) {
		dev_notice(psp->dev, "psp: unable to access the device: you might be running a broken BIOS.\n");
		return 0;
	}

	return val;
}
#endif

static int psp_check_sev_support(struct psp_device *psp,
				 unsigned int capability)
{
	/* Check if device supports SEV feature */
	if (!(capability & 1)) {
		dev_dbg(psp->dev, "psp does not support SEV\n");
		return -ENODEV;
	}

	return 0;
}

static int psp_check_tee_support(struct psp_device *psp,
				 unsigned int capability)
{
	/* Check if device supports TEE feature */
	if (!(capability & 2)) {
		dev_dbg(psp->dev, "psp does not support TEE\n");
		return -ENODEV;
	}

	return 0;
}

static int psp_check_support(struct psp_device *psp,
			     unsigned int capability)
{
	int sev_support = psp_check_sev_support(psp, capability);
	int tee_support = psp_check_tee_support(psp, capability);

	/* Return error if device neither supports SEV nor TEE */
	if (sev_support && tee_support)
		return -ENODEV;

	return 0;
}

static int psp_init(struct psp_device *psp, unsigned int capability)
{
	int ret;

	if (!psp_check_sev_support(psp, capability)) {
		ret = sev_dev_init(psp);
		if (ret)
			return ret;
	}

	if (!psp_check_tee_support(psp, capability)) {
		ret = tee_dev_init(psp);
		if (ret)
			return ret;
	}

	return 0;
}

int psp_dev_init(struct sp_device *sp)
{
	struct device *dev = sp->dev;
	struct psp_device *psp;
	unsigned int capability;
	int ret;

	ret = -ENOMEM;
	psp = psp_alloc_struct(sp);
	if (!psp)
		goto e_err;

	sp->psp_data = psp;

	psp->vdata = (struct psp_vdata *)sp->dev_vdata->psp_vdata;
	if (!psp->vdata) {
		ret = -ENODEV;
		dev_err(dev, "missing driver data\n");
		goto e_err;
	}

	psp->io_regs = sp->io_map;

#ifdef CONFIG_ARCH_MERO_CVIP
	psp->cvip_iomap = sp->cvip_iomap;
	capability = PSP_CVIP_CAPABILITY_DEFAULT;
#else
	capability = psp_get_capability(psp);
	if (!capability)
		goto e_disable;
#endif

	printk("psp_dev_init: capability 0x%x \n",capability);
	ret = psp_check_support(psp, capability);
	if (ret)
		goto e_disable;

#ifdef CONFIG_X86
	/* Disable and clear interrupts until ready */
	iowrite32(0, psp->io_regs + psp->vdata->inten_reg);
	iowrite32(-1, psp->io_regs + psp->vdata->intsts_reg);

	/* Request an irq */
	ret = sp_request_psp_irq(psp->sp, psp_irq_handler, psp->name, psp);
	if (ret) {
		dev_err(dev, "psp: unable to allocate an IRQ\n");
		goto e_err;
	}
	ATOMIC_INIT_NOTIFIER_HEAD(&cvip_wdt_notifier_list);
#endif
	ret = psp_init(psp, capability);
	if (ret)
		goto e_irq;

	if (sp->set_psp_master_device)
		sp->set_psp_master_device(sp);

#ifdef CONFIG_X86
	/* Enable interrupt */
	iowrite32(-1, psp->io_regs + psp->vdata->inten_reg);
#endif

	dev_notice(dev, "psp enabled\n");

	return 0;

e_irq:
	sp_free_psp_irq(psp->sp, psp);
e_err:
	sp->psp_data = NULL;

	dev_notice(dev, "psp initialization failed\n");

	return ret;

e_disable:
	sp->psp_data = NULL;

	return ret;
}

void psp_dev_destroy(struct sp_device *sp)
{
	struct psp_device *psp = sp->psp_data;

	if (!psp)
		return;

	sev_dev_destroy(psp);

	tee_dev_destroy(psp);

	sp_free_psp_irq(sp, psp);
}

void psp_set_sev_irq_handler(struct psp_device *psp, psp_irq_handler_t handler,
			     void *data)
{
	psp->sev_irq_data = data;
	psp->sev_irq_handler = handler;
}

void psp_clear_sev_irq_handler(struct psp_device *psp)
{
	psp_set_sev_irq_handler(psp, NULL, NULL);
}

void psp_set_tee_irq_handler(struct psp_device *psp, psp_irq_handler_t handler,
			     void *data)
{
	psp->tee_irq_data = data;
	psp->tee_irq_handler = handler;
}

void psp_clear_tee_irq_handler(struct psp_device *psp)
{
	psp_set_tee_irq_handler(psp, NULL, NULL);
}

struct psp_device *psp_get_master_device(void)
{
	struct sp_device *sp = sp_get_psp_master_device();

	return sp ? sp->psp_data : NULL;
}

void psp_pci_init(void)
{
	psp_master = psp_get_master_device();

	if (!psp_master)
		return;

	sev_pci_init();
}

void psp_pci_exit(void)
{
	if (!psp_master)
		return;

	sev_pci_exit();
}

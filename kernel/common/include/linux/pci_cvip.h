/*
 * Copyright (C) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __PCI_CVIP_H__
#define __PCI_CVIP_H__

#include <linux/pci.h>

#ifdef CONFIG_PCI_CVIP
void cvip_pci_setup_dma_mem(struct pci_dev *pdev);
void cvip_pci_flush_dram(unsigned int timeout);
u64 cvip_get_sintf_base(void);
#else
static inline void cvip_pci_setup_dma_mem(struct pci_dev *pdev)
{ /* do nothing */ }

static inline void cvip_pci_flush_dram(unsigned int timeout)
{
}

static inline u64 cvip_get_sintf_base(void)
{
	return 0;
}
#endif

#endif /* __PCI_CVIP_H__ */

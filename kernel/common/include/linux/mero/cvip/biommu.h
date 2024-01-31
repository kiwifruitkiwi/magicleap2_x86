/*
 * cvip biommu driver for AMD SoC CVIP Subsystem.
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef LINUX_CVIP_BIOMMU_H
#define LINUX_CVIP_BIOMMU_H

struct iommu_domain *biommu_get_domain_byname(char *name);
struct device *biommu_get_domain_dev_byname(char *name);

#endif /* LINUX_CVIP_BIOMMU_H */

/*
 * Char driver interface for AMD Mero SoC CVIP system cache pmu.
 *
 * Copyright (C) 2010 ARM Ltd.
 * Modification Copyright (c) 2020, Advanced Micro Devices, Inc.
 * All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CVIP_SYSCAHCE_DEV
#define __CVIP_SYSCAHCE_DEV

int syscache_dev_create(void __iomem **base);
void syscache_dev_remove(void);

#endif

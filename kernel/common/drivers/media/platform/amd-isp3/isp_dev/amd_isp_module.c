/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include "isp_module_cfg.h"
#include "isp_common.h"
#include "isp_module_if.h"
#include "isp_module_if_imp.h"
#include "log.h"
#include "i2c.h"
#include "isp_fw_if.h"

extern struct isp_isp_module_if g_ifl_interface;
struct isp_isp_module_if *get_isp_module_interface(void)
{
	isp_module_if_init(&g_ifl_interface);

	return &g_ifl_interface;
}
EXPORT_SYMBOL_GPL(get_isp_module_interface);
MODULE_LICENSE("GPL");

/*
 * cvip dsp driver for AMD Mero SoC CVIP Subsystem.
 *
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef LINUX_MERO_BIOMMU_DSP_H
#define LINUX_MERO_BIOMMU_DSP_H

struct device *cvip_dsp_get_channel_dev_byname(char *name);
struct clk *cvip_dsp_get_clk_byname(char *name);
int cvip_dsp_get_pdbypass_byname(char *name);

#endif /* LINUX_MERO_BIOMMU_DSP_H */

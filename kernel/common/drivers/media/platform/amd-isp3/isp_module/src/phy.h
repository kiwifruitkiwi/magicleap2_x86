/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"
/*#include "isp_common.h"*/
#include "buffer_mgr.h"
int start_internal_synopsys_phy(struct isp_context *pcontext,
				     unsigned int cam_id,
				     unsigned int bitrate /*in Mbps */,
				     unsigned int lane_num);
int shutdown_internal_synopsys_phy(uint32_t device_id, uint32_t lane_num);

extern struct s_properties *g_prop;

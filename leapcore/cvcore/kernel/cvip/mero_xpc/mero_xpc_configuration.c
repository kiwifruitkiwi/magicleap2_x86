/*
 * ---------------------------------------------------------------------
 * %COPYRIGHT_BEGIN%
 *
 * \copyright
 * Copyright (c) 2020-2023 Magic Leap, Inc. (COMPANY) All Rights Reserved.
 * Magic Leap, Inc. Confidential and Proprietary
 *
 *  NOTICE:  All information contained herein is, and remains the property
 *  of COMPANY. The intellectual and technical concepts contained herein
 *  are proprietary to COMPANY and may be covered by U.S. and Foreign
 *  Patents, patents in process, and are protected by trade secret or
 *  copyright law.  Dissemination of this information or reproduction of
 *  this material is strictly forbidden unless prior written permission is
 *  obtained from COMPANY.  Access to the source code contained herein is
 *  hereby forbidden to anyone except current COMPANY employees, managers
 *  or contractors who have executed Confidentiality and Non-disclosure
 *  agreements explicitly covering such access.
 *
 *  The copyright notice above does not evidence any actual or intended
 *  publication or disclosure  of  this source code, which includes
 *  information that is confidential and/or proprietary, and is a trade
 *  secret, of  COMPANY.   ANY REPRODUCTION, MODIFICATION, DISTRIBUTION,
 *  PUBLIC  PERFORMANCE, OR PUBLIC DISPLAY OF OR THROUGH USE  OF THIS
 *  SOURCE CODE  WITHOUT THE EXPRESS WRITTEN CONSENT OF COMPANY IS
 *  STRICTLY PROHIBITED, AND IN VIOLATION OF APPLICABLE LAWS AND
 *  INTERNATIONAL TREATIES.  THE RECEIPT OR POSSESSION OF  THIS SOURCE
 *  CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY RIGHTS
 *  TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE,
 *  USE, OR SELL ANYTHING THAT IT  MAY DESCRIBE, IN WHOLE OR IN PART.
 *
 * %COPYRIGHT_END%
 * --------------------------------------------------------------------
 */

#include "mero_xpc_configuration.h"

#include "mero_xpc_common.h"
#include "mero_xpc_types.h"
#include "cvcore_processor_common.h"

#define CLUSTER_1_IPC2_CHANNELS (0x7)
#define CLUSTER_1_IPC3_CHANNELS (0x3)
#define CLUSTER_1_IPC0_CHANNELS (0x0)
#define CLUSTER_1_IPC1_CHANNELS (0x0)
#define CLUSTER_1_IPC4_CHANNELS (0x3)

#define CLUSTER_0_IPC2_CHANNELS (0x7)
#define CLUSTER_0_IPC3_CHANNELS (0x3)
#define CLUSTER_0_IPC0_CHANNELS (0x0)
#define CLUSTER_0_IPC1_CHANNELS (0x0)
#define CLUSTER_0_IPC4_CHANNELS (0x3)

#define IPC_CHANNEL_TO_MASK(ch) (uint32_t)(0x1U << (ch))

#if defined(__i386__) || defined(__amd64__)
#define TGT_CORE_TO_TGT_CHANNEL(core_id)                                       \
	(uint8_t)((core_id) == CVCORE_ARM ? 21 : 14)

#define TGT_CORE_TO_SRC_CHANNEL(core_id) (15)

#define TGT_CORE_TO_IPC_ID(core_id) (IPC3_IDX)

#define IPC_VALID_TARGET_MIN (CVCORE_ARM)
#define IPC_VALID_TARGET_MAX (CVCORE_X86)
/* X86 can only target ARM and X86 */
#define IPC_VALID_TARGET_MASK (0x300)
#else
static const uint8_t tgt_core_to_tgt_channel_lookup[] = { 25, 28, 0,  3,  6,
							  9,  12, 15, 18, 14 };

#define TGT_CORE_TO_TGT_CHANNEL(core_id)                                       \
	tgt_core_to_tgt_channel_lookup[(core_id)]

#define TGT_CORE_TO_SRC_CHANNEL(core_id)                                       \
	(uint8_t)((core_id) == CVCORE_X86 ? 22 : 19)

#define TGT_CORE_TO_IPC_ID(core_id)                                            \
	(uint8_t)((core_id) == CVCORE_X86 ? IPC3_IDX : IPC2_IDX)

#define IPC_VALID_TARGET_MIN (XPC_MIN_CORE_ID)
#define IPC_VALID_TARGET_MAX (XPC_MAX_CORE_ID)
/* ARM can target any core */
#define IPC_VALID_TARGET_MASK (0x3FF)
#endif

#define TGT_CORE_IS_VALID(core_id) (IPC_VALID_TARGET_MASK & (0x1 << (core_id)))

uint32_t ipc_channel_id_masks[XPC_NUM_IPC_DEVICES][XPC_NUM_CORES] = {
	{ CLUSTER_1_IPC0_CHANNELS, CLUSTER_1_IPC0_CHANNELS,
	  CLUSTER_1_IPC0_CHANNELS, CLUSTER_1_IPC0_CHANNELS,
	  CLUSTER_1_IPC0_CHANNELS, CLUSTER_1_IPC0_CHANNELS,
	  CLUSTER_0_IPC0_CHANNELS, CLUSTER_0_IPC0_CHANNELS, 0xFFFFFFFF, 0x0 },

	{ CLUSTER_1_IPC1_CHANNELS, CLUSTER_1_IPC1_CHANNELS,
	  CLUSTER_1_IPC1_CHANNELS, CLUSTER_1_IPC1_CHANNELS,
	  CLUSTER_1_IPC1_CHANNELS, CLUSTER_1_IPC1_CHANNELS,
	  CLUSTER_0_IPC1_CHANNELS, CLUSTER_0_IPC1_CHANNELS, 0xFFFFFFFF, 0x0 },

	{ CLUSTER_1_IPC2_CHANNELS, (CLUSTER_1_IPC2_CHANNELS << 3),
	  (CLUSTER_1_IPC2_CHANNELS << 6), (CLUSTER_1_IPC2_CHANNELS << 9),
	  (CLUSTER_1_IPC2_CHANNELS << 12), (CLUSTER_1_IPC2_CHANNELS << 15),
	  (CLUSTER_0_IPC2_CHANNELS << 24), (CLUSTER_0_IPC2_CHANNELS << 27),
	  0x00FC0000, 0x0 },

	{ CLUSTER_1_IPC3_CHANNELS, CLUSTER_1_IPC3_CHANNELS << 2,
	  CLUSTER_1_IPC3_CHANNELS << 4, CLUSTER_1_IPC3_CHANNELS << 6,
	  CLUSTER_1_IPC3_CHANNELS << 8, CLUSTER_1_IPC3_CHANNELS << 10,
	  CLUSTER_0_IPC3_CHANNELS << 16, CLUSTER_0_IPC3_CHANNELS << 18,
	  0x3 << 21, 0x3 << 14 },

	{ CLUSTER_1_IPC4_CHANNELS, CLUSTER_1_IPC4_CHANNELS << 2,
	  CLUSTER_1_IPC4_CHANNELS << 4, CLUSTER_1_IPC4_CHANNELS << 6,
	  CLUSTER_1_IPC4_CHANNELS << 8, CLUSTER_1_IPC4_CHANNELS << 10,
	  CLUSTER_0_IPC4_CHANNELS << 16, CLUSTER_0_IPC4_CHANNELS << 18,
	  0x3 << 21, 0x3 << 14 }
};

void set_core_channel_mask(int core_id, int ipc_id, uint32_t mask)
{
	ipc_channel_id_masks[ipc_id][core_id] = mask;
}

uint32_t get_core_channel_mask(int core_id, int ipc_id)
{
	return ipc_channel_id_masks[ipc_id][core_id];
}

int xpc_compute_route(uint32_t destinations, struct xpc_route_solution *route)
{
	int ipc_id;
	int tgt_core, i;
	int tgt_renum;

	if (destinations == 0 || route == NULL) {
		// Must have at least one destination
		// and a route struct to store the results
		return 0;
	}

	route->valid_routes = 0;
	route->first_route.ipc_id = XPC_IPC_ID_INVALID;

	for (i = 0; i < XPC_NUM_IPC_DEVICES; i++) {
		route->routes[i].ipc_source_irq_mask = 0;
		route->routes[i].ipc_target_irq_mask = 0;
		route->routes[i].ipc_id = XPC_IPC_ID_INVALID;
	}

	for (tgt_core = IPC_VALID_TARGET_MIN,
	    destinations >>= IPC_VALID_TARGET_MIN;
	     tgt_core <= IPC_VALID_TARGET_MAX && destinations;
	     tgt_core++, destinations >>= 1) {
		if (!TGT_CORE_IS_VALID(tgt_core) || !(destinations & 0x1)) {
			continue;
		}

		tgt_renum = CVCORE_DSP_RENUMBER_Q6C5_TO_C5Q6(tgt_core);

		ipc_id = TGT_CORE_TO_IPC_ID(tgt_renum);

		route->routes[ipc_id].ipc_id = ipc_id;
		route->routes[ipc_id].ipc_source_irq_mask =
			IPC_CHANNEL_TO_MASK(TGT_CORE_TO_SRC_CHANNEL(tgt_renum));

		route->routes[ipc_id].ipc_target_irq_mask |=
			IPC_CHANNEL_TO_MASK(TGT_CORE_TO_TGT_CHANNEL(tgt_renum));

		if (route->routes[ipc_id].ipc_target_irq_mask) {
			route->valid_routes |= (0x1 << ipc_id);
			if (route->first_route.ipc_id == XPC_IPC_ID_INVALID) {
				route->first_route = route->routes[ipc_id];
			}
		}
	}

	return (route->valid_routes == 0) ? 0 : 1;
}

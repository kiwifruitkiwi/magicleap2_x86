// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/etherdevice.h>
#include <uapi/linux/if.h>
#include <linux/kernel.h>

/* Magic Leap range of MAC addresses */
static u8 ml_wifi_mac_id[IFHWADDRLEN] = {0x60, 0x4B, 0xAA, 0, 0, 0};

int dhd_get_mac_addr(unsigned char *buf)
{
	uint rand_mac;

	if (!buf)
		return -EFAULT;

	if (ml_wifi_mac_id[4] == 0 && ml_wifi_mac_id[5] == 0) {
		prandom_seed((uint)jiffies);
		rand_mac = prandom_u32();
		ml_wifi_mac_id[3] = (unsigned char)rand_mac;
		ml_wifi_mac_id[4] = (unsigned char)(rand_mac >> 8);
		ml_wifi_mac_id[5] = (unsigned char)(rand_mac >> 16);
	}
	memcpy(buf, ml_wifi_mac_id, IFHWADDRLEN);
	return 0;
}
EXPORT_SYMBOL(dhd_get_mac_addr);

static int __init dhd_mac_addr_setup(char *str)
{
	char macstr[IFHWADDRLEN * 3];
	char *macptr = macstr;
	char *token;
	int i = 0;

	if (!str)
		return 0;

	pr_info("%s: WLAN MAC = %s\n", __func__, str);
	if (strlen(str) >= sizeof(macstr))
		return 0;
	strlcpy(macstr, str, sizeof(macstr));

	while ((token = strsep(&macptr, ":")) != NULL && i < IFHWADDRLEN) {
		unsigned long val;
		int res;

		res = kstrtoul(token, 0x10, &val);
		if (res < 0)
			break;
		ml_wifi_mac_id[i++] = (u8)val;
	}
	return i == IFHWADDRLEN ? 1 : 0;
}

__setup("androidboot.wlanmacaddr=", dhd_mac_addr_setup);

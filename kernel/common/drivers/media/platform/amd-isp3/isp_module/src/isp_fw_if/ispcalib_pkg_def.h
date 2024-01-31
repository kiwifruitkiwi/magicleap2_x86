/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _PACKAGE_TD_H_
#define _PACKAGE_TD_H_

#define MAX_CALIB_RESOLUTION_NO		10

struct calib_relsolution_header {
	unsigned int size;
	unsigned int offset;
	unsigned short width;
	unsigned short height;
};

struct calib_pkg_td_header {
	unsigned int res_no;
	unsigned int length;
	struct calib_relsolution_header res[MAX_CALIB_RESOLUTION_NO];
	char revision[8];
	char date[16];
	char author[16];
	char module[16];
	unsigned int crc[7 + MAX_CALIB_RESOLUTION_NO];
};

#endif

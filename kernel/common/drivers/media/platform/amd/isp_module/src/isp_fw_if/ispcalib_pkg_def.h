#ifndef _PACKAGE_TD_H_
#define _PACKAGE_TD_H_

#define MAX_CALIB_RESOLUTION_NO		10

struct calib_relsolution_header {
	uint32 size;
	uint32 offset;
	uint16 width;
	uint16 height;
};

struct calib_pkg_td_header {
	uint32 res_no;
	uint32 length;
	struct calib_relsolution_header res[MAX_CALIB_RESOLUTION_NO];
	char revision[8];
	char date[16];
	char author[16];
	char module[16];
	uint32 crc[7 + MAX_CALIB_RESOLUTION_NO];
};

#endif

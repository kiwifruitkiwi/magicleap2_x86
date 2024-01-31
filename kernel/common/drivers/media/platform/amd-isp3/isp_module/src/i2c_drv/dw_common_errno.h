/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef DW_COMMON_ERRNO_H
#define DW_COMMON_ERRNO_H

#define DW_EPERM	1	/*operation not permitted */
#define DW_EIO		5	/*I/O error */
#define DW_ENXIO	6	/*no such device or address */
#define DW_ENOMEM	12	/*out of memory */
#define DW_EACCES	13	/*permission denied */
#define DW_EBUSY	16	/*device or resource busy */
#define DW_ENODEV	19	/*no such device */
#define DW_EINVAL	22	/*invalid argument */
#define DW_ENOSPC	28	/*no space left on device */
#define DW_ENOSYS	38	/*function not implemented/supported */
#define DW_ECHRNG	44	/*channel number out of range */
#define DW_ENODATA	61	/*no data available */
#define DW_ETIME	62	/*timer expired */
#define DW_EPROTO	71	/*protocol error */

#endif				/*DW_COMMON_ERRNO_H */

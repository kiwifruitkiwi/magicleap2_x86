/*
 * Copyright (C) 2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __SYSCACHE_IOCTL_H
#define __SYSCACHE_IOTCL_H

#define EV_ENABLE_CO		1
#define EV_ENABLE_DRHIT		2
#define EV_ENABLE_DRREQ		3
#define EV_ENABLE_DWHIT		4
#define EV_ENABLE_DWREQ		5
#define EV_ENABLE_DWTREQ	6
#define EV_ENABLE_IRHIT		7
#define EV_ENABLE_IRREQ		8
#define EV_ENABLE_WA		9

#define EV_COUNT_SIZE		256

struct ev_src {
	unsigned long src0;
	unsigned long src1;
};

#define	SYSCACHE_MAGIC		'c'
#define SYSCACHE_ENABLE_PMU		_IO(SYSCACHE_MAGIC, 1)
#define	SYSCACHE_DISABLE_PMU		_IO(SYSCACHE_MAGIC, 2)
#define SYSCACHE_SYNC			_IO(SYSCACHE_MAGIC, 3)
#define SYSCACHE_FLUSH_ALL		_IO(SYSCACHE_MAGIC, 4)
#define SYSCACHE_SET_EV_SOURCES		_IOW(SYSCACHE_MAGIC, 5, struct ev_src)
#define SYSCACHE_GET_EV_COUNT		_IOR(SYSCACHE_MAGIC, 6, char[EV_COUNT_SIZE])
#define SYSCACHE_INV_ALL		_IO(SYSCACHE_MAGIC, 7)

#endif

#
# Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
#

obj-$(CONFIG_CVIP_SYS_CACHE)        += syscache.o
ifeq ($(CONFIG_CVIP_SYS_CACHE),y)
  obj-$(CONFIG_DEBUG_FS)            += syscache_debugfs.o
  obj-$(CONFIG_DEBUG_FS)            += syscache_dev.o
endif

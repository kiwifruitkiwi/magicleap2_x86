#/*
# * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
# */
# IPC core
ifneq ($(CONFIG_IPC_XPORT),)
ccflags-y += -I$(srctree)/drivers/staging/android/ion \
	     -I$(srctree)/drivers/staging/android/uapi \
	     -I$(srctree)/drivers/char
endif

ccflags-$(CONFIG_CVIP_IPC) += -I$(srctree)/drivers/firmware

# needs integer - float conversion for isp
ifeq ($(ARCH),arm64)
CFLAGS_REMOVE_ipc_fp_ops.o += -mgeneral-regs-only
endif

obj-$(CONFIG_CVIP_IPC) 		+= ipc.o ipc_core.o ipc_queue.o ipc_config.o \
				   ipc_isp.o ipc_smu.o ipc_fp_ops.o
obj-$(CONFIG_X86_IPC) 		+= ipc.o ipc_core.o ipc_queue.o ipc_config.o
obj-$(CONFIG_ISP_SENSOR_SIM)	+= ipc_sensor.o

ifneq ($(CONFIG_CVIP),)
	ccflags-y += -I $(srctree)/$(src)/../misc
	ccflags-y += -I $(srctree)/$(src)/../mailbox
endif
ifneq ($(CONFIG_ARCH_MERO_CVIP),)
	ccflags-y += -I $(srctree)/$(src)/../misc
	ccflags-y += -I $(srctree)/$(src)/../mailbox
endif

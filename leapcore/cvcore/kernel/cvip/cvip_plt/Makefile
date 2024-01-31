#
# SPDX-License-Identifier: GPL-2.0-only
#

obj-$(CONFIG_CVIP_PLT) += cvip_plt.o
cvip_plt-y := cvip_plt.o cvip_plt_property.o cvip_plt_scpi.o cvip_plt_crc_opto.o cvip_plt_pci.o

ifneq ($(CONFIG_ML_CVIP_ARM),)
  obj-$(CONFIG_COMMON_CLK_SCPI) += cvip_plt_fclk.o
endif

ccflags-y += -I $(srctree)/include/linux/cvip
ccflags-y += -I $(srctree)/drivers/firmware
ccflags-y += -I $(srctree)/drivers/staging/android/ion
ccflags-y += -I $(srctree)/drivers/staging/android/uapi


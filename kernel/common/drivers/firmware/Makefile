# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#
obj-$(CONFIG_MERO_SCPI_PROTOCOL)+= mero_scpi.o
ifeq ($(CONFIG_MERO_SCPI_PROTOCOL),y)
  obj-$(CONFIG_DEBUG_FS)	+= mero_scpi_debugfs.o
  ccflags-y 	+= -I $(srctree)/$(src)/../ipc
  ccflags-y	+= -I $(srctree)/$(src)/../gpu/drm/amd/include/asic_reg/mp
  ccflags-y	+= -I $(srctree)/$(src)/../../kernel/power
endif
obj-$(CONFIG_MERO_SMU)+= mero_smu.o

obj-$((!CONFIG_MERO_SCPI_PROTOCOL)&&(CONFIG_ARM_SCPI_PROTOCOL))	+= arm_scpi.o
obj-$(CONFIG_ARM_SCPI_POWER_DOMAIN) += scpi_pm_domain.o
obj-$(CONFIG_ARM_SDE_INTERFACE)	+= arm_sdei.o
obj-$(CONFIG_DMI)		+= dmi_scan.o
obj-$(CONFIG_DMI_SYSFS)		+= dmi-sysfs.o
obj-$(CONFIG_EDD)		+= edd.o
obj-$(CONFIG_EFI_PCDP)		+= pcdp.o
obj-$(CONFIG_DMIID)		+= dmi-id.o
obj-$(CONFIG_INTEL_STRATIX10_SERVICE) += stratix10-svc.o
obj-$(CONFIG_INTEL_STRATIX10_RSU)     += stratix10-rsu.o
obj-$(CONFIG_ISCSI_IBFT_FIND)	+= iscsi_ibft_find.o
obj-$(CONFIG_ISCSI_IBFT)	+= iscsi_ibft.o
obj-$(CONFIG_FIRMWARE_MEMMAP)	+= memmap.o
obj-$(CONFIG_RASPBERRYPI_FIRMWARE) += raspberrypi.o
obj-$(CONFIG_FW_CFG_SYSFS)	+= qemu_fw_cfg.o
obj-$(CONFIG_QCOM_SCM)		+= qcom_scm.o
obj-$(CONFIG_QCOM_SCM_64)	+= qcom_scm-64.o
obj-$(CONFIG_QCOM_SCM_32)	+= qcom_scm-32.o
CFLAGS_qcom_scm-32.o :=$(call as-instr,.arch armv7-a\n.arch_extension sec,-DREQUIRES_SEC=1) -march=armv7-a
obj-$(CONFIG_TI_SCI_PROTOCOL)	+= ti_sci.o
obj-$(CONFIG_TRUSTED_FOUNDATIONS) += trusted_foundations.o
obj-$(CONFIG_TURRIS_MOX_RWTM)	+= turris-mox-rwtm.o

obj-$(CONFIG_ARM_SCMI_PROTOCOL)	+= arm_scmi/
obj-y				+= psci/
obj-y				+= broadcom/
obj-y				+= meson/
obj-$(CONFIG_GOOGLE_FIRMWARE)	+= google/
obj-$(CONFIG_EFI)		+= efi/
obj-$(CONFIG_UEFI_CPER)		+= efi/
obj-y				+= imx/
obj-y				+= tegra/
obj-y				+= xilinx/

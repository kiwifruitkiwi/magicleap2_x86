#
# Copyright(C) 2018-2021 Advanced Micro Devices, Inc. All rights reserved.
#
ifdef CONFIG_AMD_ISP
obj-$(CONFIG_AMD_ISP) += amd_capture.o
amd_capture-objs := isp_dev/amd_metadata.o \
			isp_dev/amd_stream.o \
			isp_dev/amd_isp.o \
			isp_dev/amd_params.o \
			isp_dev/amd_isp_module.o \
			cam_dev/fsl_error.o \
			cam_dev/fsl_interinstr.o \
			cam_dev/fsl_linker.o \
			cam_dev/fsl_log.o \
			cam_dev/fsl_parser.o \
			cam_dev/fsl_string.o \
			cam_dev/fw_parser.o \
			cam_dev/sensor_if.o \
			cam_dev/sensor_imx208.o \
			isp_module/src/i2c.o \
			isp_module/src/i2c_drv/dw_apb_i2c.o \
			isp_module/src/i2c_drv/isp_i2c_driver.o \
			isp_module/src/isp_common.o \
			isp_module/src/isp_fw_if.o \
			isp_module/src/isp_gfx_if_imp.o \
			isp_module/src/isp_mc_addr_mgr.o \
			isp_module/src/isp_module_if_imp.o \
			isp_module/src/isp_pwr.o \
			isp_module/src/isp_queue.o \
			isp_module/src/isp_soc_adpt.o \
			isp_module/src/isp_work_item.o \
			isp_module/src/log.o \
			isp_module/src/os_advance_type.o \
			emulator/amd_cam_metadata.o \
			isp_module/src/sensor_wrapper.o

ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp_module
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp_module/inc
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp_module/src
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp_module/src/i2c_drv
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp_module/src/isp_fw_if
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp_module/src/isp_fw_if/hw_reg

ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp_dev
ccflags-y += -I$(srctree)/drivers/media/platform/amd/cam_dev
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/include
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/amdgpu
ccflags-y += -I$(srctree)/include/drm
ccflags-y += -I$(srctree)/include
ccflags-y += -I$(srctree)/include/uapi/drm
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/scheduler
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/powerplay/inc
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/acp/include
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display/include
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display/dc
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display/amdgpu_dm
ccflags-y += -I$(srctree)/../external/libdrm/amdgpu

ifneq ($(call cc-option, -mpreferred-stack-boundary=4),)
ifndef CONFIG_CC_IS_CLANG
	cc_stack_align := -mpreferred-stack-boundary=4
endif
endif

ccflags-y += -mhard-float -msse $(cc_stack_align) -msse2
ccflags-y += -DSENSOR_DRIVER_ON_HOST -DHAL_ALTERA_PCI -DCONFIG_COMPAT -DUSING_KGD_CGS

ifdef CONFIG_CC_IS_CLANG
ccflags-y += -Wno-parentheses-equality -Wno-enum-conversion -Wno-self-assign
endif

endif

obj-$(CONFIG_AMD_SIMU_CAMERA) += camera_simu.o
camera_simu-objs := emulator/camera_simu.o emulator/amd_cam_metadata.o

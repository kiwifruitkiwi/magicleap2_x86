# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Magic Leap misc devices
#

# Note: Modules will be initialized in the order they
# are listed, so order with dependencies in mind.
obj-$(CONFIG_ML_GSM)           += gsm/
obj-$(CONFIG_ML_SHREGION)      += shregion/
obj-$(CONFIG_ML_MERO_XPC)      += mero_xpc/
obj-$(CONFIG_ML_MERO_NOTIFIER) += mero_notifier/
obj-$(CONFIG_ML_MLNET)         += mlnet/
obj-$(CONFIG_ML_CVCORE_TRACE)  += cvcore_trace/
obj-$(CONFIG_ML_MERO_SMMU)     += mero_smmu/
obj-$(CONFIG_CVIP_PLT)         += cvip_plt/
obj-$(CONFIG_ML_DSP_CORE)      += dsp_core/
obj-$(CONFIG_ML_CDP)           += cdp/
obj-$(CONFIG_ML_DCPRS)         += dcprs/
obj-$(CONFIG_ML_INV_MPU)       += inv_mpu/

# for debug, if needed
# $(info CONFIG_ML_GSM===========[$(CONFIG_ML_GSM)])

# mero platform support
# Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
snd-pci-acp5x-fw-y := acp5x-proxy.o acp5x-evci.o pci-acp5x.o
obj-$(CONFIG_SND_SOC_AMD_ACP5x) += snd-pci-acp5x-fw.o
CFLAGS_pci-acp5x.o := -DDEBUG
CFLAGS_acp5x-evci.o := -DDEBUG
CFLAGS_acp5x-proxy.o := -DDEBUG

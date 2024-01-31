# raven ridge platform support
# Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
snd-pci-acp3x-y	:=   acp3x-proxy.o acp3x-evci.o  pci-acp3x.o
obj-$(CONFIG_SND_SOC_AMD_ACP3x_FW)	 += snd-pci-acp3x.o
CFLAGS_pci-acp3x.o := -DDEBUG
CFLAGS_acp3x-evci.o := -DDEBUG
CFLAGS_acp3x-proxy.o := -DDEBUG

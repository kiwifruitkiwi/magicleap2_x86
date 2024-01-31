ifndef CVCORE_TOP
$(error ERROR: You must source cvip_root/setup.sh to build)
endif

ifeq ($(CVCORE_PLATFORM), $(filter $(CVCORE_PLATFORM), $(CVCORE_ARM_PLATFORMS)))
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
KDIR := ${AMD_OUT_KERNEL}
CC := $(CROSS_COMPILE)gcc
else
KVER ?= $(shell uname -r)
KDIR := /lib/modules/$(KVER)/build
CC := gcc
endif

ifeq ($(CVIP_CVCORE_USE_CCACHE), true)
CC := ${CVIP_TOOL_CCACHE} ${CC}
endif

HYDRA_MODULE_ODIR := $(CVCORE_OUT)/hydra/arm/kernel

# We handle some targets here, don't include in TARGET
TARGET := $(filter-out all clean test_build test_run test test_continue test_run_continue, $(MAKECMDGOALS))

all:
	# Make sure to clean before we copy over the contents.
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	if [ ! -d $(HYDRA_MODULE_ODIR) ]; then mkdir -p $(HYDRA_MODULE_ODIR); fi
	cp -R * $(HYDRA_MODULE_ODIR)
	$(MAKE) -C $(KDIR) M=$(HYDRA_MODULE_ODIR) CC="${CC}" modules

.PHONY: clean
clean:
	if [ -d $(HYDRA_MODULE_ODIR) ]; then\
		$(MAKE) -C $(KDIR) M=$(HYDRA_MODULE_ODIR) clean;\
	fi

test_build test_run test test_continue test_run_continue:
	@echo "The hydra driver does not have unit tests at this time."

$(TARGET):
	@true

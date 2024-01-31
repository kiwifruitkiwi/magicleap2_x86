LINUXINCLUDE := $(DKMS_INCLUDE_PREFIX) $(LINUXINCLUDE)

BACKPORT_OBJS := kcl_mmu_notifier.o

amdgpu-y += $(addprefix ../backport/,$(BACKPORT_OBJS))

ccflags-y += -include kcl/config.h

ccflags-y += \
	-I$(FULL_AMD_PATH)/backport/include \
	-include ../backport/backport.h

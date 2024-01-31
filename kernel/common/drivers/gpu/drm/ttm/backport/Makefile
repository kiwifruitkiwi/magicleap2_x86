
LINUXINCLUDE := $(DKMS_INCLUDE_PREFIX) $(LINUXINCLUDE)

ccflags-y += -include kcl/config.h

ccflags-y += \
	-I$(TTM_FULL_PATH) \
	-include backport/backport.h

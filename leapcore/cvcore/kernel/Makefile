
ifndef CVCORE_TOP
    $(error ERROR: You must source cvip_root/setup.sh to build)
endif

ifeq (, $(MAKECMDGOALS))
    targets := all
endif  # MAKECMDGOALS
targets += $(MAKECMDGOALS)

$(targets):
ifeq ($(CVCORE_PLATFORM), $(filter $(CVCORE_PLATFORM), $(CVCORE_ARM_PLATFORMS)))
	$(MAKE) -C cvip/hydra $(MAKECMDGOALS)
	$(MAKE) -C cvip/hcsm $(MAKECMDGOALS)
endif

# bcmdhd
#####################
# Basic feature
#####################

DHDCFLAGS += -Wall -Wstrict-prototypes -Dlinux -DLINUX -DBCMDRIVER      \
        -DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DBCMFILEIMAGE      \
        -DDHDTHREAD -DDHD_BCMEVENTS -DSHOW_EVENTS -DBCMDBG -DWLP2P      \
        -DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT -DKEEP_ALIVE -DCSCAN     \
        -DPKT_FILTER_SUPPORT -DEMBEDDED_PLATFORM -DPNO_SUPPORT          \
        -DSEC_ENHANCEMENT -DGET_CUSTOM_MAC_ENABLE

#####################
# Bus Interface Type
#####################
ifneq ($(CONFIG_BCMDHD_PCIE),)
	BUS_IFACE_PCIE=y
else
	BUS_IFACE_SDIO=y
endif

#####################
# SDIO I/F
#####################
ifeq ($(BUS_IFACE_SDIO),y)
	DHDCFLAGS += -DBDC -DDHD_BCMEVENTS -DMMC_SDIO_ABORT
	DHDCFLAGS += -DBCMSDIO -DBCMLXSDMMC -DUSE_SDIOFIFO_IOVAR
	DHDCFLAGS += -U__ARM_ARCH_7A__
ifeq ($(CONFIG_BCMDHD_OOB_HOST_WAKE),y)
	DHDCFLAGS += -DOOB_INTR_ONLY
endif
	# DPC priority
	DHDCFLAGS += -DCUSTOM_DPC_PRIO_SETTING=98
	# idle count
	DHDCFLAGS += -DDHD_USE_IDLECOUNT
	# SKB TAILPAD to avoid out of boundary memory access
	DHDCFLAGS += -DDHDENABLE_TAILPAD
	# Enable PROP_TXSTATUS
	DHDCFLAGS += -DPROP_TXSTATUS -DLIMIT_BORROW
	DHDCFLAGS += -DSUPPORT_P2P_GO_PS
	# Debug for DPC Thread watchdog bark
	DHDCFLAGS += -DDEBUG_DPC_THREAD_WATCHDOG
	# KSO Attempts count
	DHDCFLAGS += -DCUSTOM_MAX_KSO_ATTEMPTS=64
endif

#####################
# PCIE I/F
#####################
ifeq ($(BUS_IFACE_PCIE),y)
	DHDCFLAGS += -DPCIE_FULL_DONGLE -DBCMPCIE
	# Use the external WLAN_HOST_WAKE pin to wake up host
ifeq ($(CONFIG_BCMDHD_OOB_HOST_WAKE),y)
	DHDCFLAGS += -DBCMPCIE_OOB_HOST_WAKE
endif
	# DPC priority
	DHDCFLAGS += -DCUSTOM_DPC_PRIO_SETTING=-1
	# Disable watchdog thread
	DHDCFLAGS += -DCUSTOM_DHD_WATCHDOG_MS=0
	# Set D3_ACK response timeout value
	DHDCFLAGS += -DD3_ACK_RESP_TIMEOUT=4000
	# Use Coherent memory for ring
	DHDCFLAGS += -DDHD_USE_COHERENT_MEM_FOR_RING
	# Alloc Coherent memory from DMA coherent pool
#	DHDCFLAGS += -DDHD_ALLOC_COHERENT_MEM_FROM_ATOMIC_POOL
	# Enable Link down recovery
	DHDCFLAGS += -DSUPPORT_LINKDOWN_RECOVERY
	# Enable Dongle Isolation
	DHDCFLAGS += -DDONGLE_ENABLE_ISOLATION
	# Enable Firmware Coredump
	DHDCFLAGS += -DDHD_FW_COREDUMP
        # Enable PKTID AUDIT
        DHDCFLAGS += -DDHD_PKTID_AUDIT_ENABLED
	# Enable Load Balancing support by default.
	# DHD_LB_RXP - Perform RX Packet processing in parallel
	# DHD_LB_STATS - To display the Load Blancing statistics
	DHDCFLAGS += -DDHD_LB -DDHD_LB_RXP -DDHD_LB_TXP -DDHD_LB_STATS
	# Extended HANG event with reason codes
	DHDCFLAGS += -DDHD_USE_EXTENDED_HANG_REASON
	# DHDCFLAGS += -DDHD_RECOVER_TIMEOUT
	# HEAP ASLR
	DHDCFLAGS += -DBCM_ASLR_HEAP
ifeq ($(filter y,$(CONFIG_SOC_EXYNOS8895) $(CONFIG_SOC_EXYNOS9810) $(CONFIG_ARCH_SDM845)),y)
	# Default Tx LB Enable
	DHDCFLAGS += -DDHD_LB_TXP_DEFAULT_ENAB
endif
	# PCIe interrupt affinity control interface
#	DHDCFLAGS += -DSET_PCIE_IRQ_CPU_CORE
endif

#################
# Common feature
#################
DHDCFLAGS += -DWL_CFG80211
#DHDCFLAGS += -DDHD_INIT_DEFAULT_MEMDUMP

# DMA64 suppports on 64bit Architecture
ifeq ($(CONFIG_64BIT),y)
DHDCFLAGS := $(filter-out -DBCMDMA32,$(DHDCFLAGS))
DHDCFLAGS += -DBCMDMA64OSL
endif

# Debug
DHDCFLAGS += -DSIMPLE_MAC_PRINT
DHDCFLAGS += -DDEBUGFS_CFG80211
# Enable wakelock for legacy scan
DHDCFLAGS += -DDHD_USE_SCAN_WAKELOCK
# Enable wakelock debug function
DHDCFLAGS += -DDHD_TRACE_WAKE_LOCK
# Print out kernel panic point of file and line info when assertion happened
DHDCFLAGS += -DBCMASSERT_LOG
# Enable SI utility error log
DHDCFLAGS += -DSI_ERROR_ENFORCE

# Print 8021X
DHDCFLAGS += -DDHD_8021X_DUMP

# Print DHCP
DHDCFLAGS += -DDHD_DHCP_DUMP

# Print ICMP
DHDCFLAGS += -DDHD_ICMP_DUMP

# VSDB
DHDCFLAGS += -DVSDB

# Wi-Fi Direct
DHDCFLAGS += -DWL_CFG80211_VSDB_PRIORITIZE_SCAN_REQUEST
# For p2p connection issue
DHDCFLAGS += -DWL_SCB_TIMEOUT=10
# For TDLS tear down inactive time 10 sec
DHDCFLAGS += -DCUSTOM_TDLS_IDLE_MODE_SETTING=10000
# for TDLS RSSI HIGH for establishing TDLS link
DHDCFLAGS += -DCUSTOM_TDLS_RSSI_THRESHOLD_HIGH=-80
# for TDLS RSSI HIGH for tearing down TDLS link
DHDCFLAGS += -DCUSTOM_TDLS_RSSI_THRESHOLD_LOW=-85

# Roaming
DHDCFLAGS += -DROAM_AP_ENV_DETECTION
DHDCFLAGS += -DROAM_ENABLE -DROAM_CHANNEL_CACHE -DROAM_API
DHDCFLAGS += -DENABLE_FW_ROAM_SUSPEND
DHDCFLAGS += -DDHD_LOSSLESS_ROAMING
DHDCFLAGS += -DSKIP_ROAM_TRIGGER_RESET

# Enable Roam time thresh
DHDCFLAGS += -DENABLE_MAX_DTIM_IN_SUSPEND
DHDCFLAGS += -DCUSTOM_BCN_TIMEOUT_IN_SUSPEND=10
DHDCFLAGS += -DCUSTOM_ROAM_TIME_THRESH_IN_SUSPEND=6000
DHDCFLAGS += -DMAX_DTIM_ALLOWED_INTERVAL=925

# CCX
ifeq ($(CONFIG_BRCM_CCX),y)
  DHDCFLAGS += -DBCMCCX
endif

# SoftAP
DHDCFLAGS += -DWL_SUPPORT_AUTO_CHANNEL -DSUPPORT_HIDDEN_AP
DHDCFLAGS += -DSUPPORT_SOFTAP_SINGL_DISASSOC
DHDCFLAGS += -DDISABLE_11H_SOFTAP
# For support WPA and WPA2 mixed mode
DHDCFLAGS += -DSUPPORT_SOFTAP_WPAWPA2_MIXED

# HW4 specific features
DHDCFLAGS += -DSUPPORT_PM2_ONLY
DHDCFLAGS += -DSUPPORT_DEEP_SLEEP
DHDCFLAGS += -DSUPPORT_AMPDU_MPDU_CMD
DHDCFLAGS += -DSOFTAP_SEND_HANGEVT
DHDCFLAGS += -DNUM_SCB_MAX_PROBE=3
DHDCFLAGS += -DDHD_USE_ATOMIC_PKTGET
DHDCFLAGS += -DTDLS_MSG_ONLY_WFD
DHDCFLAGS += -DCUSTOM_KEEP_ALIVE_SETTING=30000
DHDCFLAGS += -DCUSTOM_EVENT_PM_WAKE=30
DHDCFLAGS += -DENABLE_TDLS_AUTO_MODE
DHDCFLAGS += -DP2P_SKIP_DFS
DHDCFLAGS += -DKEEP_WIFION_OPTION
DHDCFLAGS += -DPREVENT_REOPEN_DURING_HANG
DHDCFLAGS += -DEXPLICIT_DISCIF_CLEANUP
DHDCFLAGS += -DSKIP_WLFC_ON_CONCURRENT
DHDCFLAGS += -DP2P_LISTEN_OFFLOADING
#DHDCFLAGS += -DUNSET_FW_ROAM_WIPHY_FLAG
DHDCFLAGS += -DCUSTOM_LONG_RETRY_LIMIT=12
DHDCFLAGS += -DDISABLE_ANDROID_PNO

# For special PNO Event keep wake lock for 10sec
DHDCFLAGS += -DCUSTOM_PNO_EVENT_LOCK_xTIME=10

# For IPV6 Neighbor Discovery Offload Support
DHDCFLAGS += -DIPV6_NDO_SUPPORT

# Early suspend
DHDCFLAGS += -DDHD_USE_EARLYSUSPEND

# WiFi turn off delay
DHDCFLAGS += -DWIFI_TURNOFF_DELAY=100

# For Scan result patch
DHDCFLAGS += -DESCAN_RESULT_PATCH
DHDCFLAGS += -DDUAL_ESCAN_RESULT_BUFFER
DHDCFLAGS += -DESCAN_BUF_OVERFLOW_MGMT

# For Static Buffer
ifeq ($(CONFIG_BROADCOM_WIFI_RESERVED_MEM),y)
  DHDCFLAGS += -DCONFIG_DHD_USE_STATIC_BUF
  DHDCFLAGS += -DENHANCED_STATIC_BUF
  DHDCFLAGS += -DSTATIC_WL_PRIV_STRUCT
ifeq ($(BUS_IFACE_PCIE),y)
  # Use preallocated IOCTL, EVENT and INFO buffer
  DHDCFLAGS += -DDHD_USE_STATIC_CTRLBUF
ifeq ($(CONFIG_BCMDHD_PREALLOC_MEMDUMP),y)
  DHDCFLAGS += -DDHD_USE_STATIC_MEMDUMP
endif
endif # BUS_IFACE_PCIE
endif # CONFIG_BROADCOM_WIFI_RESERVED_MEM

# Ioctl timeout 5000ms
DHDCFLAGS += -DIOCTL_RESP_TIMEOUT=5000

# Priority mismatch fix with kernel stack
DHDCFLAGS += -DPKTPRIO_OVERRIDE

# Prevent rx thread monopolize
DHDCFLAGS += -DWAIT_DEQUEUE

# Control PM related value for MFG process(Read from file)
#DHDCFLAGS += -DDHD_PM_CONTROL_FROM_FILE

# Use Android wake lock mechanism
DHDCFLAGS += -DCONFIG_HAS_WAKELOCK

# Used short dwell time during initial scan
DHDCFLAGS += -DUSE_INITIAL_SHORT_DWELL_TIME

# Japan ccode revision will be fixed by nvram's value
DHDCFLAGS += -DKEEP_JP_REGREV

# FCC power limit control on ch12/13.
DHDCFLAGS += -DFCC_PWR_LIMIT_2G

# Change the WL_SCAN_TIMER_INTERVAL_MS of DHD
DHDCFLAGS += -DCUSTOMER_SCAN_TIMEOUT_SETTING

# Enable HANG event test private command
DHDCFLAGS += -DSUPPORT_TRIGGER_HANG_EVENT

# Enable Checking Blob existence
DHDCFLAGS += -DDHD_BLOB_EXISTENCE_CHECK

# Random mac scan
DHDCFLAGS += -DSUPPORT_RANDOM_MAC_SCAN

# WLC_E_SET_SSID fail
DHDCFLAGS += -DSET_SSID_FAIL_CUSTOM_RC=100

DHDCFLAGS += -DWL_WPS_SYNC -DBCMCRYPTO_COMPONENT

ifeq ($(BUS_IFACE_PCIE),y)
DHDCFLAGS += -DDHD_4WAYM4_FAIL_DISCONNECT
endif

# Android Version Check from Platform source
ifneq ($(ANDROID_VERSION),)
MAJOR_VERSION := $(shell echo $(ANDROID_VERSION) | rev | cut -c 5- | rev)
DHDCFLAGS += -DANDROID_PLATFORM_VERSION=$(MAJOR_VERSION)
else
ifneq ($(PLATFORM_VERSION),)
MAJOR_VERSION := $(shell echo $(PLATFORM_VERSION) | cut -d "." -f 1)
DHDCFLAGS += -DANDROID_PLATFORM_VERSION=$(MAJOR_VERSION)
endif
endif

##############################
# Android Platform Definition
##############################

###########
# Lollipop
###########
# Definitions are filtered by Kernel version
DHDCFLAGS += -DWL_ENABLE_P2P_IF
DHDCFLAGS += -DWL_SUPPORT_BACKPORTED_KPATCHES
# Default definitions for KitKat, Lollipop
DHDCFLAGS += -DWL_CFG80211_STA_EVENT
DHDCFLAGS += -DWL_IFACE_COMB_NUM_CHANNELS
# To support p2p private command on kernel 3.8 or above
DHDCFLAGS += -DWL_NEWCFG_PRIVCMD_SUPPORT

# Extra file list for Lollipop
DHD_ANDROID_OFILES := bcmxtlv.o bcm_app_utils.o

##########################
# driver type
# m: module type driver
# y: built-in type driver
##########################
DRIVER_TYPE ?= m

#########################
# Chip dependent feature
#########################
# Chipsets supported both SDIO and PCIE
ifneq ($(CONFIG_BCM4375),)
 # Disable Dongle Isolation
 DHDCFLAGS := $(filter-out -DDONGLE_ENABLE_ISOLATION,$(DHDCFLAGS))
 DHDCFLAGS += -DBCM4375_CHIP
# DHDCFLAGS += -DBCM4375_CHIP -DSUPPORT_MULTIPLE_REVISION
# DHDCFLAGS += -DSUPPORT_BCM4375_MIXED_MODULES -DSUPPORT_MULTIPLE_BOARDTYPE
 DHDCFLAGS += -DUSE_CID_CHECK
 DHDCFLAGS += -DENABLE_BCN_LI_BCN_WAKEUP
 DHDCFLAGS += -DCUSTOM_BCN_TIMEOUT=6
 DHDCFLAGS += -DSOFTAP_UAPSD_OFF
 DHDCFLAGS += -DDHD_SSSR_DUMP
# tput enhancement for common
 DHDCFLAGS += -DUSE_WL_TXBF
 DHDCFLAGS += -DUSE_WL_FRAMEBURST
 DHDCFLAGS += -DDISABLE_FRAMEBURST_VSDB
 DHDCFLAGS += -DDISABLE_WL_FRAMEBURST_SOFTAP
 DHDCFLAGS += -DDISABLE_PM_BCNRX

# Customer ant select and set PM
 DHDCFLAGS += -DCUSTOM_SET_ANTNPM
# Customer ocl disabe
 DHDCFLAGS += -DCUSTOM_SET_OCLOFF
 DHDCFLAGS += -DWL_FILS_ROAM_OFFLD

# SAE..if kernel < 4.17 ..back port support required
ifeq ($(CONFIG_CFG80211_SAE_BKPORT), y)
  DHDCFLAGS += -DWL_SAE
endif

#Iface force dev close
DHDCFLAGS += -DIFACE_HANG_FORCE_DEV_CLOSE

# tput enhancement for PCIE
ifeq ($(BUS_IFACE_PCIE),y)
  DHDCFLAGS += -DCUSTOM_TCPACK_SUPP_RATIO=15
  DHDCFLAGS += -DCUSTOM_TCPACK_DELAY_TIME=10
  DHDCFLAGS += -DDHDTCPACK_SUPPRESS
ifeq ($(CONFIG_X86_64),y)
  DHDCFLAGS += -DPCIE_AMD_LTR_WAR
endif #CONFIG_X86_64
endif

# New Features
  DHDCFLAGS += -DWL11U -DMFP
  DHDCFLAGS += -DBCMCCX
  DHDCFLAGS += -DWES_SUPPORT
  DHDCFLAGS += -DWLTDLS
  DHDCFLAGS += -DWLFBT
  DHDCFLAGS += -DDHD_ENABLE_LPC
  DHDCFLAGS += -DSUPPORT_2G_VHT -DSUPPORT_5G_1024QAM_VHT
  DHDCFLAGS += -DSUPPORT_WL_TXPOWER
  DHDCFLAGS += -DENABLE_IPMCAST_FILTER
# virtual interface for RSDB
  DHDCFLAGS += -DWL_VIRTUAL_APSTA
# disable pruned scan
  DHDCFLAGS += -DDISABLE_PRUNED_SCAN
# Remove common feature for BCM4375
  DHDCFLAGS :=$(filter-out -DDISABLE_11H_SOFTAP,$(DHDCFLAGS))
# Block ARP during DHCP on STA/SoftAP concurrent mode
  DHDCFLAGS += -DAPSTA_BLOCK_ARP_DURING_DHCP
# Use restricted channels on STA/SoftAP concurrent mode
  DHDCFLAGS += -DAPSTA_RESTRICTED_CHANNEL
  DHDCFLAGS += -DDHD_LOG_DUMP
  # Enable log print rate limit
  DHDCFLAGS += -DDHD_LOG_PRINT_RATE_LIMIT
ifneq ($(CUSTOM_LOG_DUMP_BUFSIZE_MB),)
  DHDCFLAGS += -DCUSTOM_LOG_DUMP_BUFSIZE_MB=$(CUSTOM_LOG_DUMP_BUFSIZE_MB)
endif

  DHDCFLAGS += -DWL_SUPP_EVENT
  DHDCFLAGS += -DFILTER_IE
  DHDCFLAGS += -DNDO_CONFIG_SUPPORT
# LOGTRACE_EVENT
  DHDCFLAGS += -DSHOW_LOGTRACE
#  DHDCFLAGS += -DLOGTRACE_FROM_FILE
# LAST ROAM EVENT LOG
  DHDCFLAGS += -DWL_LASTEVT
# Higher rate beacon transmission at softAP
  DHDCFLAGS += -DSUPPORT_AP_HIGHER_BEACONRATE
# Expand TCP tx queue to 10 times of default size
  DHDCFLAGS += -DTSQ_MULTIPLIER=10
# HANG simulation
  DHDCFLAGS += -DDHD_HANG_SEND_UP_TEST
# Enable health check event handling
  DHDCFLAGS += -DDNGL_EVENT_SUPPORT
  DHDCFLAGS += -DHCHK_COMMON_SW_EVENT

# IRQSET in DHD
# DHDCFLAGS += -DDHD_LB_IRQSET
# AXI error logging
  DHDCFLAGS += -DDNGL_AXI_ERROR_LOGGING
  #DHDCFLAGS += -DDHD_USE_WQ_FOR_DNGL_AXI_ERROR
# Disable SOFTAP_SEND_HANGEVT
  DHDCFLAGS := $(filter-out -DSOFTAP_SEND_HANGEVT,$(DHDCFLAGS))
# Debug Wakeup pkt reason
  DHDCFLAGS += -DDHD_WAKE_STATUS -DDHD_WAKE_RX_STATUS -DDHD_WAKE_EVENT_STATUS
  DHDCFLAGS += -DDHD_WAKEPKT_DUMP
# To support CAC
  DHDCFLAGS += -DSUPPORT_SET_CAC
# Use single nvram file
  DHDCFLAGS += -DDHD_USE_SINGLE_NVRAM_FILE
# RSSI Logging
  DHDCFLAGS += -DSUPPORT_RSSI_SUM_REPORT

# Change Dump path by Android Global DE Policy
ifeq ($(CONFIG_WLAN_PUB_DUMP_PATH),y)
  USE_LEGACY_DUMP_PATH=y
endif

# Use Legacy dump path
ifneq ($(USE_LEGACY_DUMP_PATH),)
  DHDCFLAGS += -DDHD_COMMON_DUMP_PATH="\"/data/media/wifi/log/\""
else
  DHDCFLAGS += -DDHD_COMMON_DUMP_PATH="\"/data/log/wifi/\""
endif

ifeq ($(CONFIG_BCM4375),y)
  DHDCFLAGS += -DENABLE_INSMOD_NO_FW_LOAD
  DHDCFLAGS += -DUSE_LATE_INITCALL_SYNC
  DRIVER_TYPE = y
endif
endif

ifeq ($(DRIVER_TYPE),y)
  # Use kernel strlcpy() implementation instead of one, defined in bcmstdlib_s.c
  DHDCFLAGS += -DBCM_USE_PLATFORM_STRLCPY
endif

#########
# Others
#########

#EXTRA_LDFLAGS += --strip-debug

EXTRA_CFLAGS += $(DHDCFLAGS) -DDHD_DEBUG
EXTRA_CFLAGS += -DSRCBASE=\"$(src)\"
EXTRA_CFLAGS += -I$(srctree)/$(src)/include/ -I$(src)/
EXTRA_CFLAGS += $(call cc-option, -Wunused-but-set-variable)
EXTRA_CFLAGS += $(call cc-option, -Wmaybe-uninitialized)
KBUILD_CFLAGS += -I$(LINUXDIR)/include -I$(CURDIR)

DHDOFILES := dhd_pno.o dhd_common.o dhd_ip.o dhd_custom_gpio.o \
	dhd_linux.o dhd_linux_sched.o dhd_cfg80211.o dhd_linux_wq.o aiutils.o bcmevent.o \
	bcmutils.o bcmwifi_channels.o hndpmu.o linux_osl.o linux_pkt.o sbutils.o siutils.o \
	wl_android.o wl_cfg80211.o wl_cfgp2p.o wl_cfg_btcoex.o wldev_common.o \
	wl_linux_mon.o wl_roam.o dhd_linux_wq.o wl_cfg_btcoex.o \
	hnd_pktq.o hnd_pktpool.o dhd_custom_cis.o dhd_debug.o dhd_debug_linux.o dhd_mschdbg.o \
	dhd_dbg_ring.o bcmstdlib_s.o frag.o dhd_linux_exportfs.o wl_cfgvendor.o

ifeq ($(BUS_IFACE_SDIO),y)
DHDOFILES += bcmsdh.o bcmsdh_linux.o bcmsdh_sdmmc.o bcmsdh_sdmmc_linux.o
DHDOFILES += dhd_cdc.o dhd_sdio.o
DHDOFILES += dhd_wlfc.o
endif

ifeq ($(BUS_IFACE_PCIE),y)
DHDOFILES += dhd_pcie.o dhd_pcie_linux.o pcie_core.o dhd_flowring.o dhd_msgbuf.o
endif

ifeq ($(CONFIG_X86_64),y)
DHDOFILES += dhd_linux_platdev_x86.o
DHDOFILES += dhd_custom_mac_addr.o
ifeq ($(CONFIG_BROADCOM_WIFI_RESERVED_MEM),y)
DHDOFILES += dhd_custom_memprealloc.o
endif # CONFIG_BROADCOM_WIFI_RESERVED_MEM
endif # CONFIG_X86_64

DHDOFILES += $(DHD_ANDROID_OFILES)

# For SPI projects
ifeq ($(CONFIG_SPI_SC8810),y)
DHDOFILES += bcmsdspi_linux.o bcmspibrcm.o
DHDOFILES :=$(filter-out bcmsdh_sdmmc.o,$(DHDOFILES))
DHDOFILES :=$(filter-out bcmsdh_sdmmc_linux.o,$(DHDOFILES))
endif

dhd-y := $(DHDOFILES)
obj-$(DRIVER_TYPE)   += dhd.o

all:
	@echo "$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules"
	@$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules

clean:
	rm -rf *.o *.ko *.mod.c *~ .*.cmd *.o.cmd .*.o.cmd \
	Module.symvers modules.order .tmp_versions modules.builtin

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install

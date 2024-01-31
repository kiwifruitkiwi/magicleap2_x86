/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include "shregion_types.h"

#define CVCORE_DSP_DATA_REGION0_VADDR  (0xD0000000U)
#define CVCORE_DSP_DATA_REGION0_SIZE   (0x2FD00000U)
#define CVCORE_DSP_DATA_REGION1_VADDR  (0x00C00000U)
#define CVCORE_DSP_DATA_REGION1_SIZE   (0xC3400000U)

#define META_INIT_MAGIC_NUM			(0xDD0011EE)
#define MAX_x86_SHARED_SHR_ENTRIES		(512)

#define MAX_DSP_SHARED_SHR_ENTRIES		(384)
#define MAX_DSP_INTERACTING_PIDS		(16)

#define DSP_SHREGION_META_FIXED_VADDR		(CVCORE_DSP_DATA_REGION1_VADDR)

// Don't change this macro def unless there's a
// specific need for it. Make sure shregion_init
// in shregion linux module sets entry_valid member
// of struct sid_meta_s to ENTRY_IS_INVALID if
// this macro is set to any other non-zero value.
#define ENTRY_IS_INVALID			(0)

#define ENTRY_IS_VALID				(1)
#define ENTRY_NOT_FOUND				(2)

#define GET_SHR_UVADDR				(0x0)
#define SET_SHR_UVADDR				(0x1)

// Any shregion can be referenced up to by max 64 processes.
#define SHREGION_MAX_REF_PIDS			(64)

// Same shregion XPC vchannel is shared between
// x86 and the DSPs. This ID info is stored
// in the first byte of xpc's command buffer
// for ARM side shregion driver to determine
// if the XPC signal came from the DSPs or the
// x86.
#define X86_SRC_CORE_ID			(0x1)
#define DSP_SRC_CORE_ID			(0x2)

#define MAX_XPC_MSG_LEN_BYTES		(27)


struct shr_prexist_meta {
	uint8_t cmd;
	uint8_t shr_uvaddr_valid;
	uint64_t shr_uvaddr;
};

struct shregion_metadata_drv {
	struct shregion_metadata meta;
	int32_t buf_fd;
	uint64_t phys_addr;
	struct shr_prexist_meta spm;
	uint32_t offset; // offset from the shregion base address.
	int32_t status; // status returned by the driver.
};

struct dsp_shregion_metadata_drv {
	uint64_t shr_name;
	uint32_t shr_size_bytes;
	uint32_t shr_cache_mask;
	int32_t buf_fd;
	uint16_t sid;
	uint32_t flags;
};

struct dsp_shregion_vaddr_meta {
	uint64_t shr_name;
	ShregionDspVaddr shr_dsp_vaddr;
	uint8_t guard_bands_config;
};

struct dmac_shregion_ioaddr_meta {
	uint64_t shr_name;
	ShregionDmacIOaddr shr_dmac_ioaddr;
	uint8_t guard_bands_config;
};

struct dmac_reg_user_buf_meta {
	uint64_t user_buf_paddr;
	ShregionDmacIOaddr user_buf_dmac_ioaddr;
	ShregionDmacIOaddr user_req_dmac_ioaddr;
	uint32_t user_buf_size;
	uint8_t  fixed_dmac_iova;
};

struct cdp_dcprs_shr_ioaddr_meta {
	uint64_t shr_name;
	ShregionCdpDcprsIOaddr shr_cdp_dcprs_ioaddr;
};

struct __attribute__ ((__packed__)) shr_xpc_msg_req {
	int8_t src_id;
	uint16_t stream_id;
};

struct __attribute__ ((__packed__)) shr_xpc_msg_resp {
	int8_t entry_status;
	uint64_t addr;
};

// IOCTL cmds.
#define SHREGION_IOC_MAGIC      ('s')

// Request to create shregion.
#define SHREGION_IOC_CREATE                 _IOW(SHREGION_IOC_MAGIC, 1, struct shregion_metadata_drv)

// Query shregion attributes and it's presence.
#define SHREGION_IOC_QUERY                  _IOW(SHREGION_IOC_MAGIC, 2, struct shregion_metadata_drv)

// Request to free/destroy shregion.
#define SHREGION_IOC_FREE                   _IOW(SHREGION_IOC_MAGIC, 3, struct shregion_metadata_drv)

// Request to get shregion metadata.
#define SHREGION_IOC_META                   _IOW(SHREGION_IOC_MAGIC, 4, struct shregion_metadata_drv)

// Get physical memory address of shregion.
#define SHREGION_IOC_GET_PHYS_ADDR          _IOW(SHREGION_IOC_MAGIC, 5, struct shregion_metadata_drv)

// Register shregion with the linux module.
#define SHREGION_IOC_REGISTER               _IOW(SHREGION_IOC_MAGIC, 6, struct shregion_metadata_drv)

// Update dsp shared shregion metadata.
#define SHREGION_IOC_DSP_META_UPDATE        _IOW(SHREGION_IOC_MAGIC, 7, struct dsp_shregion_metadata_drv)

// Get shregion dsp virtual address
#define SHREGION_IOC_GET_DSP_VADDR          _IOW(SHREGION_IOC_MAGIC, 8, struct dsp_shregion_vaddr_meta)

// Register sid meta
#define SHREGION_IOC_REGISTER_SID_META      _IOW(SHREGION_IOC_MAGIC, 9, uint32_t)

#define SHREGION_IOC_CDP_DCPRS              _IOW(SHREGION_IOC_MAGIC, 10, struct shregion_metadata_drv)

// Get shregion cdp dcprs virtual address
#define SHREGION_IOC_GET_CDP_DCPRS_IOADDR   _IOW(SHREGION_IOC_MAGIC, 11, struct cdp_dcprs_shr_ioaddr_meta)

// Sync SLC
#define SHREGION_IOC_SLC_SYNC               _IOW(SHREGION_IOC_MAGIC, 12, struct shr_slc_sync_meta)

// Get shregion dmac virtual address
#define SHREGION_IOC_GET_DMAC_IOADDR        _IOW(SHREGION_IOC_MAGIC, 14, struct dmac_shregion_ioaddr_meta)

// Register shregion with a PL330_DMAC io address.
#define SHREGION_IOC_DMAC                   _IOW(SHREGION_IOC_MAGIC, 15, struct shregion_metadata_drv)

// Register shregion with a s_intf_spcie io address.
#define SHREGION_IOC_S_INTF_SPCIE           _IOW(SHREGION_IOC_MAGIC, 16, struct shregion_metadata_drv)

#define SHREGION_IOC_REGISTER_DMAC_BUF      _IOW(SHREGION_IOC_MAGIC, 17, struct dmac_reg_user_buf_meta)

// NOTE: This structure doesn't have align attribute because it's never
// instantiated but stored in a shared memory region that is guaranteed
// to be page (4K) aligned.
struct __attribute__ ((__packed__)) dsp_shregion_metadata {

	struct __attribute__ ((__packed__)) {

		struct __attribute__ ((__packed__)) {
			uint32_t virt_addr;
			uint32_t dmac_iova;
			uint32_t size_bytes;
			uint64_t name;
		} shregion_entry[MAX_DSP_SHARED_SHR_ENTRIES];

		uint32_t sid;
		uint32_t nr_of_shr_entries;
		uint32_t meta_init_done;
	} sids_meta[MAX_DSP_INTERACTING_PIDS];

	uint32_t nr_of_sid_entries;
};

#include <linux/types.h>

#define SHREGION_GUARD_BAND_SIZE_BYTES		(64)
#define SHREGION_GUARD_BAND_PATTERN		(0x9C)
#define NR_OF_GUARD_BANDS_PER_SHREGION		(2)
#define GUARD_BANDS_ENABLED			(1)
#define GUARD_BANDS_DISABLED			(0)

struct __attribute__ ((__packed__)) shregion_guard_band_meta {
	uint8_t guard_band_pattern[SHREGION_GUARD_BAND_SIZE_BYTES];
};

struct __attribute__ ((__packed__)) x86_shared_shregion_meta {
	int nr_of_entries;
	int meta_init_done;
	struct {
		uint64_t name;
		phys_addr_t x86_phys_addr;
		uint32_t cache_mask;
		int size_bytes;
		uint8_t type;
		uint8_t guard_bands_config;
	} cvcore_name_entry[MAX_x86_SHARED_SHR_ENTRIES];
};

#ifdef CONFIG_DEBUG_FS
void shregion_app_register(void);
void shregion_app_deregister(void);
#else
static inline void shregion_app_register(void) {}
static inline void shregion_app_deregister(void) {}
#endif

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

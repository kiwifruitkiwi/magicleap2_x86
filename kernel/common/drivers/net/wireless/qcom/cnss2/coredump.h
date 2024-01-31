/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/**
 * Copyright (c) 2010 The Linux Foundation. All rights reserved.
 */

#ifndef _COREDUMP_H_
#define _COREDUMP_H_

#define CNSS_FW_CRASH_DUMP_VERSION 1

enum cnss_fw_crash_dump_type {
	CNSS_FW_CRASH_PAGING_DATA,
	CNSS_FW_CRASH_RDDM_DATA,
	CNSS_FW_REMOTE_MEM_DATA,
	CNSS_FW_CRASH_DUMP_MAX,
};

struct cnss_tlv_dump_data {
	/* see cnss_fw_crash_dump_type above */
	__le32 type;
	/* in bytes */
	__le32 tlv_len;
	/* pad to 32-bit boundaries as needed */
	u8 tlv_data[];
} __packed;

struct cnss_dump_file_data {
	/* "ATH11K-FW-DUMP" */
	char df_magic[16];
	__le32 len;
	/* file dump version */
	__le32 version;
	guid_t guid;
	/* time-of-day stamp */
	__le64 tv_sec;
	/* time-of-day stamp, nano-seconds */
	__le64 tv_nsec;
	/* room for growth w/out changing binary format */
	u8 unused[8];
	/* struct mhi_tlv_dump_data + more */
	u8 data[0];
} __packed;

void cnss_mhi_pm_rddm_worker(struct work_struct *work);

#endif

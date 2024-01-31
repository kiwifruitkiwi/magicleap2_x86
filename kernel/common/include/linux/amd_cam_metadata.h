/*
 * amd_cam_metadata.h
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 */

#ifndef __AMD_CAM_METADATA_H__
#define __AMD_CAM_METADATA_H__

#ifdef CONFIG_AMD_ISP
#include <linux/cam_api_simulation.h>
#else
#include <linux/cam_api.h>
#endif
#include <linux/amd_cam_metadata_tags.h>

#define OK 0
#define ERROR -1
#define TAG_NOT_FOUND -2
#define NAME_NOT_FOUND -3

#define FOUR_BYTES 4
#define TAG_SECTION_SHIFT 16
#define TAG_INDEX_MASK 0xFFFF

enum {
	// Unsigned 8-bit integer (uint8_t)
	TYPE_BYTE = 0,
	// Signed 32-bit integer (int32_t)
	TYPE_INT32 = 1,
	// 32-bit float (float)
	TYPE_FLOAT = 2,
	// Signed 64-bit integer (int64_t)
	TYPE_INT64 = 3,
	// 64-bit float (double)
	TYPE_DOUBLE = 4,
	// A 64-bit fraction (amd_cam_metadata_rational)
	TYPE_RATIONAL = 5,
	// Max number of type fields
	MAX_NUM_TYPES
};

// A reference to a metadata entry in a buffer.
struct amd_cam_metadata_entry {
	size_t index;
	u32 tag;
	u8 type;
	size_t count;
	union {
		u8 *u8;
		s32 *i32;
		float *f;
		s64 *i64;
		double *d;
		struct amd_cam_metadata_rational *r;
	} data;
};

struct tag_info {
	const char *tag_name;
	u32 tag_type;
};

size_t calc_amd_md_entry_data_size(u32 type, size_t data_count);
struct amd_cam_metadata_buffer_entry *amd_cam_get_entries(struct
							  amd_cam_metadata
							  *metadata);
int amd_cam_find_metadata_entry(struct amd_cam_metadata *src, u32 tag,
				struct amd_cam_metadata_entry *entry);
int amd_cam_get_metadata_entry(struct amd_cam_metadata *src, size_t index,
			       struct amd_cam_metadata_entry *entry);
int amd_cam_add_metadata_entry(struct amd_cam_metadata *dst, u32 tag,
			       u32 type, const void *data, size_t data_count);
int amd_cam_update_metadata_entry(struct amd_cam_metadata *dst, size_t index,
				  const void *data, size_t data_count,
				  struct amd_cam_metadata_entry *updated_entry);
int amd_cam_update_tag(struct amd_cam_metadata *metadata, u32 tag,
		       const void *data, size_t data_count,
		       struct amd_cam_metadata_entry *updated_entry);
int amd_cam_get_metadata_tag_type(u32 tag);
const char *amd_cam_get_metadata_tag_name(u32 tag);

#endif				//__AMD_CAM_METADATA_H__

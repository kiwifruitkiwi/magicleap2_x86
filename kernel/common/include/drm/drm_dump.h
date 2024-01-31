/* SPDX-License-Identifier: GPL-2.0 */
/*
 * DRM fornt-end of pstore:
 * Interface provided by DRM for dumping gpu status upon gpu hang
 */
#ifndef DRM_DUMP_H
#define DRM_DUMP_H

#include <stddef.h>

/*
 * Keep this list arranged in rough order of priority.
 */
enum drm_dump_reason {
	DRM_DUMP_UNDEF,
	DRM_DUMP_TIMEOUT,
	DRM_DUMP_PANIC,
};

struct drm_dumper;

typedef void (*drm_dump_cb)(struct drm_dumper *, enum drm_dump_reason);

/* DRM dumper information */
struct drm_dumper {
	drm_dump_cb dump;	// registered pstore callback
	void *data;		// drm_dumper private data
};

#ifdef CONFIG_DRM_DUMP
void drm_dump_register(drm_dump_cb dump_cb);

void drm_dump_unregister(void);

void drm_dump_add_device(void *data);

void drm_dump_remove_device(void *data);

void drm_dump_dev(void *data, enum drm_dump_reason reason);

void drm_dump(enum drm_dump_reason reason);

size_t drm_dump_get_buffer(struct drm_dumper *dumper,
			   char *buf, size_t len);

#else
static inline void drm_dump_register(drm_dump_cb dump_cb) { }

static inline void drm_dump_unregister(void) { }

static inline void drm_dump_add_device(void *data) { }

static inline void drm_dump_remove_device(void *data) { }

static inline void drm_dump_dev(void *data, enum drm_dump_reason reason) { }

static inline void drm_dump(enum drm_dump_reason reason) { }

static inline size_t drm_dump_get_buffer(struct drm_dumper *dumper,
					 char *buf, size_t len) {
	return 0;
}
#endif /* CONFIG_DRM_DUMP */

#endif /* DRM_DUMP_H */

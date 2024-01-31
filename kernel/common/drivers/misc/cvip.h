/*
 * Defines for the CVIP driver
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef __CVIP_H
#define __CVIP_H

#define CVIP_X86_BUF_SIZE	(16 * 1024 * 1024)
#define CVIP_COLDBOOT		0
#define CVIP_WATCHDOG_REBOOT	2

/**
 * struct ipc_buf - ipc ring buffer shared by x86 and CVIP
 * @buf_addr: ring buffer address
 * @buf_size: ring buffer size
 */
struct ipc_buf {
	u32 buf_addr[2];
	u32 buf_size;
};

/*
 * struct ipc_payload - ipc message data shared by x86 and CVIP
 * @data:    payload data
 * @reserve: reserved/unused
 */
struct ipc_payload {
	u32 data[2];
	u32 reserve;
};

struct ipc_msg {
	union {
		struct ipc_buf b;
		struct ipc_payload p;
	} msg;
};

enum cvip_event_type {
	on_head,
	off_head,
	s2_idle,
	cvip_resume
};

int cvip_get_mbid(u32 idx);
struct device *cvip_get_mb_device(void);
int cvip_get_num_chans(void);
int cvip_gpu_get_pages(struct page **pages, u32 *npages);
int cvip_set_gpuva(u64 gpuva);
void cvip_update_panic_count(void);

#ifdef CONFIG_CVIP
int cvip_is_alive(struct ipc_msg *msg);
int cvip_handle_panic(struct ipc_buf *buf);
void cvip_low_power_ack(u32 state);
int cvip_device_state_uevent(enum cvip_event_type event);
#else
static inline int cvip_is_alive(struct ipc_msg *msg)
{
	return 0;
}

static inline int cvip_device_state_uevent(enum cvip_event_type event)
{
	return 0;
}

static inline int cvip_handle_panic(struct ipc_buf *buf)
{
	return 0;
}

static inline void cvip_low_power_ack(u32 state) { }
#endif

#ifdef CONFIG_PM
bool set_drm_dpms(bool state);
void cvip_set_power_completion(void);
void cvip_set_allow_s2idle(bool is_allowed);
int cvip_check_power_state(struct device *dev);
struct device_link *cvip_consumer_device_link_add(struct device *supplier_dev);
void cvip_consumer_device_link_del(struct device_link *link);
#endif

extern struct atomic_notifier_head cvip_wdt_notifier_list;
/* Hotplug req function from amd_stream.c */
int cvip_power_hot_plug_request(bool hot_plug_state);

#endif /* __CVIP_H */
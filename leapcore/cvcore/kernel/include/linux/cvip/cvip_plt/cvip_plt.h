/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Platform driver for the CVIP SOC.
 *
 * Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
 */
#ifndef __CVIP_PLT_H__
#define __CVIP_PLT_H__

#include <linux/types.h>
#include <uapi/linux/cvip/cvip_plat_ioc.h>

#define PEQ                 "peq"
#define MORTY               "mortable"

#define BOOT_CONFIG_ADDR		(0x0000a000U)
#define BOOT_CONFIG_SIZE		(sizeof(uint32_t))

#define HW_MORTABLE_RANGE_A1_C2   6
#define HW_MORTABLE_A1_SEC        12
#define HW_MORTABLE_A2            54

#define RSMU_BASE (0x00800000)
#define RSMU_BASE_SIZE (4096)
#define RSMU_REG_LSB_OFFSET (0x3C)
#define RSMU_REG_MSB_OFFSET (0x38)
#define RSMU_REG_WIDTH      (4)

#define BCB_OFFSET 0xa000
#define P2C_MSG0_OFFSET 0x103810680
#define WATCHDOG_REBOOT 0x2

#define USB_GADGET_REG1  (0x1171f86b0)
#define USB_GADGET_REG2  (0x1171f8814)

/* predefined constant from AMD */
#define USB_ENABLE       (0x11)

enum {
	NOVA_ONLINE = 0,
	NOVA_OFFLINE,
	NOVA_SUSPEND,
	NOVA_S2,
	XPORT_READY,
};

enum {
	PM_STATE_NORMAL_OPERATION = 0,
	PM_STATE_STANDBY_DOZE,
	PM_STATE_S2IDLE_SLEEP,
	PM_STATE_S3_DEEP_SLEEP,
	PM_STATE_HOTPLUG,
	PM_STATE_SHUTDOWN,
	PM_STATE_FAULT,
	PM_STATE_COUNT,
};

union boot_config {
	uint32_t raw;
	struct {
		uint8_t cold       : 1;
		uint8_t secured    : 1;
		uint8_t reserved1  : 6;
		uint8_t board_id   : 6;
		uint8_t board_type : 2;
		uint16_t reserved0;
	} bits;
};

extern void notify_nova_online(void);
extern void notify_xport_ready(uint32_t type);

extern void set_nova_shared_base(uint64_t addr);
extern void set_mlnet_base(phys_addr_t addr);
extern void set_dump_buf_base(phys_addr_t addr);

extern int get_cvip_pm_state(void);
extern int cvip_pm_wait_for_completion(void);
extern void cvip_pm_reinit_completion(void);

extern struct atomic_notifier_head cvip_pci_ready_notifier_list;

extern size_t cvip_plt_property_get_safe(const char *key, char *val,
	size_t buf_size, const char *default_val);

extern int cvip_plt_property_get_from_fmr(void);
extern phys_addr_t cvip_plt_property_get_addr(void);
extern size_t cvip_plt_property_get_size(void);

extern int send_cvip_panic_message(void);

extern const struct attribute *scpi_attrs[];

int cvip_plt_pci_scan(struct cvip_plat_pci *data);
int cvip_plt_pci_cap_set(struct cvip_plat_pci *data);
int cvip_plt_pci_cap_get(struct cvip_plat_pci *data);

void cvip_notify_resume(void);
void cvip_notify_s2idle(void);

extern struct atomic_notifier_head nova_event_notifier_list;
extern struct atomic_notifier_head xport_ready_notifier_list;
extern struct atomic_notifier_head s2_idle_notifier_list;
extern struct atomic_notifier_head resume_notifier_list;

extern int cvip_plt_scpi_set_a55dsu(unsigned long freq);
extern int cvip_plt_scpi_set_decompress(unsigned long freq);

extern void cvip_plt_get_reboot_info(uint32_t *boot_reason, uint32_t *watchdog_count);

#endif

/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _DSP_MANAGER_INTERNAL_H
#define _DSP_MANAGER_INTERNAL_H

#include <linux/list.h>
#include <linux/dma-buf.h>
#include <linux/iommu.h>

#define DSP_DBG(fmt, ...) \
	pr_debug("dsp_manager: %s: " fmt, __func__, ##__VA_ARGS__)
#define DSP_ERROR(fmt, ...) \
	pr_err("dsp_manager: %s: " fmt, __func__, ##__VA_ARGS__)

#define EVENT_LOG_SIZE 2048
#define DSPMEM_MAP_NUM 4

/*PGFSM Status*/
#define PGFSM_POWER_ON			0x0
#define PGFSM_POWER_ON_PROGRESS		0x1
#define PGFSM_POWER_OFF			0x2
#define PGFSM_POWER_OFF_PROGRESS	0x3

/*
 * struct dsp_device_t
 *
 * Stores all the information about the device for access between functions
 *
 * @dsp_device_lock: A mutex lock to prevent client_list and dsp_list from
 *	being modified multiple times at the same time.
 * @char_class: The character class associated with the device driver
 * @char_device: The character device associated with the device driver
 * @status_register: Stores the base address of the DSP status registers
 * @pgfsm_register: Stores the base address of the RSMU PGFSM cvip register
 * @control_register: Stores the base address of the DSP control registers
 * @client_list: A linked list containing all registered clients
 * @dsp_list: A linked list containing all DSPs that the driver has access to
 * @event_log: An int to store the index of the returned log from cvip_logger
 * @debug_dir: The entry structure for the debugfs directory for the driver
 * @q6_cur_dpmclk: Q6 dpm clk to set
 * @c5_cur_dpmclk: C5 dpm clk to set
 */
struct dsp_device_t {
	/* Mutex lock to protect access to both client_list and dsp_list */
	struct mutex dsp_device_lock;
	struct class	*char_class;
	struct device	*char_device;
	void __iomem *ocd_register;
	void __iomem *status_register;
	void __iomem *pgfsm_register;
	void __iomem *Q6_cntrl_register;
	void __iomem *C5_cntrl_register;
	struct list_head client_list;
	struct list_head dsp_list;
	int event_log;
	struct dentry *debug_dir;
	unsigned long q6_cur_dpmclk;
	unsigned long c5_cur_dpmclk;
};

/*
 * struct dsp_memory_t
 *
 * Will store any information relating to the memory allocated for the DSP
 * during the time it is in use.
 *
 * @allocated: the ION buffer is allocated by dsp manager
 * @fd: Stores the file descriptor created through the ION driver
 * @mapcnt: count of mapped dmabuf
 * @dmabuf: A structure containing the dma buffer from the file
 * @attach: Stores the dma buffer attachment with dsp device
 * @domain0: A structure containing the IOMMU domain with SID0
 * @domain: A structure containing the domain information for the MMU
 * @sg_table: Stores sg_table from dma_buf_map_attachment
 * @mem_pool_id: Stores the ID of the memory pool used
 * @cache_attr: Stores the attribute used with the memory pool
 * @addr: Stores the address to the memory section
 * @vaddr: Stores the kernel map address
 * @size: Stores the size of the memory section
 * @size_0: Stores the iommmu mapped size for domain-0
 * @access_attr: Stores the access attribute of the MMU domain
 * @list: A list head to allow for linked listing
 */
struct dsp_memory_t {
	bool allocated;
	int fd;
	int mapcnt;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach[DSPMEM_MAP_NUM];
	struct iommu_domain *domain0;
	struct iommu_domain *domain[DSPMEM_MAP_NUM];
	struct sg_table *sg_table[DSPMEM_MAP_NUM];
	int mem_pool_id;
	int cache_attr;
	void *addr;
	void *vaddr;
	unsigned int size;
	unsigned int size_0;
	int access_attr;
	struct list_head list;
};

/*
 * struct dsp_state_t
 *
 * Will store any information relating to the current state of the DSP during
 * the time it is in use
 *
 * @status_register: Stores the address of the DSP status register
 * @control_register: Stores the address of the DSP control register
 * @pgfsm_register: Stores the base address of the RSMU PGFSM cvip register
 * @dsp_mode: Stores the current mode that the DSP is in
 * @submit_mode: Stores the mode for last submission
 * @alt_stat_vector_sel: Stores the alternative reset vector selection set
 *                       by user
 * @bypass_pd: non-zero indicates to bypass the DSP PD control logic
 * @clk: dsp variable clock
 * @clk_rate: requested clock rate in Hz
 */
struct dsp_state_t {
	void __iomem *ocd_register;
	void __iomem *status_register;
	void __iomem *pgfsm_register;
	void __iomem *control_register;
	int dsp_mode;
	int submit_mode;
	int alt_stat_vector_sel;
	int bypass_pd;
	struct clk *clk;
	unsigned long clk_rate;
};

/*
 * struct dsp_resources_t
 *
 * A single entry for the DSP list. Contains all information about a single DSP
 * including which DSP this structure is for, and pointers to other data
 * required by the DSP or program.
 *
 * @dsp_resource_lock: A mutex lock to prevent a single DSP state and memory
 *	from being modified by multiple users at the same time.
 * @dsp_id: A value used to uniquely identify the DSP that this structure is for
 * @states: A structure containing all information about the current state
 * @client: A pointer to a client that is accessing the DSP. Is set to NULL if
 *	no client as access to the DSP
 * @memory_list: A list containing all allocated memory
 * @list: A list head to allow for linked listing
 */
struct dsp_resources_t {
	/* Mutex lock to protect access to the DSP's resources */
	struct mutex dsp_resource_lock;
	int dsp_id;
	struct dsp_state_t *states;
	struct dsp_client_t *client;
	struct list_head memory_list;
	struct list_head list;
};

/*
 * struct dsp_client_t
 *
 * A structure used to represent each client that has access to a DSP. Will
 * contain any information about the client.
 *
 * @list: A list head to allow for linked listing
 */
struct dsp_client_t {
	struct list_head list;
};

int dsp_helper_read(struct dsp_state_t *states);
int dsp_helper_off(struct dsp_device_t *device, struct dsp_resources_t *res);
int dsp_helper_stop(struct dsp_device_t *device, struct dsp_state_t *states,
		int dsp_id);
int dsp_helper_pause(struct dsp_device_t *device, struct dsp_state_t *states,
		     int dsp_id);
int dsp_helper_run(struct dsp_device_t *device, struct dsp_state_t *states,
		int dsp_id);
int dsp_helper_read_clkgate_setting(struct dsp_state_t *states);
int dsp_helper_write_clkgate_setting(struct dsp_state_t *states, int clkgate);
int dsp_helper_read_stat_vector_sel(struct dsp_state_t *states);
int dsp_helper_write_stat_vector_sel(struct dsp_state_t *states, int altvec);
int pgfsm_helper_get_status(void __iomem *pgfsm_base, int dsp_id);
int pgfsm_helper_power(void __iomem *pgfsm_base, int dsp_id, int power);
int dsp_clk_dpm_cntrl(unsigned int clk_id, unsigned int dpm_level);
struct clk *helper_get_clk(int dsp_id);
int helper_get_pdbypass(int dsp_id);
#ifdef CONFIG_MERO_DSP_MANAGER
int dsp_suspend(void);
int dsp_resume(void);
#else
static inline int dsp_suspend(void)
{
	return 0;
}

static inline int dsp_resume(void)
{
	return 0;
}
#endif

#ifdef CONFIG_DEBUG_FS
int dsp_debugfs_add(struct dsp_device_t *dsp_device);
void dsp_debugfs_rm(struct dsp_device_t *dsp_device);
#else
static inline int dsp_debugfs_add(struct dsp_device_t *dsp_device)
{
	return 0;
}

static inline void dsp_debugfs_rm(struct dsp_device_t *dsp_device) {}
#endif

#endif /* _DSP_MANAGER_INTERNAL_H */

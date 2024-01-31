/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef DSP_MANAGER_H
#define DSP_MANAGER_H

#define TYPE 0
#define NO_ION -1
#define MAX_DPM_LEVEL 4

enum DSP_MODES {
	DSP_OFF,
	DSP_STOP,
	DSP_PAUSE,
	DSP_RUN,
	DSP_HALT_ON_RESET,
	DSP_UNPAUSE,
	DSP_EXIT_OCD
};

enum DSP_IDS {
	DSP_Q6_0,
	DSP_Q6_1,
	DSP_Q6_2,
	DSP_Q6_3,
	DSP_Q6_4,
	DSP_Q6_5,
	DSP_C5_0,
	DSP_C5_1,
	DSP_MAX
};

enum ACCESS_ATTR {
	// base attributes
	ICACHE  = 1 << 0,
	DCACHE  = 1 << 1,
	IDMA    = 1 << 2,
	NOSHARE = 1 << 3,
	// combined attributes
	ICACHE_NOSHARE = (ICACHE | NOSHARE),
	DCACHE_NOSHARE = (DCACHE | NOSHARE),
};

/*
 * struct mem_alloc_t
 *
 * Stores IOCTL parameters for sending information between the user application
 * and the kernel driver. Used to pass parameters for memory allocation for the
 * DSPs and return the fd from ION to the client.
 *
 * @dsp_id: Stores the ID of the DSP to allocate memory for
 * @mem_pool_id: Stores the ID of the memory pool used
 * @cache_attr: Stores the attribute used with the memory pool
 * @sect_addr: Stores the address to the memory section
 * @sect_size: Stores the size of the memory section
 * @access_attr: Stores the access attribute for the MMU domain
 * @fd: Stores the returned fd from the dsp manager
 */
struct mem_alloc_t {
	int dsp_id;
	int mem_pool_id;
	int cache_attr;
	void *sect_addr;
	unsigned int sect_size;
	int access_attr;
	int fd;
};

/*
 * struct set_mode_t
 *
 * Stores IOCTL parameters for sending information between the user application
 * and the kernel driver. Used to pass parameters for setting the mode to the
 * DSPs.
 *
 * @dsp_id: Stores the ID of the DSP to change the state of
 * @dsp_mode: Stores the mode to change the DSP to
 */
struct set_mode_t {
	int dsp_id;
	int dsp_mode;
};

/*
 * struct get_state_t
 *
 * Stores IOCTL parameters for sending information between the user application
 * and the kernel driver. Used to pass the ID of the DSP to get the mode of and
 * the mode of the DSP to be returned to the client.
 *
 * @dsp_id: Stores the ID of the DSP to change the state of
 * @dsp_mode: Stores the return mode from the dsp manager
 */
struct get_state_t {
	int dsp_id;
	int dsp_mode;
};

/*
 * struct set_brkpt_t
 *
 * Stores IOCTL parameters for sending information between the user application
 * and the kernel driver. Used for passing DSP id and DSP address for
 * breakpoint related control.
 *
 * @dsp_id: Stores the ID of the DSP to change the state of
 * @bindex: Stores the index of instruction breakpoint to use
 * @dsp_addr: Stores the DSP address for breakpoint control
 */
struct set_brkpt_t {
	int dsp_id;
	int bindex;
	int dsp_addr;
};

/*
 * Register types
 */
#define REG_AR	0x1
#define REG_SR	0x2

/*
 * struct set_reg_t
 *
 * Stores IOCTL parameters for sending information between the user application
 * and the kernel driver. Passing DSP id and value to set to a register
 *
 * @dsp_id: Stores the ID of the DSP
 * @reg_type: Stores the type of register to set
 * @regidx: Stores the register number to set
 * @val: Stores the value to write to register
 */
struct set_reg_t {
	int dsp_id;
	int reg_type;
	int reg_idx;
	unsigned int val;
};

/*
 * struct get_reg_t
 *
 * Stores IOCTL parameters for sending information between the user application
 * and the kernel driver. Passing DSP id and register number to read
 *
 * @dsp_id: Stores the ID of the DSP
 * @reg_type: Stores the type of register to read
 * @reg_idx: Stores the register number to read
 * @val: Stores the value of register
 */
struct get_reg_t {
	int dsp_id;
	int reg_type;
	int reg_idx;
	unsigned int val;
};

#define IOCTL_MEM_ALLOC		_IOWR(TYPE, 0, struct mem_alloc_t *)
#define IOCTL_DSP_SET_MODE	_IOW(TYPE, 1, struct set_mode_t *)
#define IOCTL_DSP_GET_STATE	_IOWR(TYPE, 2, struct get_state_t *)
#define IOCTL_DSP_SET_BRKPT	_IOW(TYPE, 3, struct set_brkpt_t *)
#define IOCTL_DSP_CLR_BRKPT	_IOW(TYPE, 4, struct set_brkpt_t *)
#define IOCTL_DSP_SET_REG	_IOW(TYPE, 5, struct set_reg_t *)
#define IOCTL_DSP_GET_REG	_IOWR(TYPE, 6, struct get_reg_t *)
#define IOCTL_MEM_DEALLOC	_IO(TYPE, 7)
#define IOCTL_DSP_WAIT_BRKPT	_IOW(TYPE, 8, struct set_brkpt_t *)
/*
 * user application should clear the excption manually with this ioctl cmd,
 * once get EILSEQ errno during access OCD features.
 * Passing DSP ID as parameter.
 */
#define IOCTL_DSP_CLR_EXCP	_IOW(TYPE, 9, int)
#define IOCTL_MEM_MAP		_IOWR(TYPE, 10, struct mem_alloc_t *)
#define IOCTL_MEM_UNMAP		_IOWR(TYPE, 11, struct mem_alloc_t *)

#endif


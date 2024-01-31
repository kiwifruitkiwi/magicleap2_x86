/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _ISP_I2C_PRIVATE_DRIVER_H_
#define _ISP_I2C_PRIVATE_DRIVER_H_

#include "dw_common.h"
#include "dw_apb_i2c_public.h"
#include "dw_apb_i2c_private.h"

#define MAX_MASTER_DEVICES      (6)

//#define ISP_REGISTER_BASE_ADDRESS  (0x9ffc0000)
#define ISP_REGISTER_BASE_ADDRESS  (0x0)

//For FPGA (LSB FPGA version still uses reversed I2C address)
#define REVERSE_I2C_ADDR

#if (defined(HAL_RAVEN) || defined(REVERSE_I2C_ADDR))
#define ISP_I2C1_BASE_ADDRESS   (ISP_REGISTER_BASE_ADDRESS + mmI2C1_IC_CON)
#define ISP_I2C2_BASE_ADDRESS   (ISP_REGISTER_BASE_ADDRESS + mmI2C0_IC_CON)
#else
#define ISP_I2C1_BASE_ADDRESS   (ISP_REGISTER_BASE_ADDRESS + mmI2C0_IC_CON)
#define ISP_I2C2_BASE_ADDRESS   (ISP_REGISTER_BASE_ADDRESS + mmI2C1_IC_CON)
#endif


//For external i2c masters
//#define ISP_EXT_REGISTER_BASE_ADDRESS (0x0)
#define ISP_EXT_REGISTER_BASE_ADDRESS (0x20000000)
#define ISP_I2C3_BASE_ADDRESS   (ISP_EXT_REGISTER_BASE_ADDRESS + 0x0400)
#define ISP_I2C4_BASE_ADDRESS   (ISP_EXT_REGISTER_BASE_ADDRESS + 0x0800)
#define ISP_I2C5_BASE_ADDRESS   (ISP_EXT_REGISTER_BASE_ADDRESS + 0x1000)

struct _isp_i2c_driver_conf_t {
	enum _isp_i2c_driver_run_mode_e run_mode;
};

struct _isp_i2c_driver_context_t {
	struct dw_i2c_instance instances[MAX_MASTER_DEVICES];
	struct dw_i2c_param params[MAX_MASTER_DEVICES];
	struct dw_device masters[MAX_MASTER_DEVICES];
	struct _isp_i2c_driver_conf_t confs[MAX_MASTER_DEVICES];
};

struct _isp_i2c_rw_package_t {
	unsigned int device_index;
	/*the address in slave device to access,
	 *MAX 16bit according CCI protocol
	 */
	unsigned short subaddr;
	enum _isp_i2c_subaddr_mode_e subaddr_mode;
	bool enable_restart;
	unsigned int length;
	unsigned char *buffer;
};

#endif

/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _ISP_I2C_DRIVER_H_
#define _ISP_I2C_DRIVER_H_

#include "dw_common.h"
#include "dw_apb_i2c_public.h"
#include "dw_apb_i2c_private.h"

enum _isp_i2c_driver_run_mode_e {
	isp_i2c_driver_run_mode_poll,
	isp_i2c_driver_run_mode_sync_interrupt,
	isp_i2c_driver_run_mode_async_interrupt
};

enum _isp_i2c_addr_mode_e {
	isp_i2c_addr_mode_7bit,
	isp_i2c_addr_mode_10bit
};

enum _isp_i2c_subaddr_mode_e {
	isp_i2c_subaddr_mode_none,
	isp_i2c_subaddr_mode_8bit,
	isp_i2c_subaddr_mode_16bit
};

enum _isp_i2c_err_code_e {
	isp_i2c_err_code_success,
	isp_i2c_err_code_fail
};

struct _isp_i2c_configure_t {
	enum _isp_i2c_driver_run_mode_e run_mode;
	unsigned char device_index;	/*which I2C controller. 0~3 */
	/*unsigned char target_address;//the 7 bit slave address */
	/*1: 10bit address mode; 0: 7bit address mode */
	enum _isp_i2c_addr_mode_e address_mode;
	/*in k_hz. example: 10, 30, 50, 100, 200, 300, 400.
	 *the max speed is 400.
	 */
	unsigned int speed;
	/*set duty ratio for scl. range: 0~100. example,
	 *50 means the duty ratio is 50/100.
	 */
	unsigned int scl_duty_ratio;
	unsigned int input_clk;	/*in m_hz. example: 200, 300, 400, 600. */
};

enum _isp_i2c_err_code_e isp_i2c_configure(struct _isp_i2c_configure_t *conf);

/*the cci protocol.*/
enum _isp_i2c_err_code_e isp_cci_single_random_write(
				unsigned char device_index,
				unsigned short slave_addr,
				unsigned short subaddr,
				enum _isp_i2c_subaddr_mode_e
				subaddr_mode, unsigned char data);
enum _isp_i2c_err_code_e isp_cci_sequential_random_write(
				unsigned char device_index,
				unsigned short slave_addr,
				unsigned short subaddr,
				enum _isp_i2c_subaddr_mode_e
				subaddr_mode,
				unsigned char *buffer,
				unsigned int length);
enum _isp_i2c_err_code_e isp_cci_single_random_read(
				unsigned char device_index,
				unsigned short slave_addr,
				unsigned short subaddr,
				enum _isp_i2c_subaddr_mode_e
				subaddr_mode, unsigned char *data);
enum _isp_i2c_err_code_e isp_cci_single_current_read(
				unsigned char device_index,
				unsigned short slave_addr, unsigned char *data);
enum _isp_i2c_err_code_e isp_cci_sequential_random_read(
				unsigned char device_index,
				unsigned short slave_addr,
				unsigned short subaddr,
				enum _isp_i2c_subaddr_mode_e
				subaddr_mode,
				unsigned char *buffer,
				unsigned int length);
enum _isp_i2c_err_code_e isp_cci_sequential_current_read(
				unsigned char device_index,
				unsigned short slave_addr,
				unsigned char *buffer,
				unsigned int length);

/*the i2c bus read/write. no device subaddress specified.*/
enum _isp_i2c_err_code_e isp_i2c_write(unsigned char device_index,
				unsigned short slave_addr,
				unsigned char *buffer, unsigned int length);
enum _isp_i2c_err_code_e isp_i2c_read(unsigned char device_index,
				unsigned short slave_addr,
				unsigned char *buffer, unsigned int length);

/*the i2c bus device read/write. with device subaddress.*/
enum _isp_i2c_err_code_e isp_i2c_sequential_random_write(
				unsigned char device_index,
				unsigned short slave_addr,
				unsigned short subaddr,
				enum _isp_i2c_subaddr_mode_e
				subaddr_mode,
				unsigned char *buffer,
				unsigned int length);
enum _isp_i2c_err_code_e isp_i2c_sequential_random_read(
				unsigned char device_index,
				unsigned short slave_addr,
				unsigned short subaddr,
				enum _isp_i2c_subaddr_mode_e
				subaddr_mode,
				unsigned char *buffer,
				unsigned int length);

#endif

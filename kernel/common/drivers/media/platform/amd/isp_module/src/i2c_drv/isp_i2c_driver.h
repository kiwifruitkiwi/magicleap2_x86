/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef _ISP_I2C_DRIVER_H_
#define _ISP_I2C_DRIVER_H_

#include "dw_common.h"
#include "dw_apb_i2c_public.h"
#include "dw_apb_i2c_private.h"

typedef enum _isp_i2c_driver_run_mode_e {
	isp_i2c_driver_run_mode_poll,
	isp_i2c_driver_run_mode_sync_interrupt,
	isp_i2c_driver_run_mode_async_interrupt
} isp_i2c_driver_run_mode_e;

typedef enum _isp_i2c_addr_mode_e {
	isp_i2c_addr_mode_7bit,
	isp_i2c_addr_mode_10bit
} isp_i2c_addr_mode_e;

typedef enum _isp_i2c_subaddr_mode_e {
	isp_i2c_subaddr_mode_none,
	isp_i2c_subaddr_mode_8bit,
	isp_i2c_subaddr_mode_16bit
} isp_i2c_subaddr_mode_e;

typedef enum _isp_i2c_err_code_e {
	isp_i2c_err_code_success,
	isp_i2c_err_code_fail
} isp_i2c_err_code_e;

typedef struct _isp_i2c_configure_t {
	isp_i2c_driver_run_mode_e run_mode;
	uint8 device_index;	/*which I2C controller. 0~3 */
	/*uint8 target_address;//the 7 bit slave address */
	/*1: 10bit address mode; 0: 7bit address mode */
	isp_i2c_addr_mode_e address_mode;
	/*in k_hz. example: 10, 30, 50, 100, 200, 300, 400.
	 *the max speed is 400.
	 */
	uint32 speed;
	/*set duty ratio for scl. range: 0~100. example,
	 *50 means the duty ratio is 50/100.
	 */
	uint32 scl_duty_ratio;
	uint32 input_clk;	/*in m_hz. example: 200, 300, 400, 600. */
} isp_i2c_configure_t;

isp_i2c_err_code_e isp_i2c_configure(isp_i2c_configure_t *conf);

/*the cci protocol.*/
isp_i2c_err_code_e isp_cci_single_random_write(uint8 device_index,
				uint16 slave_addr,
				uint16 subaddr,
				isp_i2c_subaddr_mode_e
				subaddr_mode, uint8 data);
isp_i2c_err_code_e isp_cci_sequential_random_write(uint8 device_index,
				uint16 slave_addr,
				uint16 subaddr,
				isp_i2c_subaddr_mode_e
				subaddr_mode,
				uint8 *buffer,
				uint32 length);
isp_i2c_err_code_e isp_cci_single_random_read(uint8 device_index,
				uint16 slave_addr,
				uint16 subaddr,
				isp_i2c_subaddr_mode_e
				subaddr_mode, uint8 *data);
isp_i2c_err_code_e isp_cci_single_current_read(uint8 device_index,
				uint16 slave_addr, uint8 *data);
isp_i2c_err_code_e isp_cci_sequential_random_read(uint8 device_index,
				uint16 slave_addr,
				uint16 subaddr,
				isp_i2c_subaddr_mode_e
				subaddr_mode,
				uint8 *buffer,
				uint32 length);
isp_i2c_err_code_e isp_cci_sequential_current_read(uint8 device_index,
				uint16 slave_addr,
				uint8 *buffer,
				uint32 length);

/*the i2c bus read/write. no device subaddress specified.*/
isp_i2c_err_code_e isp_i2c_write(uint8 device_index, uint16 slave_addr,
				 uint8 *buffer, uint32 length);
isp_i2c_err_code_e isp_i2c_read(uint8 device_index, uint16 slave_addr,
				uint8 *buffer, uint32 length);

/*the i2c bus device read/write. with device subaddress.*/
isp_i2c_err_code_e isp_i2c_sequential_random_write(uint8 device_index,
				uint16 slave_addr,
				uint16 subaddr,
				isp_i2c_subaddr_mode_e
				subaddr_mode,
				uint8 *buffer,
				uint32 length);
isp_i2c_err_code_e isp_i2c_sequential_random_read(uint8 device_index,
				uint16 slave_addr,
				uint16 subaddr,
				isp_i2c_subaddr_mode_e
				subaddr_mode,
				uint8 *buffer,
				uint32 length);

#endif

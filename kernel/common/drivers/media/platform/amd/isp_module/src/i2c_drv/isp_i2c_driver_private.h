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

#ifndef _ISP_I2C_PRIVATE_DRIVER_H_
#define _ISP_I2C_PRIVATE_DRIVER_H_

#include "dw_common.h"
#include "dw_apb_i2c_public.h"
#include "dw_apb_i2c_private.h"

#define MAX_MASTER_DEVICES		(6)

/*#define ISP_REGISTER_BASE_ADDRESS	(0x9ffc0000)*/
#define ISP_REGISTER_BASE_ADDRESS	(0x0)
#define ISP_I2C1_BASE_ADDRESS	(ISP_REGISTER_BASE_ADDRESS + mmI2C1_IC_CON)
#define ISP_I2C2_BASE_ADDRESS	(ISP_REGISTER_BASE_ADDRESS + mmI2C2_IC_CON)

//For external i2c masters
#define ISP_EXT_REGISTER_BASE_ADDRESS	(0x0)
#define ISP_I2C3_BASE_ADDRESS	(ISP_EXT_REGISTER_BASE_ADDRESS + 0x0400)
#define ISP_I2C4_BASE_ADDRESS	(ISP_EXT_REGISTER_BASE_ADDRESS + 0x0800)
#define ISP_I2C5_BASE_ADDRESS	(ISP_EXT_REGISTER_BASE_ADDRESS + 0x1000)

typedef struct _isp_i2c_driver_conf_t {
	isp_i2c_driver_run_mode_e run_mode;
} isp_i2c_driver_conf_t;

typedef struct _isp_i2c_driver_context_t {
	struct dw_i2c_instance instances[MAX_MASTER_DEVICES];
	struct dw_i2c_param params[MAX_MASTER_DEVICES];
	struct dw_device masters[MAX_MASTER_DEVICES];
	isp_i2c_driver_conf_t confs[MAX_MASTER_DEVICES];
} isp_i2c_driver_context_t;

typedef struct _isp_i2c_rw_package_t {
	uint32 device_index;
	/*the address in slave device to access,
	 *MAX 16bit according CCI protocol
	 */
	uint16 subaddr;
	isp_i2c_subaddr_mode_e subaddr_mode;
	bool enable_restart;
	uint32 length;
	uint8 *buffer;
} isp_i2c_rw_package_t;

#endif

/**************************************************************************
 *
 *copyright 2014~2020 advanced micro devices, inc.
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
 *************************************************************************
 */

#include "dw_common.h"		/*common header for all drivers */
#include "dw_apb_i2c_public.h"	/*d_w_apb_i2c public header */
#include "dw_apb_i2c_private.h"	/*d_w_apb_i2c private header */
#include "i2c.h"
#include "log.h"

/*this definition is used by the assetion macros to determine the*/
/*current file name.it is defined in the d_w_common_dbc.h header.*/
DW_DEFINE_THIS_FILE;

/**********************************************************************/

int dw_i2c_init(struct dw_device *dev)
{
	int retval;

	I2C_COMMON_REQUIREMENTS(dev);

	/*disable device */
	retval = dw_i2c_disable(dev);
	/*if device is not busy (i.e. it is now disabled) */
	if (retval == 0) {
		/*disable all interrupts */
		dw_i2c_mask_irq(dev, i2c_irq_all);
		dw_i2c_clear_irq(dev, i2c_irq_all);

		/*reset instance variables */
		dw_i2c_reset_instance(dev);

		/*auto_configure component parameters if possible */
		retval = dw_i2c_auto_comp_params(dev);
	}

	return retval;
}

/**********************************************************************/

void dw_i2c_enable(struct dw_device *dev)
{
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = 0;
	DW_BIT_SET(reg, I2C_ENABLE_ENABLE, 0x1);
	I2C_OUTP(reg, portmap->enable);

	while (!dw_i2c_is_enabled(dev))
		;
}

/**********************************************************************/

int dw_i2c_disable(struct dw_device *dev)
{
	int retval = 0;
	uint32 reg;
	struct dw_i2c_portmap *portmap;
	isp_time_tick_t start, end;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	isp_get_cur_time_tick(&start);

	while (dw_i2c_is_busy(dev)) {
		isp_get_cur_time_tick(&end);
		if (isp_is_timeout(&start, &end, MAX_I2C_TIMEOUT)) {
			I2C_COMMON_REQUIREMENTS(dev);
			portmap = (struct dw_i2c_portmap *)dev->base_address;
			reg = I2C_INP(portmap->status);

			isp_dbg_print_err("dev base address=0x%p\n",
					  dev->base_address);
			isp_dbg_print_err("reg=0x%08x\n", reg);

			isp_dbg_print_err
				("%s wait unbusy timeout(%dms)\n", __func__,
				MAX_I2C_TIMEOUT);
			isp_dbg_print_info("<- %s\n", __func__);
			return DW_ETIME;
		}
	};

	if (0) {
		I2C_COMMON_REQUIREMENTS(dev);
		portmap = (struct dw_i2c_portmap *)dev->base_address;
		reg = I2C_INP(portmap->status);

		isp_dbg_print_err("dev base address=0x%p\n",
				dev->base_address);
		isp_dbg_print_err("reg=0x%08x\n", reg);

		isp_dbg_print_err("%s wait unbusy timeout(%dms)\n", __func__,
				MAX_I2C_TIMEOUT);
	}

	if (dw_i2c_is_busy(dev) == false) {
		reg = retval = 0;
		DW_BIT_SET(reg, I2C_ENABLE_ENABLE, 0);
		I2C_OUTP(reg, portmap->enable);
	}

	isp_get_cur_time_tick(&start);

	while (dw_i2c_is_enabled(dev)) {
		isp_get_cur_time_tick(&end);
		if (isp_is_timeout(&start, &end, MAX_I2C_TIMEOUT)) {
			isp_dbg_print_err
				("%s wait disable timeout(%dms)\n", __func__,
				MAX_I2C_TIMEOUT);

			return DW_ETIME;
		}
	};

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_enabled(struct dw_device *dev)
{
	bool retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->enable);
	retval = (bool) DW_BIT_GET(reg, I2C_ENABLE_ENABLE);

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_busy(struct dw_device *dev)
{
	bool retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->status);
	if (DW_BIT_GET(reg, I2C_STATUS_ACTIVITY) == 0x0)
		retval = false;
	else
		retval = true;

	return retval;
}

/**********************************************************************/

int dw_i2c_set_speed_mode(struct dw_device *dev, enum dw_i2c_speed_mode mode)
{
	int retval;
	uint32 reg;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (mode > param->max_speed_mode)
		retval = -DW_ENOSYS;
	else if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_CON_SPEED) != (uint32) mode) {
			DW_BIT_SET(reg, I2C_CON_SPEED, mode);
			I2C_OUTP(reg, portmap->con);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

enum dw_i2c_speed_mode dw_i2c_get_speed_mode(struct dw_device *dev)
{
	uint32 reg;
	enum dw_i2c_speed_mode retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->con);
	retval = (enum dw_i2c_speed_mode)DW_BIT_GET(reg, I2C_CON_SPEED);

	return retval;
}

/**********************************************************************/

int dw_i2c_set_master_address_mode(struct dw_device *dev, enum
				dw_i2c_address_mode mode)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		if (DW_BIT_GET(reg, I2C_CON_10BITADDR_MASTER) !=
			(uint32) mode) {
			DW_BIT_SET(reg, I2C_CON_10BITADDR_MASTER, mode);
			I2C_OUTP(reg, portmap->con);
		}

		reg = I2C_INP(portmap->tar);
		if (DW_BIT_GET(reg, I2C_TAR_10BITADDR_MASTER) !=
			(uint32) mode) {
			DW_BIT_SET(reg, I2C_TAR_10BITADDR_MASTER, mode);
			I2C_OUTP(reg, portmap->tar);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

enum dw_i2c_address_mode dw_i2c_get_master_address_mode(struct dw_device
							*dev)
{
	uint32 reg;
	enum dw_i2c_address_mode retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->con);
	retval = (enum dw_i2c_address_mode)DW_BIT_GET(reg,
				I2C_CON_10BITADDR_MASTER);

	return retval;
}

/**********************************************************************/

int dw_i2c_set_slave_address_mode(struct dw_device *dev, enum
			dw_i2c_address_mode mode)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_CON_10BITADDR_SLAVE) !=
			(uint32) mode) {
			DW_BIT_SET(reg, I2C_CON_10BITADDR_SLAVE, mode);
			I2C_OUTP(reg, portmap->con);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

enum dw_i2c_address_mode dw_i2c_get_slave_address_mode(struct dw_device
				*dev)
{
	uint32 reg;
	enum dw_i2c_address_mode retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->con);
	retval = (enum dw_i2c_address_mode)DW_BIT_GET(reg,
				I2C_CON_10BITADDR_SLAVE);

	return retval;
}

/**********************************************************************/

int dw_i2c_enable_slave(struct dw_device *dev)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) != 0x0) {
			DW_BIT_SET(reg, I2C_CON_SLAVE_DISABLE, 0x0);
			I2C_OUTP(reg, portmap->con);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

int dw_i2c_disable_slave(struct dw_device *dev)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) != 0x1) {
			DW_BIT_SET(reg, I2C_CON_SLAVE_DISABLE, 0x1);
			I2C_OUTP(reg, portmap->con);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_slave_enabled(struct dw_device *dev)
{
	bool retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->con);
	if (DW_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) == 0x0)
		retval = true;
	else
		retval = false;

	return retval;
}

/**********************************************************************/

int dw_i2c_enable_master(struct dw_device *dev)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_CON_MASTER_MODE) != 0x1) {
			DW_BIT_SET(reg, I2C_CON_MASTER_MODE, 0x1);
			I2C_OUTP(reg, portmap->con);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

int dw_i2c_disable_master(struct dw_device *dev)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_CON_MASTER_MODE) != 0x0) {
			DW_BIT_SET(reg, I2C_CON_MASTER_MODE, 0x0);
			I2C_OUTP(reg, portmap->con);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_master_enabled(struct dw_device *dev)
{
	uint32 reg;
	bool retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->con);
	if (DW_BIT_GET(reg, I2C_CON_MASTER_MODE) == 0x1)
		retval = true;
	else
		retval = false;

	return retval;
}

/**********************************************************************/

int dw_i2c_enable_restart(struct dw_device *dev)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_CON_RESTART_EN) != 0x1) {
			DW_BIT_SET(reg, I2C_CON_RESTART_EN, 0x1);
			I2C_OUTP(reg, portmap->con);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

int dw_i2c_disable_restart(struct dw_device *dev)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->con);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_CON_RESTART_EN) != 0x0) {
			DW_BIT_SET(reg, I2C_CON_RESTART_EN, 0x0);
			I2C_OUTP(reg, portmap->con);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_restart_enabled(struct dw_device *dev)
{
	uint32 reg;
	bool retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->con);
	if (DW_BIT_GET(reg, I2C_CON_RESTART_EN) == 0x1)
		retval = true;
	else
		retval = false;

	return retval;
}

/**********************************************************************/

int dw_i2c_set_target_address(struct dw_device *dev, uint16 address)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);
	/*address should be no more than 10 bits long */
	DW_REQUIRE((address & 0xfc00) == 0);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->tar);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_TAR_ADDR) != address) {
			DW_BIT_SET(reg, I2C_TAR_ADDR, address);
			I2C_OUTP(reg, portmap->tar);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

uint16 dw_i2c_get_target_address(struct dw_device *dev)
{
	uint32 reg;
	uint16 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->tar);
	retval = DW_BIT_GET(reg, I2C_TAR_ADDR);

	return retval;
}

/**********************************************************************/

int dw_i2c_set_slave_address(struct dw_device *dev, uint16 address)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);
	/*address should be no more than 10 bits long */
	DW_REQUIRE((address & 0xfc00) == 0);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		reg = retval = 0;
		DW_BIT_SET(reg, I2C_SAR_ADDR, address);
		I2C_OUTP(reg, portmap->sar);
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

uint16 dw_i2c_get_slave_address(struct dw_device *dev)
{
	uint32 reg;
	uint16 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->sar);
	retval = DW_BIT_GET(reg, I2C_SAR_ADDR);

	return retval;
}

/**********************************************************************/

int dw_i2c_set_tx_mode(struct dw_device *dev, enum dw_i2c_tx_mode mode)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		retval = 0;
		reg = I2C_INP(portmap->tar);
		/*avoid bus write if possible */
		if (DW_BIT_GET(reg, I2C_TAR_TX_MODE) != (uint32) mode) {
			DW_BIT_SET(reg, I2C_TAR_TX_MODE, mode);
			I2C_OUTP(reg, portmap->tar);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

enum dw_i2c_tx_mode dw_i2c_get_tx_mode(struct dw_device *dev)
{
	uint32 reg;
	enum dw_i2c_tx_mode retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->tar);
	retval = (enum dw_i2c_tx_mode)DW_BIT_GET(reg, I2C_TAR_TX_MODE);

	return retval;
}

/**********************************************************************/

int dw_i2c_set_master_code(struct dw_device *dev, uint8 code)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);
	/*code should be no more than 3 bits wide */
	DW_REQUIRE((code & 0xf8) == 0);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (dw_i2c_is_enabled(dev) == false) {
		reg = retval = 0;
		DW_BIT_SET(reg, I2C_HS_MADDR_HS_MAR, code);
		I2C_OUTP(reg, portmap->hs_maddr);
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

uint8 dw_i2c_get_master_code(struct dw_device *dev)
{
	uint32 reg;
	uint8 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->hs_maddr);
	retval = (uint8) DW_BIT_GET(reg, I2C_HS_MADDR_HS_MAR);

	return retval;
}

/**********************************************************************/

int dw_i2c_set_scl_count(struct dw_device *dev, enum dw_i2c_speed_mode
			 mode, enum dw_i2c_scl_phase phase, uint16 count)
{
	int retval;
	uint32 reg;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;
	param = (struct dw_i2c_param *)dev->comp_param;

	if (param->hc_count_values == true)
		retval = -DW_ENOSYS;
	else if (dw_i2c_is_enabled(dev) == false) {
		reg = retval = 0;
		DW_BIT_SET(reg, I2C_SCL_COUNT, count);
		if (mode == i2c_speed_high) {
			if (phase == i2c_scl_low)
				I2C_OUTP(reg, portmap->hs_scl_lcnt);
			else
				I2C_OUTP(reg, portmap->hs_scl_hcnt);
		} else if (mode == i2c_speed_fast) {
			if (phase == i2c_scl_low)
				I2C_OUTP(reg, portmap->fs_scl_lcnt);
			else
				I2C_OUTP(reg, portmap->fs_scl_hcnt);
		} else if (mode == i2c_speed_standard) {
			if (phase == i2c_scl_low)
				I2C_OUTP(reg, portmap->ss_scl_lcnt);
			else
				I2C_OUTP(reg, portmap->ss_scl_hcnt);
		}
	} else
		retval = -DW_EPERM;

	return retval;
}

/**********************************************************************/

uint16 dw_i2c_get_scl_count(struct dw_device *dev, enum
		dw_i2c_speed_mode mode, enum dw_i2c_scl_phase phase)
{
	uint32 reg;
	uint16 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (mode == i2c_speed_high) {
		if (phase == i2c_scl_low)
			reg = I2C_INP(portmap->hs_scl_lcnt);
		else
			reg = I2C_INP(portmap->hs_scl_hcnt);
	} else if (mode == i2c_speed_fast) {
		if (phase == i2c_scl_low)
			reg = I2C_INP(portmap->fs_scl_lcnt);
		else
			reg = I2C_INP(portmap->fs_scl_hcnt);
	} else if (mode == i2c_speed_standard) {
		if (phase == i2c_scl_low)
			reg = I2C_INP(portmap->ss_scl_lcnt);
		else
			reg = I2C_INP(portmap->ss_scl_hcnt);
	} else {
		reg = 0;
		isp_dbg_print_err("illegal mode in %s %d\n", __func__, mode);
	}

	retval = (uint16) DW_BIT_GET(reg, I2C_SCL_COUNT);

	return retval;
}

/**********************************************************************/

uint8 dw_i2c_read(struct dw_device *dev)
{
	uint8 retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	/*read a byte from the DATA_CMD register */
	reg = DW_IN8P(portmap->data_cmd);
	retval = (reg & 0xff);

	return retval;
}

/**********************************************************************/

void dw_i2c_write(struct dw_device *dev, uint16 character)
{
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	/*write a byte to the DATA_CMD register */
	DW_OUT16P(character, portmap->data_cmd);
}

/**********************************************************************/

void dw_i2c_issue_read(struct dw_device *dev, uint16 stop, uint16 restart)
{
	struct dw_i2c_portmap *portmap;
	uint16 data = 0x100;

	I2C_COMMON_REQUIREMENTS(dev);
	//isp_dbg_print_info("dw_i2c_issue_read, stop %d, restart %d\n",
	//			stop,restart);
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (stop)
		data |= 0x200;

	if (restart)
		data |= 0x400;

	DW_OUT16P(data, portmap->data_cmd);
}

/**********************************************************************/

enum dw_i2c_tx_abort dw_i2c_get_tx_abort_source(struct dw_device *dev)
{
	uint32 reg;
	enum dw_i2c_tx_abort retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->tx_abrt_source);
	retval = (enum dw_i2c_tx_abort)DW_BIT_GET(reg, I2C_TX_ABRT_SRC_ALL);

	return retval;
}

/**********************************************************************/

uint16 dw_i2c_get_tx_fifo_depth(struct dw_device *dev)
{
	uint16 retval;
	struct dw_i2c_param *param;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;

	retval = param->tx_buffer_depth;

	return retval;
}

/**********************************************************************/

uint16 dw_i2c_get_rx_fifo_depth(struct dw_device *dev)
{
	uint16 retval;
	struct dw_i2c_param *param;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;

	retval = param->rx_buffer_depth;

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_tx_fifo_full(struct dw_device *dev)
{
	uint32 reg;
	bool retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->status);
	if (DW_BIT_GET(reg, I2C_STATUS_TFNF) == 0)
		retval = true;
	else
		retval = false;

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_tx_fifo_empty(struct dw_device *dev)
{
	bool retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->status);
	if (DW_BIT_GET(reg, I2C_STATUS_TFE) == 1)
		retval = true;
	else
		retval = false;

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_rx_fifo_full(struct dw_device *dev)
{
	uint32 reg;
	bool retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->status);
	if (DW_BIT_GET(reg, I2C_STATUS_RFF) == 1)
		retval = true;
	else
		retval = false;

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_rx_fifo_empty(struct dw_device *dev)
{
	uint32 reg;
	bool retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->status);
	if (DW_BIT_GET(reg, I2C_STATUS_RFNE) == 0)
		retval = true;
	else
		retval = false;

	return retval;
}

/**********************************************************************/

uint16 dw_i2c_get_tx_fifo_level(struct dw_device *dev)
{
	uint32 reg;
	uint16 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->txflr);
	retval = (uint16) DW_BIT_GET(reg, I2C_TXFLR_TXFL);

	return retval;
}

/**********************************************************************/

uint16 dw_i2c_get_rx_fifo_level(struct dw_device *dev)
{
	uint32 reg;
	uint16 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->rxflr);
	retval = (uint16) DW_BIT_GET(reg, I2C_RXFLR_RXFL);

	return retval;
}

/**********************************************************************/

int dw_i2c_set_tx_threshold(struct dw_device *dev, uint8 level)
{
	int retval;
	uint32 reg;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	/*we need to be careful here not to overwrite the tx threshold if */
	/*the interrupt handler has altered it to trigger on the last byte */
	/*of the current transfer (in order to call the user callback */
	/*function at the appropriate time).when the driver returns to */
	/*the idle state, it will update the tx threshold with the */
	/*user-specified value. */
	if (level > param->tx_buffer_depth)
		retval = -DW_EINVAL;
	else {
		/*store user tx threshold value */
		instance->tx_threshold = level;

		if (instance->state == i2c_state_idle) {
			reg = retval = 0;
			DW_BIT_SET(reg, I2C_TX_TL_TX_TL, level);
			I2C_OUTP(reg, portmap->tx_tl);
		} else
			retval = -DW_EBUSY;
	}

	return retval;
}

/**********************************************************************/

uint8 dw_i2c_get_tx_threshold(struct dw_device *dev)
{
	uint32 reg;
	uint8 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->tx_tl);
	retval = DW_BIT_GET(reg, I2C_TX_TL_TX_TL);

	return retval;
}

/**********************************************************************/

int dw_i2c_set_rx_threshold(struct dw_device *dev, uint8 level)
{
	int retval;
	uint32 reg;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	/*we need to be careful here not to overwrite the rx threshold if */
	/*the interrupt handler has altered it to trigger on the last byte */
	/*of the current transfer (in order to call the user callback */
	/*function at the appropriate time).when the driver returns to */
	/*the idle state, it will update the rx threshold with the */
	/*user-specified value. */
	if (level > param->rx_buffer_depth)
		retval = -DW_EINVAL;
	else {
		/*store user rx threshold value */
		instance->rx_threshold = level;

		if (instance->state == i2c_state_idle) {
			reg = retval = 0;
			DW_BIT_SET(reg, I2C_RX_TL_RX_TL, level);
			I2C_OUTP(reg, portmap->rx_tl);
		} else
			retval = -DW_EBUSY;
	}

	return retval;
}

/**********************************************************************/

uint8 dw_i2c_get_rx_threshold(struct dw_device *dev)
{
	uint32 reg;
	uint8 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->rx_tl);
	retval = DW_BIT_GET(reg, I2C_RX_TL_RX_TL);

	return retval;
}

/**********************************************************************/

void dw_i2c_set_listener(struct dw_device *dev, dw_callback user_function)
{
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(user_function != NULL);

	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	/*critical section of code.shared data needs to be protected. */
	/*this macro disables all d_w_apb_i2c interrupts. */
	I2C_ENTER_CRITICAL_SECTION();

	instance->listener = user_function;

	instance->intr_mask_save = (enum dw_i2c_irq)(i2c_irq_rx_under |
						i2c_irq_rx_over |
						i2c_irq_tx_over |
						i2c_irq_rd_req |
						i2c_irq_tx_abrt |
						i2c_irq_rx_done |
						i2c_irq_gen_call);
	/*don't enable rx FIFO full if DMA hardware handshaking is in use */
	if (instance->dma_rx.mode != dw_dma_hw_handshake)
		instance->intr_mask_save |= i2c_irq_rx_full;

	/*end of critical section of code. this macros restores d_w_apb_i2c */
	/*interrupts. */
	I2C_EXIT_CRITICAL_SECTION();
}

/**********************************************************************/

/*master transmit function*/
int dw_i2c_master_back2_back(struct dw_device *dev, uint16 *tx_buffer,
			unsigned int tx_length, uint8 *rx_buffer,
			unsigned int rx_length, dw_callback user_function)
{
	int retval;
	uint8 *tmp;
	uint32 reg;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(tx_buffer != NULL);
	DW_REQUIRE(tx_length != 0);
	DW_REQUIRE(rx_buffer != NULL);
	DW_REQUIRE(rx_length != 0);
	DW_REQUIRE(rx_length <= tx_length);

	instance = (struct dw_i2c_instance *)dev->instance;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	DW_REQUIRE(instance->listener != NULL);

	/*critical section of code.shared data needs to be protected. */
	/*this macro disables d_w_apb_i2c interrupts. */
	I2C_ENTER_CRITICAL_SECTION();

	if (instance->state == i2c_state_idle) {
		retval = 0;
		/*master back2back transfer mode */
		instance->state = i2c_state_back2back;
		instance->rx_buffer = rx_buffer;
		instance->rx_length = rx_length;
		instance->rx_remain = rx_length;
		instance->rx_callback = user_function;
		instance->rx_hold = 0;
		instance->rx_idx = 4;
		instance->b2b_buffer = tx_buffer;
		instance->tx_length = tx_length;
		instance->tx_remain = tx_length;
		instance->tx_callback = user_function;
		instance->tx_hold = 0;
		instance->tx_idx = 0;

		/*check if rx buffer is word-aligned */
		if (((uint64) rx_buffer & 0x3) == 0)
			instance->rx_align = true;
		else
			instance->rx_align = false;

		/*support non-word aligned 16-bit buffers */
		tmp = (uint8 *) tx_buffer;
		//isp_dbg_print_info("-> %s\n",__FUNCTION__);

		while (((((uint64) tmp) & 0x3) != 0x0)
			&& ((instance->tx_length - instance->tx_idx) > 0)) {
			instance->tx_hold |=
				((*tmp++ & 0xff) << (instance->tx_idx * 8));
			instance->tx_idx++;
		}
		//isp_dbg_print_info("<- %s\n",__FUNCTION__);
		instance->tx_buffer = tmp;

		/*set rx fifo threshold if necessary */
		if (rx_length <= instance->rx_threshold) {
			reg = 0;
			DW_BIT_SET(reg, I2C_RX_TL_RX_TL, rx_length - 1);
			I2C_OUTP(reg, portmap->rx_tl);
		}
		/*ensure transfer is underway */
		instance->intr_mask_save |= (i2c_irq_tx_empty |
					i2c_irq_tx_abrt |
					i2c_irq_rx_full);
	} else
		retval = -DW_EBUSY;

	/*end of critical section of code. this macros restores d_w_apb_i2c */
	/*interrupts. */
	I2C_EXIT_CRITICAL_SECTION();

	return retval;
}

/**********************************************************************/

/*master transmit function*/
int dw_i2c_master_transmit(struct dw_device *dev, uint8 *buffer,
			unsigned int length, dw_callback user_function)
{
	int retval;
	uint8 *tmp;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(buffer != NULL);
	DW_REQUIRE(length != 0);

	instance = (struct dw_i2c_instance *)dev->instance;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	DW_REQUIRE(instance->listener != NULL);

	/*critical section of code.shared data needs to be protected. */
	/*this macro disables d_w_apb_i2c interrupts. */
	I2C_ENTER_CRITICAL_SECTION();

	if (instance->state == i2c_state_idle) {
		/*master-transmitter */
		retval = 0;

		instance->state = i2c_state_master_tx;
		instance->tx_callback = user_function;
		instance->tx_length = length;
		instance->tx_remain = length;
		instance->tx_hold = 0;
		instance->tx_idx = 0;
		/*check for non word-aligned buffer as I2C_FIFO_WRITE() works*/
		/*efficiently on words reads from instance->tx_hold. */
		tmp = buffer;
		//isp_dbg_print_info("-> %s\n",__FUNCTION__);
		while (((((uint64) tmp) & 0x3) != 0x0)
			&& ((instance->tx_length - instance->tx_idx) > 0)) {
			instance->tx_hold |=
				((*tmp++ & 0xff) << (instance->tx_idx * 8));
			instance->tx_idx++;
		}
		//isp_dbg_print_info("<- %s\n",__FUNCTION__);
		instance->tx_buffer = tmp;

		/*ensure transfer is underway */
		instance->intr_mask_save |= (i2c_irq_tx_empty |
					i2c_irq_tx_abrt);
	} else
		retval = -DW_EBUSY;

	/*end of critical section of code. this macros restores d_w_apb_i2c */
	/*interrupts. */
	I2C_EXIT_CRITICAL_SECTION();

	return retval;
}

/**********************************************************************/

/*slave transmit function*/
int dw_i2c_slave_transmit(struct dw_device *dev, uint8 *buffer,
			unsigned int length, dw_callback user_function)
{
	uint8 *tmp;
	int retval;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(buffer != NULL);
	DW_REQUIRE(length != 0);

	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	DW_REQUIRE(instance->listener != NULL);

	/*critical section of code.shared data needs to be protected. */
	/*this macro disables d_w_apb_i2c interrupts. */
	I2C_ENTER_CRITICAL_SECTION();

	if ((instance->state == i2c_state_rd_req) ||
	(instance->state == i2c_state_slave_rx_rd_req)) {
		/*slave-transmitter */
		retval = 0;

		instance->tx_callback = user_function;
		instance->tx_length = length;
		instance->tx_remain = length;
		instance->tx_hold = 0;
		instance->tx_idx = 0;
		/*check for non word-aligned buffer as I2C_FIFO_WRITE() works*/
		/*efficiently on words reads from instance->tx_hold. */
		tmp = (uint8 *) buffer;
		//isp_dbg_print_info("-> %s\n",__FUNCTION__);

		while (((((uint64) tmp) & 0x3) != 0x0)
			&& ((instance->tx_length - instance->tx_idx) > 0)) {
			instance->tx_hold |=
				((*tmp++ & 0xff) << (instance->tx_idx * 8));
			instance->tx_idx++;
		}
		//isp_dbg_print_info("<- %s\n",__FUNCTION__);
		/*buffer is now word-aligned */
		instance->tx_buffer = tmp;
		/*write only one byte of data to the slave tx fifo */
		I2C_FIFO_WRITE(1);
		switch (instance->state) {
		case i2c_state_rd_req:
			instance->state = i2c_state_slave_tx;
			break;
		case i2c_state_slave_rx_rd_req:
			instance->state = i2c_state_slave_tx_rx;
			break;
		default:
			DW_ASSERT(false);
			break;
		}
		/*note: tx_empty is not enabled here as rd_req is the signal */
		/*used to write the next byte of data to the tx fifo. */
	} else
		retval = -DW_EPROTO;

	/*end of critical section of code. this macros restores d_w_apb_i2c */
	/*interrupts. */
	I2C_EXIT_CRITICAL_SECTION();

	return retval;
}

/**********************************************************************/

/*slave bulk transmit function*/
int dw_i2c_slave_bulk_transmit(struct dw_device *dev, uint8 *buffer,
			unsigned int length, dw_callback user_function)
{
	uint8 *tmp;
	int retval, max_bytes;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(buffer != NULL);
	DW_REQUIRE(length != 0);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	DW_REQUIRE(instance->listener != NULL);

	/*critical section of code.shared data needs to be protected. */
	/*this macro disables d_w_apb_i2c interrupts. */
	I2C_ENTER_CRITICAL_SECTION();

	if ((instance->state == i2c_state_rd_req) ||
	(instance->state == i2c_state_slave_rx_rd_req)) {
		/*slave-transmitter */
		retval = 0;

		instance->tx_callback = user_function;
		instance->tx_length = length;
		instance->tx_remain = length;
		instance->tx_hold = 0;
		instance->tx_idx = 0;
		/*check for non word-aligned buffer as I2C_FIFO_WRITE() works*/
		/*efficiently on words reads from instance->tx_hold. */
		tmp = (uint8 *) buffer;
		//isp_dbg_print_info("-> %s\n",__FUNCTION__);
		while (((((uint64) tmp) & 0x3) != 0x0)
			&& ((instance->tx_length - instance->tx_idx) > 0)) {
			instance->tx_hold |=
				((*tmp++ & 0xff) << (instance->tx_idx * 8));
			instance->tx_idx++;
		}
		//isp_dbg_print_info("<- %s\n",__FUNCTION__);
		/*buffer is now word-aligned */
		instance->tx_buffer = tmp;
		/*maximum available space in the tx fifo */
		max_bytes =
			param->tx_buffer_depth - dw_i2c_get_tx_fifo_level(dev);
		I2C_FIFO_WRITE(max_bytes);
		switch (instance->state) {
		case i2c_state_rd_req:
			instance->state = i2c_state_slave_bulk_tx;
			break;
		case i2c_state_slave_rx_rd_req:
			instance->state = i2c_state_slave_bulk_tx_rx;
			break;
		default:
			DW_ASSERT(false);
			break;
		}
		/*ensure transfer is underway */
		instance->intr_mask_save |= (i2c_irq_tx_empty |
					i2c_irq_tx_abrt);
	} else
		retval = -DW_EPROTO;

	/*end of critical section of code. this macros restores d_w_apb_i2c */
	/*interrupts. */
	I2C_EXIT_CRITICAL_SECTION();

	return retval;
}

/**********************************************************************/

int dw_i2c_master_receive(struct dw_device *dev, uint8 *buffer,
			unsigned int length, dw_callback user_function)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(buffer != NULL);
	DW_REQUIRE(length != 0);

	instance = (struct dw_i2c_instance *)dev->instance;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	DW_REQUIRE(instance->listener != NULL);

	/*critical section of code.shared data needs to be protected. */
	/*this macro disables d_w_apb_i2c interrupts. */
	I2C_ENTER_CRITICAL_SECTION();

	if (instance->state == i2c_state_idle) {
		retval = 0;
		instance->state = i2c_state_master_rx;
		/*check if rx buffer is not word-aligned */
		if (((uint64) buffer & 0x3) == 0)
			instance->rx_align = true;
		else
			instance->rx_align = false;

		/*set rx fifo threshold if necessary */
		if (length <= instance->rx_threshold) {
			reg = 0;
			DW_BIT_SET(reg, I2C_RX_TL_RX_TL, length - 1);
			I2C_OUTP(reg, portmap->rx_tl);
		}
		/*set transfer variables */
		instance->rx_buffer = buffer;
		instance->rx_length = length;
		instance->rx_remain = length;
		instance->tx_remain = length;
		instance->rx_callback = user_function;
		instance->rx_idx = 4;
		/*restore interrupts and ensure master-receive is underway */
		instance->intr_mask_save |= (i2c_irq_rx_full |
					i2c_irq_tx_empty |
					i2c_irq_tx_abrt);
	} else
		retval = -DW_EBUSY;

	/*end of critical section of code. this macros restores d_w_apb_i2c */
	/*interrupts. */
	I2C_EXIT_CRITICAL_SECTION();

	return retval;
}

/**********************************************************************/

int dw_i2c_slave_receive(struct dw_device *dev, uint8 *buffer, unsigned
			 length, dw_callback user_function)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(buffer != NULL);
	DW_REQUIRE(length != 0);

	instance = (struct dw_i2c_instance *)dev->instance;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	DW_REQUIRE(instance->listener != NULL);

	/*critical section of code.shared data needs to be protected. */
	/*this macro disables d_w_apb_i2c interrupts. */
	I2C_ENTER_CRITICAL_SECTION();

	if ((instance->state == i2c_state_rx_req)
		|| (instance->state == i2c_state_idle)
		|| (instance->state == i2c_state_slave_tx)
		|| (instance->state == i2c_state_slave_bulk_tx)
		|| (instance->state == i2c_state_slave_tx_rx_req)
		|| (instance->state == i2c_state_slave_bulk_tx_rx_req)
		|| (instance->state == i2c_state_master_tx_gen_call)) {
		retval = 0;
		/*in case the state was idle */
		switch (instance->state) {
		case i2c_state_idle:
		case i2c_state_rx_req:
			instance->state = i2c_state_slave_rx;
			break;
		case i2c_state_slave_tx:
		case i2c_state_slave_tx_rx_req:
			instance->state = i2c_state_slave_tx_rx;
			break;
		case i2c_state_slave_bulk_tx:
		case i2c_state_slave_bulk_tx_rx_req:
			instance->state = i2c_state_slave_bulk_tx_rx;
			break;
		case i2c_state_master_tx_gen_call:
			instance->state = i2c_state_master_tx_slave_rx;
			break;
		default:
			DW_ASSERT(false);
			break;
		}
		if (((uint64) buffer & 0x3) == 0)
			instance->rx_align = true;
		else
			instance->rx_align = false;

		/*set rx fifo threshold if necessary */
		if (length <= instance->rx_threshold) {
			reg = 0;
			DW_BIT_SET(reg, I2C_RX_TL_RX_TL, length - 1);
			I2C_OUTP(reg, portmap->rx_tl);
		}
		/*set transfer variables */
		instance->rx_buffer = buffer;
		instance->rx_length = length;
		instance->rx_remain = length;
		instance->rx_callback = user_function;
		instance->rx_idx = 4;

		/*ensure receive is underway */
		instance->intr_mask_save |= i2c_irq_rx_full;
	} else
		retval = -DW_EBUSY;

	/*end of critical section of code. this macros restores d_w_apb_i2c */
	/*interrupts. */
	I2C_EXIT_CRITICAL_SECTION();

	return retval;
}

/**********************************************************************/

int dw_i2c_terminate(struct dw_device *dev)
{
	uint32 reg;
	int retval, max_bytes;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	instance = (struct dw_i2c_instance *)dev->instance;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	DW_REQUIRE(instance->listener != NULL);

	/*critical section of code.shared data needs to be protected. */
	/*this macro disables d_w_apb_i2c interrupts. */
	I2C_ENTER_CRITICAL_SECTION();

	/*disable tx interrupt */
	if ((instance->state == i2c_state_master_tx)
		|| (instance->state == i2c_state_back2back)
		|| (instance->state == i2c_state_slave_tx)
		|| (instance->state == i2c_state_slave_tx_rx)
		|| (instance->state == i2c_state_slave_bulk_tx)
		|| (instance->state == i2c_state_slave_bulk_tx_rx)
		|| (instance->state == i2c_state_master_tx_slave_rx)) {
		/*ensure tx empty is not re-enabled when interrupts are */
		/*restored */
		instance->intr_mask_save &= ~i2c_irq_tx_empty;
	}
	/*flush rx fifo if necessary */
	if ((instance->state == i2c_state_master_rx)
		|| (instance->state == i2c_state_slave_rx)
		|| (instance->state == i2c_state_slave_tx_rx)
		|| (instance->state == i2c_state_slave_bulk_tx_rx)
		|| (instance->state == i2c_state_master_tx_slave_rx)
		|| (instance->state == i2c_state_back2back)) {
		max_bytes = dw_i2c_get_rx_fifo_level(dev);
		I2C_FIFO_READ(max_bytes);
		dw_i2c_flush_rx_hold(dev);
		/*number of bytes that were received during the last transfer*/
		retval = instance->rx_length - instance->rx_remain;
	} else {
		/*number of bytes that were sent during the last transfer */
		retval = instance->tx_length - instance->tx_remain;
	}
	/*sanity check .. retval should never be less than zero */
	DW_ASSERT(retval >= 0);

	/*terminate current transfer */
	instance->state = i2c_state_idle;
	instance->tx_callback = NULL;
	instance->tx_buffer = NULL;
	instance->rx_callback = NULL;
	instance->rx_buffer = NULL;

	/*restore user-specified tx/rx fifo threshold */
	reg = 0;
	DW_BIT_SET(reg, I2C_TX_TL_TX_TL, instance->tx_threshold);
	I2C_OUTP(reg, portmap->tx_tl);
	reg = 0;
	DW_BIT_SET(reg, I2C_RX_TL_RX_TL, instance->rx_threshold);
	I2C_OUTP(reg, portmap->rx_tl);

	/*end of critical section of code. this macros restores d_w_apb_i2c */
	/*interrupts. */
	I2C_EXIT_CRITICAL_SECTION();

	/*sanity check */
	DW_ASSERT(dw_i2c_is_irq_masked(dev, i2c_irq_tx_empty) == true);

	return retval;
}

/**********************************************************************/

void dw_i2c_unmask_irq(struct dw_device *dev, enum dw_i2c_irq interrupts)
{
	uint32 reg;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	reg = I2C_INP(portmap->intr_mask);
	/*avoid bus write if irq already enabled */
	if (((uint32) interrupts & reg) != (uint32) interrupts) {
		reg |= interrupts;
		/*save current value of interrupt mask register */
		instance->intr_mask_save = reg;
		I2C_OUTP(reg, portmap->intr_mask);
	}
}

/**********************************************************************/

void dw_i2c_mask_irq(struct dw_device *dev, enum dw_i2c_irq interrupts)
{
	uint32 reg;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	reg = I2C_INP(portmap->intr_mask);
	/*avoid bus write if interrupt(s) already disabled */
	if ((interrupts & reg) != 0) {
		reg &= ~interrupts;
		/*save current value of interrupt mask register */
		instance->intr_mask_save = reg;
		I2C_OUTP(reg, portmap->intr_mask);
	}
}

/**********************************************************************/

void dw_i2c_clear_irq(struct dw_device *dev, enum dw_i2c_irq interrupts)
{
	volatile uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (interrupts == i2c_irq_all)
		reg = I2C_INP(portmap->clr_intr);
	else {
		if ((interrupts & i2c_irq_rx_under) != 0)
			reg = I2C_INP(portmap->clr_rx_under);
		if ((interrupts & i2c_irq_rx_over) != 0)
			reg = I2C_INP(portmap->clr_rx_over);
		if ((interrupts & i2c_irq_tx_over) != 0)
			reg = I2C_INP(portmap->clr_tx_over);
		if ((interrupts & i2c_irq_rd_req) != 0)
			reg = I2C_INP(portmap->clr_rd_req);
		if ((interrupts & i2c_irq_tx_abrt) != 0)
			reg = I2C_INP(portmap->clr_tx_abrt);
		if ((interrupts & i2c_irq_rx_done) != 0)
			reg = I2C_INP(portmap->clr_rx_done);
		if ((interrupts & i2c_irq_activity) != 0)
			reg = I2C_INP(portmap->clr_activity);
		if ((interrupts & i2c_irq_stop_det) != 0)
			reg = I2C_INP(portmap->clr_stop_det);
		if ((interrupts & i2c_irq_start_det) != 0)
			reg = I2C_INP(portmap->clr_start_det);
		if ((interrupts & i2c_irq_gen_call) != 0)
			reg = I2C_INP(portmap->clr_gen_call);
	}
}

/**********************************************************************/

bool dw_i2c_is_irq_masked(struct dw_device *dev, enum dw_i2c_irq interrupt)
{
	int retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE((interrupt == i2c_irq_rx_under)
		|| (interrupt == i2c_irq_rx_over)
		|| (interrupt == i2c_irq_rx_full)
		|| (interrupt == i2c_irq_tx_over)
		|| (interrupt == i2c_irq_tx_empty)
		|| (interrupt == i2c_irq_rd_req)
		|| (interrupt == i2c_irq_tx_abrt)
		|| (interrupt == i2c_irq_rx_done)
		|| (interrupt == i2c_irq_activity)
		|| (interrupt == i2c_irq_stop_det)
		|| (interrupt == i2c_irq_start_det)
		|| (interrupt == i2c_irq_gen_call));

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->intr_mask);
	reg &= interrupt;

	if (reg == 0)
		retval = true;
	else
		retval = false;

	return (bool) retval;
}

/**********************************************************************/

uint32 dw_i2c_get_irq_mask(struct dw_device *dev)
{
	uint32 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	retval = I2C_INP(portmap->intr_mask);

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_irq_active(struct dw_device *dev, enum dw_i2c_irq interrupt)
{
	bool retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE((interrupt == i2c_irq_rx_under)
		|| (interrupt == i2c_irq_rx_over)
		|| (interrupt == i2c_irq_rx_full)
		|| (interrupt == i2c_irq_tx_over)
		|| (interrupt == i2c_irq_tx_empty)
		|| (interrupt == i2c_irq_rd_req)
		|| (interrupt == i2c_irq_tx_abrt)
		|| (interrupt == i2c_irq_rx_done)
		|| (interrupt == i2c_irq_activity)
		|| (interrupt == i2c_irq_stop_det)
		|| (interrupt == i2c_irq_start_det)
		|| (interrupt == i2c_irq_gen_call));

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->intr_stat);
	reg &= interrupt;

	if (reg == 0)
		retval = false;
	else
		retval = true;

	return retval;
}

/**********************************************************************/

bool dw_i2c_is_raw_irq_active(struct dw_device *dev, enum dw_i2c_irq interrupt)
{
	bool retval;
	uint32 reg;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE((interrupt == i2c_irq_rx_under)
		|| (interrupt == i2c_irq_rx_over)
		|| (interrupt == i2c_irq_rx_full)
		|| (interrupt == i2c_irq_tx_over)
		|| (interrupt == i2c_irq_tx_empty)
		|| (interrupt == i2c_irq_rd_req)
		|| (interrupt == i2c_irq_tx_abrt)
		|| (interrupt == i2c_irq_rx_done)
		|| (interrupt == i2c_irq_activity)
		|| (interrupt == i2c_irq_stop_det)
		|| (interrupt == i2c_irq_start_det)
		|| (interrupt == i2c_irq_gen_call));

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->raw_intr_stat);
	reg &= interrupt;

	if (reg == 0)
		retval = false;
	else
		retval = true;

	return retval;
}

/**********************************************************************/

int dw_i2c_set_dma_tx_mode(struct dw_device *dev, enum dw_dma_mode mode)
{
	int retval;
	uint32 reg;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	if (param->has_dma == true) {
		retval = 0;
		if (instance->dma_tx.mode != mode) {
			reg = I2C_INP(portmap->dma_cr);
			instance->dma_tx.mode = mode;
			if (mode == dw_dma_hw_handshake)
				DW_BIT_SET(reg, I2C_DMA_CR_TDMAE, 0x1);
			else
				DW_BIT_SET(reg, I2C_DMA_CR_TDMAE, 0x0);
			I2C_OUTP(reg, portmap->dma_cr);
		}
	} else
		retval = -DW_ENOSYS;

	return retval;
}

/**********************************************************************/

enum dw_dma_mode dw_i2c_get_dma_tx_mode(struct dw_device *dev)
{
	enum dw_dma_mode retval;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	instance = (struct dw_i2c_instance *)dev->instance;

	retval = instance->dma_tx.mode;

	return retval;
}

/**********************************************************************/

int dw_i2c_set_dma_rx_mode(struct dw_device *dev, enum dw_dma_mode mode)
{
	int retval;
	uint32 reg;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	if (param->has_dma == true) {
		retval = 0;
		if (instance->dma_rx.mode != mode) {
			reg = I2C_INP(portmap->dma_cr);
			instance->dma_rx.mode = mode;
			if (mode == dw_dma_hw_handshake) {
				DW_BIT_SET(reg, I2C_DMA_CR_RDMAE, 0x1);
				/*mask rx full interrupt */
				dw_i2c_mask_irq(dev, i2c_irq_rx_full);
			} else {
				DW_BIT_SET(reg, I2C_DMA_CR_RDMAE, 0x0);
				/*unmask rx full interrupt */
				dw_i2c_unmask_irq(dev, i2c_irq_rx_full);
			}
			I2C_OUTP(reg, portmap->dma_cr);
		}
	} else
		retval = -DW_ENOSYS;

	return retval;
}

/**********************************************************************/

enum dw_dma_mode dw_i2c_get_dma_rx_mode(struct dw_device *dev)
{
	enum dw_dma_mode retval;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	instance = (struct dw_i2c_instance *)dev->instance;

	retval = instance->dma_rx.mode;

	return retval;
}

/**********************************************************************/

int dw_i2c_set_dma_tx_level(struct dw_device *dev, uint8 level)
{
	int retval;
	uint32 reg;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (param->has_dma == true) {
		reg = retval = 0;
		DW_BIT_SET(reg, I2C_DMA_TDLR_DMATDL, level);
		I2C_OUTP(reg, portmap->dma_tdlr);
	} else
		retval = -DW_ENOSYS;

	return retval;
}

/**********************************************************************/

uint8 dw_i2c_get_dma_tx_level(struct dw_device *dev)
{
	uint32 reg;
	uint32 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->dma_tdlr);
	retval = DW_BIT_GET(reg, I2C_DMA_TDLR_DMATDL);

	return (uint8) retval;
}

/**********************************************************************/

int dw_i2c_set_dma_rx_level(struct dw_device *dev, uint8 level)
{
	int retval;
	uint32 reg;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	if (param->has_dma == true) {
		retval = 0;
		reg = 0;
		DW_BIT_SET(reg, I2C_DMA_RDLR_DMARDL, level);
		I2C_OUTP(reg, portmap->dma_rdlr);
	} else
		retval = -DW_ENOSYS;

	return retval;
}

/**********************************************************************/

uint8 dw_i2c_get_dma_rx_level(struct dw_device *dev)
{
	uint32 reg;
	uint32 retval;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;

	reg = I2C_INP(portmap->dma_rdlr);
	retval = DW_BIT_GET(reg, I2C_DMA_RDLR_DMARDL);

	return (uint8) retval;
}

/**********************************************************************/

int dw_i2c_set_notifier_destination_ready(struct dw_device *dev,
				dw_dma_notifier_func funcptr,
				struct dw_device *dmac,
				unsigned int channel)
{
	int retval;
	struct dw_i2c_param *param;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(funcptr != NULL);
	DW_REQUIRE(dmac != NULL);
	DW_REQUIRE(dmac->comp_type == dw_ahb_dmac);

	param = (struct dw_i2c_param *)dev->comp_param;
	instance = (struct dw_i2c_instance *)dev->instance;

	if (param->has_dma == true) {
		retval = 0;
		instance->dma_tx.notifier = funcptr;
		instance->dma_tx.dmac = dmac;
		instance->dma_tx.channel = channel;
	} else
		retval = -DW_ENOSYS;

	return retval;
}

/**********************************************************************/

int dw_i2c_set_notifier_source_ready(struct dw_device *dev,
			dw_dma_notifier_func funcptr,
			struct dw_device *dmac, unsigned int channel)
{
	int retval;
	struct dw_i2c_param *param;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);
	DW_REQUIRE(funcptr != NULL);
	DW_REQUIRE(dmac != NULL);
	DW_REQUIRE(dmac->comp_type == dw_ahb_dmac);

	param = (struct dw_i2c_param *)dev->comp_param;
	instance = (struct dw_i2c_instance *)dev->instance;

	if (param->has_dma == true) {
		retval = 0;
		instance->dma_rx.notifier = funcptr;
		instance->dma_rx.dmac = dmac;
		instance->dma_rx.channel = channel;
	} else
		retval = -DW_ENOSYS;

	return retval;
}

/**********************************************************************/
static void i2c_fifo_write16(struct dw_i2c_instance *instance,
		struct dw_i2c_portmap *portmap, int max)
{
	int i;
	unsigned short val;
	unsigned int *ptr;

	ptr = (unsigned int *)instance->b2b_buffer;
		for (i = 0; i < (max); i++) {
			switch (instance->tx_idx) {
			case 0:
				instance->tx_hold = *(ptr++);
				instance->tx_idx = 4;
				DW_OUT16P((instance->tx_hold & 0xffff),
				portmap->data_cmd);
				instance->tx_hold >>= 16;
				instance->tx_idx -= 2;
				break;
			case 2:
				DW_OUT16P((instance->tx_hold & 0xffff),
				portmap->data_cmd);
				instance->tx_hold >>= 16;
				instance->tx_idx -= 2;
				break;
			case 4:
				DW_OUT16P((instance->tx_hold & 0xffff),
				portmap->data_cmd);
				instance->tx_hold >>= 16;
				instance->tx_idx -= 2;
				break;
			case 1:
				val = instance->tx_hold & 0xff;
				instance->tx_hold = *(ptr++);
				val |= ((instance->tx_hold & 0xff) << 8);
				DW_OUT16P(val, portmap->data_cmd);
				instance->tx_hold >>= 8;
				instance->tx_idx = 3;
				break;
			case 3:
				DW_OUT16P((instance->tx_hold & 0xffff),
				portmap->data_cmd);
				instance->tx_hold >>= 16;
				instance->tx_idx -= 2;
				break;
			}
		}
	instance->b2b_buffer = (unsigned short *)ptr;
}

int dw_i2c_irq_handler(struct dw_device *dev)
{
	int retval;
	uint8 *tmp;
	uint32 reg;
	int ii, max_bytes;
	int32 callback_arg;
	dw_callback user_callback;
	enum dw_i2c_irq clear_irq_mask;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	user_callback = NULL;
	callback_arg = 0;
	clear_irq_mask = i2c_irq_none;
	/*assume an interrupt will be processed.  this will be set to false */
	/*if no active interrupt is found. */
	retval = true;

	/*what caused the interrupt? */
	reg = I2C_INP(portmap->intr_stat);

	/*if an error has occurred */
	if ((reg & (i2c_irq_tx_abrt | i2c_irq_rx_over | i2c_irq_rx_under |
		i2c_irq_tx_over)) != 0) {
		instance->state = i2c_state_error;
		user_callback = instance->listener;
		dw_i2c_mask_irq(dev, i2c_irq_tx_empty);
		/*if a tx transfer was aborted */
		if ((reg & i2c_irq_tx_abrt) != 0) {
			callback_arg = i2c_irq_tx_abrt;
			clear_irq_mask = i2c_irq_tx_abrt;
		}
		/*rx fifo overflow */
		else if ((reg & i2c_irq_rx_over) != 0) {
			callback_arg = i2c_irq_rx_over;
			clear_irq_mask = i2c_irq_rx_over;
		}
		/*rx fifo underflow */
		else if ((reg & i2c_irq_rx_under) != 0) {
			callback_arg = i2c_irq_rx_under;
			clear_irq_mask = i2c_irq_rx_under;
		}
		/*tx fifo overflow */
		else if ((reg & i2c_irq_tx_over) != 0) {
			callback_arg = i2c_irq_tx_over;
			clear_irq_mask = i2c_irq_tx_over;
		}
	}
	/*a general call was detected */
	else if ((reg & i2c_irq_gen_call) != 0) {
		user_callback = instance->listener;
		callback_arg = i2c_irq_gen_call;
		clear_irq_mask = i2c_irq_gen_call;
		/*update state -- awaiting user to start a slave rx transfer */
		switch (instance->state) {
		case i2c_state_idle:
			instance->state = i2c_state_rx_req;
			break;
		case i2c_state_master_tx:
			instance->state = i2c_state_master_tx_gen_call;
			break;
		case i2c_state_slave_rx:
			//leave state unchanged; user already has an rx buffer
			//set up to receive data.
			break;
		default:
			/*should never reach this clause */
			DW_ASSERT(false);
			break;
		}
	}
	/*rx fifo level at or above threshold */
	else if ((reg & i2c_irq_rx_full) != 0) {
		/*the rx full interrupt should not be unmasked when a DMA
		 *interface with hardware handshaking is being used.
		 */
		DW_REQUIRE(instance->dma_rx.mode != dw_dma_hw_handshake);
		if (instance->dma_rx.mode == dw_dma_sw_handshake) {
			//the user must have previously set an rx notifier via
			//dw_i2c_set_dma_rx_notifier.
			DW_REQUIRE(instance->dma_rx.notifier != NULL);
			/*disable the rx full interrupt .. this is re-enabled
			 *after the DMA has finished the current transfer
			 *(via a callback set in the DMA driver by the user).
			 */
			dw_i2c_mask_irq(dev, i2c_irq_rx_full);
			/*notify the DMA that the rx FIFO has data to be read.
			 *this function and its arguments are set by the user
			 *via the dw_i2c_set_notifier_source_ready() function.
			 */
			(instance->dma_rx.notifier) (instance->dma_rx.dmac,
						instance->dma_rx.channel,
						false, false);
		} else {
			if ((instance->state == i2c_state_idle) ||
				(instance->state == i2c_state_slave_tx) ||
				(instance->state == i2c_state_slave_bulk_tx)) {
				/*sanity check: rx_buffer should be NULL in
				 *these states
				 */
				DW_ASSERT(instance->rx_buffer == NULL);
				/*inform the user listener function of
				 *the event
				 */
				user_callback = instance->listener;
				callback_arg = i2c_irq_rx_full;
				switch (instance->state) {
				case i2c_state_idle:
					instance->state = i2c_state_rx_req;
					break;
				case i2c_state_slave_tx:
					instance->state =
						i2c_state_slave_tx_rx_req;
					break;
				case i2c_state_slave_bulk_tx:
					instance->state =
						i2c_state_slave_bulk_tx_rx_req;
					break;
				default:
					break;
				}
			} else {
				DW_ASSERT(instance->rx_buffer != NULL);
				/*does the rx buffer need to be word-aligned? */
				if (instance->rx_align == false) {
					/*align buffer: */
					tmp = (uint8 *) instance->rx_buffer;
					/*repeat until either the buffer is
					 *aligned, there is no more space in
					 *the rx buffer or there is no
					 *more data to read from the rx fifo
					 */
					while ((((uint64) tmp) & 0x3) &&
					(instance->rx_remain > 0) &&
					(dw_i2c_is_rx_fifo_empty(dev) ==
					false)) {
						*tmp++ =
							(uint8) (DW_IN16P
							(portmap->data_cmd));
						instance->rx_remain--;
					}

					instance->rx_buffer = tmp;
					if (((uint64) tmp & 0x3) == 0)
						instance->rx_align = true;
				}

				/*instance->rx_align == false */
				/*this code is only executed when the rx buffer
				 *is word-aligned as I2C_FIFO_READ works
				 *efficiently with a word-aligned buffer.
				 */
				if (instance->rx_align == true) {
					max_bytes =
						dw_i2c_get_rx_fifo_level(dev);
					I2C_FIFO_READ(max_bytes);
				}
				/*instance->rx_align == true */
				/*if the rx buffer is full */
				if (instance->rx_remain == 0) {
				/*prepare to call the user callback function to
				 *notify it that the current transfer has
				 *finished. for an rx or back-to-back transfer,
				 *the number of bytes received is passed as
				 *an argument to the listener function.
				 */
					user_callback = instance->rx_callback;
					callback_arg = instance->rx_length;
					/*flush the instance->rx_hold register
					 *to the rx buffer.
					 */
					dw_i2c_flush_rx_hold(dev);
					/*transfer complete */
					instance->rx_buffer = NULL;
					instance->rx_callback = NULL;
					/*restore rx threshold to
					 *user-specified value.
					 */
					I2C_OUTP(instance->rx_threshold,
						 portmap->rx_tl);
					/*update state */
					switch (instance->state) {
					case i2c_state_master_rx:
					/*end of master-receiver transfer.
					 *ensure that the tx empty interrupt
					 *is disabled.
					 */
						dw_i2c_mask_irq(dev,
							i2c_irq_tx_empty);
						instance->state =
							i2c_state_idle;
						break;
					case i2c_state_slave_rx:
						/*return to idle state */
						instance->state =
							i2c_state_idle;
						break;
					case i2c_state_back2back:
					/* back-to-back transfer is complete if
					 * there is no more data to send.else,
					 * the callback function is not called
					 * until all bytes have been transmitted
					 * note that tx_callback is cleared when
					 * all bytes have been sent
					 * and/or received.
					 */
						if (instance->tx_callback
						    == NULL) {
							instance->state =
								i2c_state_idle;
							DW_ASSERT
							(dw_i2c_is_irq_masked
							(dev,
							i2c_irq_tx_empty)
							== true);
						} else
							user_callback = NULL;
						break;
					case i2c_state_slave_tx_rx:
						instance->state =
							i2c_state_slave_tx;
						break;
					case i2c_state_slave_bulk_tx_rx:
						instance->state =
						i2c_state_slave_bulk_tx;
						break;
					case i2c_state_master_tx_slave_rx:
						instance->state =
							i2c_state_master_tx;
						break;
					default:
					/*this clause should never be reached */
						DW_ASSERT(false);
						break;
					}
				} /*remain == 0 */
				else if (instance->rx_remain < ((unsigned int)
					instance->rx_threshold + 1)) {
					reg = 0;
					DW_BIT_SET(reg, I2C_RX_TL_RX_TL,
						(instance->rx_remain - 1));
					I2C_OUTP(reg, portmap->rx_tl);
				}
			}	/*instance->rx_buffer != NULL */
		}
	}
	/*(reg & i2c_irq_rx_full) != 0 */
	/*read-request transfer completed (tx fifo may still contain data) */
	else if ((reg & i2c_irq_rx_done) != 0) {
		clear_irq_mask = i2c_irq_rx_done;
		switch (instance->state) {
		case i2c_state_slave_tx:
			/*return to idle state if tx transfer finished */
			if (instance->tx_remain == 0) {
				instance->state = i2c_state_idle;
				DW_ASSERT(dw_i2c_is_irq_masked(dev,
					i2c_irq_tx_empty) == true);
				callback_arg = 0;
				/*call user tx callback function */
				user_callback = instance->tx_callback;
			/*clear tx buffer and callback function pointers */
				instance->tx_buffer = NULL;
				instance->tx_callback = NULL;
			}
			break;
		case i2c_state_slave_tx_rx:
			/*return to slave-rx state if tx transfer finished */
			if (instance->tx_remain == 0) {
				instance->state = i2c_state_slave_rx;
				callback_arg = 0;
				/*call user tx callback function */
				user_callback = instance->tx_callback;
			/*clear tx buffer and callback function pointers */
				instance->tx_buffer = NULL;
				instance->tx_callback = NULL;
			}
			break;
		case i2c_state_slave_bulk_tx:
			/*mask tx empty interrupt */
			dw_i2c_mask_irq(dev, i2c_irq_tx_empty);
			/*return to idle state */
			instance->state = i2c_state_idle;
			/*call user tx callback function */
			user_callback = instance->tx_callback;
			/*number of bytes left unsent */
			callback_arg = instance->tx_remain +
				dw_i2c_get_tx_fifo_level(dev);
			/*clear tx buffer and callback function pointers */
			instance->tx_buffer = NULL;
			instance->tx_callback = NULL;
			break;
		case i2c_state_slave_bulk_tx_rx:
			/*mask tx empty interrupt */
			dw_i2c_mask_irq(dev, i2c_irq_tx_empty);
			/*return to slave rx state */
			instance->state = i2c_state_slave_rx;
			/*call user tx callback function */
			user_callback = instance->tx_callback;
			/*number of bytes left unsent */
			callback_arg = instance->tx_remain +
				dw_i2c_get_tx_fifo_level(dev);
			/*clear tx buffer and callback function pointers */
			instance->tx_buffer = NULL;
			instance->tx_callback = NULL;
			break;
		default:
			/*should not get rx_done in any other driver state */
			DW_ASSERT(false);
			break;
		}
	}
	/*read request received */
	else if ((reg & i2c_irq_rd_req) != 0) {
		switch (instance->state) {
		case i2c_state_idle:
			clear_irq_mask = i2c_irq_rd_req;
			instance->state = i2c_state_rd_req;
			user_callback = instance->listener;
			callback_arg = i2c_irq_rd_req;
			break;
		case i2c_state_slave_rx:
			clear_irq_mask = i2c_irq_rd_req;
			instance->state = i2c_state_slave_rx_rd_req;
			user_callback = instance->listener;
			callback_arg = i2c_irq_rd_req;
			break;
		case i2c_state_slave_tx:
		case i2c_state_slave_tx_rx:
			clear_irq_mask = i2c_irq_rd_req;
			/*remain in the current state and write the next byte
			 *from the tx buffer to the tx fifo.
			 */
			I2C_FIFO_WRITE(1);
			break;
		case i2c_state_slave_bulk_tx_rx:
			/*A read request has occurred because, even though we
			 *are performing a slave bulk transfer, the system did
			 *not keep the tx FIFO from emptying.  this interrupt
			 *is therefore treated the same as a tx empty
			 *interrupt.
			 */
			max_bytes = MIN((uint32) param->tx_buffer_depth -
					(uint32)
					dw_i2c_get_tx_fifo_level(dev),
					instance->tx_remain);
			/*buffer should be word-aligned (done by */
			/*dw_i2c_slave_bulk_transmit) */
			if (instance->tx_remain > 0) {
				clear_irq_mask = i2c_irq_rd_req;
				I2C_FIFO_WRITE(max_bytes);
			} else {
				/*tx buffer has all been sent in bulk mode yet
				 *the master is still requesting more data.
				 *we need to call the tx callback function
				 *first and then pass read request to the user
				 *listener. update state
				 */
				instance->state = i2c_state_slave_rx;
				/*mask tx empty interrupt */
				dw_i2c_mask_irq(dev, i2c_irq_tx_empty);
				/*call user callback function with no bytes
				 * left to send.
				 */
				user_callback = instance->tx_callback;
				callback_arg = 0;
				instance->tx_buffer = NULL;
				instance->tx_callback = NULL;
			}
			break;
		case i2c_state_slave_bulk_tx:
			/*A read request has occurred because, even though we
			 *are performing a slave bulk transfer, the system did
			 *not keep the tx FIFO from emptying.  this interrupt
			 *is therefore treated the same as a tx empty
			 *interrupt.
			 */
			max_bytes = MIN((uint32) param->tx_buffer_depth -
					(uint32)
					dw_i2c_get_tx_fifo_level(dev),
					instance->tx_remain);
			/*buffer should be word-aligned (done by */
			/*dw_i2c_slave_bulk_transmit) */
			if (instance->tx_remain > 0) {
				clear_irq_mask = i2c_irq_rd_req;
				I2C_FIFO_WRITE(max_bytes);
			} else {
				/*tx buffer has all been sent in bulk mode yet
				 *the master is still requesting more data.
				 *we need to call the tx callback function
				 *first and then pass read request to the user
				 *listener. update state.
				 */
				instance->state = i2c_state_idle;
				/*mask tx empty interrupt */
				dw_i2c_mask_irq(dev, i2c_irq_tx_empty);
				DW_ASSERT(dw_i2c_is_irq_masked(dev,
					i2c_irq_tx_empty) == true);
				/*call user callback function with no bytes
				 *left to send
				 */
				user_callback = instance->tx_callback;
				callback_arg = 0;
				instance->tx_buffer = NULL;
				instance->tx_callback = NULL;
			}
			break;
		default:
			/*should not get rd_req in any other driver state */
			DW_ASSERT(false);
			break;
		}
	}
	/*tx fifo level at or below threshold */
	else if ((reg & i2c_irq_tx_empty) != 0) {
		/*the tx empty interrupt should never be unmasked when we are
		 *using DMA with hardware handshaking.
		 */
		DW_REQUIRE(instance->dma_tx.mode != dw_dma_hw_handshake);
		if (instance->dma_tx.mode == dw_dma_sw_handshake) {
			/*the user must have previously set a tx notifier. */
			DW_REQUIRE(instance->dma_tx.notifier != NULL);
			/*disable the tx empty interrupt . this is re-enabled
			 *after the DMA has finished the current transfer
			 *(via acallback set by the user).
			 */
			dw_i2c_mask_irq(dev, i2c_irq_tx_empty);
			/*notify the dma that the tx fifo is ready to receive
			 *mode data.this function and its arguments are set by
			 *the user via the
			 *dw_i2c_set_notifier_destination_ready() function.
			 */
			(instance->dma_tx.notifier) (instance->dma_tx.dmac,
				instance->dma_tx.channel, false, false);
		} else if (instance->tx_remain == 0) {
			/*default: call callback function with zero as
			 * argument (no bytes left to send)
			 *tx callback function.
			 */
			user_callback = instance->tx_callback;
			/*number of bytes left to transmit */
			callback_arg = 0;
			dw_i2c_mask_irq(dev, i2c_irq_tx_empty);
			/*restore user-specfied tx threshold value */
			I2C_OUTP(instance->tx_threshold, portmap->tx_tl);
			/*update driver state */
			switch (instance->state) {
			case i2c_state_master_tx:
				/*return to idle state at end of tx transfer */
				instance->state = i2c_state_idle;
				DW_ASSERT(dw_i2c_is_irq_masked(dev,
						i2c_irq_tx_empty)  == true);
				/*transfer complete */
				instance->tx_buffer = NULL;
				instance->tx_callback = NULL;
				break;
			case i2c_state_master_tx_slave_rx:
				/*return to slave-rx state if slave rx transfer
				 *is still in progress.
				 */
				instance->state = i2c_state_slave_rx;
				/*transfer complete */
				instance->tx_buffer = NULL;
				instance->tx_callback = NULL;
				break;
			case i2c_state_master_rx:
			/*reset tx buffer and callback function pointers */
				instance->tx_buffer = NULL;
				instance->tx_callback = NULL;
				/*for a master-rx transfer, the callback is
				 *not called until the last byte has been
				 *received.
				 */
				user_callback = NULL;
				callback_arg = 0;
				break;
			case i2c_state_slave_bulk_tx:
			case i2c_state_slave_bulk_tx_rx:
				/*for slave bulk transfers, the callback
				 *is not called until rx_done has been
				 *received.
				 */
				user_callback = NULL;
				callback_arg = 0;
				break;
			case i2c_state_back2back:
				if (instance->rx_callback == NULL) {
					/*if there is no more data to receive
					 *either,return to the idle state and
					 *call callbackwith the number of bytes
					 *received.
					 */
					instance->state = i2c_state_idle;
					DW_ASSERT(dw_i2c_is_irq_masked(dev,
						i2c_irq_tx_empty)  == true);
					callback_arg = instance->rx_length;
				} else {
					/*otherwise, if there is still data
					 *toreceive, do not call the user
					 *callback function.
					 */
					user_callback = NULL;
					callback_arg = 0;
				}
				/*reset tx buffer and callback
				 *function pointers.
				 */
				instance->tx_buffer = NULL;
				instance->tx_callback = NULL;
				break;
			default:
				/*we shouldn't get a tx_empty interrupt in any
				 *other driver state.
				 */
				DW_ASSERT(false);
				break;
			}
		} else {
			switch (instance->state) {
			case i2c_state_master_tx:
			case i2c_state_slave_bulk_tx:
			case i2c_state_slave_bulk_tx_rx:
			case i2c_state_master_tx_slave_rx:
				/*slave-transmitter or master-transmitter */
				max_bytes = param->tx_buffer_depth -
					dw_i2c_get_tx_fifo_level(dev);
				/*buffer should already be word-aligned */
				I2C_FIFO_WRITE(max_bytes);
				break;
			case i2c_state_master_rx:
				max_bytes =
					MIN(((uint32) param->tx_buffer_depth -
					(uint32)
					dw_i2c_get_tx_fifo_level(dev)),
					instance->tx_remain);
				for (ii = 0; ii < max_bytes; ii++)
					DW_OUT16P(0x100, portmap->data_cmd);
				instance->tx_remain -= max_bytes;
				break;
			case i2c_state_back2back:
				max_bytes =
					MIN(((uint32) param->tx_buffer_depth -
					(uint32)
					dw_i2c_get_tx_fifo_level(dev)),
					instance->tx_remain);
				i2c_fifo_write16(instance, portmap,
					max_bytes);
				instance->tx_remain -= max_bytes;
				break;
			default:
				/*we shouldn't get a tx_empty interrupt in any
				 *other driver state.
				 */
				DW_ASSERT(false);
				break;
			}
			/*if the tx buffer is empty, set the tx threshold to no
			 *bytes in fifo.  this is to ensure the the tx callback
			 *function, if any, is only called when the current
			 *transfer has been completed by the d_w_apb_i2c device
			 */
			if (instance->tx_remain == 0)
				I2C_OUTP(0x0, portmap->tx_tl);
		}
	}
	/*start condition detected */
	else if ((reg & i2c_irq_start_det) != 0) {
		user_callback = instance->listener;
		callback_arg = i2c_irq_start_det;
		clear_irq_mask = i2c_irq_start_det;
	}
	/*stop condition detected */
	else if ((reg & i2c_irq_stop_det) != 0) {
		user_callback = instance->listener;
		callback_arg = i2c_irq_stop_det;
		clear_irq_mask = i2c_irq_stop_det;
	}
	/*i2c bus activity */
	else if ((reg & i2c_irq_activity) != 0) {
		user_callback = instance->listener;
		callback_arg = i2c_irq_activity;
		clear_irq_mask = i2c_irq_activity;
	} else {
		/*if we've reached this point, either the enabling and
		 *disabling of I2C interrupts is not being handled properly or
		 *this function is being called unnecessarily.
		 */
		retval = false;
	}

	/*call the user listener function, if it has been set */
	if (user_callback != NULL)
		user_callback(dev, callback_arg);

	/*if the driver is still in one of these states, the user listener */
	/*function has not correctly handled a device event/interrupt. */
	DW_REQUIRE(instance->state != i2c_state_rx_req);
	DW_REQUIRE(instance->state != i2c_state_rd_req);
	DW_REQUIRE(instance->state != i2c_state_slave_tx_rx_req);
	DW_REQUIRE(instance->state != i2c_state_slave_rx_rd_req);
	DW_REQUIRE(instance->state != i2c_state_master_tx_gen_call);
	DW_REQUIRE(instance->state != i2c_state_error);

	/*clear the serviced interrupt */
	if (clear_irq_mask != 0)
		dw_i2c_clear_irq(dev, clear_irq_mask);

	return retval;
}

/**********************************************************************/

int dw_i2c_user_irq_handler(struct dw_device *dev)
{
	bool retval;
	uint32 reg;
	int32 callback_arg;
	dw_callback user_callback;
	enum dw_i2c_irq clear_irq_mask;
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	/*assume an interrupt will be processed.user_callback will be set */
	/*to NULL and retval to false if no active interrupt is found. */
	retval = true;
	user_callback = instance->listener;
	callback_arg = 0;
	clear_irq_mask = i2c_irq_none;

	/*what caused the interrupt? */
	reg = I2C_INP(portmap->intr_stat);

	/*if a tx transfer was aborted */
	if ((reg & i2c_irq_tx_abrt) != 0) {
		callback_arg = i2c_irq_tx_abrt;
		clear_irq_mask = i2c_irq_tx_abrt;
	}
	/*rx fifo overflow */
	else if ((reg & i2c_irq_rx_over) != 0) {
		callback_arg = i2c_irq_rx_over;
		clear_irq_mask = i2c_irq_rx_over;
	}
	/*rx fifo underflow */
	else if ((reg & i2c_irq_rx_under) != 0) {
		callback_arg = i2c_irq_rx_under;
		clear_irq_mask = i2c_irq_rx_under;
	}
	/*tx fifo overflow */
	else if ((reg & i2c_irq_tx_over) != 0) {
		callback_arg = i2c_irq_tx_over;
		clear_irq_mask = i2c_irq_tx_over;
	}
	/*a general call was detected */
	else if ((reg & i2c_irq_gen_call) != 0) {
		callback_arg = i2c_irq_gen_call;
		clear_irq_mask = i2c_irq_gen_call;
	}
	/*rx fifo level at or above threshold */
	else if ((reg & i2c_irq_rx_full) != 0)
		callback_arg = i2c_irq_rx_full;
	/*read-request transfer completed (tx FIFO may still contain data) */
	else if ((reg & i2c_irq_rx_done) != 0) {
		callback_arg = i2c_irq_rx_done;
		clear_irq_mask = i2c_irq_rx_done;
	}
	/*read request received */
	else if ((reg & i2c_irq_rd_req) != 0) {
		callback_arg = i2c_irq_rd_req;
		clear_irq_mask = i2c_irq_rd_req;
	}
	/*tx fifo level at or below threshold */
	else if ((reg & i2c_irq_tx_empty) != 0)
		callback_arg = i2c_irq_tx_empty;
	/*start condition detected */
	else if ((reg & i2c_irq_start_det) != 0) {
		callback_arg = i2c_irq_start_det;
		clear_irq_mask = i2c_irq_start_det;
	}
	/*stop condition detected */
	else if ((reg & i2c_irq_stop_det) != 0) {
		callback_arg = i2c_irq_stop_det;
		clear_irq_mask = i2c_irq_stop_det;
	}
	/*i2c bus activity */
	else if ((reg & i2c_irq_activity) != 0) {
		callback_arg = i2c_irq_activity;
		clear_irq_mask = i2c_irq_activity;
	} else {
		/*no active interrupt was found */
		retval = false;
		user_callback = NULL;
	}

	/*call the user listener function, if there was an active interrupt */
	if (user_callback != NULL)
		user_callback(dev, callback_arg);

	/*clear any serviced interrupt */
	if (clear_irq_mask != 0)
		dw_i2c_clear_irq(dev, clear_irq_mask);

	return retval;
}

/**********************************************************************/
/***PRIVATE FUNCTIONS***/
/**********************************************************************/

/***
 *the following functions are all private and as such are not part of
 *the driver's public API.
 ***/

int dw_i2c_flush_rx_hold(struct dw_device *dev)
{
	int i, retval;
	uint8 *tmp;
	uint32 c, mask;
	uint32 *buf;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	instance = (struct dw_i2c_instance *)dev->instance;

	DW_REQUIRE(instance->rx_buffer != NULL);

	/*sanity check .. idx should never be greater than four */
	DW_ASSERT(instance->rx_idx <= 4);
	retval = 0;
	if (instance->rx_idx != 4) {
		/*need to handle the case where there is less */
		/*than four bytes remaining in the rx buffer */
		if (instance->rx_remain >= 4) {
			buf = (uint32 *) instance->rx_buffer;
			mask = ((uint32) (0xffffffff) >> (8 *
					instance->rx_idx));
			c = mask & (instance->rx_hold >>
					(8 * instance->rx_idx));
			*buf = (*buf & ~mask) | c;
		} else {
			/*tmp = next free location in rx buffer */
			tmp = (uint8 *) instance->rx_buffer;
			/*shift hold so that the least */
			/*significant byte contains valid data */
			c = instance->rx_hold >> (8 * instance->rx_idx);
			/*write out valid character to rx buffer */
			for (i = (4 - instance->rx_idx); i > 0; i--) {
				*tmp++ = (uint8) (c & 0xff);
				c >>= 8;
			}
		}		/*instance->rx_remain <= 4 */
	}
	/* instance->rx_idx != 4 */
	return retval;
}

/**********************************************************************/

int dw_i2c_auto_comp_params(struct dw_device *dev)
{
	int retval;
	unsigned int data_width;
	struct dw_i2c_param *param;
	struct dw_i2c_portmap *portmap;

	I2C_COMMON_REQUIREMENTS(dev);

	param = (struct dw_i2c_param *)dev->comp_param;
	portmap = (struct dw_i2c_portmap *)dev->base_address;

	dev->comp_version = DW_INP(portmap->comp_version);

	/*only version 1.03 and greater support identification registers */
	if ((DW_INP(portmap->comp_type) == dw_apb_i2c) &&
		(DW_BIT_GET(DW_INP(portmap->comp_param_1),
			I2C_PARAM_ADD_ENCODED_PARAMS) == true)) {
		retval = 0;
		param->hc_count_values =
			DW_BIT_GET(DW_INP(portmap->comp_param_1),
			I2C_PARAM_HC_COUNT_VALUES);
		param->has_dma = DW_BIT_GET(DW_INP(portmap->comp_param_1),
					I2C_PARAM_HAS_DMA);
		data_width = DW_BIT_GET(DW_INP(portmap->comp_param_1),
					I2C_PARAM_DATA_WIDTH);
		switch (data_width) {
		case 2:
			dev->data_width = 32;
			break;
		case 1:
			dev->data_width = 16;
			break;
		default:
			dev->data_width = 8;
			break;
		}
		param->max_speed_mode = (enum dw_i2c_speed_mode)
			DW_BIT_GET(DW_INP(portmap->comp_param_1),
			I2C_PARAM_MAX_SPEED_MODE);
		param->rx_buffer_depth =
			DW_BIT_GET(DW_INP(portmap->comp_param_1),
			I2C_PARAM_RX_BUFFER_DEPTH);
		param->rx_buffer_depth++;
		param->tx_buffer_depth =
			DW_BIT_GET(DW_INP(portmap->comp_param_1),
			I2C_PARAM_TX_BUFFER_DEPTH);
		param->tx_buffer_depth++;
	} else
		retval = -DW_ENOSYS;

	return retval;
}

/**********************************************************************/

void dw_i2c_reset_instance(struct dw_device *dev)
{
	struct dw_i2c_portmap *portmap;
	struct dw_i2c_instance *instance;

	I2C_COMMON_REQUIREMENTS(dev);

	portmap = (struct dw_i2c_portmap *)dev->base_address;
	instance = (struct dw_i2c_instance *)dev->instance;

	instance->state = i2c_state_idle;
	instance->intr_mask_save = I2C_INP(portmap->intr_mask);
	instance->tx_threshold = dw_i2c_get_tx_threshold(dev);
	instance->rx_threshold = dw_i2c_get_rx_threshold(dev);
	instance->listener = NULL;
	instance->tx_callback = NULL;
	instance->rx_callback = NULL;
	instance->b2b_buffer = NULL;
	instance->tx_buffer = NULL;
	instance->tx_hold = 0;
	instance->tx_idx = 0;
	instance->tx_length = 0;
	instance->tx_remain = 0;
	instance->rx_buffer = NULL;
	instance->rx_hold = 0;
	instance->rx_idx = 0;
	instance->rx_length = 0;
	instance->rx_remain = 0;
	instance->rx_align = false;
	instance->dma_tx.notifier = NULL;
	instance->dma_rx.notifier = NULL;
	instance->dma_tx.mode = dw_dma_none;
	instance->dma_rx.mode = dw_dma_none;
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

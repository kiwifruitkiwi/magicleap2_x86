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

#include "dw_common.h"
#include "dw_apb_i2c_public.h"
#include "isp_i2c_driver.h"
#include "isp_i2c_driver_private.h"
#include "log.h"

/*this definition is used by the assetion macros to determine the*/
/*current file name.it is defined in the dw_common_dbc header.*/
DW_DEFINE_THIS_FILE;

/*
 *#define PRINTF(...)	\
 *do {	\
 *	printf(__VA_ARGS__);	\
 *} while (0)
 */
/*#define PRINTF isp_dbg_print_info*/
#define PRINTF(...)
#define I2C_INPUT_CLOCK		300

/*
 *these minimum high and low times are in nanoseconds.they represent
 *the minimum amount of time a bus signal must remain either high or
 *low to be interpreted as a logical high or low as per the I2C bus
 *protocol.these values are used in conjunction with an I2C input
 *clock frequency to determine the correct values to be written to the
 *clock count registers.
 */
#define SS_MIN_SCL_HIGH		4000
#define SS_MIN_SCL_LOW		4700
#define FS_MIN_SCL_HIGH		600
#define FS_MIN_SCL_LOW		1300
#define HS_MIN_SCL_HIGH_100PF	60
#define HS_MIN_SCL_LOW_100PF	120

typedef enum _isp_i2c_msg_type_e {
	isp_i2c_msg_type_read,
	isp_i2c_msg_type_write
} isp_i2c_msg_type_e;

typedef struct _isp_i2c_msg_t {
	uint8 device_index;
	/*the address in slave device to access,
	 *MAX 16bit according CCI protocol
	 */
	uint16 subaddr;
	isp_i2c_subaddr_mode_e subaddr_mode;
	bool enable_restart;
	isp_i2c_msg_type_e msg_type;
	uint32 length;
	uint8 *buffer;
} isp_i2c_msg_t;

static isp_i2c_driver_context_t g_isp_i2c_driver_context;

static isp_i2c_err_code_e isp_i2c_poll_write(isp_i2c_rw_package_t *package);
static isp_i2c_err_code_e isp_i2c_poll_read(isp_i2c_rw_package_t *package);
static isp_i2c_err_code_e isp_i2c_poll_write_usesubaddr(isp_i2c_rw_package_t *
							package);
static isp_i2c_err_code_e isp_i2c_poll_write_nosubaddr(isp_i2c_rw_package_t *
							package);
static isp_i2c_err_code_e isp_i2c_poll_read_usesubaddr(isp_i2c_rw_package_t *
							package);
static isp_i2c_err_code_e isp_i2c_poll_read_nosubaddr(isp_i2c_rw_package_t *
							package);
static isp_i2c_err_code_e isp_i2c_sync_interrupt_read(isp_i2c_rw_package_t *
							package);
static isp_i2c_err_code_e isp_i2c_sync_interrupt_write(isp_i2c_rw_package_t *
							package);
static isp_i2c_err_code_e isp_i2c_async_interrupt_read(isp_i2c_rw_package_t *
							package);
static isp_i2c_err_code_e isp_i2c_async_interrupt_write(isp_i2c_rw_package_t *
							package);

static void isp_i2c_delay(volatile uint32 count)
{
	while (count-- != 0)
		;
}

static void *get_isp_i2c_device_base_address(uint32 device_index)
{
	void *base_address = NULL;

	switch (device_index) {
	case 0:
		if (g_isp_env_setting == ISP_ENV_SILICON)
			base_address = (void *)ISP_I2C2_BASE_ADDRESS;
		else
			base_address = (void *)ISP_I2C1_BASE_ADDRESS;
		break;
	case 1:
		if (g_isp_env_setting == ISP_ENV_SILICON)
			base_address = (void *)ISP_I2C1_BASE_ADDRESS;
		else
			base_address = (void *)ISP_I2C2_BASE_ADDRESS;
		break;
	case 2:
		base_address = (void *)ISP_I2C3_BASE_ADDRESS;
		break;
	case 3:
		base_address = (void *)ISP_I2C4_BASE_ADDRESS;
		break;
	case 4:
		base_address = (void *)ISP_I2C5_BASE_ADDRESS;
		break;
	default:
		break;
	}
	isp_dbg_print_info("i2c base for %u is 0x%p\n",
			device_index, base_address);
	return base_address;
}

static struct dw_device *isp_i2c_initialize_context(uint32 device_index)
{
	struct dw_device *master;

	if (device_index >= MAX_MASTER_DEVICES) {
		PRINTF("ERROR: isp_i2c_configure(): device_index error\n");
		return NULL;
	}

	master = &g_isp_i2c_driver_context.masters[device_index];
	master->name = NULL;
	master->data_width = 32;
	/*FIXME: need to set base_address here. */
	master->base_address = get_isp_i2c_device_base_address(device_index);
	master->instance = &g_isp_i2c_driver_context.instances[device_index];
	master->comp_param = &g_isp_i2c_driver_context.params[device_index];
	master->comp_type = dw_apb_i2c;
	master->list.next = NULL;
	master->list.prev = NULL;
	return master;
}

/*FIXME: the speed and duty_ratio are currently not used.
 *need to implement it later
 */
/*utility function which programs the clock count registers for a given*/
/*input clock frequency.*/
void dw_i2c_clock_setup(struct dw_device *dev, uint32 ic_clk, uint32 speed,
			uint32 duty_ratio)
{
	uint16 ss_scl_high, ss_scl_low;
	uint16 fs_scl_high, fs_scl_low;
	/*uint16 hs_scl_high, hs_scl_low; */
	unreferenced_parameter(duty_ratio);
	unreferenced_parameter(speed);
	unreferenced_parameter(ic_clk);
	/*ic_clk is the clock speed (in m_hz) that is being supplied to the */
	/*d_w_apb_i2c device.the correct clock count values are determined */
	/*by using this inconjunction with the minimum high and low signal */
	/*hold times as per the I2C bus specification. */

	/*calculate the IC_LCNT_*S per input ic_clk (iclk) */
	/*also leave some headroom for the clocks (98%) */
	ss_scl_high =
	((uint16) (((SS_MIN_SCL_HIGH * ic_clk * 0.98) / 1000) + 1));
	ss_scl_low =
		((uint16) (((SS_MIN_SCL_LOW * ic_clk * 0.98) / 1000) + 1));
	fs_scl_high =
		((uint16) (((FS_MIN_SCL_HIGH * ic_clk * 0.98) / 1000) + 1));
	fs_scl_low =
		((uint16) (((FS_MIN_SCL_LOW * ic_clk * 0.98) / 1000) + 1));

	dw_i2c_set_scl_count(dev, i2c_speed_standard, i2c_scl_high,
			ss_scl_high);
	dw_i2c_set_scl_count(dev, i2c_speed_standard, i2c_scl_low, ss_scl_low);
	dw_i2c_set_scl_count(dev, i2c_speed_fast, i2c_scl_high, fs_scl_high);
	dw_i2c_set_scl_count(dev, i2c_speed_fast, i2c_scl_low, fs_scl_low);
}

static void isp_i2c_set_fifo_th(int device_index)
{
	uint16 txfifo_depth;
	uint16 rxfifo_depth;
	uint16 txfifo_th;
	uint16 rxfifo_th;
	struct dw_device *master;

	master = &g_isp_i2c_driver_context.masters[device_index];

	txfifo_depth = dw_i2c_get_tx_fifo_depth(master);
	rxfifo_depth = dw_i2c_get_rx_fifo_depth(master);

	PRINTF("txfifo_depth = %d, rxfifo_depth = %d\n", txfifo_depth,
		rxfifo_depth);
	dw_i2c_set_tx_threshold(master, (uint8) (txfifo_depth / 2));
	dw_i2c_set_rx_threshold(master, (uint8) (rxfifo_depth / 2));

	txfifo_th = dw_i2c_get_tx_threshold(master);
	rxfifo_th = dw_i2c_get_rx_threshold(master);
	PRINTF("txfifo_th = %d, rxfifo_th = %d\n", txfifo_th, rxfifo_th);
}

isp_i2c_err_code_e isp_i2c_configure(isp_i2c_configure_t *conf)
{
	struct dw_device *master;
	isp_i2c_driver_conf_t *driver_conf;
	enum dw_i2c_speed_mode speed_mode = i2c_speed_fast;
	enum dw_i2c_address_mode address_mode = i2c_7bit_address;

	/*initialize the context. */
	master = isp_i2c_initialize_context(conf->device_index);
	if (master == NULL)
		return isp_i2c_err_code_fail;

	driver_conf = &g_isp_i2c_driver_context.confs[conf->device_index];

	driver_conf->run_mode = conf->run_mode;

	/*initialize the I2C device driver */
	dw_i2c_init(master);
	/*set up the clock count register.the argument I2C1_CLOCK is */
	/*specified as the I2C master input clock. */
	dw_i2c_clock_setup(master, conf->input_clk, conf->speed,
			conf->scl_duty_ratio);

	if (conf->speed <= 100) {
		speed_mode = i2c_speed_standard;
	} else if (conf->speed <= 400) {
		speed_mode = i2c_speed_fast;
	} else {
		PRINTF
	("ERROR: %s: the speed %d is out of range[0-400]\n", __func__,
			conf->speed);
		return isp_i2c_err_code_fail;
	}
	dw_i2c_set_speed_mode(master, speed_mode);

	address_mode =
		(conf->address_mode == isp_i2c_addr_mode_10bit) ?
		i2c_10bit_address : i2c_7bit_address;
	dw_i2c_set_master_address_mode(master, address_mode);

	dw_i2c_enable_restart(master);

	dw_i2c_enable_master(master);

	dw_i2c_set_tx_mode(master, i2c_tx_target);
	/*dw_i2c_set_target_address(master, conf->target_address); */
	/*dw_i2c_set_target_address(master, 0x39); */

	isp_i2c_set_fifo_th(conf->device_index);

	/*dw_i2c_enable(master); */

	return isp_i2c_err_code_success;
}

static isp_i2c_err_code_e isp_i2c_send_msg(isp_i2c_msg_t *msg)
{
	struct dw_device *master;
	isp_i2c_rw_package_t rw_package;
	isp_i2c_driver_conf_t *driver_conf;
	static isp_i2c_err_code_e ret;

	if (msg->device_index >= MAX_MASTER_DEVICES) {
		PRINTF("ERROR: %s: device_index error\n", __func__);
		return isp_i2c_err_code_fail;
	}
	master = &g_isp_i2c_driver_context.masters[msg->device_index];
	driver_conf = &g_isp_i2c_driver_context.confs[msg->device_index];

	rw_package.device_index = msg->device_index;
	rw_package.subaddr = msg->subaddr;
	rw_package.subaddr_mode = msg->subaddr_mode;
	rw_package.enable_restart = msg->enable_restart;
	rw_package.length = msg->length;
	rw_package.buffer = msg->buffer;

	if (msg->msg_type == isp_i2c_msg_type_read) {
		if (driver_conf->run_mode == isp_i2c_driver_run_mode_poll) {
			ret = isp_i2c_poll_read(&rw_package);
		} else if (driver_conf->run_mode ==
			isp_i2c_driver_run_mode_sync_interrupt) {
			ret = isp_i2c_sync_interrupt_read(&rw_package);
		} else if (driver_conf->run_mode ==
			isp_i2c_driver_run_mode_async_interrupt) {
			ret = isp_i2c_async_interrupt_read(&rw_package);
		} else {
			PRINTF
			("ERROR: %s: run_mode not supported\n", __func__);
			ret = isp_i2c_err_code_fail;
		}
	} else if (msg->msg_type == isp_i2c_msg_type_write) {
		if (driver_conf->run_mode == isp_i2c_driver_run_mode_poll) {
			ret = isp_i2c_poll_write(&rw_package);
		} else if (driver_conf->run_mode ==
			isp_i2c_driver_run_mode_sync_interrupt) {
			ret = isp_i2c_sync_interrupt_write(&rw_package);
		} else if (driver_conf->run_mode ==
			isp_i2c_driver_run_mode_async_interrupt) {
			ret = isp_i2c_async_interrupt_write(&rw_package);
		} else {
			PRINTF
			("ERROR:%s:run_mode not supported\n", __func__);
			ret = isp_i2c_err_code_fail;
		}
	} else {
		PRINTF("ERROR: %s: msg_type error\n", __func__);
		ret = isp_i2c_err_code_fail;
	}

	return ret;
}

static isp_i2c_err_code_e isp_i2c_poll_error_status(struct dw_device *master)
{
	unreferenced_parameter(master);
	return isp_i2c_err_code_success;
}

static isp_i2c_err_code_e isp_i2c_poll_write(isp_i2c_rw_package_t *package)
{
	if (package->subaddr_mode == isp_i2c_subaddr_mode_none)
		return isp_i2c_poll_write_nosubaddr(package);
	else
		return isp_i2c_poll_write_usesubaddr(package);
}

static isp_i2c_err_code_e isp_i2c_poll_read(isp_i2c_rw_package_t *package)
{
	PRINTF("in %s, package->subaddr_mode %d\n", __func__,
		package->subaddr_mode);
	if (package->subaddr_mode == isp_i2c_subaddr_mode_none)
		return isp_i2c_poll_read_nosubaddr(package);
	else
		return isp_i2c_poll_read_usesubaddr(package);
}

static isp_i2c_err_code_e isp_i2c_poll_write_usesubaddr(isp_i2c_rw_package_t *
							package)
{
	uint16 data;
	struct dw_device *master;

	master = &g_isp_i2c_driver_context.masters[package->device_index];

	if (package->subaddr_mode == isp_i2c_subaddr_mode_8bit) {
		data = package->subaddr & 0xff;
		dw_i2c_write(master, data);
	} else if (package->subaddr_mode == isp_i2c_subaddr_mode_16bit) {
		data = (package->subaddr >> 8) & 0xff;
		dw_i2c_write(master, data);

		data = package->subaddr & 0xff;
		dw_i2c_write(master, data);
	} else {
		PRINTF("ERROR: subaddr_mode is not supported\n");
		return isp_i2c_err_code_fail;
	}
	return isp_i2c_poll_write_nosubaddr(package);
}

static isp_i2c_err_code_e isp_i2c_poll_write_nosubaddr(isp_i2c_rw_package_t *
					package)
{
	uint32 i, j;
	uint16 data;
	uint16 txfifodepth;
	uint16 txfifolevel;
	struct dw_device *master;

	master = &g_isp_i2c_driver_context.masters[package->device_index];
	txfifodepth = dw_i2c_get_tx_fifo_depth(master);

	i = 0;

	while (1) {
		txfifolevel = dw_i2c_get_tx_fifo_level(master);
		for (j = 0; j < (uint32) (txfifodepth - txfifolevel); j++) {
			/*max write request at current time. */
			if (i < package->length) {
				data = package->buffer[i];
				if (i == (package->length - 1))
					data |= 0x200;	/*set stop bit. */

				dw_i2c_write(master, data);
				i++;
			} else
				goto out;
		}
		isp_i2c_delay(50);
	}

out:
	return isp_i2c_err_code_success;
}

static isp_i2c_err_code_e isp_i2c_poll_read_usesubaddr(isp_i2c_rw_package_t *
				package)
{
	uint16 data;
	struct dw_device *master;

	master = &g_isp_i2c_driver_context.masters[package->device_index];

	if (package->subaddr_mode == isp_i2c_subaddr_mode_8bit) {
		data = package->subaddr & 0xff;
		dw_i2c_write(master, data);
	} else if (package->subaddr_mode == isp_i2c_subaddr_mode_16bit) {
		data = (package->subaddr >> 8) & 0xff;
		dw_i2c_write(master, data);

		data = package->subaddr & 0xff;
		if (!package->enable_restart) {
		/*if no restart is enabled,
		 *issue stop signal shen the current subaddress finished.
		 */
			data |= 0x200;
		}
		dw_i2c_write(master, data);
	} else {
		PRINTF
("ERROR: isp_i2c_poll_write_usesubaddr(): subaddr_mode is not supported\n");
		return isp_i2c_err_code_fail;
	}
	return isp_i2c_poll_read_nosubaddr(package);
}

static isp_i2c_err_code_e isp_i2c_poll_read_nosubaddr(isp_i2c_rw_package_t *
				package)
{
	uint32 i, j;
	struct dw_device *master;

	master = &g_isp_i2c_driver_context.masters[package->device_index];

	i = 0;
	j = 0;
	PRINTF("-> %s\n", __func__);

	while (1) {
		while (!dw_i2c_is_tx_fifo_full(master)) {
			if (isp_i2c_poll_error_status(master))
				return isp_i2c_err_code_fail;

			if (i < (package->length - 1)) {
				if (i == 0 && package->enable_restart)
					dw_i2c_issue_read(master, 0, 1);
				else
					dw_i2c_issue_read(master, 0, 0);
				i++;
			} else if (i == (package->length - 1)) {
				if (i == 0 && package->enable_restart)
					dw_i2c_issue_read(master, 1, 1);
				else
					dw_i2c_issue_read(master, 1, 0);
				goto read_rest_data;
			}
		}

		while (dw_i2c_is_rx_fifo_empty(master) == false) {
		/*there's data in the receive fifo */
			if (isp_i2c_poll_error_status(master) ==
				isp_i2c_err_code_fail)
				return isp_i2c_err_code_fail;
			if (j < package->length)
				package->buffer[j++] = dw_i2c_read(master);
			else
				goto done;
		}

		if (isp_i2c_poll_error_status(master) ==
			isp_i2c_err_code_fail)
			return isp_i2c_err_code_fail;
	}

 read_rest_data:
	/*read the rest data. */
	PRINTF("read_rest_data,j %d,package->length %d\n", j, package->length);
	while (1) {
		if (isp_i2c_poll_error_status(master) ==
			isp_i2c_err_code_fail) {
			PRINTF("<- %s, for fail\n", __func__);
			return isp_i2c_err_code_fail;
		}
		if (j < package->length) {
			if (dw_i2c_is_rx_fifo_empty(master) == false) {
				package->buffer[j++] = dw_i2c_read(master);
				PRINTF
			("in %s,read rest data\n", __func__);
			}
		} else {
			goto done;
		}
	}

done:
	PRINTF("<- %s\n", __func__);
	return isp_i2c_err_code_success;
}

static isp_i2c_err_code_e isp_i2c_sync_interrupt_read(isp_i2c_rw_package_t *
				package)
{
	/*TODO: */
	unreferenced_parameter(package);
	return isp_i2c_err_code_fail;
}

static isp_i2c_err_code_e isp_i2c_sync_interrupt_write(isp_i2c_rw_package_t *
				package)
{
	/*TODO: */
	unreferenced_parameter(package);
	return isp_i2c_err_code_fail;
}

static isp_i2c_err_code_e isp_i2c_async_interrupt_read(isp_i2c_rw_package_t *
				package)
{
	/*TODO: */
	unreferenced_parameter(package);
	return isp_i2c_err_code_fail;
}

static isp_i2c_err_code_e isp_i2c_async_interrupt_write(isp_i2c_rw_package_t *
				package)
{
	/*TODO: */
	unreferenced_parameter(package);
	return isp_i2c_err_code_fail;
}

isp_i2c_err_code_e
isp_i2c_write(uint8 device_index, uint16 slave_addr, uint8 *buffer,
				uint32 length)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	master = &g_isp_i2c_driver_context.masters[device_index];

	dw_i2c_set_target_address(master, slave_addr);

	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = 0x0;	/*not used */
	i2c_msg.subaddr_mode = isp_i2c_subaddr_mode_none;
	i2c_msg.enable_restart = 0;
	i2c_msg.msg_type = isp_i2c_msg_type_write;
	i2c_msg.length = length;
	i2c_msg.buffer = buffer;

	ret = isp_i2c_send_msg(&i2c_msg);

	while (!dw_i2c_is_tx_fifo_empty(master))
		;

	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_i2c_read(uint8 device_index, uint16 slave_addr, uint8 *buffer,
				uint32 length)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	master = &g_isp_i2c_driver_context.masters[device_index];

	dw_i2c_set_target_address(master, slave_addr);

	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = 0x0;	/*not used */
	i2c_msg.subaddr_mode = isp_i2c_subaddr_mode_none;
	i2c_msg.enable_restart = 0;
	i2c_msg.msg_type = isp_i2c_msg_type_read;
	i2c_msg.length = length;
	i2c_msg.buffer = buffer;

	ret = isp_i2c_send_msg(&i2c_msg);
	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_cci_single_random_write(uint8 device_index, uint16 slave_addr,
	uint16 subaddr, isp_i2c_subaddr_mode_e subaddr_mode, uint8 data)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	if ((subaddr_mode != isp_i2c_subaddr_mode_8bit)
		&& (subaddr_mode != isp_i2c_subaddr_mode_16bit)) {
		PRINTF("ERROR: should be isp_i2c_subaddr_mode_8bit/16bit\n");
		return isp_i2c_err_code_fail;
	}

	master = &g_isp_i2c_driver_context.masters[device_index];
	dw_i2c_set_target_address(master, slave_addr);
	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = subaddr;
	i2c_msg.subaddr_mode = subaddr_mode;
	i2c_msg.enable_restart = 0;
	i2c_msg.msg_type = isp_i2c_msg_type_write;
	i2c_msg.length = 1;
	i2c_msg.buffer = &data;

	ret = isp_i2c_send_msg(&i2c_msg);

	while (!dw_i2c_is_tx_fifo_empty(master))
		;

	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_cci_sequential_random_write(uint8 device_index, uint16 slave_addr,
				uint16 subaddr,
				isp_i2c_subaddr_mode_e subaddr_mode,
				uint8 *buffer, uint32 length)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	if ((subaddr_mode != isp_i2c_subaddr_mode_8bit)
		&& (subaddr_mode != isp_i2c_subaddr_mode_16bit)) {
		PRINTF("ERROR: should be isp_i2c_subaddr_mode_8bit/16bit\n");
		return isp_i2c_err_code_fail;
	}

	master = &g_isp_i2c_driver_context.masters[device_index];
	dw_i2c_set_target_address(master, slave_addr);
	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = subaddr;
	i2c_msg.subaddr_mode = subaddr_mode;
	i2c_msg.enable_restart = 0;
	i2c_msg.msg_type = isp_i2c_msg_type_write;
	i2c_msg.length = length;
	i2c_msg.buffer = buffer;

	ret = isp_i2c_send_msg(&i2c_msg);

	while (!dw_i2c_is_tx_fifo_empty(master))
		;

	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_cci_single_random_read(uint8 device_index, uint16 slave_addr,
	uint16 subaddr, isp_i2c_subaddr_mode_e subaddr_mode, uint8 *data)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	if ((subaddr_mode != isp_i2c_subaddr_mode_8bit)
	&& (subaddr_mode != isp_i2c_subaddr_mode_16bit)) {
		PRINTF("ERROR: should be isp_i2c_subaddr_mode_8bit/16bit\n");
		return isp_i2c_err_code_fail;
	}

	master = &g_isp_i2c_driver_context.masters[device_index];
	dw_i2c_set_target_address(master, slave_addr);
	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = subaddr;
	i2c_msg.subaddr_mode = subaddr_mode;
	i2c_msg.enable_restart = 1;
	i2c_msg.msg_type = isp_i2c_msg_type_read;
	i2c_msg.length = 1;
	i2c_msg.buffer = data;

	ret = isp_i2c_send_msg(&i2c_msg);
	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_cci_single_current_read(uint8 device_index, uint16 slave_addr, uint8 *data)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	master = &g_isp_i2c_driver_context.masters[device_index];
	dw_i2c_set_target_address(master, slave_addr);
	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = 0x0;	/*not used */
	i2c_msg.subaddr_mode = isp_i2c_subaddr_mode_none;
	i2c_msg.enable_restart = 0;
	i2c_msg.msg_type = isp_i2c_msg_type_read;
	i2c_msg.length = 1;
	i2c_msg.buffer = data;

	ret = isp_i2c_send_msg(&i2c_msg);
	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_cci_sequential_random_read(uint8 device_index, uint16 slave_addr,
			uint16 subaddr,
			isp_i2c_subaddr_mode_e subaddr_mode,
			uint8 *buffer, uint32 length)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	if ((subaddr_mode != isp_i2c_subaddr_mode_8bit)
	&& (subaddr_mode != isp_i2c_subaddr_mode_16bit)) {
		PRINTF("ERROR: should be isp_i2c_subaddr_mode_8bit/16bit\n");
		return isp_i2c_err_code_fail;
	}

	master = &g_isp_i2c_driver_context.masters[device_index];
	dw_i2c_set_target_address(master, slave_addr);
	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = subaddr;
	i2c_msg.subaddr_mode = subaddr_mode;
	i2c_msg.enable_restart = 1;
	i2c_msg.msg_type = isp_i2c_msg_type_read;
	i2c_msg.length = length;
	i2c_msg.buffer = buffer;

	ret = isp_i2c_send_msg(&i2c_msg);
	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_cci_sequential_current_read(uint8 device_index, uint16 slave_addr,
				uint8 *buffer, uint32 length)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	master = &g_isp_i2c_driver_context.masters[device_index];
	dw_i2c_set_target_address(master, slave_addr);
	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = 0x0;	/*not used */
	i2c_msg.subaddr_mode = isp_i2c_subaddr_mode_none;
	i2c_msg.enable_restart = 0;
	i2c_msg.msg_type = isp_i2c_msg_type_read;
	i2c_msg.length = length;
	i2c_msg.buffer = buffer;

	ret = isp_i2c_send_msg(&i2c_msg);
	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_i2c_sequential_random_write(uint8 device_index, uint16 slave_addr,
				uint16 subaddr,
				isp_i2c_subaddr_mode_e subaddr_mode,
				uint8 *buffer, uint32 length)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	if ((subaddr_mode != isp_i2c_subaddr_mode_8bit)
	&& (subaddr_mode != isp_i2c_subaddr_mode_16bit)) {
		PRINTF("ERROR: should be isp_i2c_subaddr_mode_8bit/16bit\n");
		return isp_i2c_err_code_fail;
	}

	master = &g_isp_i2c_driver_context.masters[device_index];
	dw_i2c_set_target_address(master, slave_addr);
	dw_i2c_enable(master);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = subaddr;
	i2c_msg.subaddr_mode = subaddr_mode;
	i2c_msg.enable_restart = 0;
	i2c_msg.msg_type = isp_i2c_msg_type_write;
	i2c_msg.length = length;
	i2c_msg.buffer = buffer;

	ret = isp_i2c_send_msg(&i2c_msg);

	while (!dw_i2c_is_tx_fifo_empty(master))
		;

	dw_i2c_disable(master);
	return ret;
}

isp_i2c_err_code_e
isp_i2c_sequential_random_read(uint8 device_index, uint16 slave_addr,
			uint16 subaddr,
			isp_i2c_subaddr_mode_e subaddr_mode,
			uint8 *buffer, uint32 length)
{
	isp_i2c_msg_t i2c_msg;
	struct dw_device *master;
	isp_i2c_err_code_e ret;

	if ((subaddr_mode != isp_i2c_subaddr_mode_8bit)
	&& (subaddr_mode != isp_i2c_subaddr_mode_16bit)) {
		PRINTF("ERROR: should be isp_i2c_subaddr_mode_8bit/16bit\n");
		return isp_i2c_err_code_fail;
	}
	PRINTF("in %s 1\n", __func__);
	master = &g_isp_i2c_driver_context.masters[device_index];
	dw_i2c_set_target_address(master, slave_addr);
	PRINTF("in %s 2\n", __func__);
	dw_i2c_enable(master);
	PRINTF("in %s 3\n", __func__);

	i2c_msg.device_index = device_index;
	i2c_msg.subaddr = subaddr;
	i2c_msg.subaddr_mode = subaddr_mode;
	i2c_msg.enable_restart = 0;
	i2c_msg.msg_type = isp_i2c_msg_type_read;
	i2c_msg.length = length;
	i2c_msg.buffer = buffer;

	ret = isp_i2c_send_msg(&i2c_msg);
	PRINTF("in %s 4\n", __func__);
	dw_i2c_disable(master);
	PRINTF("in %s 5\n", __func__);
	return ret;
}

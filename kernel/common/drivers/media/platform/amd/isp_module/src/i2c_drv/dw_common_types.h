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

#ifndef DW_COMMON_TYPES_H
#define DW_COMMON_TYPES_H

#include "dw_common_list.h"

#ifndef TRUE
#define TRUE	(1)
#endif /*TRUE*/
#ifndef true
#define true	TRUE
#endif				/*true */
#ifndef FALSE
#define FALSE	(0)
#endif /*FALSE*/
#ifndef false
#define false	FALSE
#endif				/*false */
#ifndef EOF
#define EOF	(-1)
#endif /*EOF*/
#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif
/****d*include.types/dw_state
 *NAME
 *dw_state
 *DESCRIPTION
 *this is a generic data type used for 1-bit wide bitfields which have
 *a "set/clear" property.this is used when modifying registers
 *within a peripheral's memory map.
 *SOURCE
 */
enum dw_state {
	dw_err = -1,
	dw_clear = 0,
	dw_set = 1
};
/*****/

/****d*include.types/dw_comp_type
 *NAME
 *dw_comp_type -- component type identification numbers
 *DESCRIPTION
 *this data type is used to identify peripheral types.  these values
 *are typically encoded in a peripheral's portmap and are also used in
 *the dw_device structure.
 *SEE ALSO
 *dw_device
 *SOURCE
 */
enum dw_comp_type {
	dw_dev_none = 0x00000000,
	dw_memctl = 0x44572110,
	dw_ahb_dmac = 0x44571110,
	dw_ahb_ictl = 0x44571120,
	dw_ahb_pci = 0x44571130,
	dw_ahb_usb = 0x44571140,
	dw_apb_gpio = 0x44570160,
	dw_apb_i2c = 0x44570140,
	dw_apb_ictl = 0x44570180,
	dw_apb_rap = 0x44570190,
	dw_apb_rtc = 0x44570130,
	dw_apb_ssi = 0x44570150,
	dw_apb_timers = 0x44570170,
	dw_apb_uart = 0x44570110,
	dw_apb_wdt = 0x44570120
};
/*****/

/****s*include.types/dw_device
 *NAME
 *dw_device -- low-level device handle structure
 *SYNOPSIS
 *this is the handle used to identify and manipulate all design_ware
 *peripherals.
 *DESCRIPTION
 *this is the primary structure used when dealing with all devices.
 *it serves as a hardware abstraction layer for driver code and also
 *allows this code to support more than one device of the same type
 *simultaneously.this structure needs to be initialized with
 *meaningful values before a pointer to it is passed to a driver
 *initialization function.
 *PARAMETERS
 *name			name of device
 *data_width		bus data width of the device
 *base_address		physical base address of device
 *instance		device private data structure pointer
 *os			unused pointer for associating with an OS structure
 *comp_param		pointer to structure containing device's
 *core_consultant	configuration parameters structure
 *comp_version		device version identification number
 *comp_type		device identification number
 ***/
struct dw_device {
	const char *name;
	unsigned int data_width;
	void *base_address;
	void *instance;
	void *os;
	void *comp_param;
	uint32 comp_version;
	enum dw_comp_type comp_type;
	struct dw_list_head list;
};

/****d*include.types/dw_callback
 *DESCRIPTION
 *this is a generic data type used for handling callback functions
 *with each driver.
 *ARGUMENTS
 *dev		-- device handle
 *e_code	-- event/error code
 *NOTES
 *the usage of the e_code argument is typically negative for an error
 *code and positive for an event code.
 *SOURCE
 */
typedef void (*dw_callback) (struct dw_device *dev, int32 e_code);
/*****/

/****d*include.types/dw_dma_notifier_func
 *DESCRIPTION
 *this is the data type used for specifying DMA notifier functions.
 *these are needed when software handshaking is used with peripheral
 *DMA transfers in order to inform the DMA controller when data is
 *ready to be sent/received.
 *ARGUMENTS
 *dev		-- DMA device handle
 *channel	-- associated channel number
 *single	-- single or burst transfer?
 *last		-- is this the last block?
 *NOTES
 *the single and last arguments are only necessary when the peripheral
 *involved is also acting as the flow controller.
 *SOURCE
 */
typedef void (*dw_dma_notifier_func) (struct dw_device *dev,
				unsigned int channel, bool single, bool last);
/*****/

/****d*include.data/dw_dma_mode
 *DESCRIPTION
 *this is the data type used for enabling software or hardware
 *handshaking for DMA transfers.using software handshaking changes
 *how an interrupt handler processes rx full and tx empty interrupts.
 *any design_ware peripheral which supports DMA transfers has API
 *function which match those listed below.
 *SEE ALSO
 *dw_*_set_dma_tx_mode(), dw_*_set_dma_rx_mode(), dw_*_get_dma_tx_mode(),
 *dw_*_get_dma_rx_mode(), dw_*_set_dma_tx_notifier(),
 *dw_*_set_dma_tx_notifier()
 *SOURCE
 */
enum dw_dma_mode {
	dw_dma_none,		/*DMA is not being used */
	dw_dma_sw_handshake,	/*DMA using software handshaking */
	dw_dma_hw_handshake	/*DMA using hardware handshaking */
};
/*****/

/****s*include.api/dw_dma_notify
 *DESCRIPTION
 *this is the data structure used to store a DMA notifier function
 *and its related arguments.
 *SOURCE
 */
struct dw_dma_config {
	enum dw_dma_mode mode;
	dw_dma_notifier_func notifier;
	struct dw_device *dmac;
	unsigned int channel;
};
/*****/

#endif				/*DW_COMMON_TYPES_H */

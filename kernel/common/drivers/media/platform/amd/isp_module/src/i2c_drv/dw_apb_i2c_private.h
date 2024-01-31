/**************************************************************************
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
 **************************************************************************/

#ifndef DW_I2C_PRIVATE_H
#define DW_I2C_PRIVATE_H

#define APB_DATA_WIDTH		32

/****id*i2c.macros/I2C_COMMON_REQUIREMENTS
 *NAME
 *common API requirements
 *DESCRIPTION
 *these are the common preconditions which must be met for all driver
 *functions.primarily, they check that a function has been passed
 *a legitimate dw_device structure.
 *SOURCE
 */
#define I2C_COMMON_REQUIREMENTS(dev)			\
do {							\
	DW_REQUIRE(dev != NULL);			\
	DW_REQUIRE(dev->instance != NULL);		\
	DW_REQUIRE(dev->comp_param != NULL);		\
	DW_REQUIRE(dev->base_address != NULL);		\
	DW_REQUIRE(dev->comp_type == dw_apb_i2c);	\
	DW_REQUIRE((dev->data_width == 32)		\
	|| (dev->data_width == 16)			\
	|| (dev->data_width == 8));			\
} while (0)
/*****/

/****id*i2c.macros/bit_definitions
 *NAME
 *bitfield width/shift definitions
 *DESCRIPTION
 *used in conjunction with bitops.h to access register bitfields.
 *they are defined as bit offset/mask pairs for each i2c register
 *bitfield.
 *EXAMPLE
 *target_address = BIT_GET(in32(I2C_TAR), I2C_TAR_ADDR);
 *NOTES
 *bfo is the offset of the bitfield with respect to LSB;
 *bfw is the width of the bitfield
 *SEE ALSO
 *dw_common_bitops.h
 */
/*control register */
#define bfoI2C_CON_MASTER_MODE		((uint32) 0)
#define bfwI2C_CON_MASTER_MODE		((uint32) 1)
#define bfoI2C_CON_SPEED		((uint32) 1)
#define bfwI2C_CON_SPEED		((uint32) 2)
#define bfoI2C_CON_10BITADDR_SLAVE	((uint32) 3)
#define bfwI2C_CON_10BITADDR_SLAVE	((uint32) 1)
#define bfoI2C_CON_10BITADDR_MASTER	((uint32) 4)
#define bfwI2C_CON_10BITADDR_MASTER	((uint32) 1)
#define bfoI2C_CON_RESTART_EN		((uint32) 5)
#define bfwI2C_CON_RESTART_EN		((uint32) 1)
#define bfoI2C_CON_SLAVE_DISABLE	((uint32) 6)
#define bfwI2C_CON_SLAVE_DISABLE	((uint32) 1)
/*target address register*/
#define bfoI2C_TAR_ADDR			((uint32) 0)
#define bfwI2C_TAR_ADDR			((uint32) 10)
#define bfoI2C_TAR_GC_OR_START		((uint32) 10)
#define bfwI2C_TAR_GC_OR_START		((uint32) 1)
#define bfoI2C_TAR_SPECIAL		((uint32) 11)
#define bfwI2C_TAR_SPECIAL		((uint32) 1)
#define bfoI2C_TAR_10BITADDR_MASTER	((uint32) 12)
#define bfwI2C_TAR_10BITADDR_MASTER	((uint32) 1)
/*slave address register*/
#define bfoI2C_SAR_ADDR			((uint32) 0)
#define bfwI2C_SAR_ADDR			((uint32) 10)
/*high speed master mode code address register*/
#define bfoI2C_HS_MADDR_HS_MAR		((uint32) 0)
#define bfwI2C_HS_MADDR_HS_MAR		((uint32) 3)
/*tx/rx data buffer and command register*/
#define bfoI2C_DATA_CMD_DAT		((uint32) 0)
#define bfwI2C_DATA_CMD_DAT		((uint32) 8)
#define bfoI2C_DATA_CMD_CMD		((uint32) 8)
#define bfwI2C_DATA_CMD_CMD		((uint32) 1)
/*standard speed scl clock high count register*/
#define bfoI2C_SS_SCL_HCNT_COUNT	((uint32) 0)
#define bfwI2C_SS_SCL_HCNT_COUNT	((uint32) 16)
/*standard speed scl clock low count register*/
#define bfoI2C_SS_SCL_LCNT_COUNT	((uint32) 0)
#define bfwI2C_SS_SCL_LCNT_COUNT	((uint32) 16)
/*fast speed scl clock high count register*/
#define bfoI2C_FS_SCL_HCNT_COUNT	((uint32) 0)
#define bfwI2C_FS_SCL_HCNT_COUNT	((uint32) 16)
/*fast speed scl clock low count register*/
#define bfoI2C_FS_SCL_LCNT_COUNT	((uint32) 0)
#define bfwI2C_FS_SCL_LCNT_COUNT	((uint32) 16)
/*high speed scl clock high count register*/
#define bfoI2C_HS_SCL_HCNT_COUNT	((uint32) 0)
#define bfwI2C_HS_SCL_HCNT_COUNT	((uint32) 16)
/*high speed scl clock low count register*/
#define bfoI2C_HS_SCL_LCNT_COUNT	((uint32) 0)
#define bfwI2C_HS_SCL_LCNT_COUNT	((uint32) 16)
/*[raw] interrupt status & mask registers*/
#define bfoI2C_INTR_RX_UNDER		((uint32) 0)
#define bfwI2C_INTR_RX_UNDER		((uint32) 1)
#define bfoI2C_INTR_RX_OVER		((uint32) 1)
#define bfwI2C_INTR_RX_OVER		((uint32) 1)
#define bfoI2C_INTR_RX_FULL		((uint32) 2)
#define bfwI2C_INTR_RX_FULL		((uint32) 1)
#define bfoI2C_INTR_TX_OVER		((uint32) 3)
#define bfwI2C_INTR_TX_OVER		((uint32) 1)
#define bfoI2C_INTR_TX_EMPTY		((uint32) 4)
#define bfwI2C_INTR_TX_EMPTY		((uint32) 1)
#define bfoI2C_INTR_RD_REQ		((uint32) 5)
#define bfwI2C_INTR_RD_REQ		((uint32) 1)
#define bfoI2C_INTR_TX_ABRT		((uint32) 6)
#define bfwI2C_INTR_TX_ABRT		((uint32) 1)
#define bfoI2C_INTR_RX_DONE		((uint32) 7)
#define bfwI2C_INTR_RX_DONE		((uint32) 1)
#define bfoI2C_INTR_ACTIVITY		((uint32) 8)
#define bfwI2C_INTR_ACTIVITY		((uint32) 1)
#define bfoI2C_INTR_STOP_DET		((uint32) 9)
#define bfwI2C_INTR_STOP_DET		((uint32) 1)
#define bfoI2C_INTR_START_DET		((uint32) 10)
#define bfwI2C_INTR_START_DET		((uint32) 1)
#define bfoI2C_INTR_GEN_CALL		((uint32) 11)
#define bfwI2C_INTR_GEN_CALL		((uint32) 1)
/*receive fifo threshold register*/
#define bfoI2C_RX_TL_RX_TL		((uint32) 0)
#define bfwI2C_RX_TL_RX_TL		((uint32) 8)
/*transmit fifo threshold register*/
#define bfoI2C_TX_TL_TX_TL		((uint32) 0)
#define bfwI2C_TX_TL_TX_TL		((uint32) 8)
/*clear combind and individual interrupts register*/
#define bfoI2C_CLR_INTR_CLR_INTR	((uint32) 0)
#define bfwI2C_CLR_INTR_CLR_INTR	((uint32) 1)
/*clear rx_under interrupt register*/
#define bfoI2C_CLR_RX_UNDER_CLEAR	((uint32) 0)
#define bfwI2C_CLR_RX_UNDER_CLEAR	((uint32) 1)
/*clear rx_over interrupt register*/
#define bfoI2C_CLR_RX_OVER_CLEAR	((uint32) 0)
#define bfwI2C_CLR_RX_OVER_CLEAR	((uint32) 1)
/*clear tx_over interrupt register*/
#define bfoI2C_CLR_TX_OVER_CLEAR	((uint32) 0)
#define bfwI2C_CLR_TX_OVER_CLEAR	((uint32) 1)
/*clear rd_req interrupt register*/
#define bfoI2C_CLR_RD_REQ_CLEAR		((uint32) 0)
#define bfwI2C_CLR_RD_REQ_CLEAR		((uint32) 1)
/*clear rx_abrt interrupt register*/
#define bfoI2C_CLR_TX_ABRT_CLEAR	((uint32) 0)
#define bfwI2C_CLR_TX_ABRT_CLEAR	((uint32) 1)
/*clear rx_done interrupt register*/
#define bfoI2C_CLR_RX_DONE_CLEAR	((uint32) 0)
#define bfwI2C_CLR_RX_DONE_CLEAR	((uint32) 1)
/*clear activity interrupt register*/
#define bfoI2C_CLR_ACTIVITY_CLEAR	((uint32) 0)
#define bfwI2C_CLR_ACTIVITY_CLEAR	((uint32) 1)
/*clear stop detection interrupt register*/
#define bfoI2C_CLR_STOP_DET_CLEAR	((uint32) 0)
#define bfwI2C_CLR_STOP_DET_CLEAR	((uint32) 1)
/*clear start detection interrupt register*/
#define bfoI2C_CLR_START_DET_CLEAR	((uint32) 0)
#define bfwI2C_CLR_START_DET_CLEAR	((uint32) 1)
/*clear general call interrupt register*/
#define bfoI2C_CLR_GEN_CALL_CLEAR	((uint32) 0)
#define bfwI2C_CLR_GEN_CALL_CLEAR	((uint32) 1)
/*enable register*/
#define bfoI2C_ENABLE_ENABLE		((uint32) 0)
#define bfwI2C_ENABLE_ENABLE		((uint32) 1)
/*status register*/
#define bfoI2C_STATUS_ACTIVITY		((uint32) 0)
#define bfwI2C_STATUS_ACTIVITY		((uint32) 1)
#define bfoI2C_STATUS_TFNF		((uint32) 1)
#define bfwI2C_STATUS_TFNF		((uint32) 1)
#define bfoI2C_STATUS_TFE		((uint32) 2)
#define bfwI2C_STATUS_TFE		((uint32) 1)
#define bfoI2C_STATUS_RFNE		((uint32) 3)
#define bfwI2C_STATUS_RFNE		((uint32) 1)
#define bfoI2C_STATUS_RFF		((uint32) 4)
#define bfwI2C_STATUS_RFF		((uint32) 1)
/*transmit fifo level register*/
#define bfoI2C_TXFLR_TXFL		((uint32) 0)
#define bfwI2C_TXFLR_TXFL		((uint32) 9)
/*receive fifo level register*/
#define bfoI2C_RXFLR_RXFL		((uint32) 0)
#define bfwI2C_RXFLR_RXFL		((uint32) 9)
/*soft reset register*/
#define bfoI2C_SRESET_SRST		((uint32) 0)
#define bfwI2C_SRESET_SRST		((uint32) 1)
#define bfoI2C_SRESET_MASTER_SRST	((uint32) 1)
#define bfwI2C_SRESET_MASTER_SRST	((uint32) 1)
#define bfoI2C_SRESET_SLAVE_SRST	((uint32) 2)
#define bfwI2C_SRESET_SLAVE_SRST	((uint32) 1)
/*transmit abort status register*/
#define bfoI2C_TX_ABRT_SRC_7B_ADDR_NOACK	((uint32) 0)
#define bfwI2C_TX_ABRT_SRC_7B_ADDR_NOACK	((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_10ADDR1_NOACK	((uint32) 1)
#define bfwI2C_TX_ABRT_SRC_10ADDR1_NOACK	((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_10ADDR2_NOACK	((uint32) 2)
#define bfwI2C_TX_ABRT_SRC_10ADDR2_NOACK	((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_TXDATA_NOACK		((uint32) 3)
#define bfwI2C_TX_ABRT_SRC_TXDATA_NOACK		((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_GCALL_NOACK		((uint32) 4)
#define bfwI2C_TX_ABRT_SRC_GCALL_NOACK		((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_GCALL_READ		((uint32) 5)
#define bfwI2C_TX_ABRT_SRC_GCALL_READ		((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_HS_ACKDET		((uint32) 6)
#define bfwI2C_TX_ABRT_SRC_HS_ACKDET		((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_SBYTE_ACKDET		((uint32) 7)
#define bfwI2C_TX_ABRT_SRC_SBYTE_ACKDET		((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_HS_NORSTRT		((uint32) 8)
#define bfwI2C_TX_ABRT_SRC_HS_NORSTRT		((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_SBYTE_NORSTRT	((uint32) 9)
#define bfwI2C_TX_ABRT_SRC_SBYTE_NORSTRT	((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_10B_RD_NORSTRT	((uint32) 10)
#define bfwI2C_TX_ABRT_SRC_10B_RD_NORSTRT	((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_ARB_MASTER_DIS	((uint32) 11)
#define bfwI2C_TX_ABRT_SRC_ARB_MASTER_DIS	((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_ARB_LOST		((uint32) 12)
#define bfwI2C_TX_ABRT_SRC_ARB_LOST		((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_SLVFLUSH_TXFIFO	((uint32) 13)
#define bfwI2C_TX_ABRT_SRC_SLVFLUSH_TXFIFO	((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_SLV_ARBLOST		((uint32) 14)
#define bfwI2C_TX_ABRT_SRC_SLV_ARBLOST		((uint32) 1)
#define bfoI2C_TX_ABRT_SRC_SLVRD_INTX		((uint32) 15)
#define bfwI2C_TX_ABRT_SRC_SLVRD_INTX		((uint32) 1)
/*dma control register*/
#define bfoI2C_DMA_CR_RDMAE		((uint32) 0)
#define bfwI2C_DMA_CR_RDMAE		((uint32) 1)
#define bfoI2C_DMA_CR_TDMAE		((uint32) 1)
#define bfwI2C_DMA_CR_TDMAE		((uint32) 1)
/*dma transmit data level register*/
#define bfoI2C_DMA_TDLR_DMATDL		((uint32) 0)
#define bfwI2C_DMA_TDLR_DMATDL		((uint32) 8)
/*dma receive data level register*/
#define bfoI2C_DMA_RDLR_DMARDL		((uint32) 0)
#define bfwI2C_DMA_RDLR_DMARDL		((uint32) 8)
/*i2c component parameters*/
#define bfoI2C_PARAM_DATA_WIDTH		((uint32) 0)
#define bfwI2C_PARAM_DATA_WIDTH		((uint32) 2)
#define bfoI2C_PARAM_MAX_SPEED_MODE	((uint32) 2)
#define bfwI2C_PARAM_MAX_SPEED_MODE	((uint32) 2)
#define bfoI2C_PARAM_HC_COUNT_VALUES	((uint32) 4)
#define bfwI2C_PARAM_HC_COUNT_VALUES	((uint32) 1)
#define bfoI2C_PARAM_INTR_IO		((uint32) 5)
#define bfwI2C_PARAM_INTR_IO		((uint32) 1)
#define bfoI2C_PARAM_HAS_DMA		((uint32) 6)
#define bfwI2C_PARAM_HAS_DMA		((uint32) 1)
#define bfoI2C_PARAM_ADD_ENCODED_PARAMS	((uint32) 7)
#define bfwI2C_PARAM_ADD_ENCODED_PARAMS	((uint32) 1)
#define bfoI2C_PARAM_RX_BUFFER_DEPTH	((uint32) 8)
#define bfwI2C_PARAM_RX_BUFFER_DEPTH	((uint32) 8)
#define bfoI2C_PARAM_TX_BUFFER_DEPTH	((uint32) 16)
#define bfwI2C_PARAM_TX_BUFFER_DEPTH	((uint32) 8)
/*//*/
/*derived bitfield definitions*/
/*//*/
/*the following bitfield is a concatenation of SPECIAL and GC_OR_START*/
#define bfoI2C_TAR_TX_MODE		((uint32) 10)
#define bfwI2C_TAR_TX_MODE		((uint32) 2)
/*generic definition used for all scl clock count reads/modifications*/
#define bfoI2C_SCL_COUNT		((uint32) 0)
#define bfwI2C_SCL_COUNT		((uint32) 16)
/*group bitfield of everything in TX_ABRT_SOURCE*/
#define bfoI2C_TX_ABRT_SRC_ALL		((uint32) 0)
#define bfwI2C_TX_ABRT_SRC_ALL		((uint32) 16)

/****id*i2c.macros/DW_CC_DEFINE_I2C_PARAMS
 *USAGE
 *DW_CC_DEFINE_I2C_PARAMS(prefix);
 *ARGUMENTS
 *prefix prefix of peripheral (can be blank/empty)
 *DESCRIPTION
 *this macro is intended for use in initializing values for the
 *dw_i2c_param structure (upon which it is dependent).these
 *values are obtained from d_w_apb_i2c_defs.h (upon which this
 *macro is also dependent).
 *NOTES
 *the relevant d_w_apb_i2c core_kit C header must be included before
 *this macro can be used.
 *EXAMPLE
 *const struct dw_i2c_param i2c = DW_CC_DEFINE_I2C_PARAMS();
 *const struct dw_i2c_param i2c_m = DW_CC_DEFINE_I2C_PARAMS(master_);
 *SEE ALSO
 *struct dw_i2c_param
 *SOURCE
 */
#define DW_CC_DEFINE_I2C_PARAMS(x) DW_CC_DEFINE_I2C_PARAMS_1_03(x)

#define DW_CC_DEFINE_I2C_PARAMS_1_03(prefix) {		\
	prefix ## CC_IC_HC_COUNT_VALUES,			\
	prefix ## CC_IC_HAS_DMA,				\
	prefix ## CC_IC_RX_BUFFER_DEPTH,			\
	prefix ## CC_IC_TX_BUFFER_DEPTH,			\
	(enum dw_i2c_speed_mode) prefix ## CC_IC_MAX_SPEED_MODE	\
}

#define DW_CC_DEFINE_I2C_PARAMS_1_01(prefix) {		\
	prefix ## CC_IC_HC_COUNT_VALUES,			\
	false,						\
	prefix ## CC_IC_RX_BUFFER_DEPTH,			\
	prefix ## CC_IC_TX_BUFFER_DEPTH,			\
	(enum dw_i2c_speed_mode) prefix ## CC_IC_MAX_SPEED_MODE	\
}
/*****/

/****id*i2c.macros/APB_ACCESS
 *DESCRIPTION
 *this macro is used to hardcode the APB data accesses, if the APB
 *data width is the same for an entire system.simply defining
 *APB_DATA_WIDTH at compile time will force all d_w_apb_i2c memory map
 *I/O accesses to be performed with the specified data width.by
 *default, no I/O access is performed until the APB data width of a
 *device is checked in the dw_device data structure.
 *SOURCE
 */
#ifdef APB_DATA_WIDTH
#if (APB_DATA_WIDTH == 32)
#define I2C_INP		DW_IN32_32P
#define I2C_OUTP	DW_OUT32_32P
#elif (APB_DATA_WIDTH == 16)
#define I2C_INP		DW_IN32_16P
#define I2C_OUTP	DW_OUT32_16P
#else
#define I2C_INP		DW_IN32_8P
#define I2C_OUTP	DW_OUT32_8P
#endif				/*(APB_DATA_WIDTH == 32) */
#else
#define I2C_INP		DW_INP
#define I2C_OUTP	DW_OUTP
#endif				/*APB_DATA_WIDTH */
  /*****/

/****id*i2c.macros/I2C_FIFO_READ
 *DESCRIPTION
 *this macro reads up to MAX bytes from the I2C rx FIFO.however, to
 *improve AMBA bus efficieny, it only writes to the user-specified
 *buffer in memory once for every four bytes received.it
 *accomplishes this using an intermediate holding variable
 *instance->rx_hold.
 *SEE ALSO
 *I2C_FIFO_WRITE, I2C_FIFO_WRITE16
 *SOURCE
 */
#define I2C_FIFO_READ(MAX)	\
do {				\
	int i;			\
	uint32 *ptr;		\
	ptr = (uint32 *) instance->rx_buffer;		\
	for (i = 0; i < (MAX); i++) {			\
		instance->rx_hold >>= 8;		\
		instance->rx_hold |= (DW_IN16P(portmap->data_cmd) << 24);\
		if (--instance->rx_idx == 0) {		\
			*(ptr++) = instance->rx_hold;	\
			instance->rx_idx = 4;		\
		}	\
		if (--instance->rx_remain == 0)		\
			break;				\
	}	\
	instance->rx_buffer = (uint8 *) ptr;	\
} while (0)
/*****/

/****id*i2c.macros/I2C_FIFO_WRITE
 *DESCRIPTION
 *this macro writes up to MAX bytes to the I2C tx FIFO.however, to
 *improve AMBA bus efficieny, it only reads from the user-specified
 *buffer in memory once for every four bytes transmitted.it
 *accomplishes this using an intermediate holding variable
 *instance->tx_hold.
 *SEE ALSO
 *I2C_FIFO_READ, I2C_FIFO_WRITE16
 *SOURCE
 */
#define I2C_FIFO_WRITE(MAX)				\
do {							\
	int i;						\
	uint32 *ptr;					\
	ptr = (uint32 *) instance->tx_buffer;		\
	for (i = 0; i < (MAX); i++) {			\
		if (instance->tx_idx == 0) {		\
			instance->tx_hold = *(ptr++);	\
			instance->tx_idx = 4;		\
		}	\
		DW_OUT16P((instance->tx_hold & 0xff), portmap->data_cmd);\
		instance->tx_hold >>= 8;				\
		instance->tx_idx--;					\
		if (--instance->tx_remain == 0)				\
			break;						\
	}	\
	instance->tx_buffer = (uint8 *) ptr;	\
} while (0)
/*****/

/****id*i2c.macros/I2C_FIFO_WRITE16
 *DESCRIPTION
 *this macro writes up to MAX words to the I2C tx FIFO.  however, to
 *improve AMBA bus efficieny, it only reads from the user-specified
 *buffer in memory once for every two words transmitted.  it
 *accomplishes this using an intermediate holding variable
 *instance->tx_hold.
 *SEE ALSO
 *I2C_FIFO_READ, I2C_FIFO_WRITE
 *SOURCE
 */
/*****/

/****id*i2c.macros/I2C_ENTER_CRITICAL_SECTION
 *DESCRIPTION
 *this macro disables d_w_apb_i2c interrupts, to avoid shared data
 *issues when entering a critical section of the driver code.A copy
 *of the current intr_mask value is kept in the instance structure so
 *that the interrupts can be later restored.
 *SEE ALSO
 *I2C_EXIT_CRITICAL_SECTION, dw_i2c_instance
 *SOURCE
 */
/*disable I2C interrupts */
#define I2C_ENTER_CRITICAL_SECTION() I2C_OUTP(0x0, portmap->intr_mask)
/*****/

/****id*i2c.macros/I2C_EXIT_CRITICAL_SECTION
 *DESCRIPTION
 *this macro restores d_w_apb_i2x interrupts, after a critical code
 *section.  it uses the saved intr_mask value in the instance
 *structure to accomplish this.
 *SEE ALSO
 *I2C_ENTER_CRITICAL_SECTION, dw_i2c_instance
 *SOURCE
 */
/*restore I2C interrupts */
#define I2C_EXIT_CRITICAL_SECTION() \
		I2C_OUTP(instance->intr_mask_save, portmap->intr_mask)

/*****/

/****id*i2c.data/dw_i2c_state
 *NAME
 *dw_i2c_state
 *DESCRIPTION
 *this is the data type used for managing the i2c driver state.
 *SOURCE
 */
enum dw_i2c_state {
	/*driver is idle */
	i2c_state_idle = 0,
	/*driver in back2back transfer mode */
	i2c_state_back2back,
	/*driver in master-transmitter mode */
	i2c_state_master_tx,
	/*driver in master-receiver mode */
	i2c_state_master_rx,
	/*driver in slave-transmitter mode */
	i2c_state_slave_tx,
	/*driver in slave-transmitter bulk transfer mode */
	i2c_state_slave_bulk_tx,
	/*driver in slave-receiver mode */
	i2c_state_slave_rx,
	/*waiting for user to set up a slave-tx transfer (read request) */
	i2c_state_rd_req,
	/*waiting for user to set up slave-rx transfer (data already in rx */
	/*fifo or general call received) */
	i2c_state_rx_req,
	/*master tx mode is general call and slave on same device is */
	/*responding to the general call.waiting for user to set up a */
	/*slave-rx transfer. */
	i2c_state_master_tx_gen_call,
	/*master is transmitting a general call which the slave is */
	/*receiving */
	i2c_state_master_tx_slave_rx,
	/*slave-tx transfer is in progress and the rx full interrupt is */
	/*triggered.waiting for user to set up a slave-rx transfer. */
	i2c_state_slave_tx_rx_req,
	/*slave-tx bulk transfer is in progress and the rx full interrupt */
	/*is triggered.waiting for user to set up a slave-rx transfer. */
	i2c_state_slave_bulk_tx_rx_req,
	/*slave-rx transfer is in progress and the read request interrupt */
	/*is triggered.waiting for user to set up a slave-tx transfer. */
	i2c_state_slave_rx_rd_req,
	/*slave is both servicing read requests and is also receiving data */
	/*(e.g. when it is the target of a back-to-back transfer) */
	i2c_state_slave_tx_rx,
	/*slave is both servicing read requests (bulk transfer mode) and is */
	/*also receiving data (e.g. when it is the target of a back-to-back */
	/*transfer) */
	i2c_state_slave_bulk_tx_rx,
	/*A tx abort or fifo over/underflow error has occurred.waiting */
	/*for user to call dw_i2c_terminate(). */
	i2c_state_error
};
/*****/

/****is*i2c.api/dw_i2c_param
 *NAME
 *dw_i2c_param -- i2c hardware configuration parameters
 *DESCRIPTION
 *this structure comprises the i2c hardware parameters that affect
 *the software driver.this structure needs to be initialized with
 *the correct values and be pointed to by the (struct
 *dw_device).comp_param member of the relevant i2c device structure.
 *SOURCE
 */
struct dw_i2c_param {
	bool hc_count_values;	/*hardcoded scl count values? */
	bool has_dma;		/*i2c has a dma interface? */
	uint16 rx_buffer_depth;	/*rx fifo depth */
	uint16 tx_buffer_depth;	/*tx fifo depth */
	enum dw_i2c_speed_mode max_speed_mode;	/*standard, fast or high */
};
/*****/

/****is*i2c.api/dw_i2c_portmap
 *NAME
 *dw_i2c_portmap
 *DESCRIPTION
 *this is the structure used for accessing the i2c register
 *portmap.
 *EXAMPLE
 *struct dw_i2c_portmap *portmap;
 *portmap = (struct dw_i2c_portmap *) DW_APB_I2C_BASE;
 *foo = in32p(portmap->TX_ABRT_SOURCE);
 *SOURCE
 */
struct dw_i2c_portmap {
	volatile uint32 con;	/*control register	(0x00) */
	volatile uint32 tar;	/*target address	(0x04) */
	volatile uint32 sar;	/*slave address	 (0x08) */
	volatile uint32 hs_maddr;	/*high speed master code(0x0c) */
	volatile uint32 data_cmd;	/*tx/rx data/command buffer (0x10) */
	volatile uint32 ss_scl_hcnt;	/*standard SCL high count (0x14) */
	volatile uint32 ss_scl_lcnt;	/*standard SCL low count(0x18) */
	volatile uint32 fs_scl_hcnt;	/*full speed SCL high count (0x1c) */
	volatile uint32 fs_scl_lcnt;	/*full speed SCL low count(0x20) */
	volatile uint32 hs_scl_hcnt;	/*high speed SCL high count (0x24) */
	volatile uint32 hs_scl_lcnt;	/*high speed SCL low count(0x28) */
	volatile uint32 intr_stat;	/*irq status		(0x2c) */
	volatile uint32 intr_mask;	/*irq mask		(0x30) */
	volatile uint32 raw_intr_stat;	/*raw irq status	(0x34) */
	volatile uint32 rx_tl;	/*rx fifo threshold	 (0x38) */
	volatile uint32 tx_tl;	/*tx fifo threshold	 (0x3c) */
	volatile uint32 clr_intr;	/*clear all interrupts(0x40) */
	volatile uint32 clr_rx_under;	/*clear RX_UNDER irq	(0x44) */
	volatile uint32 clr_rx_over;	/*clear RX_OVER irq	 (0x48) */
	volatile uint32 clr_tx_over;	/*clear TX_OVER irq	 (0x4c) */
	volatile uint32 clr_rd_req;	/*clear RD_REQ irq	(0x50) */
	volatile uint32 clr_tx_abrt;	/*clear TX_ABRT irq	 (0x54) */
	volatile uint32 clr_rx_done;	/*clear RX_DONE irq	 (0x58) */
	volatile uint32 clr_activity;	/*clear ACTIVITY irq	(0x5c) */
	volatile uint32 clr_stop_det;	/*clear STOP_DET irq	(0x60) */
	volatile uint32 clr_start_det;	/*clear START_DET irq (0x64) */
	volatile uint32 clr_gen_call;	/*clear GEN_CALL irq	(0x68) */
	volatile uint32 enable;	/*i2c enable register (0x6c) */
	volatile uint32 status;	/*i2c status register (0x70) */
	volatile uint32 txflr;	/*tx fifo level register(0x74) */
	volatile uint32 rxflr;	/*rx fifo level register(0x78) */
	volatile uint32 reserved1;	/*reserved		(0x7c) */
	volatile uint32 tx_abrt_source;	/*tx abort status register(0x80) */
	volatile uint32 reserved2;	/*reserved		(0x84) */
	volatile uint32 dma_cr;	/*dma control register(0x88) */
	volatile uint32 dma_tdlr;	/*dma transmit data level (0x8c) */
	volatile uint32 dma_rdlr;	/*dma receive data level(0x90) */
	volatile uint32 reserved3[24];	/*reserved	 (0x94-0xf0) */
	volatile uint32 comp_param_1;	/*component parameters 1(0xf4) */
	volatile uint32 comp_version;	/*component version	 (0xf8) */
	volatile uint32 comp_type;	/*component type	(0xfc) */
};
/*****/

/****is*i2c.api/dw_i2c_instance
 *DESCRIPTION
 *this structure contains variables which relate to each individual
 *i2c instance.cumulatively, they can be thought of as the "state
 *variables" for each i2c instance (or as the global variables per
 *i2c driver instance).
 *SOURCE
 */
struct dw_i2c_instance {
	enum dw_i2c_state state;	/*i2c driver state */
	uint32 intr_mask_save;	/*saved value of interrupt mask */
	dw_callback listener;	/*user event listener */
	uint16 *b2b_buffer;	/*pointer to user tx buffer */
	uint8 *tx_buffer;	/*pointer to user tx buffer */
	uint32 tx_hold;		/*tx holding register */
	unsigned int tx_idx;	/*tx holding register index */
	unsigned int tx_length;	/*length of user tx buffer */
	unsigned int tx_remain;	/*space left in user tx buffer */
	dw_callback tx_callback;	/*user tx callback function */
	uint8 *rx_buffer;	/*pointer to user rx buffer */
	uint32 rx_hold;		/*rx holding register */
	unsigned int rx_idx;	/*rx holding register index */
	unsigned int rx_length;	/*length of user rx buffer */
	unsigned int rx_remain;	/*space left in user rx buffer */
	dw_callback rx_callback;	/*user rx callback function */
	uint8 rx_threshold;	/*default rx fifo threshold level */
	uint8 tx_threshold;	/*default tx fifo threshold level */
	bool rx_align;		/*is rx buffer 32-bit word-aligned? */
	struct dw_dma_config dma_tx;	/*DMA tx configuration */
	struct dw_dma_config dma_rx;	/*DMA rx configuration */
};
/*****/

/****if*i2c.api/dw_i2c_reset_instance
 *NAME
 *dw_i2c_reset_instance
 *USAGE
 *dw_i2c_reset_instance(i2c);
 *DESCRIPTION
 *this function resets/zeros all variables found in the
 *dw_i2c_instance structure, except for dw_i2c_statistics.
 *ARGUMENTS
 *i2c		i2c device handle
 *RETURN VALUE
 *none
 *SEE ALSO
 *dw_i2c_init(), dw_i2c_reset_statistics()
 *SOURCE
 */
void dw_i2c_reset_instance(struct dw_device *dev);
/*****/

/****if*i2c.api/dw_i2c_auto_comp_params
 *DESCRIPTION
 *this function attempts to automatically discover the hardware
 *component parameters, if this supported by the i2c in question.
 *this is usually controlled by the ADD_ENCODED_PARAMS core_consultant
 *parameter.
 *ARGUMENTS
 *i2c		i2c device handle
 *RETURN VALUE
 *0		if successful
 *-ENOSYS	function not supported
 *USES
 *accesses the following d_w_apb_i2c register/bitfield(s):
 *- ic_comp_type
 *- ic_comp_version
 *- ic_comp_param_1/all bits
 *NOTES
 *this function does not allocate any memory.an instance of
 *dw_i2c_param must already be allocated and properly referenced from
 *the relevant comp_param dw_device structure member.
 *SEE ALSO
 *dw_i2c_init()
 *SOURCE
 */
int dw_i2c_auto_comp_params(struct dw_device *dev);
/*****/

/****if*i2c.api/dw_i2c_flush_rx_hold
 *NAME
 *dw_i2c_flush_rx_hold
 *USAGE
 *ecode = dw_i2c_flush_rx_hold(i2c);
 *DESCRIPTION
 *this functions virtually flushes any data in the hold variable to
 *the buffer (both in the dw_i2c_instance structure).the 'hold'
 *variable normally stores up to four data bytes before they are
 *written to memory (i.e. the user buffer) to optimize bus performace.
 *flushing the
 *(instance->) hold variable only makes sense when the i2c is in
 *either master-receiver or slave-receiver mode.
 *ARGUMENTS
 *i2c		i2c device handle
 *RETURN VALUE
 *0		if successful
 *-EPERM	if the i2c is not in a receive mode (master-rx/slave-rx)
 *NOTES
 *this function comprises part of the interrupt-driven interface and
 *normally should never need to be called directly.the
 *dw_i2c_terminate function always calls dw_i2c_flush_rx_hold before
 *terminating a transfer.
 *SEE ALSO
 *dw_i2c_terminate()
 *SOURCE
 */
int dw_i2c_flush_rx_hold(struct dw_device *dev);
/*****/

#endif				/*DW_I2C_PRIVATE_H */

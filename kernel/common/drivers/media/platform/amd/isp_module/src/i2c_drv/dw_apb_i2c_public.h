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
 *************************************************************************
 */

#ifndef DW_APB_I2C_PUBLIC_H
#define DW_APB_I2C_PUBLIC_H

/****h*drivers.i2c/i2c.api
 *NAME
 *d_w_apb_i2c API overview
 *DESCRIPTION
 *this section gives an overview of the d_w_apb_i2c software driver
 *application programming interface (API).
 *SEE ALSO
 *i2c.data, i2c.functions
 ***/

/****h*drivers.i2c/i2c.data
 *NAME
 *d_w_apb_i2c driver data types and definitions
 *DESCRIPTION
 *this section details all the public data types and definitions used
 *with the d_w_apb_i2c software driver.
 *SEE ALSO
 *i2c.api, i2c.functions
 ***/

/****h*drivers.i2c/i2c.functions
 *NAME
 *d_w_apb_i2c driver data types and definitions
 *DESCRIPTION
 *this section details all the public functions available for use with
 *the d_w_apb_i2c software driver.
 *SEE ALSO
 *i2c.api, i2c.data
 ***/

/****h*i2c.api/i2c.data_types
 *NAME
 *d_w_apb_i2c driver data types and definitions
 *DESCRIPTION
 *enum dw_i2c_address_mode-- 7-bit or 10-bit addressing
 *enum dw_i2c_irq	 -- I2C interrupts
 *enum dw_i2c_scl_phase -- scl phase (low or high)
 *enum dw_i2c_speed_mode-- standard-, fast- or high-speed mode
 *enum dw_i2c_tx_abort	-- reasons for transmit aborts
 *enum dw_i2c_tx_mode	 -- use target address, start byte
 *protocol or general call
 *SEE ALSO
 *i2c.configuration, i2c.command, i2c.status, i2c.interrupt,
 ***/

/****h*i2c.api/i2c.configuration
 *NAME
 *d_w_apb_i2c driver configuration functions
 *DESCRIPTION
 *dw_i2c_enable_restart	-- enable restart conditions
 *dw_i2c_disable_restart -- disable restart conditions
 *dw_i2c_set_speed_mode	 -- set speed mode
 *dw_i2c_set_master_address_mode -- set master address mode
 *dw_i2c_set_slave_address_mode-- set slave address mode
 *dw_i2c_set_target_address -- set target slave address
 *dw_i2c_set_slave_address-- set slave device address
 *dw_i2c_set_tx_mode	-- set transmit transfer mode
 *dw_i2c_set_master_code	-- set high speed master code
 *dw_i2c_set_scl_count	-- set clock counts
 *dw_i2c_set_notifier_destination_ready -- set DMA tx notifier
 *dw_i2c_set_notifier_source_ready-- set DMA rx notifier
 *dw_i2c_unmask_irq	-- unmask interrupt(s)
 *dw_i2c_mask_irq	-- mask interrupt(s)
 *dw_i2c_set_tx_threshold -- set transmit FIFO threshold
 *dw_i2c_set_rx_threshold -- set receive FIFO threshold
 *dw_i2c_set_dma_tx_mode	 -- set DMA transmit mode
 *dw_i2c_set_dma_rx_mode	 -- set DMA receive mode
 *dw_i2c_set_dma_tx_level    -- set DMA transmit data level threshold
 *dw_i2c_set_dma_rx_level    -- set DMA receive data level threshold
 *SEE ALSO
 *i2c.data_types, i2c.command, i2c.status, i2c.interrupt
 ***/

/****h*i2c.api/i2c.command
 *NAME
 *d_w_apb_i2c driver command functions
 *DESCRIPTION
 *dw_i2c_init	 -- initialize device driver
 *dw_i2c_enable	 -- enable I2C
 *dw_i2c_disable	-- disable I2C
 *dw_i2c_enable_master -- enable I2C master
 *dw_i2c_disable_master-- disable I2C master
 *dw_i2c_enable_slave-- enable I2C slave
 *dw_i2c_disable_slave -- disable I2C slave
 *dw_i2c_read	 -- read byte from the receive FIFO
 *dw_i2c_write	-- write byte to the transmit FIFO
 *dw_i2c_issue_read	-- write a read command to the transmit FIFO
 *dw_i2c_clear_irq	 -- clear interrupt(s)
 *SEE ALSO
 *i2c.data_types, i2c.configuration, i2c.status, i2c.interrupt
 ***/

/****h*i2c.api/i2c.status
 *NAME
 *d_w_apb_i2c driver status functions
 *DESCRIPTION
 *dw_i2c_is_enabled            -- is device enabled?
 *dw_i2c_is_busy               -- is device busy?
 *dw_i2c_is_master_enabled      -- is master enabled?
 *dw_i2c_is_slave_enabled       -- is slave enabled?
 *dw_i2c_is_restart_enabled     -- are restart conditions enabled?
 *dw_i2c_is_tx_fifo_full         -- is transmit FIFO full?
 *dw_i2c_is_tx_fifo_empty        -- is transmit FIFO empty?
 *dw_i2c_is_rx_fifo_full         -- is receive FIFO full?
 *dw_i2c_is_rx_fifo_empty        -- is receive FIFO empty?
 *dw_i2c_is_irq_masked          -- is interrupt masked?
 *dw_i2c_is_irq_active          -- is interrupt status active?
 *dw_i2c_is_raw_irq_active       -- is raw interrupt status active?
 *dw_i2c_get_irq_mask           -- get the interrupt mask
 *dw_i2c_get_dma_tx_mode         -- get DMA transmit mode
 *dw_i2c_get_dma_rx_mode         -- get DMA receive mode
 *dw_i2c_get_dma_tx_level        -- get DMA transmit data level threshold
 *dw_i2c_get_dma_rx_level        -- get DMA receive data level threshold
 *dw_i2c_get_speed_mode         -- get speed mode
 *dw_i2c_get_tx_mode            -- get transmit transfer mode
 *dw_i2c_get_master_address_mode -- get master address mode
 *dw_i2c_get_slave_address_mode  -- get slave address mode
 *dw_i2c_get_target_address     -- get target slave address
 *dw_i2c_get_slave_address      -- get slave device address
 *dw_i2c_get_scl_count          -- get clock counts
 *dw_i2c_get_tx_threshold       -- get transmit FIFO threshold
 *dw_i2c_get_rx_threshold       -- get receive FIFO threshold
 *dw_i2c_get_tx_fifo_level       -- number of valid entries in transmit
 *FIFO
 *dw_i2c_get_rx_fifo_level       -- number of valid entries in receive
 *FIFO
 *dw_i2c_get_tx_abort_source     -- get reason for last transmit abort
 *dw_i2c_get_tx_fifo_depth       -- get transmit FIFO depth
 *dw_i2c_get_rx_fifo_depth       -- get receive FIFO depth
 *dw_i2c_get_master_code        -- get high speed master code
 *SEE ALSO
 *i2c.data_types, i2c.configuration, i2c.command, i2c.interrupt
 ***/

/****h*i2c.api/i2c.interrupt
 *NAME
 *d_w_apb_i2c driver interrupt interface functions
 *DESCRIPTION
 *dw_i2c_set_listener          -- set the user listener function
 *dw_i2c_master_back2_back      -- master back-to-back transfer
 *dw_i2c_master_transmit       -- master-transmitter transfer
 *dw_i2c_master_receive        -- master-receiver transfer
 *dw_i2c_slave_transmit        -- slave-transmitter transfer
 *dw_i2c_slave_bulk_transmit    -- slave bulk transmit transfer
 *dw_i2c_slave_receive         -- slave-receiver transfer
 *dw_i2c_terminate            -- terminate current transfer(s)
 *dw_i2c_irq_handler           -- I2C interrupt handler
 *dw_i2c_user_irq_handler       -- minimal I2C interrupt handler
 *SEE ALSO
 *i2c.data_types, i2c.configuration, i2c.command, i2c.status
 ***/

/****d*i2c.data/dw_i2c_irq
 *DESCRIPTION
 *this is the data type used for specifying I2C interrupts.  one of
 *these is passed at a time to the user listener function which should
 *deal with it accordingly.  the exceptions to this, which are handled
 *
 *by the driver, are: i2c_irq_tx_empty, i2c_irq_rx_done, and
 *i2c_irq_all.
 *NOTES
 *this data type relates to the following register bit field(s):
 *- ic_intr_mask/all bits
 *- ic_intr_stat/all bits
 *- ic_raw_intr_stat/all bits
 *- ic_clr_intr/clr_intr
 *- ic_clr_rx_under/clr_rx_under
 *- ic_clr_rx_over/clr_rx_over
 *- ic_clr_tx_over/clr_tx_over
 *- ic_clr_rd_req/clr_rd_req
 *- ic_clr_tx_abrt/clr_tx_abrt
 *- ic_clr_rx_done/clr_rx_done
 *- ic_clr_activity/clr_activity
 *- ic_clr_stop_det/clr_stop_det
 *- ic_clr_start_det/clr_start_det
 *- ic_clr_gen_call/clr_gen_call
 *SEE ALSO
 *dw_i2c_unmask_irq(), dw_i2c_mask_irq(), dw_i2c_clear_irq(),
 *dw_i2c_is_irq_masked(), dw_i2c_set_listener()
 *SOURCE
 */
enum dw_i2c_irq {
	i2c_irq_none = 0x000,	/*specifies no interrupt */
	i2c_irq_rx_under = 0x001,/*set if the processor attempts to read */
	/*the receive FIFO when it is empty. */
	i2c_irq_rx_over = 0x002,	/*set if the receive FIFO was */
	/*completely filled and more data */
	/*arrived.  that data is lost. */
	i2c_irq_rx_full = 0x004,/*set when the transmit FIFO reaches or */
	/*goes above the receive FIFO */
	/*threshold. it is automatically */
	/*cleared by hardware when the receive */
	/*FIFO level goes below the threshold. */
	i2c_irq_tx_over = 0x008,/*set during transmit if the transmit */
	/*FIFO is filled and the processor */
	/*attempts to issue another I2C command */
	/*(read request or write). */
	i2c_irq_tx_empty = 0x010,/*set when the transmit FIFO is at or */
	/*below the transmit FIFO threshold */
	/*level. it is automatically cleared by */
	/*hardware when the transmit FIFO level */
	/*goes above the threshold. */
	i2c_irq_rd_req = 0x020,	/*set when the I2C is acting as a slave */
	/*and another I2C master is attempting */
	/*to read data from the slave. */
	i2c_irq_tx_abrt = 0x040,/*in general, this is set when the I2C */
	/*acting as a master is unable to */
	/*complete a command that the processor */
	/*has sent. */
	i2c_irq_rx_done = 0x080,	/*when the I2C is acting as a */
	/*slave-transmitter, this is set if the */
	/*master does not acknowledge a */
	/*transmitted byte. this occurs on the */
	/*last byte of the transmission, */
	/*indicating that the transmission is */
	/*done. */
	i2c_irq_activity = 0x100,/*this is set whenever the I2C is busy */
	/*(reading from or writing to the I2C */
	/*bus). */
	i2c_irq_stop_det = 0x200,	/*indicates whether a stop condition */
	/*has occurred on the I2C bus. */
	i2c_irq_start_det = 0x400,	/*indicates whether a start condition */
	/*has occurred on the I2C bus. */
	i2c_irq_gen_call = 0x800,/*indicates that a general call request */
	/*was received. the I2C stores the */
	/*received data in the receive FIFO. */
	i2c_irq_all = 0xfff	/*specifies all I2C interrupts.  this */
	/*combined enumeration that can be */
	/*used with some functions such as */
	/*dw_i2c_clear_irq(), dw_i2c_mask_irq(), */
	/*and so on. */
};

/*****/

/****d*i2c.data/dw_i2c_tx_abort
 *DESCRIPTION
 *this is the data type used for reporting one or more transmit
 *transfer aborts.the value returned by dw_i2c_get_tx_abort_source()
 *should be compared to these enumerations to determine what caused
 *the last transfer to abort.
 *NOTES
 *this data type relates to the following register bit field(s):
 *- ic_tx_abrt_source/all bits
 *
 *if i2c_abrt_slv_arblost is true, i2c_abrt_arb_lost is also
 *true.the way to distinguish between slave and master arbitration
 *loss is to first check i2c_abrt_slv_arblost (slave arbitration loss)
 *and then check i2c_abrt_arb_lost (master or slave arbitration loss).
 *SEE ALSO
 *dw_i2c_get_tx_abort_source()
 *SOURCE
 */
enum dw_i2c_tx_abort {
	/*master in 7-bit address mode and the address sent was not */
	/*acknowledged by any slave. */
	i2c_abrt_7b_addr_noack = 0x0001,
	/*master in 10-bit address mode and the first address byte of the */
	/*10-bit address was not acknowledged by the slave. */
	i2c_abrt_10addr1_noack = 0x0002,
	/*master in 10-bit address mode and the second address byte of the */
	/*10-bit address was not acknowledged by the slave. */
	i2c_abrt_10addr2_noack = 0x0004,
	/*master has received an acknowledgment for the address, but when */
	/*it sent data byte(s) following the address, it did not receive */
	/*and acknowledge from the remote slave(s). */
	i2c_abrt_txdata_noack = 0x0008,
	/*master sent a general call address and no slave on the bus */
	/*responded with an ack. */
	i2c_abrt_gcall_noack = 0x0010,
	/*master sent a general call but the user tried to issue a read */
	/*following this call. */
	i2c_abrt_gcall_read = 0x0020,
	/*master is in high-speed mode and the high speed master code was */
	/*acknowledged (wrong behavior). */
	i2c_abrt_hs_ackdet = 0x0040,
	/*master sent a start byte and the start byte was acknowledged */
	/*(wrong behavior). */
	i2c_abrt_sbyte_ackdet = 0x0080,
	/*the restart is disabled and the user is trying to use the master */
	/*to send data in high speed mode. */
	i2c_abrt_hs_norstrt = 0x0100,
	/*the restart is disabled and the user is trying to send a start */
	/*byte. */
	i2c_abrt_sbyte_norstrt = 0x0200,
	/*the restart is disabled and the master sends a read command in */
	/*the 10-bit addressing mode. */
	i2c_abrt_10b_rd_norstrt = 0x0400,
	/*user attempted to use disabled master. */
	i2c_abrt_master_dis = 0x0800,
	/*arbitration lost. */
	i2c_abrt_arb_lost = 0x1000,
	/*slave has received a read command and some data exists in the */
	/*transmit FIFO so that the slave issues a TX_ABRT to flush old */
	/*data in the transmit FIFO. */
	i2c_abrt_slvflush_txfifo = 0x2000,
	/*slave lost bus while it is transmitting data to a remote master. */
	i2c_abrt_slv_arblost = 0x5000,
	/*slave requests data to transfer and the user issues a read. */
	i2c_abrt_slvrd_intx = 0x8000
};
/*****
 *note that i2c_abrt_slv_arblost represents two bits.this is because
 *of the hardware implementaion of the transmit abort status register.
 *****/

/****d*i2c.data/dw_i2c_address_mode
 *DESCRIPTION
 *this is the data type used for specifying the addressing mode used
 *for transfers.an I2C master begins all transfer with the
 *specified addressing mode.an I2C slave only responds to
 *transfers of the same type as its addressing mode.
 *NOTES
 *this data type relates to the following register bit field(s):
 *- ic_con/ic_10bitaddr_slave
 *- ic_con/ic_10bitaddr_master
 *SEE ALSO
 *dw_i2c_set_master_address_mode(), dw_i2c_get_master_address_mode(),
 *dw_i2c_set_slave_address_mode(), dw_i2c_get_slave_address_mode()
 *SOURCE
 */
enum dw_i2c_address_mode {
	i2c_7bit_address = 0x0,	/*7-bit address mode.only the 7 l_s_bs */
	/*of the slave and/or target address */
	/*are relevant. */
	i2c_10bit_address = 0x1	/*10-bit address mode.the 10 l_s_bs of */
	/*the slave and/or target address are */
	/*relevant. */
};
/*****/

/****d*i2c.data/dw_i2c_speed_mode
 *DESCRIPTION
 *this is the data type used for setting and getting the speed mode.
 *it is also used when specifying the scl count values.
 *NOTES
 *this data type relates to the following register bit field(s):
 *- ic_con/speed
 *- ic_ss_scl_lcnt/ic_ss_scl_lcnt
 *- ic_ss_scl_hcnt/ic_ss_scl_hcnt
 *- ic_fs_scl_lcnt/ic_fs_scl_lcnt
 *- ic_fs_scl_hcnt/ic_fs_scl_hcnt
 *- ic_hs_scl_lcnt/ic_hs_scl_lcnt
 *- ic_hs_scl_hcnt/ic_hs_scl_hcnt
 *SEE ALSO
 *dw_i2c_set_speed_mode(), dw_i2c_get_speed_mode(), dw_i2c_set_scl_count(),
 *dw_i2c_get_scl_count()
 *SOURCE
 */
enum dw_i2c_speed_mode {
	i2c_speed_standard = 0x1,	/*standard speed (100 kbps) */
	i2c_speed_fast = 0x2,	/*fast speed (400 kbps) */
	i2c_speed_high = 0x3	/*high speed (3400 kbps) */
};
/*****/

/****d*i2c.data/dw_i2c_tx_mode
 *DESCRIPTION
 *this is the data type used for specifying what type of the transfer
 *is initiated upon the next write to the transmit FIFO.there are
 *three possible types of transfers that may be initiated by an I2C
 *master:
 *
 *- start condition followed by the programmed target address.
 *- start byte protocol. this is identical to the start condition
 *except that a start byte is issued before the target address.
 *- general call. addresses every slave attached to the I2C bus.
 *NOTES
 *this data type relates to the following register bit field(s):
 *- ic_tar/gc_or_start
 *- ic_tar/special
 *SEE ALSO
 *dw_i2c_set_tx_mode(), dw_i2c_get_tx_mode(), dw_i2c_set_target_address()
 *SOURCE
 */
enum dw_i2c_tx_mode {
	i2c_tx_target = 0x0,	/*normal transfer using target address */
	i2c_tx_gen_call = 0x2,	/*issue a general call */
	i2c_tx_start_byte = 0x3	/*issue a start byte I2C command */
};
/*****/

/****d*i2c.data/dw_i2c_scl_phase
 *DESCRIPTION
 *this is the data type used for specifying whether the high or low
 *count of the scl clock for whatever speed is being read/modified.
 *NOTES
 *this data type relates to the following register bit field(s):
 *- ic_ss_scl_lcnt/ic_ss_scl_lcnt
 *- ic_ss_scl_hcnt/ic_ss_scl_hcnt
 *- ic_fs_scl_lcnt/ic_fs_scl_lcnt
 *- ic_fs_scl_hcnt/ic_fs_scl_hcnt
 *- ic_hs_scl_lcnt/ic_hs_scl_lcnt
 *- ic_hs_scl_hcnt/ic_hs_scl_hcnt
 *SEE ALSO
 *dw_i2c_set_scl_count(), dw_i2c_get_scl_count()
 *SOURCE
 */
enum dw_i2c_scl_phase {
	i2c_scl_low = 0x0,	/*SCL clock count low phase */
	i2c_scl_high = 0x1	/*SCL clock count high phase */
};
/*****/

/****f*i2c.functions/dw_i2c_init
 *DESCRIPTION
 *this function initializes the I2C driver.it disables and
 *clears all interrupts, sets the DMA mode to software handshaking,
 *sets the DMA transmit and receive notifier function pointers to NULL
 *and disables the I2C.it also attempts to determine the hardware
 *parameters of the device, if supported by the device.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *0		-- if successful
 *-DW_ENOSYS	-- if hardware parameters for the device could not be
 *automatically determined
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *- ic_clr_intr/clr_intr
 *- ic_rx_tl/rx_tl
 *- ic_enable/enable
 *
 *this function is affected by the ADD_ENCODED_PARAMS hardware
 *parameter. if set to false, it is necessary for the user to create
 *an appropriate dw_i2c_param structure as part of the dw_device
 *structure. if set to true, dw_i2c_init() will automatically
 *initialize this structure (space for which must have been already
 *allocated).
 *SOURCE
 */
int dw_i2c_init(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_enable
 *DESCRIPTION
 *this function enables the I2C.
 *ARGUMENTS
 *dev	-- d_w_apb_i2c device handle
 *RETURN VALUE
 *none
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_enable/enable
 *SEE ALSO
 *dw_i2c_disable(), dw_i2c_is_enabled()
 *SOURCE
 */
void dw_i2c_enable(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_disable
 *DESCRIPTION
 *this functions disables the I2C, if it is not busy (determined by
 *the activity interrupt bit).the I2C should not be disabled during
 *interrupt-driven transfers as the resulting driver behavior is
 *undefined.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *0		-- if successful
 *-DW_EBUSY	-- if the I2C is busy
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_enable/enable
 *SEE ALSO
 *dw_i2c_enable(), dw_i2c_is_enabled()
 *SOURCE
 */
int dw_i2c_disable(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_enabled
 *DESCRIPTION
 *this function returns whether the I2C is enabled or not.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- the I2C is enabled
 *false		-- the I2C is disabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_enable/enable
 *SEE ALSO
 *dw_i2c_enable(), dw_i2c_disable()
 *SOURCE
 */
bool dw_i2c_is_enabled(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_busy
 *DESCRIPTION
 *this function returns whether the I2C is busy (transmitting
 *or receiving) or not.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- the I2C device is busy
 *false		-- the I2C device is not busy
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_status/activity
 *SOURCE
 */
bool dw_i2c_is_busy(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_speed_mode
 *DESCRIPTION
 *this function sets the speed mode used for I2C transfers.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *mode		-- the speed mode to set
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *-DW_ENOSYS	-- if the specified speed is not supported
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/speed
 *
 *the I2C must be disabled in order to change the speed mode.
 *this function is affected by the MAX_SPEED_MODE hardware parameter.
 *it is not possible to set the speed higher than the maximum speed
 *mode.
 *SEE ALSO
 *dw_i2c_get_speed_mode(), enum dw_i2c_speed_mode
 *SOURCE
 */
int dw_i2c_set_speed_mode(struct dw_device *dev, enum dw_i2c_speed_mode mode);
/*****/

/****f*i2c.functions/dw_i2c_get_speed_mode
 *DESCRIPTION
 *this function returns the speed mode currently in use by the I2C.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current I2C speed mode.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/speed
 *SEE ALSO
 *dw_i2c_set_speed_mode(), enum dw_i2c_speed_mode
 *SOURCE
 */
enum dw_i2c_speed_mode dw_i2c_get_speed_mode(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_master_address_mode
 *DESCRIPTION
 *this function sets the master addressing mode (7-bit or 10-bit).
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *mode		-- the addressing mode to set
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_10bitaddr_master
 *
 *the I2C must be disabled in order to change the master addressing
 *mode.
 *SEE ALSO
 *dw_i2c_get_master_address_mode(), enum dw_i2c_address_mode
 *SOURCE
 */
int dw_i2c_set_master_address_mode(struct dw_device *dev, enum
			dw_i2c_address_mode mode);
/*****/

/****f*i2c.functions/dw_i2c_get_master_address_mode
 *DESCRIPTION
 *this function returns the current master addressing mode (7-bit or
 * 10-bit).
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current master addressing mode.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_10bitaddr_master
 *SEE ALSO
 *dw_i2c_set_slave_address_mode(), enum dw_i2c_address_mode
 *SOURCE
 */
enum dw_i2c_address_mode dw_i2c_get_master_address_mode(struct dw_device
							*dev);
/*****/

/****f*i2c.functions/dw_i2c_set_slave_address_mode
 *DESCRIPTION
 *this function sets the I2C slave addressing mode (7-bit or 10-bit).
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *mode		-- the addressing mode to set
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_10bitaddr_slave
 *
 *the I2C must be disabled in order to change the slave addressing
 *mode.
 *SEE ALSO
 *dw_i2c_get_slave_address_mode(), enum dw_i2c_address_mode
 *SOURCE
 */
int dw_i2c_set_slave_address_mode(struct dw_device *dev, enum
				dw_i2c_address_mode mode);
/*****/

/****f*i2c.functions/dw_i2c_get_slave_address_mode
 *DESCRIPTION
 *this function returns the current slave addressing mode (7-bit or
 *10-bit).
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current slave addressing mode.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_10bitaddr_slave
 *SEE ALSO
 *dw_i2c_set_slave_address_mode(), enum dw_i2c_address_mode
 *SOURCE
 */
enum dw_i2c_address_mode dw_i2c_get_slave_address_mode(struct dw_device
						*dev);
/*****/

/****f*i2c.functions/dw_i2c_enable_slave
 *DESCRIPTION
 *this function enables the I2C slave.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_slave_disable
 *
 *the I2C must be disabled in order to enable the slave.
 *SEE ALSO
 *dw_i2c_disable_slave(), dw_i2c_is_slave_enabled()
 *SOURCE
 */
int dw_i2c_enable_slave(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_disable_slave
 *DESCRIPTION
 *this function disables the I2C slave.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_slave_disable
 *
 *the I2C must be disabled in order to disable the slave.
 *SEE ALSO
 *dw_i2c_enable_slave(), dw_i2c_is_slave_enabled()
 *SOURCE
 */
int dw_i2c_disable_slave(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_slave_enabled
 *DESCRIPTION
 *this function returns whether the I2C slave is enabled or not.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- slave is enabled
 *false		-- slave is disabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_slave_disable
 *SEE ALSO
 *dw_i2c_enable_slave(), dw_i2c_disable_slave()
 *SOURCE
 */
bool dw_i2c_is_slave_enabled(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_enable_master
 *DESCRIPTION
 *this function enables the I2C master.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/master_mode
 *
 *the I2C must be disabled in order to enable the master.
 *SEE ALSO
 *dw_i2c_disable_master(), dw_i2c_is_master_enabled()
 *SOURCE
 */
int dw_i2c_enable_master(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_disable_master
 *DESCRIPTION
 *this function disables the I2C master.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/master_mode
 *
 *the I2C must be disabled in order to disable the master.
 *SEE ALSO
 *dw_i2c_enable_master(), dw_i2c_is_master_enabled()
 *SOURCE
 */
int dw_i2c_disable_master(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_master_enabled
 *DESCRIPTION
 *this function returns whether the I2C master is enabled or not.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- master is enabled
 *false		-- master is disabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/master_mode
 *SEE ALSO
 *dw_i2c_enable_master(), dw_i2c_disable_master()
 *SOURCE
 */
bool dw_i2c_is_master_enabled(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_enable_restart
 *DESCRIPTION
 *this function enables the use of restart conditions.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_restart_en
 *
 *the I2C must be disabled in order to enable restart conditions.
 *SEE ALSO
 *dw_i2c_disable_restart()
 *SOURCE
 */
int dw_i2c_enable_restart(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_disable_restart
 *DESCRIPTION
 *this function disables the use of restart conditions.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_restart_en
 *
 *the I2C must be disabled in order to disable restart conditions.
 *SEE ALSO
 *dw_i2c_enable_restart()
 *SOURCE
 */
int dw_i2c_disable_restart(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_restart_enabled
 *DESCRIPTION
 *this function returns whether restart conditions are currently in
 *use or not by the I2C.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- restart conditions are enabled
 *false		-- restart conditions are disabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_con/ic_restart_en
 *SEE ALSO
 *dw_i2c_enable_restart(), dw_i2c_disable_restart()
 *SOURCE
 */
bool dw_i2c_is_restart_enabled(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_target_address
 *DESCRIPTION
 *this function sets the target address used by the I2C master.when
 *not issuing a general call or using a start byte, this is the
 *address the master uses when performing transfers over the I2C bus.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *address	-- target address to set
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_tar/ic_tar
 *
 *the I2C must be disabled in order to set the target address.only
 *the 10 least significant bits of the address are relevant.
 *SEE ALSO
 *dw_i2c_get_target_address()
 *SOURCE
 */
int dw_i2c_set_target_address(struct dw_device *dev, uint16 address);
/*****/

/****f*i2c.functions/dw_i2c_get_target_address
 *DESCRIPTION
 *this function returns the current target address in use by the I2C
 *master.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current target address.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_tar/ic_tar
 *SEE ALSO
 *dw_i2c_set_target_address()
 *SOURCE
 */
uint16 dw_i2c_get_target_address(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_slave_address
 *DESCRIPTION
 *this function sets the slave address to which the I2C slave
 *responds, when enabled.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *address	-- slave address to set
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_sar/ic_sar
 *
 *the I2C must be disabled in order to set the target address.only
 *the 10 least significant bits of the address are relevant.
 *SEE ALSO
 *dw_i2c_get_slave_address()
 *SOURCE
 */
int dw_i2c_set_slave_address(struct dw_device *dev, uint16 address);
/*****/

/****f*i2c.functions/dw_i2c_get_slave_address
 *DESCRIPTION
 *this function returns the current address in use by the I2C slave.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current I2C slave address.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_sar/ic_sar
 *SEE ALSO
 *dw_i2c_set_slave_address()
 *SOURCE
 */
uint16 dw_i2c_get_slave_address(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_tx_mode
 *DESCRIPTION
 *this function sets the master transmit mode.that is, whether to
 *use a start byte, general call, or the programmed target address.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *mode		-- transfer mode to set
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_tar/special
 *- ic_tar/gc_or_start
 *
 *the I2C must be disabled in order to set the master transmit mode.
 *SEE ALSO
 *dw_i2c_get_tx_mode(), enum dw_i2c_tx_mode
 *SOURCE
 */
int dw_i2c_set_tx_mode(struct dw_device *dev, enum dw_i2c_tx_mode mode);
/*****/

/****f*i2c.functions/dw_i2c_get_tx_mode
 *DESCRIPTION
 *this function returns the current transmit mode in use by an I2C
 *master.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current master transmit mode.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_tar/special
 *- ic_tar/gc_or_start
 *SEE ALSO
 *dw_i2c_set_tx_mode(), enum dw_i2c_tx_mode
 *SOURCE
 */
enum dw_i2c_tx_mode dw_i2c_get_tx_mode(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_master_code
 *DESCRIPTION
 *this function sets the master code, used during high-speed mode
 *transfers.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *code		-- master code to set
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_hs_maddr/ic_hs_mar
 *
 *the I2C must be disabled in order to set the high-speed mode master
 *code.
 *SEE ALSO
 *dw_i2c_get_master_code()
 *SOURCE
 */
int dw_i2c_set_master_code(struct dw_device *dev, uint8 code);
/*****/

/****f*i2c.functions/dw_i2c_get_master_code
 *DESCRIPTION
 *initializes an I2C peripheral.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current high-speed mode master code.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_hs_maddr/ic_hs_mar
 *SEE ALSO
 *dw_i2c_set_master_code()
 *SOURCE
 */
uint8 dw_i2c_get_master_code(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_scl_count
 *DESCRIPTION
 *this function set the scl count value for a particular speed mode
 *(standard, fast, high) and clock phase (low, high).
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *mode		-- speed mode of count value to set
 *phase		-- scl phase of count value to set
 *value		-- count value to set
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPERM	-- if the I2C is enabled
 *-DW_ENOSYS	-- if the scl count registers are hardcoded
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_ss_scl_lcnt/ic_ss_scl_lcnt
 *- ic_ss_scl_hcnt/ic_ss_scl_hcnt
 *- ic_fs_scl_lcnt/ic_fs_scl_lcnt
 *- ic_fs_scl_hcnt/ic_fs_scl_hcnt
 *- ic_hs_scl_lcnt/ic_hs_scl_lcnt
 *- ic_hs_scl_hcnt/ic_hs_scl_hcnt
 *
 *the I2C must be disabled in order to set any of the scl count
 *values.the minimum programmable value for any of these registers
 *is 6.
 *this function is affected by the HC_COUNT_VALUES hardware parameter.
 *SEE ALSO
 *dw_i2c_get_scl_count(), enum dw_i2c_speed_mode, enum dw_i2c_scl_phase
 *SOURCE
 */
int dw_i2c_set_scl_count(struct dw_device *dev, enum dw_i2c_speed_mode
			 mode, enum dw_i2c_scl_phase phase, uint16 value);
/*****/

/****f*i2c.functions/dw_i2c_get_scl_count
 *DESCRIPTION
 *this function returns the current scl count value for all speed
 *modes (standard, fast, high) and phases (low, high).
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *mode		-- speed mode to get count value of
 *phase		-- scl phase to get count value of
 *RETURN VALUE
 *the current specified scl count value.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_ss_scl_lcnt/ic_ss_scl_lcnt
 *- ic_ss_scl_hcnt/ic_ss_scl_hcnt
 *- ic_fs_scl_lcnt/ic_fs_scl_lcnt
 *- ic_fs_scl_hcnt/ic_fs_scl_hcnt
 *- ic_hs_scl_lcnt/ic_hs_scl_lcnt
 *- ic_hs_scl_hcnt/ic_hs_scl_hcnt
 *
 *this function returns 0x0000 for any non-existent scl count
 *registers.
 *SEE ALSO
 *dw_i2c_set_scl_count(), enum dw_i2c_speed_mode, enum dw_i2c_scl_phase
 *SOURCE
 */
uint16 dw_i2c_get_scl_count(struct dw_device *dev, enum
				dw_i2c_speed_mode mode,
				enum dw_i2c_scl_phase phase);
/*****/

/****f*i2c.functions/dw_i2c_read
 *DESCRIPTION
 *this function reads a single byte from the I2C receive FIFO.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the character read from the I2C FIFO
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_data_cmd/dat
 *
 *this function also does not check whether there is valid data or not
 *in the FIFO beforehand.as such, it can cause an receive underflow
 *error if used improperly.
 *SEE ALSO
 *dw_i2c_write(), dw_i2c_issue_read(), dw_i2c_master_receive(),
 *dw_i2c_slave_receive()
 *SOURCE
 */
uint8 dw_i2c_read(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_write
 *DESCRIPTION
 *this function writes a single byte to the I2C transmit FIFO.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *datum		-- byte to write to FIFO
 *RETURN VALUE
 *none
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_data_cmd/dat
 *
 *this function does not check whether the I2C transmit FIFO is full
 *or not beforehand.as such, it can cause a transmit overflow error
 *if used improperly.
 *SEE ALSO
 *dw_i2c_read(), dw_i2c_issue_read(), dw_i2c_master_transmit(),
 *dw_i2c_slave_transmit()
 *SOURCE
 */
void dw_i2c_write(struct dw_device *dev, uint16 datum);
/*****/

/****f*i2c.functions/dw_i2c_issue_read
 *DESCRIPTION
 *this function writes a read command to the I2C transmit FIFO.this
 *is used during master-receiver/slave-transmitter transfers and is
 *typically followed by a read from the master receive FIFO after the
 *slave responds with data.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *none
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_data_cmd/cmd
 *
 *this function does not check whether the I2C FIFO is full or not
 *before writing to it.as such, it can result in a transmit FIFO
 *overflow if used improperly.
 *SEE ALSO
 *dw_i2c_read(), dw_i2c_write(), dw_i2c_master_receive()
 *SOURCE
 */
void dw_i2c_issue_read(struct dw_device *dev, uint16 stop, uint16 restart);
/*****/

/****f*i2c.functions/dw_i2c_get_tx_abort_source
 *DESCRIPTION
 *this function returns the current value of the I2C transmit abort
 *status register.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current transmit abort status.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_tx_abrt_source/all bits
 *
 *the transmit abort status register is cleared by the I2C upon
 *reading.note that it is possible for more than one bit of this
 *register to be active simultaneously and this should be dealt with
 *properly by any function operating on this return value.
 *SEE ALSO
 *enum dw_i2c_tx_abort
 *SOURCE
 */
enum dw_i2c_tx_abort dw_i2c_get_tx_abort_source(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_get_tx_fifo_depth
 *DESCRIPTION
 *returns how many bytes deep theI2C transmit FIFO is.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *FIFO depth in bytes (from 2 to 256)
 *NOTES
 *this function is affected by the TX_BUFFER_DEPTH hardware parameter.
 *SEE ALSO
 *dw_i2c_get_rx_fifo_depth(), dw_i2c_get_tx_fifo_level()
 *SOURCE
 */
uint16 dw_i2c_get_tx_fifo_depth(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_get_rx_fifo_depth
 *DESCRIPTION
 *returns how many bytes deep the I2C transmit FIFO is.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *FIFO depth in bytes (from 2 to 256)
 *NOTES
 *this function is affected by the RX_BUFFER_DEPTH hardware parameter.
 *SEE ALSO
 *dw_i2c_get_tx_fifo_depth(), dw_i2c_get_rx_fifo_level()
 *SOURCE
 */
uint16 dw_i2c_get_rx_fifo_depth(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_tx_fifo_full
 *DESCRIPTION
 *returns whether the transmitter FIFO is full or not.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- the transmit FIFO is full
 *false		-- the transmit FIFO is not full
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_status/tfnf
 *SEE ALSO
 *dw_i2c_is_tx_fifo_empty()
 *SOURCE
 */
bool dw_i2c_is_tx_fifo_full(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_tx_fifo_empty
 *DESCRIPTION
 *returns whether the transmitter FIFO is empty or not.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- the transmit FIFO is empty
 *false		-- the transmit FIFO is not empty
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_status/tfe
 *SEE ALSO
 *dw_i2c_is_tx_fifo_full()
 *SOURCE
 */
bool dw_i2c_is_tx_fifo_empty(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_rx_fifo_full
 *DESCRIPTION
 *this function returns whether the receive FIFO is full or not.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- the receive FIFO is full
 *false		-- the receive FIFO is not full
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_status/rff
 *SEE ALSO
 *dw_i2c_is_rx_fifo_empty()
 *SOURCE
 */
bool dw_i2c_is_rx_fifo_full(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_rx_fifo_empty
 *DESCRIPTION
 *this function returns whether the receive FIFO is empty or not.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- the receive FIFO is empty
 *false		-- the receive FIFO is not empty
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_status/rfne
 *SEE ALSO
 *dw_i2c_is_rx_fifo_full()
 *SOURCE
 */
bool dw_i2c_is_rx_fifo_empty(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_get_tx_fifo_level
 *DESCRIPTION
 *this function returns the number of valid data entries currently
 *present in the transmit FIFO.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *number of valid data entries in the transmit FIFO.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_txflr/txflr
 *SEE ALSO
 *dw_i2c_get_rx_fifo_level(), dw_i2c_is_tx_fifo_full(),
 *dw_i2c_is_tx_fifo_empty(), dw_i2c_set_tx_threshold(),
 *dw_i2c_get_tx_threshold()
 *SOURCE
 */
uint16 dw_i2c_get_tx_fifo_level(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_get_rx_fifo_level
 *DESCRIPTION
 *this function returns the number of valid data entries currently
 *present in the receiver FIFO.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *number of valid data entries in the receive FIFO.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_rxflr/rxflr
 *SEE ALSO
 *dw_i2c_get_tx_fifo_level(), dw_i2c_is_rx_fifo_full(),
 *dw_i2c_is_rx_fifo_empty(), dw_i2c_set_rx_threshold(),
 *dw_i2c_get_rx_threshold()
 *SOURCE
 */
uint16 dw_i2c_get_rx_fifo_level(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_tx_threshold
 *DESCRIPTION
 *this function sets the threshold level for the transmit FIFO.when
 *the number of data entries in the transmit FIFO is at or below this
 *level, the tx_empty interrupt is triggered.if an interrupt-driven
 *transfer is already in progress, the transmit threshold level is not
 *updated until the end of the transfer.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *level		-- level at which to set threshold
 *RETURN VALUE
 *0		-- if successful
 *-DW_EINVAL	-- if the level specified is greater than the transmit
 *FIFO depth; the threshold is set to the transmit FIFO
 *depth.
 *-DW_EBUSY	-- if an interrupt-driven transfer is currently in
 *progress; the requested level will be written to the
 *transmit threshold register when the current transfer
 *completes.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_tx_tl/tx_tl
 *
 *the driver keeps a copy of the last transmit threshold specified by
 *the user as it manipulates the transmit threshold level at the end
 *of interrupt-driven transmit transfers.this copy is used to
 *restore the transmit threshold upon completion of a transmit
 *transfer.
 *SEE ALSO
 *dw_i2c_get_tx_threshold(), dw_i2c_set_rx_threshold(),
 *dw_i2c_get_rx_threshold(), dw_i2c_get_tx_fifo_level()
 *SOURCE
 */
int dw_i2c_set_tx_threshold(struct dw_device *dev, uint8 level);
/*****/

/****f*i2c.functions/dw_i2c_get_tx_threshold
 *DESCRIPTION
 *this function returns the current threshold level for the transmit
 *FIFO.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the transmit FIFO threshold level.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_tx_tl/tx_tl
 *SEE ALSO
 *dw_i2c_set_tx_threshold(), dw_i2c_set_rx_threshold(),
 *dw_i2c_get_rx_threshold(), dw_i2c_get_tx_fifo_level()
 *SOURCE
 */
uint8 dw_i2c_get_tx_threshold(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_rx_threshold
 *DESCRIPTION
 *this function sets the threshold level for the receive FIFO.when
 *the number of data entries in the receive FIFO is at or above this
 *level, the rx_full interrupt is triggered.if an interrupt-driven
 *transfer is already in progress, the receive threshold level is not
 *updated until the end of the transfer.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *level		-- level at which to set threshold
 *RETURN VALUE
 *0		-- if successful
 *-DW_EINVAL	-- if the level specified is greater than the receive
 *FIFO depth, the threshold is set to the receive FIFO
 *depth.
 *-DW_EBUSY	-- if an interrupt-driven transfer is currently in
 *progress, the requested level is written to the
 *receive threshold register when the current transfer
 *completes.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_rx_tl/rx_tl
 *
 *the driver keeps a copy of the last receive threshold specified by
 *the user as it manipulates the receive threshold level at the end of
 *interrupt-driven receive transfers.this copy is used to restore
 *the receive threshold upon completion of an receive transfer.
 *WARNINGS
 *when this function is called, if the following slave receive
 *transfer is less than the threshold set, the rx_full interrupt is
 *not immediately triggered.
 *SEE ALSO
 *dw_i2c_get_rx_threshold(), dw_i2c_set_tx_threshold(),
 *dw_i2c_get_tx_threshold(), dw_i2c_terminate(), dw_i2c_get_rx_fifo_level()
 *SOURCE
 */
int dw_i2c_set_rx_threshold(struct dw_device *dev, uint8 level);
/*****/

/****f*i2c.functions/dw_i2c_get_rx_threshold
 *DESCRIPTION
 *this function returns the current threshold level for the receive
 *FIFO.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the receive FIFO threshold level.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_rx_tl/rx_tl
 *
 *it is possible that the returned value is not equal to what the user
 *has previously set the receive threshold to be.this is because the
 *driver manipulates the threshold value in order to complete receive
 *transfers.the previous user-specified threshold is restored upon
 *completion of each transfer.
 *SEE ALSO
 *dw_i2c_set_rx_threshold(), dw_i2c_set_tx_threshold(),
 *dw_i2c_get_tx_threshold(), dw_i2c_get_rx_fifo_level()
 *SOURCE
 */
uint8 dw_i2c_get_rx_threshold(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_listener
 *DESCRIPTION
 *this function is used to set a user listener function.the listener
 *function is responsible for handling all interrupts that are not
 *handled by the driver kit interrupt handler.this encompasses all
 *error interrupts, general calls, read requests, and receive full
 *when no receive buffer is available.there is no need to clear any
 *interrupts in the listener as this is handled automatically by the
 *driver kit interrupt handlers.
 *
 *A listener must be setup up before using any of the other functions
 *of the interrupt API.note that if the dw_i2c_user_irq_handler
 *interrupt handler is being used, none of the other interrupt API
 *functions can be used with it.this is because they are symbiotic
 *with the dw_i2c_irq_handler() interrupt handler.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *listener	-- function pointer to user listener function
 *RETURN VALUE
 *none
 *NOTES
 *this function enables the following interrupts: i2c_irq_rx_under,
 *i2c_irq_rx_over, i2c_irq_tx_over, i2c_irq_rd_req, i2c_irq_tx_abrt,
 *i2c_irq_rx_done, and i2c_irq_gen_call.it also enables
 *i2c_irq_rx_full is the dma_rx_mode is not set to hardware handshaking.
 *the dw_callback function pointer typedef is defined in the common
 *header files.
 *
 *whether the dw_i2c_user_irq_handler() or dw_i2c_irq_handler() interrupt
 *handler is being used, this function is used to set the user
 *listener function that is called by both of them.
 *EXAMPLE
 *in the case of new data being received, the irq handler
 *(dw_i2c_irq_handler) would call the user listener function as
 *follows:
 *
 *user_listener(dev, i2c_irq_rx_full);
 *
 *it is the listener function's responsibility to properly handle
 *this. for example:
 *
 *dw_i2c_slave_receive(dev, buffer, length, callback);
 *
 *SEE ALSO
 *dw_i2c_set_dma_rx_mode(), dw_i2c_user_irq_hanler(), dw_i2c_irq_handler(),
 *dw_i2c_irq, dw_callback
 *SOURCE
 */
void dw_i2c_set_listener(struct dw_device *dev, dw_callback listener);
/*****/

/****f*i2c.functions/dw_i2c_master_back2_back
 *DESCRIPTION
 *this function initiates an interrupt-driven master back-to-back
 *transfer.to do this, the I2C must first be properly configured,
 *enabled and a transmit buffer must be setup which contains the
 *sequential reads and writes to perform.an associated receive
 *buffer of suitable size must also be specified when issuing the
 *transfer.as data is received, it is written to the receive buffer.
 *the callback function is called (if it is not NULL) when the final
 *byte is received and there is no more data to send.
 *
 *A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *which returns the number of bytes that are sent before the transfer
 *is interrupted.A terminated transfer's callback function is never
 *called.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *tx_buffer	-- buffer from which to send data
 *tx_length	-- length of transmit buffer/number of bytes to send
 *rx_buffer	-- buffer to write received data to
 *rx_length	-- length of receive buffer/number of bytes to receive
 *callback	-- function to call when transfer is complete
 *RETURN VALUE
 *0		-- if successful
 *-DW_EBUSY	-- if the I2C is busy (transfer already in progress)
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *- ic_rx_tl/rx_tl
 *
 *this function enables the tx_empty and tx_abrt interrupts.
 *the dw_callback function pointer typedef is defined in the common
 *header files.
 *the transmit buffer must be 16 bits wide as a read command is 9 bits
 *long (0x100).restart conditions must be enabled in order to
 *perform back-to-back transfers.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_master_transmit(), dw_i2c_master_receive(),
 *dw_i2c_slave_transmit(), dw_i2c_slave_bulk_transmit(),
 *dw_i2c_slave_receive(), dw_i2c_terminate()
 *SOURCE
 */
int dw_i2c_master_back2_back(struct dw_device *dev, uint16 *tx_buffer,
				unsigned int tx_length, uint8 *rx_buffer,
				unsigned int rx_length, dw_callback callback);
/*****/

/****f*i2c.functions/dw_i2c_master_transmit
 *DESCRIPTION
 *this function initiates an interrupt-driven master transmit
 *transfer.to do this, the I2C must first be properly configured and
 *enabled.this function configures a master transmit transfer and
 *enables the transmit interrupt to keep the transmit FIFO filled.
 *upon completion, the callback function is called (if it is not
 *NULL).
 *
 *A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *which returns the number of bytes that are sent before the transfer
 *is interrupted.A terminated transfer's callback function is never
 *called.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *buffer	-- buffer from which to send data
 *length	-- length of buffer/number of bytes to send
 *callback	-- function to call when transfer is complete
 *RETURN VALUE
 *0		-- if successful
 *-DW_EBUSY	-- if the I2C is busy (transfer already in progress)
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *
 *this function enables the tx_empty and tx_abrt interrupts.
 *the dw_callback function pointer typedef is defined in the common
 *header files.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_master_back2_back(), dw_i2c_master_receive(),
 *dw_i2c_slave_transmit(), dw_i2c_slave_bulk_transmit(),
 *dw_i2c_slave_receive(), dw_i2c_terminate()
 *SOURCE
 */
int dw_i2c_master_transmit(struct dw_device *dev, uint8 *buffer,
			unsigned int length, dw_callback callback);
/*****/

/****f*i2c.functions/dw_i2c_slave_transmit
 *DESCRIPTION
 *this function initiates an interrupt-driven slave transmit transfer.
 *to do this, the I2C must first be properly configured, enabled and
 *must also receive a read request (i2c_irq_rd_req) from an I2C
 *master.this function fills the transmit FIFO and, if there is more
 *data to send, sets up and enables the transmit interrupts to keep
 *the FIFO filled.upon completion, the callback function is called
 *(if it is not NULL).
 *
 *A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *which returns the number of bytes that were sent before the transfer
 *was interrupted.A terminated transfer's callback function is never
 *called.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *buffer	-- buffer from which to send data
 *length	-- length of buffer/number of bytes to send
 *callback	-- function to call when transfer is complete
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPROTO	-- if a read request was not received
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *
 *this function enables the tx_empty and tx_abrt interrupts.
 *this function may only be called from the user listener function
 *after a read request has been received.
 *the dw_callback function pointer typedef is defined in the common
 *header files.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_master_back2_back(), dw_i2c_master_transmit(),
 *dw_i2c_slave_bulk_transmit(), dw_i2c_master_receive(),
 *dw_i2c_slave_receive(), dw_i2c_terminate()
 *SOURCE
 */
int dw_i2c_slave_transmit(struct dw_device *dev, uint8 *buffer,
			unsigned int length, dw_callback callback);
/*****/

/****f*i2c.functions/dw_i2c_slave_bulk_transmit
 *DESCRIPTION
 *this function initiates an interrupt-driven slave transmit transfer.
 *to do this, the I2C must first be properly configured, enabled and
 *must also receive a read request (i2c_irq_rd_req) from an I2C
 *master.this function fills the transmit FIFO and, if there is more
 *data to send, sets up and enables the transmit interrupts to keep
 *the FIFO filled.upon completion, the callback function is called
 *(if it is not NULL).
 *
 *A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *which returns the number of bytes that were sent before the transfer
 *was interrupted.A terminated transfer's callback function is never
 *called.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *buffer	-- buffer from which to send data
 *length	-- length of buffer/number of bytes to send
 *callback	-- function to call when transfer is complete
 *RETURN VALUE
 *0		-- if successful
 *-DW_EPROTO	-- if a read request was not received
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *
 *this function enables the tx_empty and tx_abrt interrupts.
 *this function may only be called from the user listener function
 *after a read request has been received.
 *the dw_callback function pointer typedef is defined in the common
 *header files.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_master_back2_back(), dw_i2c_master_transmit(),
 *dw_i2c_slave_transmit(), dw_i2c_master_receive(),
 *dw_i2c_slave_receive(), dw_i2c_terminate()
 *SOURCE
 */
int dw_i2c_slave_bulk_transmit(struct dw_device *dev, uint8 *buffer,
			unsigned int length, dw_callback callback);
/*****/

/****f*i2c.functions/dw_i2c_master_receive
 *DESCRIPTION
 *this function initiates an interrupt-driven master receive transfer.
 *to do this, the I2C must first be properly configured and enabled.
 *this function sets up the transmit FIFO to be loaded with read
 *commands.in parallel, this function sets up and enables the
 *receive interrupt to fill the buffer from the receive FIFO (the same
 *number of times as writes to the transmit FIFO).upon completion,
 *the callback function is called (if it is not NULL).
 *
 *A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *which returns the number of bytes that were received before the
 *transfer was interrupted.A terminated transfer's callback function
 *is never called.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *buffer	-- buffer to write received data to
 *length	-- length of buffer/max number of bytes to receive
 *callback	-- function to call when transfer is complete
 *RETURN VALUE
 *0		-- if successful
 *-DW_EBUSY	-- if the I2C is busy (transfer already in progress)
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *- ic_rx_tl/rx_tl
 *
 *this function enables the tx_empty, tx_abrt & rx_full interrupts.
 *the dw_callback function pointer typedef is defined in the common
 *header files.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_master_back2_back(), dw_i2c_master_transmit(),
 *dw_i2c_slave_transmit(), dw_i2c_slave_bulk_transmit(),
 *dw_i2c_slave_receive(), dw_i2c_terminate()
 *SOURCE
 */
int dw_i2c_master_receive(struct dw_device *dev, uint8 *buffer,
			unsigned int length, dw_callback callback);
/*****/

/****f*i2c.functions/dw_i2c_slave_receive
 *DESCRIPTION
 *this function initiates an interrupt-driven slave receive transfer.
 *to do this, the I2C must first be properly configured and enabled.
 *this function sets up and enables the receive interrupt to fill the
 *buffer from the receive FIFO.upon completion, the callback
 *function is called (if it is not NULL).
 *
 *A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *which returns the number of bytes that were received before the
 *transfer was interrupted.A terminated transfer's callback function
 *is never called.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *buffer	-- buffer to write received data to
 *length	-- length of buffer/max number of bytes to receive
 *callback	-- function to call when transfer is complete
 *RETURN VALUE
 *0		-- if successful
 *-DW_EBUSY	-- if the I2C is busy (transfer already in progress)
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *- ic_rx_tl/rx_tl
 *
 *this function enables the rx_full interrupt.
 *the dw_callback function pointer typedef is defined in the common
 *header files.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_master_back2_back(), dw_i2c_master_transmit(),
 *dw_i2c_master_receive(), dw_i2c_slave_transmit(),
 *dw_i2c_slave_bulk_transmit(), dw_i2c_terminate()
 *SOURCE
 */
int dw_i2c_slave_receive(struct dw_device *dev, uint8 *buffer, unsigned
			 length, dw_callback callback);
/*****/

/****f*i2c.functions/dw_i2c_terminate
 *DESCRIPTION
 *this function terminates the current I2C interrupt-driven transfer
 *in progress, if any.this function must be called to end an
 *unfinished interrupt-driven transfer as driver instability would
 *ensue otherwise.
 *any data received after calling this function is treated as a new
 *transfer by the driver.therefore, it would be prudent to wait
 *until the next detected stop condition when receiving data in order
 *to avoid a misalignment between the device and driver.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the number of bytes sent/received during the interrupted transfer,
 *if any.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *- ic_rxflr/rxflr
 *- ic_data_cmd/dat
 *- ic_rx_tl/rx_tl
 *
 *this function is part of the interrupt-driven interface and should
 *not be called when using the I2C in a poll-driven manner.
 *this function disables the tx_empty and enables the rx_full
 *interrupts.
 *this function restores the receive FIFO threshold to the previously
 *user-specified value.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_master_back2_back(), dw_i2c_master_transmit(),
 *dw_i2c_master_receive(), dw_i2c_slave_transmit(),
 *dw_i2c_slave_bulk_transmit(), dw_i2c_slave_receive()
 *SOURCE
 */
int dw_i2c_terminate(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_unmask_irq
 *DESCRIPTION
 *unmasks specified I2C interrupt(s).
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *interrupts	-- interrupt(s) to enable
 *RETURN VALUE
 *none
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *SEE ALSO
 *dw_i2c_mask_irq(), dw_i2c_clear_irq(), dw_i2c_is_irq_masked(),
 *enum dw_i2c_irq
 *SOURCE
 */
void dw_i2c_unmask_irq(struct dw_device *dev, enum dw_i2c_irq interrupts);
/*****/

/****f*i2c.functions/dw_i2c_mask_irq
 *DESCRIPTION
 *masks specified I2C interrupt(s).
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *interrupts	-- interrupt(s) to disable
 *RETURN VALUE
 *none
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *SEE ALSO
 *dw_i2c_unmask_irq(), dw_i2c_clear_irq(), dw_i2c_is_irq_masked(),
 *enum dw_i2c_irq
 *SOURCE
 */
void dw_i2c_mask_irq(struct dw_device *dev, enum dw_i2c_irq interrupts);
/*****/

/****f*i2c.functions/dw_i2c_clear_irq
 *DESCRIPTION
 *clears specified I2C interrupt(s).only the following interrupts
 *can be cleared in this fashion: rx_under, rx_over, tx_over, rd_req,
 *tx_abrt, rx_done, activity, stop_det, start_det, gen_call.although
 *they can be specified, the tx_empty and rd_req interrupts cannot be
 *cleared using this function.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *interrupts	-- interrupt(s) to clear
 *RETURN VALUE
 *none
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_clr_intr/clr_intr
 *- ic_clr_rx_under/clr_rx_under
 *- ic_clr_rx_over/clr_rx_over
 *- ic_clr_tx_over/clr_tx_over
 *- ic_clr_rd_req/clr_rd_req
 *- ic_clr_tx_abrt/clr_tx_abrt
 *- ic_clr_rx_done/clr_rx_done
 *- ic_clr_activity/clr_activity
 *- ic_clr_stop_det/clr_stop_det
 *- ic_clr_start_det/clr_start_det
 *- ic_clr_gen_call/clr_gen_call
 *SEE ALSO
 *dw_i2c_unmask_irq(), dw_i2c_mask_irq(), dw_i2c_is_irq_masked(),
 *enum dw_i2c_irq
 *SOURCE
 */
void dw_i2c_clear_irq(struct dw_device *dev, enum dw_i2c_irq interrupts);
/*****/

/****f*i2c.functions/dw_i2c_is_irq_masked
 *DESCRIPTION
 *returns whether the specified I2C interrupt is masked or not.only
 *one interrupt can be specified at a time.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *interrupt	-- interrupt to check
 *RETURN VALUE
 *true		-- interrupt is enabled
 *false		-- interrupt is disabled
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *SEE ALSO
 *dw_i2c_unmask_irq(), dw_i2c_mask_irq(), dw_i2c_clear_irq(),
 *dw_i2c_get_irq_mask(), enum dw_i2c_irq
 *SOURCE
 */
bool dw_i2c_is_irq_masked(struct dw_device *dev, enum dw_i2c_irq interrupt);
/*****/

/****f*i2c.functions/dw_i2c_get_irq_mask
 *DESCRIPTION
 *returns the current interrupt mask.  for each bitfield, a value of
 *'0' indicates that an interrupt is masked while a value of '1'
 *indicates that an interrupt is enabled.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the d_w_apb_i2c interrupt mask.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *SEE ALSO
 *dw_i2c_unmask_irq(), dw_i2c_mask_irq(), dw_i2c_clear_irq(),
 *dw_i2c_is_irq_masked(), enum dw_i2c_irq
 *SOURCE
 */
uint32 dw_i2c_get_irq_mask(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_is_irq_active
 *DESCRIPTION
 *returns whether an I2C interrupt is active or not, after the masking
 *stage.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *interrupt	-- interrupt to check
 *RETURN VALUE
 *true		-- irq is active
 *false		-- irq is inactive
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_stat/all bits
 *SEE ALSO
 *dw_i2c_is_raw_irq_active(), dw_i2c_unmask_irq(), dw_i2c_mask_irq(),
 *dw_i2c_is_irq_masked(), dw_i2c_clear_irq(), enum dw_i2c_irq
 *SOURCE
 */
bool dw_i2c_is_irq_active(struct dw_device *dev, enum dw_i2c_irq interrupt);
/*****/

/****f*i2c.functions/dw_i2c_is_raw_irq_active
 *DESCRIPTION
 *returns whether an I2C raw interrupt is active or not, regardless of
 *masking.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *interrupt	-- interrupt to check
 *RETURN VALUE
 *true		-- raw irq is active
 *false		-- raw irq is inactive
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_raw_intr_stat/all bits
 *SEE ALSO
 *dw_i2c_is_irq_active(), dw_i2c_unmask_irq(), dw_i2c_mask_irq(),
 *dw_i2c_is_irq_masked(), dw_i2c_clear_irq(), enum dw_i2c_irq
 *SOURCE
 */
bool dw_i2c_is_raw_irq_active(struct dw_device *dev, enum dw_i2c_irq interrupt);
/*****/

/****f*i2c.functions/dw_i2c_set_dma_tx_mode
 *DESCRIPTION
 *this function is used to set the DMA mode for transmit transfers.
 *possible options are none (disabled), software or hardware
 *handshaking.for software handshaking, a transmit notifier function
 *(notifies the DMA that the I2C is ready to accept more data) must
 *first be set via the dw_i2c_set_notifier_destination_ready() function.
 *the transmitter empty interrupt is masked for hardware handshaking
 *and unmasked (and managed) for software handshaking or when the DMA
 *mode is set to none.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *mode		-- DMA mode to set (none, hw or sw handshaking)
 *RETURN VALUE
 *0		-- if successful
 *-DW_ENOSYS	-- if device does not have a DMA interface
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_dma_cr/tdmae
 *
 *this function is affected by the HAS_DMA hardware parameter.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_get_dma_tx_mode(), dw_i2c_get_dma_tx_level(),
 *dw_i2c_set_notifier_destination_ready(), dw_i2c_set_dma_rx_mode()
 *SOURCE
 */
int dw_i2c_set_dma_tx_mode(struct dw_device *dev, enum dw_dma_mode mode);
/*****/

/****f*i2c.functions/dw_i2c_get_dma_tx_mode
 *DESCRIPTION
 *this function returns the current DMA mode for I2C transmit
 *transfers.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current DMA transmit mode.
 *NOTES
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_set_dma_tx_mode(), dw_i2c_get_dma_rx_mode()
 *SOURCE
 */
enum dw_dma_mode dw_i2c_get_dma_tx_mode(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_dma_rx_mode
 *DESCRIPTION
 *this function is used to set the DMA mode for receive transfers.
 *possible options are none (disabled), software or hardware
 *handshaking.for software handshaking, a receive notifier function
 *(notifies the DMA that the I2C is ready to accept more data) must
 *first be setup via the dw_i2c_set_notifier_source_ready() function.
 *the receiver full interrupt is masked for hardware handshaking and
 *unmasked for software handshaking or when the DMA mode is set to
 *none.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *mode		-- DMA mode to set (none, hw or sw handshaking)
 *RETURN VALUE
 *0		-- if successful
 *-DW_ENOSYS	-- if device does not have a DMA interface
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_dma_cr/tdmae
 *
 *this function is affected by the HAS_DMA hardware parameter.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_get_dma_rx_mode(), dw_i2c_get_dma_tx_level(), dw_dma_mode,
 *dw_i2c_set_notifier_source_ready(), dw_i2c_set_dma_rx_mode()
 *SOURCE
 */
int dw_i2c_set_dma_rx_mode(struct dw_device *dev, enum dw_dma_mode mode);
/*****/

/****f*i2c.functions/dw_i2c_get_dma_rx_mode
 *DESCRIPTION
 *this function returns the current DMA mode for I2C transmit
 *transfers.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current DMA transmit mode.
 *NOTES
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_set_dma_tx_mode(), dw_i2c_get_dma_rx_mode()
 *SOURCE
 */
enum dw_dma_mode dw_i2c_get_dma_rx_mode(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_dma_tx_level
 *DESCRIPTION
 *this function sets the threshold level at which new data is
 *requested from the DMA.this is used for DMA hardware handshaking
 *mode only.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *level		-- DMA request threshold level
 *RETURN VALUE
 *0		-- if successful
 *-DW_ENOSYS	-- if device does not have a DMA interface
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_dma_tdlr/dmatdl
 *
 *this function is affected by the HAS_DMA hardware parameter.
 *SEE ALSO
 *dw_i2c_get_dma_tx_level(), dw_i2c_set_dma_tx_mode()
 *SOURCE
 */
int dw_i2c_set_dma_tx_level(struct dw_device *dev, uint8 level);
/*****/

/****f*i2c.functions/dw_i2c_get_dma_tx_level
 *DESCRIPTION
 *this functions gets the current DMA transmit data threshold level.
 *this is the FIFO level at which the DMA is requested to send more
 *data from the I2C.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current DMA transmit data level threshold.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_dma_tdlr/dmatdl
 *SEE ALSO
 *dw_i2c_set_dma_tx_level(), dw_i2c_set_dma_tx_mode()
 *SOURCE
 */
uint8 dw_i2c_get_dma_tx_level(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_dma_rx_level
 *DESCRIPTION
 *this function sets the threshold level at which the DMA is requested
 *to receive data from the I2C.this is used for DMA hardware
 *handshaking mode only.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *level		-- DMA request threshold level
 *RETURN VALUE
 *0		-- if successful
 *-DW_ENOSYS	-- if device does not have a DMA interface
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_dma_rdlr/dmardl
 *
 *this function is affected by the HAS_DMA hardware parameter.
 *SEE ALSO
 *dw_i2c_get_dma_rx_level(), dw_i2c_set_dma_rx_mode()
 *SOURCE
 */
int dw_i2c_set_dma_rx_level(struct dw_device *dev, uint8 level);
/*****/

/****f*i2c.functions/dw_i2c_get_dma_rx_level
 *DESCRIPTION
 *this functions gets the current DMA receive data threshold level.
 *this is the FIFO level at which the DMA is requested to receive from
 *the I2C.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *the current DMA receive data level threshold.
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_dma_rdlr/dmardl
 *SEE ALSO
 *dw_i2c_set_dma_rx_level(), dw_i2c_set_dma_rx_mode()
 *SOURCE
 */
uint8 dw_i2c_get_dma_rx_level(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_set_notifier_destination_ready
 *DESCRIPTION
 *this function sets the user DMA transmit notifier function.this
 *function is required when the DMA transmit mode is software
 *handshaking.the I2C driver calls this function at a predefined
 *threshold to request the DMA to send more data to the I2C.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *funcptr	-- called to request more data from the DMA
 *dmac		-- associated d_w_ahb_dmac device handle
 *channel	-- channel number used for the transfer
 *RETURN VALUE
 *0		-- if successful
 *-DW_ENOSYS	-- if device does not have a DMA interface
 *NOTES
 *this function is affected by the HAS_DMA hardware parameter.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_set_notifier_source_ready(), dw_i2c_set_dma_tx_mode(),
 *dw_i2c_set_tx_threshold()
 *SOURCE
 */
int dw_i2c_set_notifier_destination_ready(struct dw_device *dev,
				dw_dma_notifier_func funcptr,
				struct dw_device *dmac,
				unsigned int channel);
/*****/

/****f*i2c.functions/dw_i2c_set_notifier_source_ready
 *DESCRIPTION
 *this function sets the user DMA receive notifier function.this
 *function is required when the DMA receive mode is software
 *handshaking.the I2C driver calls this function at a predefined
 *threshold to inform the DMA that data is ready to be read from the
 *I2C.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *funcptr	-- called to inform the DMA to fetch more data
 *dmac		-- associated DMA device handle
 *channel	-- channel number used for the transfer
 *RETURN VALUE
 *0		-- if successful
 *-DW_ENOSYS	-- if device does not have a DMA interface
 *NOTES
 *this function is affected by the HAS_DMA hardware parameter.
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *
 *this function cannot be used when using an interrupt handler other
 *than dw_i2c_irq_handler().
 *SEE ALSO
 *dw_i2c_set_notifier_destination_ready(), dw_i2c_set_dma_rx_mode(),
 *dw_i2c_set_rx_threshold()
 *SOURCE
 */
int dw_i2c_set_notifier_source_ready(struct dw_device *dev,
				dw_dma_notifier_func funcptr,
				struct dw_device *dmac, unsigned int channel);
/*****/

/****f*i2c.functions/dw_i2c_irq_handler
 *DESCRIPTION
 *this function handles and processes I2C interrupts.it works in
 *conjunction with the interrupt API and a user listener function
 *to manage interrupt-driven transfers.when fully using the
 *interrupt API, this function should be called whenever a d_w_apb_i2c
 *interrupt occurs.there is an alternate interrupt handler
 *available, dw_i2c_user_irq_handler(), but this cannot be used in
 *conjunction with the other interrupt API functions.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- an interrupt was processed
 *false		-- no interrupt was processed
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_mask/all bits
 *- ic_intr_stat/all bits
 *- ic_clr_rx_under/clr_rx_under
 *- ic_clr_rx_over/clr_rx_over
 *- ic_clr_tx_over/clr_tx_over
 *- ic_clr_rd_req/clr_rd_req
 *- ic_clr_tx_abrt/clr_tx_abrt
 *- ic_clr_rx_done/clr_rx_done
 *- ic_clr_activity/clr_activity
 *- ic_clr_stop_det/clr_stop_det
 *- ic_clr_start_det/clr_start_det
 *- ic_clr_gen_call/clr_gen_call
 *- ic_status/rfne
 *- ic_rxflr/rxflr
 *- ic_data_cmd/cmd
 *- ic_data_cmd/dat
 *- ic_rx_tl/rx_tl
 *- ic_txflr/txflr
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *WARNINGS
 *the user listener function is run in interrupt context and, as such,
 *care must be taken to ensure that any data shared between it and
 *normal code is adequately protected from corruption.depending on
 *the target platform, spinlocks, mutexes or semaphores may be used to
 *achieve this.the other interrupt API functions disable I2C
 *interrupts before entering critical sections of code to avoid any
 *shared data issues.
 *SEE ALSO
 *dw_i2c_master_transmit(), dw_i2c_master_receive(),
 *dw_i2c_master_back2_back(),dw_i2c_slave_transmit(),
 *dw_i2c_slave_receive(), dw_i2c_terminate(),
 *dw_i2c_slave_bulk_transmit()
 *SOURCE
 */
int dw_i2c_irq_handler(struct dw_device *dev);
/*****/

/****f*i2c.functions/dw_i2c_user_irq_handler
 *DESCRIPTION
 *this function identifies the current highest priority active
 *interrupt, if any, and forwards it to a user-provided listener
 *function for processing.this allows a user absolute control over
 *how each I2C interrupt is processed.
 *
 *none of the other interrupt API functions can be used with this
 *interrupt handler.this is because they are symbiotic with the
 *dw_i2c_irq_handler() interrupt handler.all command and status API
 *functions, however, can be used within the user listener function.
 *this is in contrast to dw_i2c_irq_handler(), where dw_i2c_read(),
 *dw_i2c_write() and dw_i2c_issue_read() cannot be used within the user
 *listener function.
 *ARGUMENTS
 *dev		-- d_w_apb_i2c device handle
 *RETURN VALUE
 *true		-- an interrupt was processed
 *false		-- no interrupt was processed
 *NOTES
 *accesses the following d_w_apb_i2c register(s)/bit field(s):
 *- ic_intr_stat/all bits
 *- ic_clr_rx_under/clr_rx_under
 *- ic_clr_rx_over/clr_rx_over
 *- ic_clr_tx_over/clr_tx_over
 *- ic_clr_rd_req/clr_rd_req
 *- ic_clr_tx_abrt/clr_tx_abrt
 *- ic_clr_rx_done/clr_rx_done
 *- ic_clr_activity/clr_activity
 *- ic_clr_stop_det/clr_stop_det
 *- ic_clr_start_det/clr_start_det
 *- ic_clr_gen_call/clr_gen_call
 *
 *this function is part of the interrupt API and should not be called
 *when using the I2C in a poll-driven manner.
 *WARNINGS
 *the user listener function is run in interrupt context and, as such,
 *care must be taken to ensure that any data shared between it and
 *normal code is adequately protected from corruption.depending on
 *the target platform, spinlocks, mutexes or semaphores may be used to
 *achieve this.
 *SEE ALSO
 *dw_i2c_set_listener()
 *SOURCE
 */
int dw_i2c_user_irq_handler(struct dw_device *dev);
/*****/

#endif				/*DW_APB_I2C_PUBLIC_H */

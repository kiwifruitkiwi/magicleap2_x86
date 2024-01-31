// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
//
// AMD SPI controller driver
//
// Copyright (c) 2020-2021, Advanced Micro Devices, Inc.
//
// Authors: Sanjay R Mehta <sanju.mehta@amd.com>
//     Nehal Bakulchnadra Shah <nehal-bakulchandra.shah@amd.com>

#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#define AMD_SPI_CTRL0_REG	0x00
#define AMD_SPI_OPCODE_REG  0x45
#define AMD_SPI_CMD_TRIGGER_REG 0x47
#define AMD_SPI_EXEC_CMD	BIT(16)
#define AMD_SPI_FIFO_CLEAR	BIT(20)
#define AMD_SPI_BUSY		BIT(31)
#define AMD_SPI_TRIGGER_CMD	BIT(7)
#define AMD_SPI_OPCODE_MASK	0xFF

#define AMD_SPI_ALT_CS_REG	0x1D
#define AMD_SPI_ALT_CS_MASK	0x3

#define AMD_SPI_FIFO_BASE	0x80
#define AMD_SPI_TX_COUNT_REG	0x48
#define AMD_SPI_RX_COUNT_REG	0x4B
#define AMD_SPI_STATUS_REG	0x4C

#define AMD_SPI_FIFO_SIZE	72
#define AMD_SPI_MEM_SIZE	200

/* M_CMD OP codes for SPI */
#define AMD_SPI_XFER_TX		1
#define AMD_SPI_XFER_RX		2

#ifdef CONFIG_SPI_TPS6598X_CLIENT
#define AMD_TX_OPCODE	0x0B
#define AMD_RX_OPCODE	0x0A
#endif

#define AMD_SPI_DDRCMDCODE	0x40
#define AMD_SPI_QDRCMDCODE	0x41
#define AMD_SPI_DPRCMDCODE	0x42
#define AMD_SPI_QPRCMDCODE	0x43

#define AMD_SPI_DPW_CMD		0x14
#define AMD_SPI_QPW_CMD		0x15

struct amd_spi {
	void __iomem *io_remap_addr;
	unsigned long io_base_addr;
	u32 rom_addr;
	u8 chip_select;
	u8 ctrl_id;
	struct spi_device	*spi_dev;
	struct spi_board_info	info;
	struct spi_master *master;

};

static inline u8 amd_spi_readreg8(struct spi_master *master, int idx)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);

	return ioread8((u8 __iomem *)amd_spi->io_remap_addr + idx);
}

static inline void amd_spi_writereg8(struct spi_master *master, int idx,
				     u8 val)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);

	iowrite8(val, ((u8 __iomem *)amd_spi->io_remap_addr + idx));
}

static inline void amd_spi_setclear_reg8(struct spi_master *master, int idx,
					 u8 set, u8 clear)
{
	u8 tmp = amd_spi_readreg8(master, idx);

	tmp = (tmp & ~clear) | set;
	amd_spi_writereg8(master, idx, tmp);
}

static inline u32 amd_spi_readreg32(struct spi_master *master, int idx)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);

	return ioread32((u8 __iomem *)amd_spi->io_remap_addr + idx);
}

static inline void amd_spi_writereg32(struct spi_master *master, int idx,
				      u32 val)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);

	iowrite32(val, ((u8 __iomem *)amd_spi->io_remap_addr + idx));
}

static inline void amd_spi_setclear_reg32(struct spi_master *master, int idx,
					  u32 set, u32 clear)
{
	u32 tmp = amd_spi_readreg32(master, idx);

	tmp = (tmp & ~clear) | set;
	amd_spi_writereg32(master, idx, tmp);
}

static void amd_spi_select_chip(struct spi_master *master)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);
	u8 chip_select = amd_spi->chip_select;

	amd_spi_setclear_reg8(master, AMD_SPI_ALT_CS_REG, chip_select,
			      AMD_SPI_ALT_CS_MASK);
}

static void amd_spi_clear_chip(struct spi_master *master)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);
	u8 chip_select = amd_spi->chip_select;

	amd_spi_writereg8(master, AMD_SPI_ALT_CS_REG, chip_select & 0XFC);
}

static void amd_spi_clear_fifo_ptr(struct spi_master *master)
{
	amd_spi_setclear_reg32(master, AMD_SPI_CTRL0_REG, AMD_SPI_FIFO_CLEAR,
			       AMD_SPI_FIFO_CLEAR);
}

static void amd_spi_set_opcode(struct spi_master *master, u8 cmd_opcode)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);

	if (!amd_spi->ctrl_id)
		amd_spi_setclear_reg32(master, AMD_SPI_CTRL0_REG, cmd_opcode,
				       AMD_SPI_OPCODE_MASK);
	else
		amd_spi_writereg8(master, AMD_SPI_OPCODE_REG, cmd_opcode);

}

static inline void amd_spi_set_rx_count(struct spi_master *master,
					u8 rx_count)
{
	amd_spi_setclear_reg8(master, AMD_SPI_RX_COUNT_REG, rx_count, 0xff);
}

static inline void amd_spi_set_tx_count(struct spi_master *master,
					u8 tx_count)
{
	amd_spi_setclear_reg8(master, AMD_SPI_TX_COUNT_REG, tx_count, 0xff);
}

static inline int amd_spi_busy_wait(struct amd_spi *amd_spi)
{
	bool spi_busy;
	int timeout = 100000;

	/* poll for SPI bus to become idle */
	if (!amd_spi->ctrl_id)
		spi_busy = (ioread32((u8 __iomem *)amd_spi->io_remap_addr +
				AMD_SPI_CTRL0_REG) & AMD_SPI_BUSY) == AMD_SPI_BUSY;
	else
		spi_busy = (ioread32((u8 __iomem *)amd_spi->io_remap_addr +
				AMD_SPI_STATUS_REG) & AMD_SPI_BUSY) == AMD_SPI_BUSY;

	while (spi_busy) {
		usleep_range(10, 40);
		if (timeout-- < 0)
			return -ETIMEDOUT;

		/* poll for SPI bus to become idle */
		if (!amd_spi->ctrl_id)
			spi_busy = (ioread32((u8 __iomem *)amd_spi->io_remap_addr +
					AMD_SPI_CTRL0_REG) & AMD_SPI_BUSY) == AMD_SPI_BUSY;
		else
			spi_busy = (ioread32((u8 __iomem *)amd_spi->io_remap_addr +
					AMD_SPI_STATUS_REG) & AMD_SPI_BUSY) == AMD_SPI_BUSY;
	}

	return 0;
}

static void amd_spi_execute_opcode(struct spi_master *master)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);

	if (!amd_spi->ctrl_id) {
		amd_spi_busy_wait(amd_spi);
		/* Set ExecuteOpCode bit in the CTRL0 register */
		amd_spi_setclear_reg32(master, AMD_SPI_CTRL0_REG, AMD_SPI_EXEC_CMD,
				       AMD_SPI_EXEC_CMD);
		amd_spi_busy_wait(amd_spi);
	} else {
		amd_spi_busy_wait(amd_spi);
		amd_spi_setclear_reg8(master, AMD_SPI_CMD_TRIGGER_REG, AMD_SPI_TRIGGER_CMD,
				      AMD_SPI_TRIGGER_CMD);
		amd_spi_busy_wait(amd_spi);
	}
}

static int amd_spi_master_setup(struct spi_device *spi)
{
	struct spi_master *master = spi->master;

	amd_spi_clear_fifo_ptr(master);

	return 0;
}

static inline int amd_spi_fifo_xfer(struct amd_spi *amd_spi,
				    struct spi_master *master,
				    struct spi_message *message)
{
	struct spi_transfer *xfer = NULL;
	u8 cmd_opcode;
	u8 *buf = NULL;
	u32 m_cmd = 0;
	u32 i = 0, it = 0, tx_index = 0, rx_index = 0;
	u32 tx_len = 0, rx_len = 0, iters = 0, remaining =  0;

	list_for_each_entry(xfer, &message->transfers,
			    transfer_list) {
		if (xfer->rx_buf)
			m_cmd = AMD_SPI_XFER_RX;
		if (xfer->tx_buf)
			m_cmd = AMD_SPI_XFER_TX;

		if (m_cmd & AMD_SPI_XFER_TX) {
			buf = (u8 *)xfer->tx_buf;
			cmd_opcode = *(u8 *)xfer->tx_buf;

#ifdef CONFIG_SPI_TPS6598X_CLIENT
			if (amd_spi->ctrl_id) {
				tx_len = xfer->len;
				cmd_opcode = AMD_TX_OPCODE;
				amd_spi_set_opcode(master, cmd_opcode);
			}
#else
			tx_len = xfer->len - 1;
			buf++;
			if (amd_spi->ctrl_id) {
				buf--;
				tx_len = xfer->len;
				cmd_opcode = 0;
			}
#endif
			tx_index = 0;
			iters = tx_len / AMD_SPI_FIFO_SIZE;
			remaining = tx_len % AMD_SPI_FIFO_SIZE;

			for (it = 0; it < iters; it++) {
				amd_spi_clear_fifo_ptr(master);
				amd_spi_set_opcode(master, cmd_opcode);

				amd_spi_set_tx_count(master, AMD_SPI_FIFO_SIZE);
				/* Write data into the FIFO. */
				for (i = 0; i < AMD_SPI_FIFO_SIZE; i++) {
					iowrite8(buf[tx_index],
						 ((u8 __iomem *)amd_spi->io_remap_addr +
						 AMD_SPI_FIFO_BASE + i));
					tx_index++;
				}

				amd_spi_set_rx_count(master, 0);
				/* Execute command */
				amd_spi_execute_opcode(master);
			}

			amd_spi_clear_fifo_ptr(master);
			amd_spi_set_opcode(master, cmd_opcode);

			amd_spi_set_tx_count(master, remaining);
			/* Write data into the FIFO. */
			for (i = 0; i < remaining; i++) {
				iowrite8(buf[tx_index],
					 ((u8 __iomem *)amd_spi->io_remap_addr +
					 AMD_SPI_FIFO_BASE + i));
				tx_index++;
			}

			amd_spi_set_rx_count(master, 0);
			/* Execute command */
			amd_spi_execute_opcode(master);
		}
		if (m_cmd & AMD_SPI_XFER_RX) {
			/*
			 * Store no. of bytes to be received from
			 * FIFO
			 */
			rx_len = xfer->len;
			rx_index = 0;
			iters = rx_len / AMD_SPI_FIFO_SIZE;
			remaining = rx_len % AMD_SPI_FIFO_SIZE;
			buf = (u8 *)xfer->rx_buf;

#ifdef CONFIG_SPI_TPS6598X_CLIENT
			if (amd_spi->ctrl_id) {
				cmd_opcode = AMD_RX_OPCODE;
				amd_spi_set_opcode(master, cmd_opcode);
			}
#else
			if (amd_spi->ctrl_id) {
				cmd_opcode = 0;
				amd_spi_set_opcode(master, cmd_opcode);
			}
#endif

			for (it = 0 ; it < iters; it++) {
				amd_spi_clear_fifo_ptr(master);
				amd_spi_set_tx_count(master, 0);
				amd_spi_set_rx_count(master, AMD_SPI_FIFO_SIZE);

				/* Execute command */
				amd_spi_execute_opcode(master);
				/* Read data from FIFO to receive buffer  */
				for (i = 0; i < AMD_SPI_FIFO_SIZE; i++) {
					buf[rx_index] = amd_spi_readreg8(master,
									 AMD_SPI_FIFO_BASE +
									 i);
					rx_index++;
				}
			}

			amd_spi_clear_fifo_ptr(master);
			amd_spi_set_tx_count(master, 0);
			amd_spi_set_rx_count(master, remaining);

			/* Execute command */
			amd_spi_execute_opcode(master);
			/* Read data from FIFO to receive buffer  */
			for (i = 0; i < remaining; i++) {
				buf[rx_index] = amd_spi_readreg8(master,
								 AMD_SPI_FIFO_BASE +
								 i);
				rx_index++;
			}
		}
	}

	/* Update statistics */
	message->actual_length = tx_len + rx_len + 1;
	/* complete the transaction */
	message->status = 0;
	spi_finalize_current_message(master);

	return 0;
}

static int amd_spi_master_transfer(struct spi_master *master,
				   struct spi_message *msg)
{
	struct amd_spi *amd_spi = spi_master_get_devdata(master);
	struct spi_device *spi = msg->spi;

	amd_spi->chip_select = spi->chip_select;
	amd_spi_select_chip(master);

	/*
	 * Extract spi_transfers from the spi message and
	 * program the controller.
	 */
	amd_spi_fifo_xfer(amd_spi, master, msg);
	if (amd_spi->ctrl_id)
		amd_spi_clear_chip(master);
	return 0;
}

#ifdef CONFIG_SPI_TPS6598X_CLIENT

static int amd_spi_populate_client(struct amd_spi *amd_spi)
{
	strlcpy(amd_spi->info.modalias, "tps6598x_new", sizeof(amd_spi->info.modalias));
	amd_spi->info.bus_num = amd_spi->master->bus_num;
	amd_spi->info.chip_select = 0;
	amd_spi->info.mode = amd_spi->master->mode_bits;
	amd_spi->spi_dev = spi_new_device(amd_spi->master, &amd_spi->info);
	return PTR_ERR_OR_ZERO(amd_spi->spi_dev);
}
#endif

#ifdef CONFIG_OF
static void amd_spi_read_mode_quirk(struct spi_master *master)
{
	const u32 *mode_quirk = of_get_property(master->dev.of_node,
						"amd,read-mode-quirk", NULL);
	if (mode_quirk) {
		u8 val = be32_to_cpu(*mode_quirk);

		amd_spi_writereg8(master, AMD_SPI_DDRCMDCODE, val);
		amd_spi_writereg8(master, AMD_SPI_QDRCMDCODE, val);
		amd_spi_writereg8(master, AMD_SPI_DPRCMDCODE, val);
		amd_spi_writereg8(master, AMD_SPI_QPRCMDCODE, val);
		amd_spi_writereg8(master, AMD_SPI_DPW_CMD, val);
		amd_spi_writereg8(master, AMD_SPI_QPW_CMD, val);
	}
}
#endif

static int amd_spi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spi_master *master;
	struct amd_spi *amd_spi;
	struct resource *res;
	int err = 0;

	/* Allocate storage for spi_master and driver private data */
	master = spi_alloc_master(dev, sizeof(struct amd_spi));
	if (!master) {
		dev_err(dev, "Error allocating SPI master\n");
		return -ENOMEM;
	}

	amd_spi = spi_master_get_devdata(master);

#if defined(CONFIG_ACPI) && defined(CONFIG_X86)
	if (acpi_dev_get_first_match_dev("AMDI0061", NULL, -1))
		amd_spi->ctrl_id = 0;

	if (acpi_dev_get_first_match_dev("AMDI0062", NULL, -1))
		amd_spi->ctrl_id = 1;
#else
	amd_spi->ctrl_id = 1;
#endif
	dev_dbg(dev, "amd_spi->ctrl_id =%d\n", amd_spi->ctrl_id);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	amd_spi->io_remap_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(amd_spi->io_remap_addr)) {
		err = PTR_ERR(amd_spi->io_remap_addr);
		dev_err(dev, "error %d ioremap of SPI registers failed\n", err);
		goto err_free_master;
	}
	dev_dbg(dev, "io_remap_address: %p\n", amd_spi->io_remap_addr);
	/* Initialize the spi_master fields */
	master->bus_num = 0;
	master->num_chipselect = 4;
	master->mode_bits = 0;
	if (!amd_spi->ctrl_id)
		master->flags = SPI_MASTER_HALF_DUPLEX;
	master->setup = amd_spi_master_setup;
	master->transfer_one_message = amd_spi_master_transfer;
#ifdef CONFIG_OF

	master->dev.of_node = pdev->dev.of_node;
#endif

	/* Register the controller with SPI framework */
	err = devm_spi_register_master(dev, master);
	if (err) {
		dev_err(dev, "error %d registering SPI controller\n", err);
		goto err_free_master;
	}

#ifdef CONFIG_SPI_TPS6598X_CLIENT
	amd_spi->master = master;
	amd_spi_populate_client(amd_spi);
#endif

#ifdef CONFIG_OF
	amd_spi_read_mode_quirk(master);
#endif

	return 0;

err_free_master:
	spi_master_put(master);

	return err;
}

static const struct acpi_device_id spi_acpi_match[] = {
	{ "AMDI0061", 0 },
	{ "AMDI0062", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, spi_acpi_match);

#ifdef CONFIG_OF
static const struct of_device_id amd_spi_dt_id[] = {
	{ .compatible = "amd,amdi0061-spi" },
	{ },
};
MODULE_DEVICE_TABLE(of, amd_spi_dt_id);
#endif

static struct platform_driver amd_spi_driver = {
	.driver = {
		.name = "amd_spi",
		.acpi_match_table = ACPI_PTR(spi_acpi_match),
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(amd_spi_dt_id),
#endif
	},
	.probe = amd_spi_probe,
};

module_platform_driver(amd_spi_driver);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Sanjay Mehta <sanju.mehta@amd.com>");
MODULE_AUTHOR("Nehal Bakulchnadra Shah <nehal-bakulchandra.shah@amd.com>");
MODULE_DESCRIPTION("AMD SPI Master Controller Driver");

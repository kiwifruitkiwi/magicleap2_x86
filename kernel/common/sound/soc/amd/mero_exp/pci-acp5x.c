/*
 * ACP PCI Driver
 * Copyright (C) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "acp5x-evci.h"
#include <linux/delay.h> /* ML CHANGE: usleep_range */
#include <linux/spinlock.h>

struct acp_pm_state {
	spinlock_t lock;
	int is_active;
};

static struct acp_pm_state pm_state;

static int acp_fw_load = 1;
module_param(acp_fw_load, int, 0644);
MODULE_PARM_DESC(acp_fw_load, "enables acp firmware loading");

static int acp_power_gating;
module_param(acp_power_gating, int, 0644);
MODULE_PARM_DESC(acp_power_gating, "acp power gating flag");

static int acp_unsigned_image = 1;
module_param(acp_unsigned_image, int, 0644);
MODULE_PARM_DESC(acp_unsigned_image, "acp unsigned image flag");

static int disable_runtime_pm = 0;
module_param(disable_runtime_pm, int, 0644);
MODULE_PARM_DESC(disable_runtime_pm, "disable runtime pm");

MODULE_FIRMWARE(FIRMWARE_ACP5x_UNSIGNED);
MODULE_FIRMWARE(FIRMWARE_ACP5x_DATA_BIN);

static int acp5x_runtime_suspend(struct device *dev);
static int acp5x_runtime_resume(struct device *dev);

static int acp5x_runtime_can_sr(void)
{
	// on occasion, starting of audioserver/hal is preceded with putting acp
	// into standby. it is not immediately obvious why this is required on
	// some boots but not others. additionally, some hardware units express
	// this behavior more so then others and even then, not all the time.
	// from iteration testing, this activity seems to occur within 10-12s
	// after kernel boot, per dmesg time. to compensate for this behavior,
	// we allow suspend/resume in early boot stage but not after.
	return (ktime_get_boottime_seconds() <= 15);
}

static int acp5x_can_sr(void)
{
	return false;
}

static void acp5x_srbm_write(struct acp5x_dev_data *acp5x_data,
			     unsigned int addr,
			     unsigned int data)
{
	acp_writel(addr, acp5x_data->acp5x_base + ACP_SRBM_TARG_IDX_ADDR);
	acp_writel(data, acp5x_data->acp5x_base + ACP_SRBM_TARG_IDX_DATA);
}

static unsigned int acp5x_srbm_read(struct acp5x_dev_data *acp5x_data,
				    unsigned int addr)
{
	unsigned int data;

	acp_writel(addr, acp5x_data->acp5x_base + ACP_SRBM_TARG_IDX_ADDR);
	data = acp_readl(acp5x_data->acp5x_base + ACP_SRBM_TARG_IDX_DATA);
	return data;
}

static int acp5x_power_on(void __iomem *acp5x_base)
{
	u32 val;
	int timeout;

	val = acp_readl(acp5x_base + ACP_PGFSM_STATUS);
	pr_debug("ACP_PGFSM_STATUS: 0x%x\n", val);
	if (val == 0)
		return val;
	if ((val & ACP_PGFSM_STATUS_MASK) !=
				ACP_POWER_ON_IN_PROGRESS)
		acp_writel(ACP_PGFSM_CNTL_POWER_ON_MASK,
			   acp5x_base + ACP_PGFSM_CONTROL);
	timeout = 0;
	while (++timeout < 500) {
		val = acp_readl(acp5x_base + ACP_PGFSM_STATUS);
		if (!val) {
			pr_debug("acp powered on\n");
			return 0;
		}
		udelay(1);
	}
	return -ETIMEDOUT;
}

static int acp5x_power_off(void __iomem *acp5x_base)
{
	u32 val;
	int timeout;

	acp_writel(ACP_PGFSM_CNTL_POWER_OFF_MASK,
		   acp5x_base + ACP_PGFSM_CONTROL);
	timeout = 0;
	while (++timeout < 500) {
		val = acp_readl(acp5x_base + ACP_PGFSM_STATUS);
		if ((val & ACP_PGFSM_STATUS_MASK) == ACP_POWERED_OFF) {
			pr_debug("ACP is powered off\n");
			return 0;
		}
		udelay(1);
	}
	return -ETIMEDOUT;
}

static int acp5x_reset(void __iomem *acp5x_base)
{
	u32 val;
	int timeout;

	acp_writel(1, acp5x_base + ACP_SOFT_RESET);
	timeout = 0;
	while (++timeout < 500) {
		val = acp_readl(acp5x_base + ACP_SOFT_RESET);
		if (val & ACP_SOFT_RESET_SOFTRESET_AUDDONE_MASK)
			break;
		cpu_relax();
	}
	acp_writel(0, acp5x_base + ACP_SOFT_RESET);
	timeout = 0;
	while (++timeout < 500) {
		val = acp_readl(acp5x_base + ACP_SOFT_RESET);
		if (!val) {
			pr_debug("ACP SOFT reset done\n");
			return 0;
		}
		cpu_relax();
	}
	return -ETIMEDOUT;
}

static void acp5x_clear_and_disable_dspsw_intr(struct acp5x_dev_data *adata)
{
	unsigned int val = 0;

	val = 0x07;
	acp_writel(val, adata->acp5x_base + ACP_DSP_SW_INTR_STAT);
	val = 0;
	acp_writel(val, adata->acp5x_base + ACP_DSP_SW_INTR_CNTL);
	val = acp_readl(adata->acp5x_base + ACP_DSP_SW_INTR_STAT);
	pr_debug("ACP_DSP_SW_INTR_STAT: 0x%x\n", val);
	val = acp_readl(adata->acp5x_base + ACP_DSP_SW_INTR_CNTL);
	pr_debug("ACP_DSP_SW_INTR_CNTL: 0x%x\n", val);
}

static void acp5x_clear_and_disable_external_intr(struct acp5x_dev_data *adata)
{
	unsigned int val = 0xFFFFFFFF;

	acp_writel(val, adata->acp5x_base + ACP_EXTERNAL_INTR_STAT);
	val = 0x00;
	acp_writel(val, adata->acp5x_base + ACP_EXTERNAL_INTR_CNTL);
}

static int acp5x_init(void __iomem *acp5x_base)
{
	int ret;

	/* power on */
	ret = acp5x_power_on(acp5x_base);
	if (ret) {
		pr_err("acp5x power on failed\n");
		return ret;
	}

	acp_writel(ACP_CLK_ENABLE, acp5x_base + ACP_CONTROL);
	/* Reset */
	ret = acp5x_reset(acp5x_base);
	if (ret) {
		pr_err("acp5x reset failed\n");
		return ret;
	}
	pr_debug("ACP Initialized\n");
	return 0;
}

static int acp5x_deinit(void __iomem *acp5x_base)
{
	int ret = 0;
	unsigned int val = 0x00;

	val = acp_readl(acp5x_base + ACP_DSP0_RUNSTALL);
	if (!val)
		acp_writel(0x01, acp5x_base + ACP_DSP0_RUNSTALL);
	/* Reset */
	/* TODO: Identify repercussions, commenting fixes ACP suspend reboot */
	/* ret = acp5x_reset(acp5x_base);  */
	if (ret) {
		pr_err("acp5x reset failed\n");
		return ret;
	}
	/* TODO: Identify repercussions, commenting fixes ACP suspend reboot */
	/* acp_writel(ACP_CLK_DISABLE, acp5x_base + ACP_CONTROL); */
	if (acp_power_gating) {
		/* power off */
		ret = acp5x_power_off(acp5x_base);
		if (ret) {
			pr_err("acp5x power off failed\n");
			return ret;
		}
	}
	pr_debug("ACP De-Initialized\n");
	return 0;
}

/* Set up clock registers for 48Khz operation. */
static void acp5x_set_mlacp_registers(void __iomem *acp5x_base)
{
	u32 val = 0;

	/*
	 * 1) Set ACP_PIN_CONFIG = 0
	 */
	/* Read */
	val = acp_readl(acp5x_base + ACP_PIN_CONFIG);
	pr_debug("Initial: ACP_PIN_CONFIG: 0x%x\n", val);

	/* Modify */
	val &= 0;
	acp_writel(val, acp5x_base + ACP_PIN_CONFIG);

	/* Write */
	val = acp_readl(acp5x_base + ACP_PIN_CONFIG);
	pr_debug("Modified: ACP_PIN_CONFIG: 0x%x\n", val);

	/*
	 * 2) Set ACP_I2STDM_REFCLKGEN = 0x2001
	 */
	/* Read */
	val = acp_readl(acp5x_base + ACP_I2STDM_REFCLKGEN);
	pr_debug("Initial: ACP_I2STDM_REFCLKGEN: 0x%x\n", val);

	/* Modify */
	val |= 0x2001;
	acp_writel(val, acp5x_base + ACP_I2STDM_REFCLKGEN);

	/* Write */
	val = acp_readl(acp5x_base + ACP_I2STDM_REFCLKGEN);
	pr_debug("Modified: ACP_I2STDM_REFCLKGEN: 0x%x\n", val);

	/*
	 * 3) ACP_I2STDM0_MSTRCLKGEN bit0 = 0x1, leave rest of bits as is
	 */
	/* Read */
	val = acp_readl(acp5x_base + ACP_I2STDM0_MSTRCLKGEN);
	pr_debug("Initial: ACP_I2STDM0_MSTRCLKGEN: 0x%x\n", val);

	/* Modify */
	val &= ~1;
	acp_writel(val, acp5x_base + ACP_I2STDM0_MSTRCLKGEN);

	/* Write */
	val = acp_readl(acp5x_base + ACP_I2STDM0_MSTRCLKGEN);
	pr_debug("Modified: ACP_I2STDM0_MSTRCLKGEN: 0x%x\n", val);

	/*
	 * 4) ACP_I2STDM1_MSTRCLKGEN bit0 = 0x1, leave rest of bits as is
	 */
	/* Read */
	val = acp_readl(acp5x_base + ACP_I2STDM1_MSTRCLKGEN);
	pr_debug("Initial: ACP_I2STDM1_MSTRCLKGEN: 0x%x\n", val);

	// Modify
	val &= ~1;
	acp_writel(val, acp5x_base + ACP_I2STDM1_MSTRCLKGEN);

	// Write
	val = acp_readl(acp5x_base + ACP_I2STDM1_MSTRCLKGEN);
	pr_debug("Modified: ACP_I2STDM1_MSTRCLKGEN: 0x%x\n", val);

	/*
	 * 5) ACP_I2STDM2_MSTRCLKGEN bit0 = 0x1, leave rest of bits as is
	 */
	/* Read */
	val = acp_readl(acp5x_base + ACP_I2STDM2_MSTRCLKGEN);
	pr_debug("Initial: ACP_I2STDM2_MSTRCLKGEN: 0x%x\n", val);

	/* Modify */
	val &= ~1;
	acp_writel(val, acp5x_base + ACP_I2STDM2_MSTRCLKGEN);

	/* Write */
	val = acp_readl(acp5x_base + ACP_I2STDM2_MSTRCLKGEN);
	pr_debug("Modified: ACP_I2STDM2_MSTRCLKGEN: 0x%x\n", val);
}

/* Group1 & Group2 With 4kb as page size and offset mapped to DRAM */
static void configure_acp5x_groupregisters(struct acp5x_dev_data *adata)
{
	u32 index, val;

	for (index = 0; index < 2; index++) {
		val = (ACP_SRBM_DATA_RAM_BASE_OFFSET + (index * 0x4000))
		       | BIT(31);
		acp_writel(val, adata->acp5x_base +
			   ACPAXI2AXI_ATU_BASE_ADDR_GRP_1 + (index * 0x08));
		acp_writel(PAGE_SIZE_4K_ENABLE, adata->acp5x_base +
			   ACPAXI2AXI_ATU_PAGE_SIZE_GRP_1 + (index * 0x08));
	}
	acp_writel(0x01, adata->acp5x_base + ACPAXI2AXI_ATU_CTRL);
}

static void configure_dma_descriptiors(struct acp5x_dev_data *adata)
{
	unsigned int dma_desc_base_addr = ACP_SRBM_DRAM_DMA_BASE_ADDR;

	acp_writel(dma_desc_base_addr, adata->acp5x_base +
		   ACP_DMA_DESC_BASE_ADDR);
	acp_writel(0x00, adata->acp5x_base + ACP_DMA_DESC_MAX_NUM_DSCR);
}

static void configure_ptelist(int type,
			      int num_pages,
			      struct acp5x_dev_data *adata)
{
	u16 page_idx;
	u32 low, high, val, offset = 0, pte_offset = 0;
	dma_addr_t addr = 0;

	pr_debug("%s type:%x num_pages:%x\n", __func__, type, num_pages);
	switch (type) {
	case FW_BIN:
		pte_offset = FW_BIN_PTE_OFFSET;
		addr = adata->dma_addr;
		break;
	case FW_DATA_BIN:
		pte_offset = adata->fw_bin_page_count * 8;
		addr = adata->fw_data_dma_addr;
		break;
	case ACP_LOGGER:
		pte_offset = (adata->fw_bin_page_count +
			      ACP_DRAM_PAGE_COUNT) * 8;
		addr = adata->acp_log_dma_addr;
		break;
	case ACP_DMA_BUFFER_TX:
		pte_offset = (adata->fw_bin_page_count +
			      ACP_DRAM_PAGE_COUNT +
			      ACP_LOG_BUFFER_PAGE_COUNT) * 8;
		addr = adata->acp_dma_addr;
		break;
	case ACP_DMA_BUFFER_RX:
		pte_offset = (adata->fw_bin_page_count +
			      ACP_DRAM_PAGE_COUNT +
			      ACP_LOG_BUFFER_PAGE_COUNT +
			      ACP_DMA_BUFFER_TX_PAGE_COUNT) * 8;
		addr = adata->acp_dma_rx_addr;
		break;
	}
	offset = ACP_SRBM_DATA_RAM_BASE_OFFSET + pte_offset;
	val = acp5x_srbm_read(adata, offset);
	pr_debug("DRAM_ADDR_[0x%x] : 0x%x\n", offset, val);
	for (page_idx = 0; page_idx < num_pages; page_idx++) {
		low = lower_32_bits(addr);
		high = upper_32_bits(addr);

		acp5x_srbm_write(adata, offset + (page_idx * 8), low);
		high |= BIT(31);
		acp5x_srbm_write(adata, offset + (page_idx * 8) + 0x04, high);
		addr += PAGE_SIZE;
	}
}

void ev_get_response(struct acp5x_dev_data *adata)
{
	unsigned int   read_count = 0;
	bool         processed_responses;

	mutex_lock(&adata->acp_ev_res_mutex);
	processed_responses = false;

	/* Process responses as long as they are available */
	for (;;) {
		read_count =
		read_response(&adata->acp_sregmap->acp_resp_ring_buf_info,
			      adata->acp_sregmap->acp_resp_buf,
			      adata->packet,
			      ACP_COMMAND_PACKET_SIZE);
		pr_debug("%s read_count: 0x%x\n", __func__, read_count);
		if (read_count > 0) {
			ev_process_response(adata->packet, read_count, adata);
			processed_responses = true;
		} else {
			break;
		}
	}
	if (processed_responses) {
		/* If responses were processed, signal any threads waiting on
		 * responses
		 */
		adata->response_wait_event = 1;
		wake_up_interruptible(&adata->evresp);
	}
	/* memset response buffer contents to zero */
	memset(adata->packet, 0, ACP_COMMAND_PACKET_SIZE);
	mutex_unlock(&adata->acp_ev_res_mutex);
}

static void evresponse_handler(struct work_struct *work)
{
	struct acp5x_dev_data *adata = container_of(work, struct acp5x_dev_data,
						    evresponse_work);

	ev_get_response(adata);
}

static irqreturn_t acp5x_irq_handler(int irq, void *dev_id)
{
	u16 acp5x_flag = 0, dsp_flag = 0, play_flag = 0, cap_flag = 0;
	u32 val;
	u32 error_val = 0;
	u32 dsp_intr_stat = 0x00;
	u32 dma_stat  = 0x00;
	struct acp5x_dev_data *adata = dev_id;

	if (!adata)
		return IRQ_NONE;
	val = acp_readl(adata->acp5x_base + ACP_EXTERNAL_INTR_STAT);

	if (val & ACP_WOV_STAT) {
		acp_writel(ACP_WOV_STAT, adata->acp5x_base +
			   ACP_EXTERNAL_INTR_STAT);
		pr_err("ACP WOV IRQ raised\n");
		acp5x_flag = 0x01;
	}
	if (val & ACP_EXT_INTR_ERROR_STAT) {
		acp_writel(ACP_EXT_INTR_ERROR_STAT, adata->acp5x_base +
			   ACP_EXTERNAL_INTR_STAT);
		/* To know the source of acp error, error_val variable
		 * is used
		 */
		error_val = acp_readl(adata->acp5x_base + ACP_ERROR_STATUS);
		pr_err("ACP_ERROR_STATUS:0x%x\n", error_val);
		acp5x_flag =  0x01;
	}
	if (val & ACP_SHA_STAT) {
		adata->sha_irq_stat = 0x01;
		wake_up_interruptible(&adata->wait_queue_sha_dma);
		acp_writel(ACP_SHA_STAT, adata->acp5x_base +
			   ACP_EXTERNAL_INTR_STAT);
		acp5x_flag =  0x01;
	}

	if (val & ACP_DMA_STAT) {
		adata->dma_ioc_event = 0x01;
		dma_stat = val & ACP_DMA_STAT;
		wake_up_interruptible(&adata->dma_queue);
		acp_writel(dma_stat, adata->acp5x_base +
			   ACP_EXTERNAL_INTR_STAT);
		acp5x_flag = 0x01;
	}

	if (val & BIT(BT_TX_THRESHOLD)) {
		acp_writel(BIT(BT_TX_THRESHOLD), adata->acp5x_base +
			   ACP_EXTERNAL_INTR_STAT);
		play_flag = 1;
	}
	if (val & BIT(I2S_TX_THRESHOLD)) {
		acp_writel(BIT(I2S_TX_THRESHOLD), adata->acp5x_base +
			   ACP_EXTERNAL_INTR_STAT);
		play_flag = 1;
	}
	if (val & BIT(BT_RX_THRESHOLD)) {
		acp_writel(BIT(BT_RX_THRESHOLD), adata->acp5x_base +
			   ACP_EXTERNAL_INTR_STAT);
		cap_flag = 1;
	}
	if (val & BIT(I2S_RX_THRESHOLD)) {
		acp_writel(BIT(I2S_RX_THRESHOLD), adata->acp5x_base +
			   ACP_EXTERNAL_INTR_STAT);
		cap_flag = 1;
	}

	dsp_intr_stat = acp_readl(adata->acp5x_base + ACP_DSP_SW_INTR_STAT);

	if (dsp_intr_stat != 0x00 || dsp_intr_stat != 0xffffffff) {
		if (dsp_intr_stat & 0x04) {
			schedule_work(&adata->evresponse_work);
			dsp_intr_stat |= 0x04;
			acp_writel(dsp_intr_stat, adata->acp5x_base +
				   ACP_DSP_SW_INTR_STAT);
			dsp_flag = 0x01;
		}
	}
	if (acp5x_flag | dsp_flag | play_flag | cap_flag)
		return IRQ_HANDLED;
	else
		return IRQ_NONE;
}

static void acp5x_enable_extirq(struct acp5x_dev_data *adata)
{
	int val = 0;

	acp_writel(0x01, adata->acp5x_base + ACP_EXTERNAL_INTR_ENB);
	val = acp_readl(adata->acp5x_base + ACP_EXTERNAL_INTR_CNTL);
	val |= ACP_ERROR_MASK | ACP_DMA_MASK | ACP_WOV_MASK;
	acp_writel(val, adata->acp5x_base + ACP_EXTERNAL_INTR_CNTL);
	val = acp_readl(adata->acp5x_base + ACP_DSP_SW_INTR_CNTL);
	val |= 0x01;
	acp_writel(val, adata->acp5x_base + ACP_DSP_SW_INTR_CNTL);
}

static void acpbus_init_ring_buffer(struct acp5x_dev_data *adata,
				    struct acp_ring_buffer_info *ring_buf_info,
				    unsigned char *ring_buffer, u32 size)
{
	writel(0, (u32 *)&ring_buf_info->read_index);
	writel(0, (u32 *)&ring_buf_info->write_index);
	writel(size, (u32 *)&ring_buf_info->size);
	writel(0, (u32 *)&ring_buf_info->current_write_index);
}

static void  acpbus_config_command_resp_buffers(struct acp5x_dev_data *adata)
{
	acpbus_init_ring_buffer(adata,
				&adata->acp_sregmap->acp_cmd_ring_buf_info,
				adata->acp_sregmap->acp_cmd_buf,
				sizeof(adata->acp_sregmap->acp_cmd_buf)
				/ sizeof(unsigned char));

	acpbus_init_ring_buffer(adata,
				&adata->acp_sregmap->acp_resp_ring_buf_info,
				adata->acp_sregmap->acp_resp_buf,
				sizeof(adata->acp_sregmap->acp_resp_buf)
				/ sizeof(unsigned char));
}

static void init_scratchmemory(struct acp5x_dev_data *adata)
{
	pr_debug("%s\n", __func__);
	adata->acp_sregmap = (struct acp_scratch_register_config *)
			     (adata->acp5x_base + ACP_SCRATCH_REG_0 -
			      ACPBUS_REG_BASE_OFFSET);
	memset(adata->acp_sregmap_context, 0x00,
	       sizeof(struct acp_scratch_register_config));
	acp_copy_to_scratch_memory((unsigned char *)adata->acp_sregmap,
				   0x00,
				   adata->acp_sregmap_context,
				   sizeof(struct acp_scratch_register_config));
	acpbus_config_command_resp_buffers(adata);
}

void  acpbus_trigger_host_to_dsp_swintr(struct acp5x_dev_data *adata)
{
	u32 swintr_trigger = 0x00;

	swintr_trigger = acp_readl(adata->acp5x_base + ACP_SW_INTR_TRIG);
	swintr_trigger |= 0x01;
	acp_writel(swintr_trigger, adata->acp5x_base + ACP_SW_INTR_TRIG);
}

static int ev_test_cmd(unsigned int data, struct acp5x_dev_data *adata)
{
#pragma pack(1)
	struct {
		struct acpev_command      cmdid;
		struct acpev_cmd_test_ev1 cmd;
	} ev_cmd;
#pragma pack()
	int status  = 0;

	memset(&ev_cmd, 0x00, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_TEST_EV1;
	ev_cmd.cmd.data = data;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

static int ev_get_versioninfo(struct acp5x_dev_data *adata)
{
#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_get_ver_info   cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_GET_VER_INFO;

	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

static int ev_set_log_buffer(struct acp5x_dev_data *acp5x_data)
{
#pragma pack(1)
	struct {
		struct acpev_command             cmdid;
		struct acpev_cmd_set_log_buf     cmd;
	} ev_cmd;
#pragma pack()

	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id         = ACP_EV_CMD_SET_LOG_BUFFER;
	ev_cmd.cmd.flags           = 0;
	ev_cmd.cmd.log_buffer_offset = (acp5x_data->fw_bin_page_count +
					ACP_DRAM_PAGE_COUNT) *
					ACP_PAGE_SIZE;
	ev_cmd.cmd.log_size         = ACP_LOG_BUFFER_SIZE;

	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), acp5x_data, NULL);
	return status;
}

int ev_set_acp_clk_frequency(u32 clock_type,
			     u64 clock_freq,
			     struct acp5x_dev_data *acp5x_data)
{
#pragma pack(1)
	struct {
		struct acpev_command                cmdid;
		struct acpev_cmd_set_clock_frequency    cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id         = ACP_EV_CMD_SET_CLOCK_FREQUENCY;
	ev_cmd.cmd.clock_freq       = (unsigned int)clock_freq;
	ev_cmd.cmd.clock_type       = clock_type;
	acp5x_data->aclk_clock_freq = (unsigned int)clock_freq;
	status = acp_ev_send_cmd_and_wait_for_response(acp5x_data,
						       &ev_cmd,
						       sizeof(ev_cmd));

	if (status < 0)
		pr_err("%s status:0x%x\n", __func__, status);
	else
		pr_debug("ACLK Clock frequency : %d\n",
			 acp5x_data->aclk_clock_freq);
	return status;
}
EXPORT_SYMBOL(ev_set_acp_clk_frequency);

static int ev_acp_dma_start_from_sys_mem(struct acp5x_dev_data *acp5x_data)
{
#pragma pack(1)
	struct {
		struct acpev_command             cmdid;
		struct acpev_cmd_start_dma_frm_sys_mem  cmd;
	} ev_cmd;
#pragma pack()

	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id         = ACP_EV_CMD_START_DMA_FRM_SYS_MEM;
	ev_cmd.cmd.flags           = ACP_DMA_CIRCULAR;
	ev_cmd.cmd.dma_buf_offset = (acp5x_data->fw_bin_page_count +
				     ACP_DRAM_PAGE_COUNT +
				     ACP_LOG_BUFFER_PAGE_COUNT) *
				     ACP_PAGE_SIZE;
	ev_cmd.cmd.dma_buf_size  = ACP_DMA_BUFFER_TX_SIZE;
	pr_debug("fw_bin page count:0x%x\n", acp5x_data->fw_bin_page_count);
	pr_debug("dma_buf_offset:0x%x\n", ev_cmd.cmd.dma_buf_offset);
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), acp5x_data, NULL);
	return status;
}

static int ev_acp_dma_start_to_sys_mem(struct acp5x_dev_data *acp5x_data)
{
#pragma pack(1)
	struct {
		struct acpev_command             cmdid;
		struct acpev_cmd_start_dma_to_sys_mem cmd;
	} ev_cmd;
#pragma pack()

	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id         = ACP_EV_CMD_START_DMA_TO_SYS_MEM;
	ev_cmd.cmd.flags           = ACP_DMA_CIRCULAR;
	ev_cmd.cmd.dma_buf_offset = (acp5x_data->fw_bin_page_count +
				     ACP_DRAM_PAGE_COUNT +
				     ACP_LOG_BUFFER_PAGE_COUNT) *
				     ACP_PAGE_SIZE;
	ev_cmd.cmd.dma_buf_size  = ACP_DMA_BUFFER_TX_SIZE;

	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), acp5x_data, NULL);
	return status;
}

static int ev_acp_dma_stop_from_sys_mem(struct acp5x_dev_data *adata)
{
#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_stop_dma_frm_sys_mem  cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_STOP_DMA_FRM_SYS_MEM;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

static int ev_acp_dma_stop_to_sys_mem(struct acp5x_dev_data *adata)
{
#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_stop_dma_to_sys_mem cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_STOP_DMA_TO_SYS_MEM;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

int acp5x_start_acp_dma_from_sys_mem(struct acp5x_dev_data *adata)
{
	int ret  = 0;
	struct pci_dev *pci;

	if (!adata->acp_dma_virt_addr) {
		pci = pci_get_device(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID, NULL);
		/* ACP DMA Buffer allocation */
		adata->acp_dma_virt_addr =
				pci_alloc_consistent(pci,
						     ACP_DMA_BUFFER_TX_SIZE,
						     &adata->acp_dma_addr);
		if (!adata->acp_dma_virt_addr) {
			pr_err("unable to allocate acp dma buffer\n");
			return -ENOMEM;
		}
	}

	/* pte programming for ACP DMA buffer */
	configure_ptelist(ACP_DMA_BUFFER_TX, ACP_DMA_BUFFER_TX_PAGE_COUNT,
			  adata);
	/* sending ACP DMA buffer EV command to firmware */
	ret = ev_acp_dma_start_from_sys_mem(adata);
	return ret;
}
EXPORT_SYMBOL(acp5x_start_acp_dma_from_sys_mem);

int acp5x_start_acp_dma_to_sys_mem(struct acp5x_dev_data *adata)
{
	int ret  = 0;
	struct pci_dev *pci;

	if (!adata->acp_dma_virt_rx_addr) {
		pci = pci_get_device(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID, NULL);
		/* ACP DMA Buffer allocation */
		adata->acp_dma_virt_rx_addr =
				pci_alloc_consistent(pci,
						     ACP_DMA_BUFFER_RX_SIZE,
						     &adata->acp_dma_rx_addr);
		if (!adata->acp_dma_virt_rx_addr) {
			pr_err("unable to allocate acp dma buffer\n");
			return -ENOMEM;
		}
		memset((void *)adata->acp_dma_virt_rx_addr, 0x00,
		       ACP_DMA_BUFFER_RX_SIZE);
	}
	/* pte programming for ACP DMA buffer */
	configure_ptelist(ACP_DMA_BUFFER_RX, ACP_DMA_BUFFER_RX_PAGE_COUNT,
			  adata);
	/* sending ACP DMA buffer EV command to firmware */
	ret = ev_acp_dma_start_to_sys_mem(adata);
	return ret;
}
EXPORT_SYMBOL(acp5x_start_acp_dma_to_sys_mem);

static int ev_acp_ml_dma_start_from_sys_mem(struct acp5x_dev_data *acp5x_data)
{
#pragma pack(1)
	struct {
		struct acpev_command             cmdid;
		struct acpev_cmd_start_dma_frm_sys_mem  cmd;
	} ev_cmd;
#pragma pack()

	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id       = ACP_EV_CMD_START_DMA_FRM_SYS_MEM;
	ev_cmd.cmd.flags          = ACP_DMA_CIRCULAR;
	ev_cmd.cmd.dma_buf_offset = (acp5x_data->fw_bin_page_count +
				     ACP_DRAM_PAGE_COUNT +
				     ACP_LOG_BUFFER_PAGE_COUNT) *
				     ACP_PAGE_SIZE;
	ev_cmd.cmd.dma_buf_size   = ACP_DMA_BUFFER_TX_SIZE;
	pr_debug("fw_bin page count:0x%x\n", acp5x_data->fw_bin_page_count);
	pr_debug("dma_buf_offset:0x%x\n", ev_cmd.cmd.dma_buf_offset);
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), acp5x_data, NULL);
	return status;
}

static int ev_acp_ml_dma_start_to_sys_mem(struct acp5x_dev_data *acp5x_data)
{
#pragma pack(1)
	struct {
		struct acpev_command             cmdid;
		struct acpev_cmd_start_dma_to_sys_mem cmd;
	} ev_cmd;
#pragma pack()

	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id       = ACP_EV_CMD_START_DMA_TO_SYS_MEM;
	ev_cmd.cmd.flags          = ACP_DMA_CIRCULAR;
	ev_cmd.cmd.dma_buf_offset = (acp5x_data->fw_bin_page_count +
				     ACP_DRAM_PAGE_COUNT +
				     ACP_LOG_BUFFER_PAGE_COUNT +
				     ACP_DMA_BUFFER_TX_PAGE_COUNT) *
				     ACP_PAGE_SIZE;
	ev_cmd.cmd.dma_buf_size   = ACP_DMA_BUFFER_RX_SIZE;

	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), acp5x_data, NULL);
	return status;
}

static int ev_acp_ml_dma_stop_from_sys_mem(struct acp5x_dev_data *adata)
{

#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_stop_dma_frm_sys_mem  cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_STOP_DMA_FRM_SYS_MEM;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

static int ev_acp_ml_dma_stop_to_sys_mem(struct acp5x_dev_data *adata)
{
#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_stop_dma_to_sys_mem cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_STOP_DMA_TO_SYS_MEM;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

static int ev_acp_ml_dsp_loopback(struct acp5x_dev_data *adata)
{
#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_stop_dma_to_sys_mem cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_ML_DSP_LOOPBACK;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

int acp5x_ml_start_acp_dma_from_sys_mem(struct acp5x_dev_data *adata)
{
	int ret  = 0;
	struct pci_dev *pci;

	if (!adata->acp_dma_virt_addr) {
		pci = pci_get_device(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID, NULL);
		/* ACP DMA Buffer allocation */
		adata->acp_dma_virt_addr =
				pci_alloc_consistent(pci,
						     ACP_DMA_BUFFER_TX_SIZE,
						     &adata->acp_dma_addr);
		if (!adata->acp_dma_virt_addr) {
			pr_err("unable to allocate acp dma buffer\n");
			return -ENOMEM;
		}
	}

	/* pte programming for ACP DMA buffer */
	configure_ptelist(ACP_DMA_BUFFER_TX, ACP_DMA_BUFFER_TX_PAGE_COUNT,
			  adata);
	/* sending ACP DMA buffer EV command to firmware */
	ret = ev_acp_ml_dma_start_from_sys_mem(adata);
	return ret;
}
EXPORT_SYMBOL(acp5x_ml_start_acp_dma_from_sys_mem);

int acp5x_ml_start_acp_dma_to_sys_mem(struct acp5x_dev_data *adata)
{
	int ret  = 0;
	struct pci_dev *pci;

	if (!adata->acp_dma_virt_rx_addr) {
		pci = pci_get_device(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID, NULL);
		/* ACP DMA Buffer allocation */
		adata->acp_dma_virt_rx_addr =
				pci_alloc_consistent(pci,
						     ACP_DMA_BUFFER_RX_SIZE,
						     &adata->acp_dma_rx_addr);
		if (!adata->acp_dma_virt_rx_addr) {
			pr_err("unable to allocate acp dma buffer\n");
			return -ENOMEM;
		}
		memset((void *)adata->acp_dma_virt_rx_addr, 0x00,
		       ACP_DMA_BUFFER_RX_SIZE);
	}
	/* pte programming for ACP DMA buffer */
	configure_ptelist(ACP_DMA_BUFFER_RX, ACP_DMA_BUFFER_RX_PAGE_COUNT,
			  adata);
	/* sending ACP DMA buffer EV command to firmware */
	ret = ev_acp_ml_dma_start_to_sys_mem(adata);
	return ret;
}
EXPORT_SYMBOL(acp5x_ml_start_acp_dma_to_sys_mem);

int ev_acp_i2s_dma_start(struct acp5x_dev_data *acp5x_data,
			 struct stream_param *param)
{
#pragma pack(1)
	struct {
		struct acpev_command             cmdid;
		struct acpev_cmd_i2s_dma_start   cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id         = ACP_EV_CMD_I2S_DMA_START;
	ev_cmd.cmd.sample_rate      = param->sample_rate;
	ev_cmd.cmd.bit_depth        = param->bit_depth;
	ev_cmd.cmd.ch_count         = param->ch_count;
	ev_cmd.cmd.i2s_instance     = param->i2s_instance;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), acp5x_data, NULL);
	return status;
}
EXPORT_SYMBOL(ev_acp_i2s_dma_start);

int ev_acp_i2s_dma_stop(struct acp5x_dev_data *adata,
			unsigned int i2s_instance)
{
#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_i2s_dma_stop   cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_I2S_DMA_STOP;
	ev_cmd.cmd.i2s_instance = i2s_instance;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}
EXPORT_SYMBOL(ev_acp_i2s_dma_stop);

static int ev_acp_start_timer(struct acp5x_dev_data *adata)
{
#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_start_timer    cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_START_TIMER;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

static int ev_acp_stop_timer(struct acp5x_dev_data *adata)
{
#pragma pack(1)
	struct {
		struct acpev_command            cmdid;
		struct acpev_cmd_stop_timer     cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_STOP_TIMER;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}

int ev_acp_memory_pg_shutdown_enable(struct acp5x_dev_data *adata,
				     unsigned int bank_num)
{
#pragma pack(1)
	struct {
		struct acpev_command    cmdid;
		struct acpev_cmd_memory_pg_shutdown cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_MEMORY_PG_SHUTDOWN_ENABLE;
	ev_cmd.cmd.mem_bank_number = bank_num;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}
EXPORT_SYMBOL(ev_acp_memory_pg_shutdown_enable);

int ev_acp_memory_pg_shutdown_disable(struct acp5x_dev_data *adata,
				      unsigned int bank_num)
{
#pragma pack(1)
	struct {
		struct acpev_command    cmdid;
		struct acpev_cmd_memory_pg_shutdown cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_MEMORY_PG_SHUTDOWN_DISABLE;
	ev_cmd.cmd.mem_bank_number = bank_num;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}
EXPORT_SYMBOL(ev_acp_memory_pg_shutdown_disable);

int ev_acp_memory_pg_deepsleep_on(struct acp5x_dev_data *adata,
				  unsigned int bank_num)
{
#pragma pack(1)
	struct {
		struct acpev_command  cmdid;
		struct acpev_cmd_memory_pg_deepsleep cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_MEMORY_PG_DEEPSLEEP_ON;
	ev_cmd.cmd.mem_bank_number = bank_num;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}
EXPORT_SYMBOL(ev_acp_memory_pg_deepsleep_on);

int ev_acp_memory_pg_deepsleep_off(struct acp5x_dev_data *adata,
				   unsigned int bank_num)
{
#pragma pack(1)
	struct {
		struct acpev_command  cmdid;
		struct acpev_cmd_memory_pg_deepsleep cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id = ACP_EV_CMD_MEMORY_PG_DEEPSLEEP_OFF;
	ev_cmd.cmd.mem_bank_number = bank_num;
	status = ev_send_cmd(&ev_cmd, sizeof(ev_cmd), adata, NULL);
	return status;
}
EXPORT_SYMBOL(ev_acp_memory_pg_deepsleep_off);

int ev_set_i2s_mclk_frequency(u64 clock_freq,
			      struct acp5x_dev_data *acp5x_data)
{
#pragma pack(1)
	struct {
		struct acpev_command                cmdid;
		struct acpev_cmd_set_i2s_mclk_freq  cmd;
	} ev_cmd;
#pragma pack()
	int status = 0;

	memset(&ev_cmd, 0, sizeof(ev_cmd));
	ev_cmd.cmdid.cmd_id         = ACP_EV_CMD_SET_I2S_MCLK_FREQ;
	ev_cmd.cmd.clock_freq       = (unsigned int)clock_freq;
	acp5x_data->i2s_mclk_freq = (unsigned int)clock_freq;
	status = acp_ev_send_cmd_and_wait_for_response(acp5x_data,
						       &ev_cmd,
						       sizeof(ev_cmd));

	if (status < 0)
		pr_err("%s status:0x%x\n", __func__, status);
	else
		pr_debug("I2S Master  Clock frequency : %d\n",
			 acp5x_data->i2s_mclk_freq);
	return status;
}
EXPORT_SYMBOL(ev_set_i2s_mclk_frequency);

int send_ev_command(struct acp5x_dev_data *adata, unsigned int cmd_id)
{
	int status = 0;
	int do_transition = 0;
	struct pci_dev *p_acpdev;
	unsigned long flags;

	pr_debug("%s cmd_id:0x%x\n", __func__, cmd_id);
	switch (cmd_id) {
	case ACP_EV_CMD_GET_VER_INFO:
		status = ev_get_versioninfo(adata);
		break;
	case ACP_EV_CMD_TEST_EV1:
		status = ev_test_cmd(0xDEAD, adata);
		break;
	case ACP_EV_CMD_SET_LOG_BUFFER:
		status = ev_set_log_buffer(adata);
		break;
	case ACP_EV_CMD_START_DMA_FRM_SYS_MEM:
		status = acp5x_start_acp_dma_from_sys_mem(adata);
		break;
	case ACP_EV_CMD_STOP_DMA_FRM_SYS_MEM:
		status = ev_acp_dma_stop_from_sys_mem(adata);
		break;
	case ACP_EV_CMD_START_DMA_TO_SYS_MEM:
		status = acp5x_start_acp_dma_to_sys_mem(adata);
		break;
	case ACP_EV_CMD_STOP_DMA_TO_SYS_MEM:
		status = ev_acp_dma_stop_to_sys_mem(adata);
		break;
	case ACP_EV_CMD_START_TIMER:
		status = ev_acp_start_timer(adata);
		break;
	case ACP_EV_CMD_STOP_TIMER:
		status = ev_acp_stop_timer(adata);
		break;
	case ACP_EV_CMD_ML_START_DMA_FRM_SYS_MEM:
		status = acp5x_ml_start_acp_dma_from_sys_mem(adata);
		break;
	case ACP_EV_CMD_ML_STOP_DMA_FRM_SYS_MEM:
		status = ev_acp_ml_dma_stop_from_sys_mem(adata);
		break;
	case ACP_EV_CMD_ML_START_DMA_TO_SYS_MEM:
		status = acp5x_ml_start_acp_dma_to_sys_mem(adata);
		break;
	case ACP_EV_CMD_ML_STOP_DMA_TO_SYS_MEM:
		status = ev_acp_ml_dma_stop_to_sys_mem(adata);
		break;
	case ACP_EV_CMD_ML_DSP_LOOPBACK:
		status = ev_acp_ml_dsp_loopback(adata);
		break;
	case ACP_EV_CMD_ML_SET_POWER_MODE_SUSPEND:
		spin_lock_irqsave(&pm_state.lock, flags);
		if (pm_state.is_active) {
			pm_state.is_active = 0;
			do_transition = 1;
		}
		spin_unlock_irqrestore(&pm_state.lock, flags);

		if (do_transition) {
			p_acpdev = pci_get_device(PCI_VENDOR_ID_AMD,
				ACP_DEVICE_ID, NULL);
			acp5x_runtime_suspend(&p_acpdev->dev);
			pr_debug("ACP EV Set Power Mode Suspend\n");
		}
		break;
	case ACP_EV_CMD_ML_SET_POWER_MODE_RESUME:
		spin_lock_irqsave(&pm_state.lock, flags);
		if (!pm_state.is_active) {
			pm_state.is_active = 1;
			do_transition = 1;
		}
		spin_unlock_irqrestore(&pm_state.lock, flags);

		if (do_transition) {
			p_acpdev = pci_get_device(PCI_VENDOR_ID_AMD,
				ACP_DEVICE_ID, NULL);
			acp5x_runtime_resume(&p_acpdev->dev);
			pr_debug("ACP EV Set Power Mode Resume\n");
		}
		break;
	default:
		break;
	}
	pr_debug("%s satus:%d\n", __func__, status);
	return status;
}
EXPORT_SYMBOL(send_ev_command);

int acp5x_set_fw_logging(struct acp5x_dev_data *adata)
{
	int ret  = 0;
	struct pci_dev *pci;

	if (!adata->acp_log_virt_addr) {
		pci = pci_get_device(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID, NULL);
		/* ACP Log Buffer allocation */
		adata->acp_log_virt_addr =
				pci_alloc_consistent(pci,
						     ACP_LOG_BUFFER_SIZE,
						     &adata->acp_log_dma_addr);
		if (!adata->acp_log_virt_addr) {
			pr_err("unable to allocate acp log buffer\n");
			return -ENOMEM;
		}
		memset((void *)adata->acp_log_virt_addr, 0,
		       ACP_LOG_BUFFER_SIZE);
	}
	/* pte programming for ACP Log buffer */
	configure_ptelist(ACP_LOGGER, 0x01, adata);
	/* sending Log buffer EV command to firmware */
	ret = send_ev_command(adata, ACP_EV_CMD_SET_LOG_BUFFER);
	return ret;
}
EXPORT_SYMBOL(acp5x_set_fw_logging);

static int  acpbus_configure_and_run_sha_dma(struct acp5x_dev_data *adata,
					     void *image_addr,
					     u32  sha_dma_start_addr,
					     u32 sha_dma_dest_addr,
					     u32  image_length)
{
	int  ret = 0;
	int  val = 0;
	int  sha_dma_status = 0;
	u32  delay_count = 0;
	struct app_secr_header_rv *pheader = NULL;
	u32  image_offset = 0;
	u32  sha_message_length = 0;

	u32  acp_sha_dma_start_addr;
	u32  acp_sha_dma_dest_addr;
	u32  acp_sha_msg_length;
	u32  acp_sha_dma_cmd =  0;
	u32  acp_sha_transfer_byte_count =  0;
	u32  acp_sha_dsp_fw_qualifier;
	u32  sha_includes_header = 0;
	unsigned long jiffies = msecs_to_jiffies(5000);
	unsigned int value = 0;

	if (image_addr) {
		pheader = (struct app_secr_header_rv *)image_addr;
		if (pheader->sign_option == 0x01) {
			pr_debug("Signed Image header version:0x%x\n",
				 pheader->header_version);
			image_offset = 0;
			sha_message_length = pheader->size_fw_to_be_signed +
					   sizeof(struct app_secr_header_rv);
			sha_includes_header = 0x01;
		} else  {
			pr_debug("Unsigned Image\n");
			image_offset = 0;
			sha_message_length = image_length;
		}
		val = acp_readl(adata->acp5x_base + ACP_SHA_DMA_CMD);
		if (val & 0x01) {
			val = 0x02;
			acp_writel(val, adata->acp5x_base + ACP_SHA_DMA_CMD);
			delay_count = 0;

			while (1) {
				val = acp_readl(adata->acp5x_base +
						ACP_SHA_DMA_CMD_STS);
				if (!(val & 0x02)) {
					if (delay_count > 0x100) {
						pr_err("SHA DMA Failed to Reset\n");
						ret  = -ETIMEDOUT;
						break;
					}
					udelay(1);
					delay_count++;
				} else {
					val = 0x0;
					acp_writel(val, adata->acp5x_base +
						  ACP_SHA_DMA_CMD);
					break;
				}
			}
		}
		acp_sha_dma_start_addr = sha_dma_start_addr + image_offset;
		acp_writel(acp_sha_dma_start_addr, adata->acp5x_base +
			   ACP_SHA_DMA_STRT_ADDR);
		acp_sha_dma_dest_addr = sha_dma_dest_addr;
		acp_writel(acp_sha_dma_dest_addr, adata->acp5x_base +
			   ACP_SHA_DMA_DESTINATION_ADDR);
		acp_sha_msg_length = sha_message_length;
		acp_writel(acp_sha_msg_length, adata->acp5x_base +
			   ACP_SHA_MSG_LENGTH);
		if (sha_includes_header)
			acp_writel(0x01, adata->acp5x_base +
				   ACP_SHA_INCLUDE_HEADER);
		/* enables SHA DMA IOC bit & Run Bit */
		acp_sha_dma_cmd = 0x05;
		acp_writel(acp_sha_dma_cmd, adata->acp5x_base +
			   ACP_SHA_DMA_CMD);
		acp_sha_dma_cmd = acp_readl(adata->acp5x_base +
					    ACP_SHA_DMA_CMD);
		pr_debug("ACP_SHA_DMA_CMD 0x%x\n", acp_sha_dma_cmd);

		delay_count = 0;
		while (1) {
			acp_sha_transfer_byte_count =
				acp_readl(adata->acp5x_base
					  + ACP_SHA_TRANSFER_BYTE_CNT);
			if (acp_sha_transfer_byte_count != sha_message_length) {
				if (delay_count > 0xA00) {
					pr_err("sha dma count :0X%x\n",
					       acp_sha_transfer_byte_count);
					ret  = -ETIMEDOUT;
					break;
				}
				udelay(1);
				delay_count++;
			} else {
				pr_debug("sha dma transfer count :0X%x\n",
					 acp_sha_transfer_byte_count);
				break;
			}
		}

		if (ret) {
			pr_err("SHA DMA Failed to Transfer Message Length\n");
			goto sha_exit;
		}
		sha_dma_status =
		wait_event_interruptible_timeout(adata->wait_queue_sha_dma,
						 adata->sha_irq_stat != 0,
						 jiffies);
		if (!sha_dma_status)
			pr_debug("sha dma waitqueue timeout error :0x%x\n",
				 sha_dma_status);
		else if (sha_dma_status < 0)
			pr_debug("sha dma wait queue error: 0x%x\n",
				 sha_dma_status);
		pr_debug("sha irq status : 0x%x\n", adata->sha_irq_stat);
		adata->sha_irq_stat  = 0;
		value = acp_readl(adata->acp5x_base +
				  ACP_SHA_TRANSFER_BYTE_CNT);
		pr_debug("ACP_SHA_TRANSFER_BYTE_CNT 0x%x\n", value);
		value = acp_readl(adata->acp5x_base + ACP_SHA_DMA_ERR_STATUS);
		pr_debug("ACP_SHA_DMA_ERR_STATUS 0x%x\n", value);
		acp_sha_dsp_fw_qualifier = acp_readl(adata->acp5x_base +
						     ACP_SHA_DSP_FW_QUALIFIER);
		pr_debug("ACP_SHA_DSP_FW_QUALIFIER 0x%x\n",
			 acp_sha_dsp_fw_qualifier);

		if ((acp_sha_dsp_fw_qualifier & 0x01) != 0x01)
			pr_err("PSP validation failed on ACP FW/Plugin image\n");
	}

sha_exit:
	return ret;
}

static int acp_dma_status(struct acp5x_dev_data *adata, unsigned char ch_num)
{
	int dma_status = 0;
	u32 acp_dma_cntl0 = 0, acp_dma_ch_stat = 0;
	u32 val = 0;
	unsigned long jiffies = msecs_to_jiffies(5000);

	acp_dma_cntl0 = acp_readl(adata->acp5x_base + ACP_DMA_CNTL_0 +
				 (ch_num * sizeof(u32)));
	pr_debug("ACP_DMA_CNTL_0 + 0x%x acp_dma_cntl0:0x%x\n", ch_num,
		 acp_dma_cntl0);
	dma_status =
		wait_event_interruptible_timeout(adata->dma_queue,
						 adata->dma_ioc_event != 0,
						 jiffies);
	if (dma_status == 0x00) {
		/* when DMA IoC Interrupt is missed, check DMA channel status
		 * if DMA transfer is not completed report Timeout error
		 */
		acp_dma_ch_stat = acp_readl(adata->acp5x_base +
					    ACP_DMA_CH_STS);
		pr_err("DMA_TIMEOUT: acp dma channel status 0x%x\n",
		       acp_dma_ch_stat);
		if (acp_dma_ch_stat & (1 << ch_num)) {
			val =  acp_readl(adata->acp5x_base +
					(ACP_DMA_ERR_STS_0 +
					(ch_num * sizeof(u32))));
			pr_err("DMA_TIMEOUT: ACP_DMA_ERR_STS_0 : 0x%x\n", val);
			return -ETIMEDOUT;
		}
	} else if (dma_status < 0) {
		pr_debug("acp dma wait queue error: 0x%x\n",
			 dma_status);
		return dma_status;
	}
	pr_debug("acp dma irq status : 0x%x\n", adata->dma_ioc_event);
	adata->dma_ioc_event  = 0;
	acp_dma_cntl0 = acp_readl(adata->acp5x_base + ACP_DMA_CNTL_0 +
				 (ch_num * sizeof(u32)));
	/* clear the dma channel run bit to indicate this channel
	 * is free for new transfer
	 */
	acp_dma_cntl0 &= ACP_DMA_RUN_IOC_DISABLE_MASK;
	acp_writel(acp_dma_cntl0, adata->acp5x_base + ACP_DMA_CNTL_0 +
		  (ch_num * sizeof(u32)));
	val =  acp_readl(adata->acp5x_base + (ACP_DMA_ERR_STS_0 +
			(ch_num * sizeof(u32))));
	pr_debug("ACP_DMA_ERR_STS_0 : 0x%x\n", val);
	return 0;
}

static void acpbus_configure_dma_descriptor(struct acp5x_dev_data *adata,
					    unsigned short dscr_idx,
					    struct acp_config_dma_descriptor
					    *dscr_info)
{
	unsigned int mem_offset = ACP_SRBM_DRAM_DMA_BASE_ADDR +
				(dscr_idx *
				 sizeof(struct acp_config_dma_descriptor));

	acp5x_srbm_write(adata, mem_offset, dscr_info->src_addr);
	acp5x_srbm_write(adata, mem_offset + 0x04,
			 dscr_info->dest_addr);
	acp5x_srbm_write(adata, mem_offset + 0x08,
			 dscr_info->dma_transfer_count.u32_all);
}

static int acpbus_config_dma_channel(struct acp5x_dev_data *adata,
				     unsigned char ch_num,
				     unsigned short dscr_start_idx,
				     unsigned short dscr_count,
				     unsigned int priority_level)
{
	int status = 0;
	u32 val = 0;
	u32 acp_error_stat = 0, dma_error_stat = 0;
	u32 acp_dma_dscr_cnt0 = 0, acp_dma_dscr_start_idx0 = 0;
	u32 acp_dma_prio0 = 0,  acp_dma_cntl0 = 0;
	u32 timeout  = 0x00;

	val = acp_readl(adata->acp5x_base +
		       (ACP_DMA_CNTL_0 + (ch_num * sizeof(u32))));
	pr_debug("mACP_DMA_CNTL_0 : 0x%x\n", val);
	acp_writel(ACP_DMA_DISABLE_MASK, adata->acp5x_base +
		  (ACP_DMA_CNTL_0 + (ch_num * sizeof(u32))));
	val = acp_readl(adata->acp5x_base + ACP_DMA_CNTL_0 +
		       (ch_num * sizeof(u32)));
	pr_debug("mACP_DMA_CNTL_0 : 0x%x\n", val);

	while (1) {
		val = acp_readl(adata->acp5x_base + ACP_DMA_CH_RST_STS);
		pr_debug("ACP_DMA_CH_RST_STS : 0x%x\n", val);
		val &= 0xFF;
		if (!(val & (1 << ch_num))) {
			udelay(5);
			if (timeout > 0xff) {
				pr_err("DMA_CHANNEL RESET ERROR TIMEOUT\n");
				acp_error_stat = acp_readl(adata->acp5x_base +
							   ACP_ERROR_STATUS);
				pr_err("ACP_ERROR_STATUS :0x%x\n",
				       acp_error_stat);
				dma_error_stat = acp_readl(adata->acp5x_base +
							  (ACP_DMA_ERR_STS_0 +
							  (ch_num *
							   sizeof(u32))));
				pr_err("ACP_DMA_ERR_STS :0x%x\n",
				       dma_error_stat);
				status = -ETIMEDOUT;
				break;
			}
			timeout++;
		} else {
			pr_debug("ACP_DMA_CH_RST_%d is done\n", ch_num);
			break;
		}
	}

	if (status < 0)
		return status;
	acp_dma_cntl0 = 0x00;
	acp_writel(acp_dma_cntl0, adata->acp5x_base +
		  (ACP_DMA_CNTL_0 + (ch_num * sizeof(u32))));
	acp_dma_cntl0 =  acp_readl(adata->acp5x_base +
				  (ACP_DMA_CNTL_0 +
				  (ch_num * sizeof(u32))));
	pr_debug("ACP_DMA_CNTL_0_%d : 0x%x\n", ch_num, acp_dma_cntl0);

	acp_dma_dscr_cnt0 = dscr_count;
	acp_writel(acp_dma_dscr_cnt0, adata->acp5x_base +
		  (ACP_DMA_DSCR_CNT_0 + (ch_num * sizeof(u32))));
	acp_dma_dscr_cnt0 =  acp_readl(adata->acp5x_base +
				      (ACP_DMA_DSCR_CNT_0 +
				      (ch_num * sizeof(u32))));
	pr_debug("ACP_DMA_DSCR_CNT_0 : 0x%x\n", acp_dma_dscr_cnt0);

	acp_dma_dscr_start_idx0 = dscr_start_idx;
	acp_writel(acp_dma_dscr_start_idx0, adata->acp5x_base +
		  (ACP_DMA_DSCR_STRT_IDX_0 + (ch_num * sizeof(u32))));
	acp_dma_dscr_start_idx0 =  acp_readl(adata->acp5x_base +
					    (ACP_DMA_DSCR_STRT_IDX_0 +
					    (ch_num * sizeof(u32))));
	pr_debug("ACP_DMA_DSCR_STRT_IDX_0 : 0x%x\n", acp_dma_dscr_start_idx0);
	acp_dma_prio0 = priority_level;
	acp_writel(acp_dma_prio0, adata->acp5x_base +
		  (ACP_DMA_PRIO_0 + (ch_num * sizeof(u32))));
	acp_dma_prio0 =  acp_readl(adata->acp5x_base +
				  (ACP_DMA_PRIO_0 + (ch_num * sizeof(u32))));
	pr_debug("ACP_DMA_PRIO_0 : 0x%x\n", acp_dma_prio0);

	acp_dma_cntl0 |= ACP_DMA_CH_RUN | ACP_DMA_CH_IOC_ENABLE;
	acp_writel(acp_dma_cntl0, adata->acp5x_base +
		  (ACP_DMA_CNTL_0 + (ch_num * sizeof(u32))));
	acp_dma_cntl0 =  acp_readl(adata->acp5x_base +
				  (ACP_DMA_CNTL_0 + (ch_num * sizeof(u32))));
	pr_debug("ACP_DMA_CNTL_0_%d : 0x%x\n", ch_num, acp_dma_cntl0);
	return  status;
}

static int acpbus_dma_start(struct acp5x_dev_data *adata,
			    unsigned char *p_ch_num,
			    unsigned short *p_dscr_count,
			    struct acp_config_dma_descriptor *p_dscr_info,
			    unsigned int priority_level)
{
	int status = -1;
	u16 dscr_start_idx = 0x00;
	u16 dscr;
	u16 dscr_count;
	u16 dscr_num;

	if ((p_ch_num) && (p_dscr_count) && (p_dscr_info)) {
		status = 0x00;
		dscr_count = *p_dscr_count;
		/* currently channel number hard coded to zero,
		 * As single channel is used for ACP DMA transfers
		 * from host driver.
		 * Below are the instances ACP DMA transfer
		 * initiated from ACP PCI driver.
		 * - For loading firmware data binary to
		 * ACP DRAM from system memory.
		 * - saving DRAM context before ACP device entering
		 * in to suspend state
		 * - restoring DRAM context during ACP device resume
		 * Note: In all above mentioned scenarios firmware will
		 * stalled.
		 */
		*p_ch_num = 0x00;
		for (dscr = 0; dscr < *p_dscr_count; dscr++) {
			dscr_num = dscr_start_idx + dscr;
			/* configure particular descriptor registers */
			acpbus_configure_dma_descriptor(adata,
							dscr_num,
							p_dscr_info++);
		}
		/* configure the dma channel and start the dma transfer */
		status = acpbus_config_dma_channel(adata,
						   *p_ch_num,
						   dscr_start_idx,
						   *p_dscr_count,
						    priority_level);
		if (status < 0)
			pr_err("acpbus config dma channel failed:%d\n", status);
	}
	return status;
}

static int acpbus_configure_and_run_dma(struct acp5x_dev_data *adata,
					unsigned int src_addr,
					unsigned int  dest_addr,
					unsigned int dsp_data_size,
					unsigned char *p_ch_num)
{
	int status = 0x00;
	u32 priority_level = 0x00;
	unsigned int desc_count = 0;
	unsigned char ch_no = 0xFF;
	unsigned int index;

	while (dsp_data_size >= ACP_PAGE_SIZE) {
		adata->dscr_info[desc_count].src_addr = src_addr +
							(desc_count *
							 ACP_PAGE_SIZE);
		adata->dscr_info[desc_count].dest_addr = dest_addr +
							 (desc_count *
							  ACP_PAGE_SIZE);
		adata->dscr_info[desc_count].dma_transfer_count.bits.count =
								ACP_PAGE_SIZE;
		desc_count++;
		dsp_data_size -= ACP_PAGE_SIZE;
	}
	if (dsp_data_size > 0) {
		adata->dscr_info[desc_count].src_addr = src_addr +
							(desc_count *
							 ACP_PAGE_SIZE);
		adata->dscr_info[desc_count].dest_addr = dest_addr +
							 (desc_count *
							  ACP_PAGE_SIZE);
		adata->dscr_info[desc_count].dma_transfer_count.bits.count =
								dsp_data_size;
		adata->dscr_info[desc_count].dma_transfer_count.bits.ioc = 0x01;
		desc_count++;
	} else if (desc_count > 0) {
		adata->dscr_info[desc_count - 1].dma_transfer_count.bits.ioc =
									0x01;
	}

	status = acpbus_dma_start(adata, &ch_no, (unsigned short *)&desc_count,
				  adata->dscr_info, priority_level);
	if (status == 0x00)
		*p_ch_num = ch_no;
	else
		pr_debug("acpbus_dma_start failed\n");
	for (index = 0; index < desc_count; index++) {
		memset(&adata->dscr_info[index], 0x00,
		       sizeof(struct acp_config_dma_descriptor));
	}
	return status;
}

static int acp5x_bus_get_dspfw_runstatus(struct acp5x_dev_data *adata)
{
	unsigned int val = 0;
	u32 delay_count = 0;
	int ret = 0;

	while (1) {
		val = acp_readl(adata->acp5x_base +
				ACP_FUTURE_REG_ACLK_4);
		if (val != ACP_FW_RUN_STATUS_ACK) {
			delay_count++;
			/* 2 seconds set as timeout value */
			if (delay_count > 200000) {
				pr_err("FW run status timeout occurred\n");
				ret = -ETIMEDOUT;
				break;
			}
			usleep_range(5, 10);
		} else {
			pr_debug("ACP_FUTURE_REG_ACLK_4 : 0x%x\n", val);
			val = 0x00;
			acp_writel(val, adata->acp5x_base +
				   ACP_FUTURE_REG_ACLK_4);
			break;
		}
	}
	return ret;
}

static int acp5x_save_dram_context(struct acp5x_dev_data *adata)
{
	int ret = 0;
	unsigned char ch_num = 0x00;
	unsigned int dest_addr = 0x00;

	if (adata->virt_addr) {
		memcpy((void *)adata->virt_addr, (void *)adata->acp_fw->data,
		       adata->fw_bin_size);
	}
	if (adata->fw_data_virt_addr) {
		memset((void *)adata->fw_data_virt_addr, 0,
		       ACP_DEFAULT_DRAM_LENGTH);
		dest_addr =  ACP_SYSTEM_MEMORY_WINDOW +
			     (adata->fw_bin_page_count * ACP_PAGE_SIZE);
		ret = acpbus_configure_and_run_dma(adata,
						   ACP_DATA_RAM_BASE_ADDRESS,
						   dest_addr,
						   ACP_DEFAULT_DRAM_LENGTH,
						   &ch_num);
		if (ret == 0x00) {
			ret = acp_dma_status(adata, ch_num);
			if (ret < 0)
				pr_err("acp dma status error : %d\n", ret);
		} else {
			pr_err("acp dma ch reset failed error: %d\n", ret);
		}
	}
	return ret;
}

static int acp5x_restore_context(struct acp5x_dev_data *adata)
{
	int ret = 0;
	unsigned int val = 0;
	unsigned int src_addr;
	unsigned char ch_num = 0x00;
	union acp_sr_fence_update acp_resume;
	unsigned int acp_sha_dsp_fw_qualifier;

	acp_resume.u32_all = 0;
	if (adata->virt_addr) {
		configure_ptelist(FW_BIN, adata->fw_bin_page_count, adata);
		ret = acpbus_configure_and_run_sha_dma(adata,
						       adata->virt_addr,
						       ACP_SYSTEM_MEMORY_WINDOW,
						       ACP_IRAM_BASE_ADDRESS,
						       adata->fw_bin_size);
		if (ret < 0) {
			pr_err("SHA DMA transfer failed status: 0x%x\n", ret);
			return ret;
		}
	}

	if (adata->fw_data_virt_addr) {
		configure_ptelist(FW_DATA_BIN, ACP_DRAM_PAGE_COUNT, adata);
		src_addr =  ACP_SYSTEM_MEMORY_WINDOW +
			    (adata->fw_bin_page_count * ACP_PAGE_SIZE);
		ret = acpbus_configure_and_run_dma(adata,
						   src_addr,
						   ACP_DATA_RAM_BASE_ADDRESS,
						   ACP_DEFAULT_DRAM_LENGTH,
						   &ch_num);
		if (ret < 0) {
			pr_err("acp dma transfer failed:%d\n", ret);
			return ret;
		}
		ret = acp_dma_status(adata, ch_num);
		if (ret < 0) {
			pr_err("acp dma transfer status :%d\n", ret);
			return ret;
		}
		acp_resume.bits.is_resume = 0x02;
		acp_writel(acp_resume.u32_all, adata->acp5x_base +
			   ACP_FUTURE_REG_ACLK_3);
		val = acp_readl(adata->acp5x_base + ACP_FUTURE_REG_ACLK_3);
		pr_debug("ACP_FUTURE_REG_ACLK_3:0x%x\n", val);
		acp_sha_dsp_fw_qualifier = acp_readl(adata->acp5x_base +
						     ACP_SHA_DSP_FW_QUALIFIER);
		pr_debug("ACP_SHA_DSP_FW_QUALIFIER 0x%x\n",
			 acp_sha_dsp_fw_qualifier);

		val = acp_readl(adata->acp5x_base + ACP_DSP0_RUNSTALL);
		pr_debug("ACP_DSP0_RUNSTALL : 0x%0x\n", val);
		if (val) {
			val = 0;
			acp_writel(val, adata->acp5x_base + ACP_DSP0_RUNSTALL);
			val = acp_readl(adata->acp5x_base + ACP_DSP0_RUNSTALL);
			pr_debug("ACP_DSP0_RUNSTALL : 0x%0x\n", val);
		}
	}
	return ret;
}

static int acp5x_runtime_suspend(struct device *dev)
{
	unsigned int val;
	ktime_t start, finish;
	s64 time_diff;
	int ret = 0;
	struct acp5x_dev_data *adata = dev_get_drvdata(dev);
	unsigned int mem_size =  sizeof(struct acp_scratch_register_config);

	if (acp5x_runtime_can_sr() == 0) {
		pr_debug("%s!!---ACP D3HOT RUNTIME SUSPEND SUPPRESSED\n",
			__func__);
		return 0;
	}

	pr_debug("%s!!---ACP D3HOT ENTRY STARTED---!!\n", __func__);
	start = ktime_get();
	/* when acp firmware loading is failed skip below sequence */
	if (!adata->fw_load_error) {
		val = acp_readl(adata->acp5x_base + ACP_DSP0_RUNSTALL);
		pr_debug("%s ACP_DSP0_RUNSTALL : 0x%0x\n", __func__, val);

		if (!val) {
			val = 0x01;
			acp_writel(val, adata->acp5x_base + ACP_DSP0_RUNSTALL);
			val = acp_readl(adata->acp5x_base + ACP_DSP0_RUNSTALL);
			pr_debug("%s ACP_DSP0_RUNSTALL : 0x%0x\n", __func__,
				 val);
		}
		ret = acp5x_save_dram_context(adata);
		if (ret < 0) {
			pr_err("acp dram context save failed\n");
			return ret;
		}
		/* save scratch memory register context */
		if (adata->acp_sregmap_context) {
			memset(adata->acp_sregmap_context, 0x00,
			       sizeof(struct acp_scratch_register_config));
			acp_copy_from_scratch_memory((unsigned char *)
						     adata->acp_sregmap,
						     0x00,
						     adata->acp_sregmap_context,
						     mem_size);
		}
	}
	acp_remove_responses(adata, true, NULL, NULL);
	acp5x_clear_and_disable_dspsw_intr(adata);
	acp5x_clear_and_disable_external_intr(adata);
	acp5x_deinit(adata->acp5x_base);
	adata->is_acp_active = 0x00;
	finish = ktime_get();
	time_diff = ktime_us_delta(finish, start);
	pr_debug("%s!!---ACP D3HOT FINISHED in %5lld micro seconds---!!\n",
		 __func__, time_diff);

	return 0;
}

static int acp5x_runtime_resume(struct device *dev)
{
	int ret = 0;
	int fw_run_status = 0;
	ktime_t start, finish;
	s64 time_diff;
	struct acp5x_dev_data *adata = dev_get_drvdata(dev);
	unsigned int mem_size =  sizeof(struct acp_scratch_register_config);

	if (acp5x_runtime_can_sr() == 0) {
		pr_debug("%s!!---ACP D0 RUNTIME RESUME SUPPRESSED\n", __func__);
		return 0;
	}

	pr_debug("%s!!---ACP D0 ENTRY STARTED---!!\n", __func__);
	start = ktime_get();
	acp5x_init(adata->acp5x_base);
	acp5x_set_mlacp_registers(adata->acp5x_base);
	acp5x_enable_extirq(adata);
	configure_acp5x_groupregisters(adata);
	configure_dma_descriptiors(adata);
	/* when acp firmware loading is failed skip below sequence */
	if (!adata->fw_load_error) {
		/* restore scratch memory context */
		if (adata->acp_sregmap_context) {
			acp_copy_to_scratch_memory((unsigned char *)
						   adata->acp_sregmap,
						   0x00,
						   adata->acp_sregmap_context,
						   mem_size);
		}
		ret = acp5x_restore_context(adata);
		if (ret < 0) {
			pr_err("acp restore context failed\n");
			return ret;
		}
		fw_run_status = acp5x_bus_get_dspfw_runstatus(adata);
		pr_debug("fw_run_status 0x%x\n", fw_run_status);
		if (fw_run_status == 0x00)
			adata->is_acp_active = 0x01;
		acpbus_config_command_resp_buffers(adata);
	}
	finish = ktime_get();
	time_diff = ktime_us_delta(finish, start);
	pr_debug("%s!!---ACP D0 ENTRY FINISHED in %5lld micro seconds---!!\n",
		 __func__, time_diff);

	return 0;
}

static int acp5x_suspend(struct device *dev)
{
	unsigned int val;
	int ret = 0;
	s64 time_diff;
	ktime_t start, finish;
	struct acp5x_dev_data *adata = dev_get_drvdata(dev);
	unsigned int mem_size =  sizeof(struct acp_scratch_register_config);

	if (acp5x_can_sr() == 0) {
		pr_debug("%s!!---ACP D3HOT SUSPEND SUPPRESSED\n", __func__);
		return 0;
	}

	start = ktime_get();
	pr_debug("%s!!---ACP D3HOT ENTRY STARTED---!!\n", __func__);
	/* when acp firmware loading is failed skip below sequence */
	if (!adata->fw_load_error) {
		val = acp_readl(adata->acp5x_base + ACP_DSP0_RUNSTALL);
		pr_debug("%s ACP_DSP0_RUNSTALL : 0x%0x\n", __func__, val);
		if (!val) {
			val = 0x01;
			acp_writel(val, adata->acp5x_base + ACP_DSP0_RUNSTALL);
			val = acp_readl(adata->acp5x_base + ACP_DSP0_RUNSTALL);
			pr_debug("%s ACP_DSP0_RUNSTALL : 0x%0x\n", __func__,
				 val);
		}

		ret = acp5x_save_dram_context(adata);
		if (ret < 0) {
			pr_err("acp dram context save failed\n");
			return ret;
		}
		/* save scratch memory register context */
		if (adata->acp_sregmap_context) {
			memset(adata->acp_sregmap_context, 0x00,
			       sizeof(struct acp_scratch_register_config));
			acp_copy_from_scratch_memory((unsigned char *)
						     adata->acp_sregmap,
						     0x00,
						     adata->acp_sregmap_context,
						     mem_size);
		}
	}
	acp_remove_responses(adata, true, NULL, NULL);
	acp5x_clear_and_disable_dspsw_intr(adata);
	acp5x_clear_and_disable_external_intr(adata);
	acp5x_deinit(adata->acp5x_base);
	adata->is_acp_active = 0x00;
	finish = ktime_get();
	time_diff = ktime_us_delta(finish, start);
	pr_debug("%s!!---ACP D3HOT FINISHED in %5lld micro seconds---!!\n",
		 __func__, time_diff);
	return 0;
}

static int acp5x_resume(struct device *dev)
{
	s64 time_diff;
	ktime_t start, finish;
	int ret = 0;
	int fw_run_status = 0;
	struct acp5x_dev_data *adata = dev_get_drvdata(dev);
	unsigned int mem_size =  sizeof(struct acp_scratch_register_config);

	if (acp5x_can_sr() == 0) {
		pr_debug("%s!!---ACP D3HOT RESUME SUPPRESSED\n", __func__);
		return 0;
	}

	start = ktime_get();
	pr_debug("%s!!---ACP D0 ENTRY STARTED---!!\n", __func__);
	acp5x_init(adata->acp5x_base);
	acp5x_set_mlacp_registers(adata->acp5x_base);
	acp5x_enable_extirq(adata);
	configure_acp5x_groupregisters(adata);
	configure_dma_descriptiors(adata);
	/* when acp firmware loading is failed skip below sequence */
	if (!adata->fw_load_error) {
		/* restore scratch memory context */
		if (adata->acp_sregmap_context) {
			acp_copy_to_scratch_memory((unsigned char *)
					adata->acp_sregmap,
					0x00,
					adata->acp_sregmap_context,
					mem_size);
		}
		ret = acp5x_restore_context(adata);
		if (ret < 0) {
			pr_err("acp restore context failed\n");
			return ret;
		}
		fw_run_status = acp5x_bus_get_dspfw_runstatus(adata);
		pr_debug("%s fw_run_status 0x%x\n", __func__, fw_run_status);
		if (fw_run_status == 0x00)
			adata->is_acp_active = 0x01;
		acpbus_config_command_resp_buffers(adata);
	}
	finish = ktime_get();
	time_diff = ktime_us_delta(finish, start);
	pr_debug("%s!!---ACP D0 ENTRY FINISHED in %5lld micro seconds---!!\n",
		 __func__, time_diff);
	return 0;
}

static const struct dev_pm_ops acp5x_pm = {
	.runtime_suspend = acp5x_runtime_suspend,
	.runtime_resume = acp5x_runtime_resume,
	.suspend = acp5x_suspend,
	.resume = acp5x_resume
};

static int acp5x_probe_continue(struct acp5x_dev_data *adata)
{
	struct pci_dev *pci = pci_get_device(PCI_VENDOR_ID_AMD,
					     ACP_DEVICE_ID,
					     NULL);
	const char *fw_name;
	const char *fw_data_bin_name;
	u32 fw_bin_buf_size = 0;
	int ret = 0;
	int val  = 0;
	unsigned char ch_num = 0x00;
	unsigned int src_addr = 0;
	unsigned int clk_mux = 0;
	union acp_sr_fence_update acp_resume;
	unsigned int acp_firmware_attempts = 0;

	acp5x_init(adata->acp5x_base);
	acp_writel(0x03, adata->acp5x_base + ACP_CLKMUX_SEL);
	clk_mux = acp_readl(adata->acp5x_base + ACP_CLKMUX_SEL);
	pr_debug("ACP_CLKMUX_SEL: 0x%x\n", clk_mux);
	acp5x_set_mlacp_registers(adata->acp5x_base);
	acp5x_enable_extirq(adata);
	configure_acp5x_groupregisters(adata);
	configure_dma_descriptiors(adata);

	adata->acp_sregmap_context =
		devm_kzalloc(&pci->dev,
			     sizeof(struct acp_scratch_register_config),
			     GFP_KERNEL);
	if (!adata->acp_sregmap_context) {
		pr_err("buffer allocation failed for sregmap_conext\n");
		return -ENOMEM;
	}
	init_scratchmemory(adata);

	acp_resume.u32_all = 0;
	if (acp_unsigned_image)
		fw_name =  FIRMWARE_ACP5x_UNSIGNED;
	else
		fw_name = FIRMWARE_ACP5x;

	/* BEGIN ML CHANGE */
	/**
	 * Try to load acp firmware up to 100 times, sleeping 0.1 seconds each time,
	 * until vendor image is mounted, meaning, the wait time is up to 10 seconds.
	 */
	while (acp_firmware_attempts < 100) {
		ret =  request_firmware(&adata->acp_fw, fw_name, &pci->dev);
		if (-ENOENT == ret) {
			usleep_range(100000, 100001); // sleep 0.1 seconds.
			++acp_firmware_attempts;
			continue;
		}
		break;
	}
	/* END ML CHANGE */

	if (ret < 0) {
		dev_err(&pci->dev, "failed to request firmware\n");
			release_firmware(adata->acp_fw);
			adata->acp_fw = NULL;
			return ret;
	}
	adata->fw_bin_size = adata->acp_fw->size;
	adata->fw_bin_page_count = PAGE_ALIGN(adata->fw_bin_size)
					      >> PAGE_SHIFT;
	fw_bin_buf_size = adata->fw_bin_page_count * ACP_PAGE_SIZE;
	adata->virt_addr = pci_alloc_consistent(pci, fw_bin_buf_size,
						&adata->dma_addr);
	if (!adata->virt_addr) {
		pr_err("unable to allocate dma buffer for fw bin\n");
		release_firmware(adata->acp_fw);
		adata->acp_fw = NULL;
		return -ENOMEM;
	}
	memset((void *)adata->virt_addr, 0, fw_bin_buf_size);
	memcpy((void *)adata->virt_addr, (void *)adata->acp_fw->data,
	       adata->fw_bin_size);
	configure_ptelist(FW_BIN, adata->fw_bin_page_count, adata);
	ret = acpbus_configure_and_run_sha_dma(adata, adata->virt_addr,
					       ACP_SYSTEM_MEMORY_WINDOW,
					       ACP_IRAM_BASE_ADDRESS,
					       adata->fw_bin_size);
	if (ret < 0) {
		pr_err("SHA DMA transfer failed status: 0x%x\n", ret);
		return ret;
	}
	fw_data_bin_name =  FIRMWARE_ACP5x_DATA_BIN;
	ret =  request_firmware(&adata->acp_fw_data, fw_data_bin_name,
				&pci->dev);
	if (ret < 0) {
		dev_err(&pci->dev, "request to data bin fw failed\n");
		release_firmware(adata->acp_fw_data);
		adata->acp_fw_data = NULL;
		return ret;
	}
	adata->fw_data_bin_size = adata->acp_fw_data->size;
	adata->fw_data_bin_page_count =	PAGE_ALIGN(adata->fw_data_bin_size)
					>> PAGE_SHIFT;
	adata->fw_data_virt_addr =
				pci_alloc_consistent(pci,
						     ACP_DEFAULT_DRAM_LENGTH,
						     &adata->fw_data_dma_addr);
	if (!adata->fw_data_virt_addr) {
		pr_err("unable to allocate dma buffer for data bin\n");
		release_firmware(adata->acp_fw_data);
		adata->acp_fw_data = NULL;
		return -ENOMEM;
	}
	memset((void *)adata->fw_data_virt_addr, 0, ACP_DEFAULT_DRAM_LENGTH);
	memcpy((void *)adata->fw_data_virt_addr,
	       (void *)adata->acp_fw_data->data,
	       adata->fw_data_bin_size);
	release_firmware(adata->acp_fw_data);
	adata->acp_fw_data = NULL;
	configure_ptelist(FW_DATA_BIN, ACP_DRAM_PAGE_COUNT, adata);
	src_addr =  ACP_SYSTEM_MEMORY_WINDOW +
		    (adata->fw_bin_page_count * ACP_PAGE_SIZE);
	pr_info("fw data bin buffer size: 0x%x\n", adata->fw_data_bin_size);
	ret = acpbus_configure_and_run_dma(adata,
					   src_addr,
					   ACP_DATA_RAM_BASE_ADDRESS,
					   adata->fw_data_bin_size,
					   &ch_num);
	if (ret < 0) {
		pr_err("acp dma channel reset failed:%d\n", ret);
		return ret;
	}
	ret = acp_dma_status(adata, ch_num);
	if (ret < 0) {
		pr_err("acp dma transfer status :%d\n", ret);
		return ret;
	}
	acp_resume.bits.is_resume = 0x0;
	acp_writel(acp_resume.u32_all, adata->acp5x_base +
		   ACP_FUTURE_REG_ACLK_3);
	val = acp_readl(adata->acp5x_base + ACP_FUTURE_REG_ACLK_3);
	pr_debug("%s ACP_FUTURE_REG_ACLK_3:0x%x\n", __func__, val);

	val = 0;
	acp_writel(val, adata->acp5x_base + ACP_DSP0_RUNSTALL);
	val = acp_readl(adata->acp5x_base + ACP_DSP0_RUNSTALL);
	pr_debug("ACP_DSP0_RUNSTALL : 0x%0x\n", val);
	/* check the fw run status */
	ret = acp5x_bus_get_dspfw_runstatus(adata);
	pr_debug("fw_run_status 0x%x\n", ret);
	return ret;
}

static void acp5x_probe_work(struct work_struct *work)
{
	int ret = 0;
	unsigned int buf_size = 0;
	struct pci_dev *pci = pci_get_device(PCI_VENDOR_ID_AMD,
					     ACP_DEVICE_ID,
					     NULL);
	struct acp5x_dev_data *adata = container_of(work, struct acp5x_dev_data,
						    probe_work);

	ret = acp5x_probe_continue(adata);
	if (ret < 0) {
		pr_err("acp5x fw load sequence failed\n");
		/* in case of acp firmware loading failure scenario, free the
		 * allocated pci dma buffers
		 */
		if (adata->virt_addr) {
			buf_size = adata->fw_bin_page_count * ACP_PAGE_SIZE;
			pci_free_consistent(pci, buf_size, adata->virt_addr,
					    adata->dma_addr);
			adata->virt_addr = NULL;
		}
		if (adata->fw_data_virt_addr) {
			pci_free_consistent(pci,
					    ACP_DEFAULT_DRAM_LENGTH,
					    adata->fw_data_virt_addr,
					    adata->fw_data_dma_addr);
			adata->fw_data_virt_addr = NULL;
		}
		adata->fw_load_error = 0x01;
	} else {
		pr_debug("ACP firmware loaded successfully\n");
		adata->is_acp_active = 0x01;
		acp5x_proxy_driver_init();
	}
}

static int snd_acp5x_probe(struct pci_dev *pci,
			   const struct pci_device_id *pci_id)
{
	int ret;
	u32 addr;
	struct acp5x_dev_data *adata;
	unsigned int irqflags = IRQF_SHARED;

	if (pci_enable_device(pci)) {
		dev_err(&pci->dev, "pci_enable_device failed\n");
		return -ENODEV;
	}

	ret = pci_request_regions(pci, "AMD acp5x audio");
	if (ret < 0) {
		dev_err(&pci->dev, "pci_request_regions failed\n");
		goto disable_pci;
	}

	adata = devm_kzalloc(&pci->dev, sizeof(struct acp5x_dev_data),
			     GFP_KERNEL);
	if (!adata) {
		ret = -ENOMEM;
		goto release_regions;
	}

	addr = pci_resource_start(pci, 0);
	adata->acp5x_base = devm_ioremap(&pci->dev, addr,
					 pci_resource_len(pci, 0));
	if (!adata->acp5x_base) {
		ret = -ENOMEM;
		goto release_regions;
	}
	pci_set_master(pci);
	pci_set_drvdata(pci, adata);

	ret = devm_request_irq(&pci->dev, pci->irq, acp5x_irq_handler,
			       irqflags, KBUILD_MODNAME, adata);
	if (ret) {
		dev_err(&pci->dev, "acp5x IRQ request failed %x\n", pci->irq);
		goto  release_regions;
	}

	spin_lock_init(&pm_state.lock);
	pm_state.is_active = 1;

	if (acp_fw_load) {
		adata->ev_command_wait_time = EV_COMMAND_WAIT_TIME_JTAG;
		adata->virt_addr = NULL;
		adata->fw_data_virt_addr = NULL;
		adata->acp_log_virt_addr = NULL;
		adata->disable_runtime_pm = disable_runtime_pm;
		mutex_init(&adata->acp_ev_cmd_mutex);
		mutex_init(&adata->acp_ev_res_mutex);
		INIT_WORK(&adata->evresponse_work, evresponse_handler);
		INIT_WORK(&adata->probe_work, acp5x_probe_work);
		init_waitqueue_head(&adata->evresp);
		init_waitqueue_head(&adata->wait_queue_sha_dma);
		init_waitqueue_head(&adata->ev_event);
		init_waitqueue_head(&adata->dma_queue);
		schedule_work(&adata->probe_work);
	}
	if (!disable_runtime_pm) {
		pm_runtime_set_autosuspend_delay(&pci->dev, 10000);
		pm_runtime_use_autosuspend(&pci->dev);
		pm_runtime_put_noidle(&pci->dev);
		pm_runtime_allow(&pci->dev);
	}
	return 0;

release_regions:
	pci_release_regions(pci);
disable_pci:
	pci_disable_device(pci);
	return ret;
}

static void snd_acp5x_remove(struct pci_dev *pci)
{
	unsigned int buf_size = 0;
	struct acp5x_dev_data *adata = pci_get_drvdata(pci);

	if (acp_fw_load) {
		if (adata->is_acp_active)
			acp5x_proxy_driver_exit();
		cancel_work_sync(&adata->evresponse_work);
		acp5x_clear_and_disable_dspsw_intr(adata);
		acp5x_clear_and_disable_external_intr(adata);
		acp5x_deinit(adata->acp5x_base);
		release_firmware(adata->acp_fw);
		adata->acp_fw = NULL;
		if (adata->virt_addr) {
			buf_size = adata->fw_bin_page_count * ACP_PAGE_SIZE;
			pci_free_consistent(pci, buf_size, adata->virt_addr,
					    adata->dma_addr);
			adata->virt_addr = NULL;
		}
		if (adata->fw_data_virt_addr) {
			pci_free_consistent(pci,
					    ACP_DEFAULT_DRAM_LENGTH,
					    adata->fw_data_virt_addr,
					    adata->fw_data_dma_addr);
			adata->fw_data_virt_addr = NULL;
		}
		if (adata->acp_log_virt_addr) {
			pci_free_consistent(pci, ACP_LOG_BUFFER_SIZE,
					    adata->acp_log_virt_addr,
					    adata->acp_log_dma_addr);
			adata->acp_log_virt_addr = NULL;
		}
		if (adata->acp_dma_virt_addr) {
			pci_free_consistent(pci, ACP_DMA_BUFFER_TX_SIZE,
					    adata->acp_dma_virt_addr,
					    adata->acp_dma_addr);
			adata->acp_dma_virt_addr = NULL;
		}
		if (adata->acp_dma_virt_rx_addr) {
			pci_free_consistent(pci, ACP_DMA_BUFFER_RX_SIZE,
					    adata->acp_dma_virt_rx_addr,
					    adata->acp_dma_rx_addr);
			adata->acp_dma_virt_rx_addr = NULL;
		}
	}
	if (!disable_runtime_pm) {
		pm_runtime_forbid(&pci->dev);
		pm_runtime_get_noresume(&pci->dev);
	}
	pci_release_regions(pci);
	pci_disable_device(pci);
}

static const struct pci_device_id snd_acp5x_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_AMD, ACP_DEVICE_ID),
	.class = PCI_CLASS_MULTIMEDIA_OTHER << 8,
	.class_mask = 0xffffff },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, snd_acp5x_ids);

static struct pci_driver acp5x_driver  = {
	.name = KBUILD_MODNAME,
	.id_table = snd_acp5x_ids,
	.probe = snd_acp5x_probe,
	.remove = snd_acp5x_remove,
	.driver = {
		.pm = &acp5x_pm,
	},
};

module_pci_driver(acp5x_driver);

MODULE_DESCRIPTION("AMD acp5x PCI driver");
MODULE_LICENSE("GPL v2");

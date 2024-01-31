/*
 * Copyright (C) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/scatterlist.h>
#include <linux/syscalls.h>
#include <linux/mero/cvip/cvip_dsp.h>
#include <linux/mero/cvip/biommu.h>
#include <ion/ion.h>
#include <uapi/mero_ion.h>
#include <linux/cvip_event_logger.h>
#include <linux/dsp_manager.h>
#include <linux/clk.h>
#include <mero_scpi.h>
#include "dsp_manager_internal.h"
#include <linux/reboot.h>

#define DEVICE_NAME "dsp_manager"
#define CLASS_NAME "dsp"

/* Base Address */
#define DEST_POWER_CLK_CNTRL	     0x800000
#define RSMU_PGFSM_CVIP_BASE	     0x109114000

/*PGFSM Register Offset*/
#define PGFSM_CONTROL	     0x784
#define PGFSM_WR_DATA	     0x788
#define PGFSM_STATUS	     0x790

/* Register Size */
#define DSP_STATUS_WIDTH 0x4
#define DSP_CONTROL_WIDTH 0x4
#define PGFSM_WIDTH	  0x1000

/*PGFSM WRITE DATA*/
#define RESET_DELAY_PWRDOWN 0x31503
#define RESET_DELAY_PWRUP   0x53501
#define DEFAULT_PWRDOWN	  0x31405
#define DEFAULT_PWRUP	  0x44204

/*PGFSM Control Config*/
#define CNTRL_RESET_PWRDOWN(id)		((0x100 << (id)) + 0x25)
#define CNTRL_RESET_PWRUP(id)		((0x100 << (id)) + 0x35)
#define CNTRL_DEF_PWRDOWN(id)		((0x100 << (id)) + 0x45)
#define CNTRL_DEF_PWRUP(id)		((0x100 << (id)) + 0x55)
#define CNTRL_PWR_DOWN(id)		((0x100 << (id)) + 0x1)
#define CNTRL_PWR_UP(id)		((0x100 << (id)) + 0x3)

/* DEST_power_clk_cntrl offsets */
#define DSP_STATUS_BASE 0x0018
#define DSP_Q6_CONTROL_BASE 0x080C
#define DSP_C5_CONTROL_BASE 0x082C

/* DEST_power_clk_status masks */
#define POWER_IS_UP		BIT(0)
#define DSP_ACTIVE_STATUS	BIT(1)
#define ADB400_MASTER_PD_ACK	BIT(2)
#define ADB400_SLAVE_PD_ACK	BIT(3)
#define ADB400_DTIUP_ACK	BIT(4)
#define ADB400_DTIDWN_ACK	BIT(5)
#define DSP_XOCDMODE		BIT(7)

/* DEST_power_clk_cntrl masks */
#define ENABLE_POWER		BIT(0)
#define RESET_RELEASE		BIT(1)
#define CLKGATE_EN		BIT(2)
#define STAT_VECTOR_SEL		BIT(3)
#define ADB400_MASTER_PD	BIT(4)
#define ADB400_SLAVE_PD		BIT(5)
#define ADB400_DTIUP_PD		BIT(6)
#define ADB400_DTIDWN_PD	BIT(7)
#define DRESET			BIT(8)
#define OCDHALT_ON_RESET	BIT(9)
#define RUN_STALL		BIT(11)
#define QREQN_PD		BIT(12)

/* DSP MMU address */
#define DSP_Q6_0_ADDR "c4000000"
#define DSP_Q6_1_ADDR "c4020000"
#define DSP_Q6_2_ADDR "c4040000"
#define DSP_Q6_3_ADDR "c4060000"
#define DSP_Q6_4_ADDR "c4080000"
#define DSP_Q6_5_ADDR "c40a0000"
#define DSP_C5_0_ADDR "c6000000"
#define DSP_C5_1_ADDR "c6800000"

/* DSP CLOCK RATE */
#define MAX_Q6_CLK_RATE	1236000000
#define MAX_C5_CLK_RATE	971000000
#define MIN_Q6_CLK_RATE	600000000
#define MIN_C5_CLK_RATE	400000000
#define DSP_CLK_OFF		0

/* OCD definitions */
#define OCD_ERR_OK		0
#define OCD_ERR_GENERAL		EINVAL
#define OCD_ERR_EXCEPTION	EILSEQ
#define OCD_ERR_TIMEOUT		ETIME

#define OCD_BASE	(0x2000000 + 0x30000)
#define OCD_SIZE	0x10000

/* reg address */
#define OCD_OCDID(base)		((base) + 0x2000)
#define OCD_DCRCLR(base)	((base) + 0x2008)
#define OCD_DCRSET(base)	((base) + 0x200c)
#define OCD_DSR(base)		((base) + 0x2010)
#define OCD_DDR(base)		((base) + 0x2014)
#define OCD_DDREXEC(base)	((base) + 0x2018)
#define OCD_DIR0EXEC(base)	((base) + 0x201c)
#define OCD_DIR(base, n)	((base) + (0x2020 + (n) * 0x4))
#define OCD_PWRCTL(base)	((base) + 0x3020)
#define OCD_PWRSTAT(base)	((base) + 0x3024)

#define OCD_DCR_ENABLEOCD	BIT(0)
#define OCD_DCR_DEBUGINTERRUPT	BIT(1)

#define OCD_DSR_EXECDONE	BIT(0)
#define OCD_DSR_EXECEXCEPTION	BIT(1)
#define OCD_DSR_EXECBUSY	BIT(2)
#define OCD_DSR_EXECOVERRUN	BIT(3)
#define OCD_DSR_EXECMASK	0xf
#define OCD_DSR_STOPPED		BIT(4)
#define OCD_DSR_STOPPEDCAUSE	(0xf << 5)
#define OCD_DSR_STOPPEDCAUSE_SHIFT	5
#define OCD_DSR_COREWROTEDDR	BIT(10)
#define OCD_DSR_COREREADDDR	BIT(11)
#define OCD_DSR_DBGPWR		BIT(31)

#define OCD_PWR_DEBUG_RESET	BIT(28)
#define OCD_PWR_CORE_RESET	BIT(16)

#define OCD_SR_IBREAKA(n)	(128 + (n))
#define OCD_SR_IBREAKA_EN	96
#define OCD_SR_IBREAKC(n)	(192 + (n))
#define OCD_SR_DDR		104
#define OCD_SR_EPC		177
#define OCD_SR_ICOUNT		236
#define OCD_SR_DEBUGCAUSE	233

#define OCD_SR_AR		0x4
#define OCD_SR_IMM4		0x0

#define OCD_SR_I_RSR(sr, ar)	((0x3 << 16) | ((sr) << 8) | ((ar) << 4))
#define OCD_SR_I_WSR(sr, ar)	((0x13 << 16) | ((sr) << 8) | ((ar) << 4))
#define OCD_SR_I_RFDO(imm4)	((0xf1e << 12) | ((imm4) << 8))

#define DISABLE 0
#define ENABLE 1

#define DSP_IS_XEA3(dsp_id)	((dsp_id) <= DSP_Q6_5)

struct dsp_core_reg {
	char *name;
	u32 num;
};

static int	dsp_open(struct inode *, struct file *);
static ssize_t	dsp_read(struct file *file, char __user *user_buffer,
			 size_t size, loff_t *offset);
static int	dsp_release(struct inode *, struct file *);
static long	dsp_ioctl(struct file *file, unsigned int ioctl_num,
			  unsigned long arg);
static int	ocd_enter(void __iomem *base);
static int	ocd_exit(void __iomem *base);
static int	ocd_unhalt_for_hwbrkpt(void __iomem *base, int dsp_id);
static int	ocd_wait_ocdenable(void __iomem *base);
static int	reg_get_ar(void __iomem *base, int ar, unsigned int *v);
static int	reg_set_ar(void __iomem *base, int ar, unsigned int v);
static int	reg_get_sr(void __iomem *base, int sr, unsigned int *v);
static int	reg_set_sr(void __iomem *base, int sr, unsigned int v);
static int	ocd_helper_exec(void __iomem *base, u32 _val);

static const struct dsp_core_reg dspregs[] = {
	{ "PC",         176 },
	{ "PS",         230 },
	{ "DDR",        104 },
	{ "EPC1",       177 },
	{ "EXCCAUSE",   232 },
	{ "CCOUNT",     234 },
	{ "ICOUNT",     236 },
};

static const struct file_operations dsp_fops = {
	.open = dsp_open,
	.read = dsp_read,
	.release = dsp_release,
	.unlocked_ioctl = dsp_ioctl,
};

static int			dsp_major;
static struct dsp_device_t	*dsp_manager_dev;

static void dsp_poll_wait(u32 usec)
{
	while (usec--)
		udelay(1);
}

static bool is_dsp_inuse(struct dsp_resources_t *dsp_resource,
			 struct dsp_client_t *client)
{
	if (dsp_resource->client && dsp_resource->client != client) {
		DSP_ERROR("DSP is already in use.\n");
		return true;
	}

	if (!dsp_resource->client) {
		dsp_resource->client = client;
		CVIP_EVTLOG(dsp_manager_dev->event_log, "set Client", client, dsp_resource->dsp_id);
		dsp_resource->states->bypass_pd = helper_get_pdbypass(dsp_resource->dsp_id);
	}

	return false;
}

static void dsp_qreqn_pd_enable(struct dsp_device_t *device, struct dsp_state_t *states)
{
	u32 reg;

	/*
	 * Setting qreqn_pd it to high to ensure TBU is moved back to
	 * Q_RUN state.
	 */
	reg = readl_relaxed(states->control_register);
	if (!(reg & QREQN_PD)) {
		reg |= QREQN_PD;
		writel_relaxed(reg, states->control_register);
		/* make sure control register updated */
		wmb();
		CVIP_EVTLOG(device->event_log, "control_reg", states->control_register, reg);
		dsp_poll_wait(20);
	}
}

static void dsp_qreqn_pd_disable(struct dsp_device_t *device, struct dsp_state_t *states)
{
	u32 reg;

	/*
	 * Setting qreqn_pd it to low to ensure TBU is moved to Q_STOP state.
	 */
	reg = readl_relaxed(states->control_register);
	if ((reg & QREQN_PD)) {
		reg &= ~QREQN_PD;
		writel_relaxed(reg, states->control_register);
		/* make sure control register updated */
		wmb();
		CVIP_EVTLOG(device->event_log, "control_reg", states->control_register, reg);
		udelay(1);
	}
}

static int q6clk_dpm[MAX_DPM_LEVEL] = {
	MIN_Q6_CLK_RATE,
	800000000,
	1133000000,
	MAX_Q6_CLK_RATE,
};

static int c5clk_dpm[MAX_DPM_LEVEL] = {
	MIN_C5_CLK_RATE,
	600000000,
	850000000,
	MAX_C5_CLK_RATE,
};

static void dsp_helper_iommu_unmap(struct dsp_resources_t *dsp_resource)
{
	int i;
	struct list_head *pos;
	struct list_head *q;
	struct dsp_memory_t *memory;
	int logid = dsp_manager_dev->event_log;
	size_t ret;

	list_for_each_safe(pos, q, &dsp_resource->memory_list) {
		memory = list_entry(pos, struct dsp_memory_t, list);
		if (memory->domain0 && memory->size_0) {
			ret = iommu_unmap(memory->domain0,
					  (unsigned long)memory->addr,
					  memory->size_0);
			CVIP_EVTLOG(logid, "iommu_unmap domain0",
				    memory->addr, memory->size_0, ret);
		}

		for (i = 0; i < memory->mapcnt; i++) {
			if (memory->domain[i]) {
				ret = iommu_unmap(memory->domain[i],
						  (unsigned long)memory->addr,
						  memory->size);
				CVIP_EVTLOG(logid, "iommu_unmap domain",
					    i, memory->addr, memory->size, ret);
			}
		}
	}
}

static void dsp_helper_all_dspoff_nolock(void)
{
	struct list_head *pos;
	struct dsp_resources_t *tmp_resource;

	DSP_ERROR("Shutting down all the DSPs.\n");
	CVIP_EVTLOG(dsp_manager_dev->event_log, "Shutdown");

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		tmp_resource = list_entry(pos, struct dsp_resources_t, list);
		tmp_resource->states->submit_mode = DSP_OFF;
		dsp_helper_iommu_unmap(tmp_resource);
		dsp_helper_off(dsp_manager_dev, tmp_resource);
	}

	DSP_ERROR("All the DSPs are shutdown successfully.\n");
}

static int dsp_helper_shutdown(struct notifier_block *nb,
			       unsigned long event, void *ptr)
{
	dsp_helper_all_dspoff_nolock();
	return NOTIFY_DONE;
}

static void dsp_helper_dsp_reg_dump(struct dsp_resources_t *res)
{
	void __iomem *base;
	int log = dsp_manager_dev->event_log;
	int dsp_mode;
	u32 regs[16];
	int i;

	/* log DSP cntr and status regs in log */
	CVIP_EVTLOG(log, "---DSPDUMP---", res->dsp_id,
		    readl_relaxed(res->states->control_register),
		    readl_relaxed(res->states->status_register));

	dsp_mode = dsp_helper_read(res->states);
	if (dsp_mode == DSP_OFF)
		return;

	/* if DSP is running, pause it first, so we can dump the registers */
	if (dsp_helper_pause(dsp_manager_dev, res->states, res->dsp_id))
		return;

	base = res->states->ocd_register;
	for (i = 0; i < 16; i++)
		reg_get_ar(base, i, &regs[i]);
	CVIP_EVTLOG(log, "AR0-7", regs[0], regs[1], regs[2], regs[3],
		    regs[4], regs[5], regs[6], regs[7]);
	CVIP_EVTLOG(log, "AR8-15", regs[8], regs[9], regs[10], regs[11],
		    regs[12], regs[13], regs[14], regs[15]);

	for (i = 0; i < ARRAY_SIZE(dspregs); i++) {
		if (reg_get_sr(base, dspregs[i].num, &regs[0]))
			DSP_ERROR("failed get SR[%s]\n", dspregs[i].name);
		else
			CVIP_EVTLOG(log, dspregs[i].name, regs[0]);
	}

	/* exec WAITI instruction */
	ocd_helper_exec(base, 0x7000);
}

static int dsp_helper_panic_dump(struct notifier_block *nb,
				 unsigned long event, void *ptr)
{
	struct list_head *pos;
	struct dsp_resources_t *tmp_resource;

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		tmp_resource = list_entry(pos, struct dsp_resources_t, list);
		dsp_helper_dsp_reg_dump(tmp_resource);
	}

	dsp_helper_all_dspoff_nolock();
	return NOTIFY_DONE;
}

static struct notifier_block dsp_manager_panic_notifier = {
	.notifier_call = dsp_helper_panic_dump,
};

static struct notifier_block dsp_manager_reboot_notifier = {
	.notifier_call = dsp_helper_shutdown,
};

int dsp_helper_read_clkgate_setting(struct dsp_state_t *states)
{
	u32 control_status;

	control_status = readl_relaxed(states->control_register);
	return (control_status & CLKGATE_EN) ? 1 : 0;
} EXPORT_SYMBOL(dsp_helper_read_clkgate_setting);

int dsp_helper_write_clkgate_setting(struct dsp_state_t *states,
				     int setting_val)
{
	u32 control_status;

	control_status = readl_relaxed(states->control_register);
	if (setting_val == ENABLE)
		control_status = control_status | CLKGATE_EN;
	else
		control_status = control_status & ~CLKGATE_EN;
	writel_relaxed(control_status, states->control_register);
	CVIP_EVTLOG(dsp_manager_dev->event_log, "clkgate", setting_val);
	return setting_val;
} EXPORT_SYMBOL(dsp_helper_write_clkgate_setting);

int dsp_helper_read_stat_vector_sel(struct dsp_state_t *states)
{
	return states->alt_stat_vector_sel;
} EXPORT_SYMBOL(dsp_helper_read_stat_vector_sel);

int dsp_helper_write_stat_vector_sel(struct dsp_state_t *states,
				     int val)
{
	states->alt_stat_vector_sel = (val == DISABLE) ? DISABLE : ENABLE;
	CVIP_EVTLOG(dsp_manager_dev->event_log, "alt_vec", val);
	return states->alt_stat_vector_sel;
} EXPORT_SYMBOL(dsp_helper_write_stat_vector_sel);

int dsp_helper_read(struct dsp_state_t *states)
{
	int dsp_mode;
	u32 control_status;
	u32 mask;

	mask = POWER_IS_UP | DSP_ACTIVE_STATUS | DSP_XOCDMODE;
	control_status = readl_relaxed(states->status_register);
	DSP_DBG("status_reg=0x%x\n", control_status);

	if (states->submit_mode == DSP_OFF &&
	    (control_status & mask) == DSP_ACTIVE_STATUS) {
		dsp_mode = DSP_OFF;
	} else if (control_status & mask) {
		control_status = readl_relaxed(states->control_register);
		DSP_DBG("control_reg=0x%x\n", control_status);
		if (control_status & (RESET_RELEASE | OCDHALT_ON_RESET)) {
			// Check for pause with OCD
			dsp_mode = DSP_RUN;
		} else {
			dsp_mode = DSP_STOP;
		}
	} else if (states->submit_mode == DSP_RUN) {
		DSP_DBG("dsp is not active, in WAITI state\n");
		dsp_mode = DSP_RUN;
	} else {
		if (states->submit_mode == DSP_RUN)
			dsp_mode = DSP_RUN;
		else
			dsp_mode = DSP_OFF;
	}

	DSP_DBG("return dsp_mode=%d\n", dsp_mode);
	return dsp_mode;
}

static int pgfsm_config_setup(void __iomem *pgfsm_base, int dsp_id)
{
	/*reset power down delay*/
	writel_relaxed(RESET_DELAY_PWRDOWN, pgfsm_base + PGFSM_WR_DATA);
	writel_relaxed(CNTRL_RESET_PWRDOWN(dsp_id), pgfsm_base + PGFSM_CONTROL);

	/*reset power up delay*/
	writel_relaxed(RESET_DELAY_PWRUP, pgfsm_base + PGFSM_WR_DATA);
	writel_relaxed(CNTRL_RESET_PWRUP(dsp_id), pgfsm_base + PGFSM_CONTROL);

	/*set default power down delay*/
	writel_relaxed(DEFAULT_PWRDOWN, pgfsm_base + PGFSM_WR_DATA);
	writel_relaxed(CNTRL_DEF_PWRDOWN(dsp_id), pgfsm_base + PGFSM_CONTROL);

	/*set default power up delay*/
	writel_relaxed(DEFAULT_PWRUP, pgfsm_base + PGFSM_WR_DATA);
	writel_relaxed(CNTRL_DEF_PWRUP(dsp_id), pgfsm_base + PGFSM_CONTROL);
	/* make sure control register updated */
	wmb();
	return 0;
}

static int pgfsm_power_up(void __iomem *pgfsm_base, int dsp_id, int bypass)
{
	int timeout;
	unsigned long status;

	writel_relaxed(CNTRL_PWR_UP(dsp_id), pgfsm_base + PGFSM_CONTROL);
	/* make sure control register updated */
	wmb();

	/*
	 * PGFSM is not modelled in SimNow, skip status poll
	 */
	if (bypass) {
		DSP_DBG("bypassing power up status poll...\n");
		CVIP_EVTLOG(dsp_manager_dev->event_log, "bypass PGFSM", dsp_id);
		goto bypass_end;
	}

	/*wait for power up status b'00*/
	timeout = 10;
	while (--timeout) {
		status = readl_relaxed(pgfsm_base + PGFSM_STATUS);
		/*PGFSM0 status_reg maps to [1:0], PGFSM1 maps to [3:2], etc.*/
		status = status >> (2 * dsp_id);
		if (!(test_bit(0, &status)) && !(test_bit(1, &status))) {
			DSP_DBG(" PGFSM Power On status confirmed\n");
			CVIP_EVTLOG(dsp_manager_dev->event_log, "<-PGFSM On", dsp_id);
			break;
		}
		udelay(1);
	}
	if (timeout == 0) {
		DSP_ERROR("PGFSM DSP %d power up timeout\n", dsp_id);
		CVIP_EVTLOG(dsp_manager_dev->event_log, "PGFSM On timeout", dsp_id);
		return -ETIME;
	}

bypass_end:
	return 0;
}

static int pgfsm_power_down(void __iomem *pgfsm_base, int dsp_id, int bypass)
{
	int timeout;
	unsigned long status;

	writel_relaxed(CNTRL_PWR_DOWN(dsp_id), pgfsm_base + PGFSM_CONTROL);
	/* make sure control register updated */
	wmb();

	/*
	 * PGFSM is not modelled in SimNow, skip status poll
	 */
	if (bypass) {
		DSP_DBG("bypassing power down status poll...\n");
		CVIP_EVTLOG(dsp_manager_dev->event_log, "bypass PGFSM", dsp_id);
		goto bypass_end;
	}

	/*wait for power down status b'10*/
	timeout = 10;
	while (--timeout) {
		status = readl_relaxed(pgfsm_base + PGFSM_STATUS);
		/*PGFSM0 status_reg maps to [1:0], PGFSM1 maps to [3:2], etc.*/
		status = status >> (2 * dsp_id);
		if (!(test_bit(0, &status)) && (test_bit(1, &status))) {
			DSP_DBG(" PGFSM Power Down status confirmed\n");
			CVIP_EVTLOG(dsp_manager_dev->event_log, "<-PGFSM off", dsp_id);
			break;
		}
		udelay(1);
	}
	if (timeout == 0) {
		DSP_ERROR("PGFSM DSP %d power down timeout\n", dsp_id);
		CVIP_EVTLOG(dsp_manager_dev->event_log, "PGFSM off timeout", dsp_id);
		return -ETIME;
	}

bypass_end:
	return 0;
}

int pgfsm_helper_get_status(void __iomem *pgfsm_base, int dsp_id)
{
	unsigned long status;

	status = readl_relaxed(pgfsm_base + PGFSM_STATUS);
	/*PGFSM0 status_reg maps to [1:0], PGFSM1 maps to [3:2], etc.*/
	status = status >> (2 * dsp_id);

	if (!(test_bit(0, &status)) && (test_bit(1, &status)))
		return PGFSM_POWER_OFF;
	else if (!(test_bit(0, &status)) && !(test_bit(1, &status)))
		return PGFSM_POWER_ON;
	else if ((test_bit(0, &status)) && !(test_bit(1, &status)))
		return PGFSM_POWER_ON_PROGRESS;
	else if ((test_bit(0, &status)) && (test_bit(1, &status)))
		return PGFSM_POWER_OFF_PROGRESS;
	else
		return -1;
}

int pgfsm_helper_power(void __iomem *pgfsm_base, int dsp_id, int power)
{
	CVIP_EVTLOG(dsp_manager_dev->event_log, "->PGFSM", dsp_id, power);
	writel_relaxed(RESET_DELAY_PWRDOWN, pgfsm_base + PGFSM_WR_DATA);
	writel_relaxed(CNTRL_RESET_PWRDOWN(dsp_id), pgfsm_base + PGFSM_CONTROL);

	writel_relaxed(RESET_DELAY_PWRUP, pgfsm_base + PGFSM_WR_DATA);
	writel_relaxed(CNTRL_RESET_PWRUP(dsp_id), pgfsm_base + PGFSM_CONTROL);

	writel_relaxed(DEFAULT_PWRDOWN, pgfsm_base + PGFSM_WR_DATA);
	writel_relaxed(CNTRL_DEF_PWRDOWN(dsp_id), pgfsm_base + PGFSM_CONTROL);

	writel_relaxed(DEFAULT_PWRUP, pgfsm_base + PGFSM_WR_DATA);
	writel_relaxed(CNTRL_DEF_PWRUP(dsp_id), pgfsm_base + PGFSM_CONTROL);
	/* make sure control register updated */
	wmb();

	if (power)
		writel_relaxed(CNTRL_PWR_UP(dsp_id),
			       pgfsm_base + PGFSM_CONTROL);
	else
		writel_relaxed(CNTRL_PWR_DOWN(dsp_id),
			       pgfsm_base + PGFSM_CONTROL);

	/* make sure control register updated */
	wmb();

	return 0;
}

int dsp_clk_dpm_cntrl(unsigned int clk_id, unsigned int dpm_level)
{
	struct dsp_state_t *states;
	unsigned int clk_rate;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	int ret = 0;

	if (dpm_level >= MAX_DPM_LEVEL)
		return -EINVAL;

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (clk_id == Q6CLK) {
		dsp_manager_dev->q6_cur_dpmclk = q6clk_dpm[dpm_level];
	} else if (clk_id == C5CLK) {
		dsp_manager_dev->c5_cur_dpmclk = c5clk_dpm[dpm_level];
	} else {
		ret = -EINVAL;
		goto error_res;
	}

	if (list_empty(&dsp_manager_dev->dsp_list))
		goto error_res;

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		states = dsp_resources->states;

		if (states->dsp_mode != DSP_OFF) {
			clk_rate = (dsp_resources->dsp_id <= DSP_Q6_5) ?
				     dsp_manager_dev->q6_cur_dpmclk :
				     dsp_manager_dev->c5_cur_dpmclk;

			ret = clk_set_rate(states->clk, clk_rate);
			if (ret) {
				DSP_ERROR("Failed to set dsp%d clock rate.\n",
					  dsp_resources->dsp_id);
				goto error_res;
			}
			states->clk_rate = clk_rate;
		}
	}
error_res:
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
error:
	return ret;
}
EXPORT_SYMBOL(dsp_clk_dpm_cntrl);

int dsp_helper_clk_set_rate_nolock(struct dsp_device_t *device,
				   struct dsp_state_t *states,
				   unsigned long clk_rate)
{
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	int ret;

	states->clk_rate = clk_rate;

	if (list_empty(&device->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&device->dsp_device_lock);
		return -EFAULT;
	}

	DSP_DBG("%s.%d: clk_rate:%lu\n", __func__, __LINE__, clk_rate);
	list_for_each(pos, &device->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (clk_is_match(dsp_resources->states->clk, states->clk)) {
			DSP_DBG("%s.%d: id:%d clk:%lu\n", __func__, __LINE__,
				dsp_resources->dsp_id,
				dsp_resources->states->clk_rate);
			if (dsp_resources->states->clk_rate > clk_rate)
				clk_rate = dsp_resources->states->clk_rate;
		}
	}

	DSP_DBG("%s.%d: vote clk_rate:%lu\n", __func__, __LINE__, clk_rate);

	ret = clk_set_rate(states->clk, clk_rate);

	return ret;
}

int dsp_helper_clk_set_rate(struct dsp_device_t *device,
			    struct dsp_state_t *states,
			    unsigned long clk_rate)
{
	int ret;

	ret = mutex_lock_interruptible(&device->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		return ret;
	}

	ret = dsp_helper_clk_set_rate_nolock(device, states, clk_rate);

	mutex_unlock(&device->dsp_device_lock);

	return ret;
}

int dsp_helper_off(struct dsp_device_t *device, struct dsp_resources_t *res)
{
	int timeout;
	u32 control, status;
	u32 mask;
	int ret = 0;
	struct dsp_state_t *states = res->states;
	int dsp_id = res->dsp_id;

	if (states->dsp_mode == DSP_OFF) {
		dsp_qreqn_pd_enable(device, states);
		return 0;
	}

	/* Make sure DSP is not active */
	timeout = 10;
	status = readl_relaxed(states->status_register);
	while (--timeout && (status & DSP_ACTIVE_STATUS)) {
		udelay(1);
		status = readl_relaxed(states->status_register);
	}
	if (timeout == 0) {
		DSP_ERROR("DSP[%d] not in WAITI state, cannot stop!\n", dsp_id);
		CVIP_EVTLOG(device->event_log, "not in WAITI", dsp_id);
	}

	/*
	 * Write 0 to qreqn_pd bit in the control register to power down
	 * DSP and switch TBU to Q_STOPPED state
	 */
	dsp_qreqn_pd_disable(device, states);

	if (states->bypass_pd)
		goto pd_end;

	/* Write all ADB400 PD interface to 0 */
	control = readl_relaxed(states->control_register);
	control &= ~(ADB400_MASTER_PD | ADB400_SLAVE_PD |
			    ADB400_DTIUP_PD | ADB400_DTIDWN_PD);
	writel_relaxed(control, states->control_register);
	/* make sure control register updated */
	wmb();

	/* wait for ADB400 ack signal goes to 0 */
	timeout = 200;
	mask = ADB400_MASTER_PD_ACK | ADB400_SLAVE_PD_ACK | ADB400_DTIUP_ACK |
		ADB400_DTIDWN_ACK;
	while (--timeout) {
		status = readl_relaxed(states->status_register);
		if ((status & mask) == 0) {
			DSP_DBG("ADB400 PD low ACK received %d\n", timeout);
			break;
		}
		udelay(1);
	}
	if (timeout == 0) {
		CVIP_EVTLOG(device->event_log, "ADB400_PD lowack", 0xdeadbeef);
		DSP_ERROR("DSP:%d ADB400 PD low ack timeout.\n", dsp_id);
	}

	/* Write all ADB400 PD interface to 1 */
	control = readl_relaxed(states->control_register);
	control |= (ADB400_MASTER_PD | ADB400_SLAVE_PD |
			    ADB400_DTIUP_PD | ADB400_DTIDWN_PD);
	writel_relaxed(control, states->control_register);
	/* make sure control register updated */
	wmb();

	/* wait for ADB400 ack signal goes to 1 */
	timeout = 10;
	mask = ADB400_MASTER_PD_ACK | ADB400_SLAVE_PD_ACK | ADB400_DTIUP_ACK |
		ADB400_DTIDWN_ACK;
	while (--timeout) {
		status = readl_relaxed(states->status_register);
		if ((status & mask) == mask) {
			DSP_DBG("ADB400 PD high ACK received %d\n", timeout);
			break;
		}
		udelay(1);
	}
	if (timeout == 0) {
		CVIP_EVTLOG(device->event_log, "ADB400_PD highack", 0xdead);
		DSP_ERROR("DSP:%d ADB400 PD high ack timeout.\n", dsp_id);
	}

pd_end:
	/* Enable DSP clockgate */
	control = readl_relaxed(states->control_register);
	control = control | CLKGATE_EN;
	writel_relaxed(control, states->control_register);
	/* make sure control register updated */
	wmb();

	/* De-assert DSP power (only have effect to C5) */
	control = readl_relaxed(states->control_register);
	control = control & ~ENABLE_POWER;
	writel_relaxed(control, states->control_register);
	/* make sure control register updated */
	wmb();

	/* Put DSP in reset */
	control = readl_relaxed(states->control_register);
	CVIP_EVTLOG(device->event_log,
		    "Read from address [1] and received data [2]",
		    states->control_register, control);
	control = control & ~RESET_RELEASE;
	writel_relaxed(control, states->control_register);

	/* make sure control register updated */
	wmb();
	udelay(1);

	CVIP_EVTLOG(device->event_log, "Wrote to address [1] with data [2]",
		    states->control_register, control);
	timeout = 100;
	while ((--timeout) && (dsp_helper_read(states) != DSP_OFF))
		udelay(1);
	if (timeout == 0) {
		CVIP_EVTLOG(device->event_log, "<-DSP_OFF", dsp_id, 0xdeadbeef);
		DSP_ERROR("Off DSP timeout.\n");
		ret = -ETIME;
	} else {
		states->dsp_mode = DSP_OFF;
		CVIP_EVTLOG(device->event_log, "<-DSP_OFF", dsp_id, 0);
		DSP_DBG(" *** DSP%d_OFF ***\n", dsp_id);
		if (states->clk) {
			ret = dsp_helper_clk_set_rate_nolock(device, states,
							     DSP_CLK_OFF);
			if (ret) {
				DSP_ERROR("Failed to set dsp%d off.\n", dsp_id);
				ret = -EFAULT;
			}
		}
	}

	/*PGFSM power down*/
	pgfsm_config_setup(states->pgfsm_register, dsp_id);
	ret = pgfsm_power_down(states->pgfsm_register, dsp_id,
			       states->bypass_pd);
	if (ret != 0)
		return ret;

	dsp_qreqn_pd_enable(device, states);
	states->dsp_mode = DSP_OFF;
	return ret;
}

int dsp_helper_stop(struct dsp_device_t *device, struct dsp_state_t *states,
		 int dsp_id)
{
	states->dsp_mode = DSP_STOP;
	return 0;
}

int dsp_helper_pause(struct dsp_device_t *device, struct dsp_state_t *states,
		     int dsp_id)
{
	int ret;
	unsigned long clk_rate;

	CVIP_EVTLOG(device->event_log, "->pause", dsp_id);
	clk_rate = (dsp_id <= DSP_Q6_5) ? dsp_manager_dev->q6_cur_dpmclk :
		    dsp_manager_dev->c5_cur_dpmclk;
	ret = dsp_helper_clk_set_rate(device, states, clk_rate);
	if (ret) {
		DSP_ERROR("Failed to set dsp%d clock rate.\n", dsp_id);
		return -EFAULT;
	}

	/* Call DSP OCD Driver to pause DSP */
	ret = ocd_enter(states->ocd_register);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("Failed enter OCD mode, err=%d\n", ret);
		return ret;
	}

	ret = ocd_wait_ocdenable(states->ocd_register);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("Failed wait for OCD mode, err=%d\n", ret);
		return ret;
	}

	return 0;
}

int dsp_helper_unpause(struct dsp_device_t *device, struct dsp_state_t *states,
		       int dsp_id)
{
	int ret;
	unsigned long clk_rate;

	/*
	 * OCD is not modelled in SimNow, so just issue a run to simulate
	 * a unhalt from OCD state
	 */
	if (states->bypass_pd)
		return dsp_helper_run(device, states, dsp_id);

	CVIP_EVTLOG(device->event_log, "->unpause", dsp_id);
	clk_rate = (dsp_id <= DSP_Q6_5) ? dsp_manager_dev->q6_cur_dpmclk :
		    dsp_manager_dev->c5_cur_dpmclk;
	ret = dsp_helper_clk_set_rate(device, states, clk_rate);
	if (ret) {
		DSP_ERROR("Failed to set dsp%d clock rate.\n", dsp_id);
		return -EFAULT;
	}

	/* Call DSP OCD Driver to un-pause DSP */
	ret = ocd_unhalt_for_hwbrkpt(states->ocd_register, dsp_id);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("Failed UNHALT DSP, err=%d\n", ret);
		return ret;
	}

	return 0;
}

int dsp_helper_exit_ocd(struct dsp_device_t *device, struct dsp_state_t *states,
			int dsp_id)
{
	int ret;
	unsigned long clk_rate;

	/*
	 * OCD is not modelled in SimNow, so just issue a run to simulate
	 * a unhalt from OCD state
	 */
	if (states->bypass_pd)
		return dsp_helper_run(device, states, dsp_id);

	CVIP_EVTLOG(device->event_log, "->OCD exit", dsp_id);
	clk_rate = (dsp_id <= DSP_Q6_5) ? dsp_manager_dev->q6_cur_dpmclk :
		    dsp_manager_dev->c5_cur_dpmclk;
	ret = dsp_helper_clk_set_rate(device, states, clk_rate);
	if (ret) {
		DSP_ERROR("Failed to set dsp%d clock rate.\n", dsp_id);
		return -EFAULT;
	}

	/* Call DSP OCD Driver to un-pause DSP */
	ret = ocd_exit(states->ocd_register);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("Failed UNHALT DSP, err=%d\n", ret);
		return ret;
	}
	return 0;
}

int dsp_helper_run(struct dsp_device_t *device, struct dsp_state_t *states,
		int dsp_id)
{
	int timeout;
	u32 control_status;
	int ret;
	unsigned long clk_rate;

	dsp_qreqn_pd_enable(device, states);

	clk_rate = (dsp_id <= DSP_Q6_5) ? dsp_manager_dev->q6_cur_dpmclk :
		    dsp_manager_dev->c5_cur_dpmclk;
	if (states->clk) {
		ret = dsp_helper_clk_set_rate(device, states, clk_rate);
		if (ret) {
			DSP_ERROR("Failed to set dsp%d clock rate.\n", dsp_id);
			return -EFAULT;
		}
	}

	/*PGFSM power up*/
	pgfsm_config_setup(states->pgfsm_register, dsp_id);
	ret = pgfsm_power_up(states->pgfsm_register, dsp_id, states->bypass_pd);
	if (ret != 0)
		return ret;

	control_status = readl_relaxed(states->control_register);
	CVIP_EVTLOG(device->event_log,
		    "Read from address [1] and received data [2]",
		    states->control_register, control_status);
	control_status |= DRESET;
	writel_relaxed(control_status, states->control_register);
	/* make sure control register updated */
	wmb();

	control_status = control_status | ENABLE_POWER;
	control_status = control_status & ~OCDHALT_ON_RESET;
	control_status = control_status & ~CLKGATE_EN;
	if (states->alt_stat_vector_sel == ENABLE)
		control_status = control_status | STAT_VECTOR_SEL;
	else
		control_status = control_status & ~STAT_VECTOR_SEL;

	if (!states->bypass_pd)
		/* Write all ADB400 PD interface back to 1 */
		control_status |= (ADB400_MASTER_PD | ADB400_SLAVE_PD |
				   ADB400_DTIUP_PD | ADB400_DTIDWN_PD);

	writel_relaxed(control_status, states->control_register);

	/* make sure control register updated */
	wmb();
	CVIP_EVTLOG(device->event_log, "Wrote to address [1] with data [2]",
		    states->control_register, control_status);

	control_status = control_status | RESET_RELEASE;
	writel_relaxed(control_status, states->control_register);
	/* make sure control register updated */
	wmb();
	CVIP_EVTLOG(device->event_log, "Wrote to address [1] with data [2]",
		    states->control_register, control_status);

	udelay(1);
	control_status &= ~DRESET;
	writel_relaxed(control_status, states->control_register);

	timeout = 100;
	while (timeout--) {
		if (dsp_helper_read(states) == DSP_RUN) {
			states->dsp_mode = DSP_RUN;
			CVIP_EVTLOG(device->event_log, "<-DSP_RUN", dsp_id, 0);
			DSP_DBG(" *** DSP%d_RUN ***\n", dsp_id);
			return 0;
		}
		udelay(1);
	}
	CVIP_EVTLOG(device->event_log, "<-DSP_RUN", dsp_id, 0xdead);
	DSP_ERROR("Run DSP%d timeout: control_register=%x status_register=%x\n",
		  dsp_id, readl_relaxed(states->control_register),
		  readl_relaxed(states->status_register));
	return -ETIME;
}

int dsp_helper_haltonreset(struct dsp_device_t *device,
			   struct dsp_state_t *states,
			   int dsp_id)
{
	int timeout;
	u32 control_status;
	int ret;
	unsigned long clk_rate;

	dsp_qreqn_pd_enable(device, states);

	clk_rate = (dsp_id <= DSP_Q6_5) ? dsp_manager_dev->q6_cur_dpmclk :
		    dsp_manager_dev->c5_cur_dpmclk;
	if (states->clk) {
		ret = dsp_helper_clk_set_rate(device, states, clk_rate);
		if (ret) {
			DSP_ERROR("Failed to set dsp%d clock rate.\n", dsp_id);
			return -EFAULT;
		}
	}

	/*PGFSM power up*/
	pgfsm_config_setup(states->pgfsm_register, dsp_id);
	ret = pgfsm_power_up(states->pgfsm_register, dsp_id, states->bypass_pd);
	if (ret != 0)
		return ret;

	DSP_DBG("set halt on reset for DSP%d\n", dsp_id);
	control_status = readl_relaxed(states->control_register);
	CVIP_EVTLOG(device->event_log,
		    "control_reg from address [1] and received data [2]",
		    states->control_register, control_status);

	/*
	 * OCD halt is not supported in SimNow, so just do nothing to
	 * simulate a OCD halt
	 */
	states->bypass_pd = helper_get_pdbypass(dsp_id);
	if (states->bypass_pd) {
		states->dsp_mode = DSP_HALT_ON_RESET;
		return 0;
	}

	control_status |= DRESET;
	writel_relaxed(control_status, states->control_register);
	/* make sure control register updated */
	wmb();
	CVIP_EVTLOG(device->event_log, "Wrote to address [1] with data [2]",
		    states->control_register, control_status);

	control_status |= ENABLE_POWER;
	writel_relaxed(control_status, states->control_register);
	/* make sure control register updated */
	wmb();
	CVIP_EVTLOG(device->event_log, "Wrote to address [1] with data [2]",
		    states->control_register, control_status);

	control_status = control_status | OCDHALT_ON_RESET;
	control_status = control_status & ~CLKGATE_EN;
	if (states->alt_stat_vector_sel == ENABLE)
		control_status = control_status | STAT_VECTOR_SEL;
	else
		control_status = control_status & ~STAT_VECTOR_SEL;
	writel_relaxed(control_status, states->control_register);
	/* make sure control register updated */
	wmb();
	CVIP_EVTLOG(device->event_log, "Wrote to address [1] with data [2]",
		    states->control_register, control_status);

	control_status |= RESET_RELEASE;
	writel_relaxed(control_status, states->control_register);
	/* make sure control register updated */
	wmb();
	CVIP_EVTLOG(device->event_log, "Wrote to address [1] with data [2]",
		    states->control_register, control_status);
	udelay(1);

	control_status &= ~DRESET;
	writel_relaxed(control_status, states->control_register);
	/* make sure control register updated */
	wmb();
	CVIP_EVTLOG(device->event_log, "Wrote to address [1] with data [2]",
		    states->control_register, control_status);

	DSP_DBG("before wait: control_register=%x status_register=%x\n",
		readl_relaxed(states->control_register),
		readl_relaxed(states->status_register));

	timeout = 100;
	while (timeout--) {
		control_status = readl_relaxed(states->status_register);
		if (control_status & DSP_XOCDMODE) {
			states->dsp_mode = DSP_HALT_ON_RESET;
			CVIP_EVTLOG(device->event_log, "<-DSP_HALT_ON_RESET", dsp_id, 0);
			DSP_DBG(" *** DSP_HALT_ON_RESET ***\n");

			return 0;
		}
		udelay(1);
	}
	CVIP_EVTLOG(device->event_log, "<-DSP_HALT_ON_RESET", dsp_id, 0xdead);
	DSP_ERROR("HaltOnReset timeout. control_reg=%x status_reg=%x\n",
		  readl_relaxed(states->control_register),
		  readl_relaxed(states->status_register));
	return -ETIME;
}

static int is_mempool_internal(int dsp_id, int mempool_id)
{
	switch (mempool_id) {
	case 1 << ION_HEAP_CVIP_SRAM_Q6_M0_ID:
		if (dsp_id != DSP_Q6_0)
			return -EINVAL;
		return 1;
	case 1 << ION_HEAP_CVIP_SRAM_Q6_M1_ID:
		if (dsp_id != DSP_Q6_1)
			return -EINVAL;
		return 1;
	case 1 << ION_HEAP_CVIP_SRAM_Q6_M2_ID:
		if (dsp_id != DSP_Q6_2)
			return -EINVAL;
		return 1;
	case 1 << ION_HEAP_CVIP_SRAM_Q6_M3_ID:
		if (dsp_id != DSP_Q6_3)
			return -EINVAL;
		return 1;
	case 1 << ION_HEAP_CVIP_SRAM_Q6_M4_ID:
		if (dsp_id != DSP_Q6_4)
			return -EINVAL;
		return 1;
	case 1 << ION_HEAP_CVIP_SRAM_Q6_M5_ID:
		if (dsp_id != DSP_Q6_5)
			return -EINVAL;
		return 1;
	case 1 << ION_HEAP_CVIP_SRAM_C5_M0_ID:
		if (dsp_id != DSP_C5_0)
			return -EINVAL;
		return 1;
	case 1 << ION_HEAP_CVIP_SRAM_C5_M1_ID:
		if (dsp_id != DSP_C5_1)
			return -EINVAL;
		return 1;
	}
	return 0;
}

static struct dma_buf *helper_get_dma_buf(int fd)
{
	struct dma_buf *dmabuf;

	CVIP_EVTLOG(dsp_manager_dev->event_log, "fd", fd);
	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		DSP_ERROR("Invalid fd unable to get dma buffer.\n");
		return ERR_PTR(-EINVAL);
	}
	return dmabuf;
}

static void helper_create_dspname(int dsp_id, char *dsp_name, int len)
{
	switch (dsp_id) {
	case DSP_Q6_0:
		snprintf(dsp_name, len, "dsp@%s", DSP_Q6_0_ADDR);
		break;
	case DSP_Q6_1:
		snprintf(dsp_name, len, "dsp@%s", DSP_Q6_1_ADDR);
		break;
	case DSP_Q6_2:
		snprintf(dsp_name, len, "dsp@%s", DSP_Q6_2_ADDR);
		break;
	case DSP_Q6_3:
		snprintf(dsp_name, len, "dsp@%s", DSP_Q6_3_ADDR);
		break;
	case DSP_Q6_4:
		snprintf(dsp_name, len, "dsp@%s", DSP_Q6_4_ADDR);
		break;
	case DSP_Q6_5:
		snprintf(dsp_name, len, "dsp@%s", DSP_Q6_5_ADDR);
		break;
	case DSP_C5_0:
		snprintf(dsp_name, len, "dsp@%s", DSP_C5_0_ADDR);
		break;
	case DSP_C5_1:
		snprintf(dsp_name, len, "dsp@%s", DSP_C5_1_ADDR);
		break;
	default:
		DSP_ERROR("Invalid argument for dsp ID %d.\n", dsp_id);
	}
}

struct device *helper_get_iommu_device(int dsp_id, int *access_attr)
{
	struct device *iommu_device;
	char iommu_name[64];
	char channel_name[64];
	int access_mask = 0;

	if (!access_attr) {
		DSP_ERROR("Invalid argument.\n");
		return ERR_PTR(-EINVAL);
	}

	helper_create_dspname(dsp_id, iommu_name, ARRAY_SIZE(iommu_name));

	if  (*access_attr & ICACHE) {
		snprintf(channel_name, sizeof(channel_name), "%s-icache",
			 iommu_name);
		access_mask |= ICACHE;
		if (dsp_id == DSP_C5_0 || dsp_id == DSP_C5_1)
			access_mask |= DCACHE;
	} else if (*access_attr & DCACHE) {
		if (dsp_id == DSP_C5_0 || dsp_id == DSP_C5_1) {
			snprintf(channel_name, sizeof(channel_name),
				 "%s-icache", iommu_name);
			access_mask |= ICACHE;
		} else {
			snprintf(channel_name, sizeof(channel_name),
				 "%s-dcache", iommu_name);
		}
		access_mask |= DCACHE;
	} else if (*access_attr & IDMA) {
		snprintf(channel_name, sizeof(channel_name), "%s-idma",
			 iommu_name);
		access_mask |= IDMA;
	} else {
		DSP_ERROR("Invalid argument for access attribute %d.\n",
			  *access_attr);
		return ERR_PTR(-EINVAL);
	}

	DSP_DBG("iommu domain lookup: %s\n", channel_name);
	iommu_device = cvip_dsp_get_channel_dev_byname(channel_name);
	if (!iommu_device) {
		DSP_ERROR("No device with name %s.\n", channel_name);
		return ERR_PTR(-EINVAL);
	}

	*access_attr &= ~access_mask;

	return iommu_device;
}

struct clk *helper_get_clk(int dsp_id)
{
	struct clk *clk;
	char dsp_name[64];

	helper_create_dspname(dsp_id, dsp_name, ARRAY_SIZE(dsp_name));
	clk = cvip_dsp_get_clk_byname(dsp_name);
	if (IS_ERR_OR_NULL(clk))
		DSP_ERROR("No clock device with name %s, err=%ld.\r",
			  dsp_name, PTR_ERR(clk));
	return clk;
}

int helper_get_pdbypass(int dsp_id)
{
	char dsp_name[64];

	helper_create_dspname(dsp_id, dsp_name, ARRAY_SIZE(dsp_name));
	return cvip_dsp_get_pdbypass_byname(dsp_name);
}

static void dsp_memory_entry_deallocate_nolock(struct dsp_memory_t *memory)
{
	int i;

	if (memory->domain0 && memory->size_0) {
		CVIP_EVTLOG(dsp_manager_dev->event_log, "iommu_unmap domain0", memory->addr, memory->size);
		iommu_unmap(memory->domain0, (unsigned long)memory->addr, memory->size_0);
	}
	if (memory->vaddr)
		dma_buf_kunmap(memory->dmabuf, 0, memory->vaddr);

	for (i = 0; i < memory->mapcnt; i++) {
		if (memory->domain[i]) {
			CVIP_EVTLOG(dsp_manager_dev->event_log, "iommu_unmap domain", i, memory->addr, memory->size);
			iommu_unmap(memory->domain[i], (unsigned long)memory->addr, memory->size);
		}
		if (memory->sg_table[i] && memory->attach[i])
			dma_buf_unmap_attachment(memory->attach[i], memory->sg_table[i],
						 DMA_BIDIRECTIONAL);
		if (memory->attach[i])
			dma_buf_detach(memory->dmabuf, memory->attach[i]);
	}
	if (memory->dmabuf)
		dma_buf_put(memory->dmabuf);
	if (memory->fd >= 0) {
		if (memory->allocated) {
			CVIP_EVTLOG(dsp_manager_dev->event_log, "close FD", memory->fd);
			ksys_close(memory->fd);
		}
		memory->fd = -1;
	}
}

static void dsp_memory_deallocate_nolock(struct dsp_resources_t *dsp_resource, bool fd_release)
{
	struct list_head *pos;
	struct list_head *q;
	struct dsp_memory_t *memory;

	CVIP_EVTLOG(dsp_manager_dev->event_log, "->dsp", dsp_resource->dsp_id);
	list_for_each_safe(pos, q, &dsp_resource->memory_list) {
		memory = list_entry(pos, struct dsp_memory_t, list);
		/*
		 * If this is a user mode process close related release, then
		 * there is no need to close the buffer FD, since the process
		 * exit will trigger all the FD close system call
		 */
		if (fd_release)
			memory->allocated = 0;
		dsp_memory_entry_deallocate_nolock(memory);
		list_del(pos);
		kfree(memory);
	}
}

static void dsp_device_deallocate_nolock(struct dsp_device_t *dsp_device)
{
	struct list_head *pos;
	struct list_head *q;
	struct dsp_resources_t *tmp_resource;
	struct dsp_client_t *tmp_client;

	if (dsp_device) {
		list_for_each_safe(pos, q, &dsp_device->dsp_list) {
			tmp_resource = list_entry(pos, struct dsp_resources_t,
						  list);
			tmp_resource->states->submit_mode = DSP_OFF;
			dsp_memory_deallocate_nolock(tmp_resource, false);
			dsp_helper_off(dsp_device, tmp_resource);
			kfree(tmp_resource->states);
			list_del(pos);
			kfree(tmp_resource);
		}
		list_for_each_safe(pos, q, &dsp_device->client_list) {
			tmp_client = list_entry(pos, struct dsp_client_t, list);
			list_del(pos);
			kfree(tmp_client);
		}
		if (dsp_device->ocd_register)
			iounmap(dsp_device->ocd_register);
		if (dsp_device->status_register)
			iounmap(dsp_device->status_register);
		if (dsp_device->pgfsm_register)
			iounmap(dsp_device->pgfsm_register);
		if (dsp_device->Q6_cntrl_register)
			iounmap(dsp_device->Q6_cntrl_register);
		if (dsp_device->C5_cntrl_register)
			iounmap(dsp_device->C5_cntrl_register);
		dsp_debugfs_rm(dsp_device);
		kfree(dsp_device);
	}
}

static struct dsp_device_t *dsp_device_allocate(void)
{
	int i;
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *tmp_resource;

	/* Allocate DSP Device */
	dsp_device = kzalloc(sizeof(*dsp_device), GFP_KERNEL);
	if (!dsp_device) {
		DSP_ERROR("Failed to allocate device.\n");
		goto error;
	}

	dsp_device->event_log = CVIP_EVTLOGINIT(EVENT_LOG_SIZE);
	if (dsp_device->event_log < 0) {
		DSP_ERROR("Failed to create an event log.\n");
		goto error;
	}

	dsp_device->ocd_register = ioremap(OCD_BASE, DSP_MAX * OCD_SIZE);
	if (!dsp_device->ocd_register) {
		DSP_ERROR("Failed to map dsp ocd registers.\n");
		goto error;
	}

	dsp_device->status_register =
		ioremap(DEST_POWER_CLK_CNTRL + DSP_STATUS_BASE,
			DSP_STATUS_WIDTH * DSP_MAX);
	if (!dsp_device->status_register) {
		DSP_ERROR("Failed to map dsp staus registers.\n");
		goto error;
	}

	dsp_device->pgfsm_register =
		ioremap(RSMU_PGFSM_CVIP_BASE, PGFSM_WIDTH);
	if (!dsp_device->pgfsm_register) {
		DSP_ERROR("Failed to map dsp PGFSM registers.\n");
		goto error;
	}

	dsp_device->Q6_cntrl_register =
		ioremap(DEST_POWER_CLK_CNTRL + DSP_Q6_CONTROL_BASE,
			DSP_CONTROL_WIDTH * DSP_C5_0);
	if (!dsp_device->Q6_cntrl_register) {
		DSP_ERROR("Failed to map dsp control registers.\n");
		goto error;
	}

	dsp_device->C5_cntrl_register =
		ioremap(DEST_POWER_CLK_CNTRL + DSP_C5_CONTROL_BASE,
			DSP_CONTROL_WIDTH * (DSP_MAX - DSP_C5_0));
	if (!dsp_device->C5_cntrl_register) {
		DSP_ERROR("Failed to map dsp control registers.\n");
		goto error;
	}

	/* Initialize Mutex Lock */
	mutex_init(&dsp_device->dsp_device_lock);

	/* Initialize client_list */
	INIT_LIST_HEAD(&dsp_device->client_list);

	/* Initialize dsp_list */
	INIT_LIST_HEAD(&dsp_device->dsp_list);
	for (i = DSP_Q6_0; i < DSP_MAX; i++) {
		tmp_resource =
			kzalloc(sizeof(struct dsp_resources_t), GFP_KERNEL);
		if (!tmp_resource) {
			DSP_ERROR("Failed to allocate resource for DSP %d.\n",
				  i);
			goto error;
		}
		mutex_init(&tmp_resource->dsp_resource_lock);
		tmp_resource->dsp_id = i;
		INIT_LIST_HEAD(&tmp_resource->memory_list);
		tmp_resource->states =
			kzalloc(sizeof(struct dsp_state_t), GFP_KERNEL);
		if (!tmp_resource->states) {
			DSP_ERROR("Failed to allocate states for DSP %d.\n", i);
			goto post_resource_error;
		}

		if (i >= DSP_Q6_0 && i <= DSP_Q6_5)
			tmp_resource->states->ocd_register =
				dsp_device->ocd_register + (2 + i) * OCD_SIZE;
		else if (i >= DSP_C5_0 && i <= DSP_C5_1)
			tmp_resource->states->ocd_register =
				dsp_device->ocd_register + (i - DSP_C5_0) *
				OCD_SIZE;

		tmp_resource->states->status_register =
			dsp_device->status_register + DSP_STATUS_WIDTH * i;
		tmp_resource->states->pgfsm_register =
			dsp_device->pgfsm_register;
		if (i >= DSP_Q6_0 && i <= DSP_Q6_5)
			tmp_resource->states->control_register =
				dsp_device->Q6_cntrl_register +
				DSP_CONTROL_WIDTH * i;
		else if (i >= DSP_C5_0 && i <= DSP_C5_1)
			tmp_resource->states->control_register =
				dsp_device->C5_cntrl_register +
				DSP_CONTROL_WIDTH * (i - DSP_C5_0);

		tmp_resource->states->submit_mode = DSP_OFF;
		tmp_resource->states->bypass_pd = ENABLE;
		dsp_helper_off(dsp_device, tmp_resource);
		tmp_resource->states->bypass_pd = DISABLE;
		tmp_resource->states->alt_stat_vector_sel = DISABLE;
		tmp_resource->client = NULL;
		list_add(&tmp_resource->list, &dsp_device->dsp_list);
	}
	if (dsp_debugfs_add(dsp_device)) {
		DSP_ERROR("Failed to create debugfs %d.\n", i);
		goto error;
	}

	/* Initialize Q6 and C5 current dpm clk */
	dsp_device->q6_cur_dpmclk = MIN_Q6_CLK_RATE;
	dsp_device->c5_cur_dpmclk = MIN_C5_CLK_RATE;

	return dsp_device;

post_resource_error:
	kfree(tmp_resource);
error:
	dsp_device_deallocate_nolock(dsp_device);
	return ERR_PTR(-ENOMEM);
}

static int __init dsp_init(void)
{
	int ret;

	/* Registering character device */
	ret = register_chrdev(0, DEVICE_NAME, &dsp_fops);
	if (ret < 0) {
		DSP_ERROR("Failed to register with %d\n", ret);
		goto error;
	}
	dsp_major = ret;
	DSP_DBG("Device registered at %d\n", dsp_major);

	/* Allocating memory for DSP structures */
	dsp_manager_dev = dsp_device_allocate();
	if (IS_ERR(dsp_manager_dev)) {
		DSP_ERROR("Failed to allocate DSP device.\n");
		ret = PTR_ERR(dsp_manager_dev);
		goto post_chrdev_error;
	}

	/* Register the device class */
	dsp_manager_dev->char_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(dsp_manager_dev->char_class)) {
		DSP_ERROR("Failed to register device class.\n");
		ret = PTR_ERR(dsp_manager_dev->char_class);
		goto post_dsp_alloc_error;
	}

	/* Register the device driver */
	dsp_manager_dev->char_device =
		device_create(dsp_manager_dev->char_class,
			      NULL, MKDEV(dsp_major, 0),
			      NULL, DEVICE_NAME);
	if (IS_ERR(dsp_manager_dev->char_device)) {
		DSP_ERROR("Failed to create the device.\n");
		ret = PTR_ERR(dsp_manager_dev->char_device);
		goto post_class_error;
	}

	atomic_notifier_chain_register(&panic_notifier_list,
				       &dsp_manager_panic_notifier);
	register_reboot_notifier(&dsp_manager_reboot_notifier);

	DSP_DBG("Initialized successfully. Hello\n");
	return 0;

post_class_error:
	class_destroy(dsp_manager_dev->char_class);
post_dsp_alloc_error:
	dsp_device_deallocate_nolock(dsp_manager_dev);
post_chrdev_error:
	unregister_chrdev(dsp_major, DEVICE_NAME);
error:
	return ret;
}

static void __exit dsp_exit(void)
{
	unregister_reboot_notifier(&dsp_manager_reboot_notifier);
	atomic_notifier_chain_unregister(&panic_notifier_list,
					 &dsp_manager_panic_notifier);
	device_destroy(dsp_manager_dev->char_class, MKDEV(dsp_major, 0));
	class_unregister(dsp_manager_dev->char_class);
	class_destroy(dsp_manager_dev->char_class);
	unregister_chrdev(dsp_major, DEVICE_NAME);
	dsp_device_deallocate_nolock(dsp_manager_dev);
	DSP_DBG("Exit successfully. Bye\n");
}

static int dsp_open(struct inode *inode, struct file *file)
{
	struct dsp_client_t *tmp_client;
	int ret;

	if (!try_module_get(THIS_MODULE)) {
		DSP_ERROR("Device closed, failed to open.\n");
		ret = 0;
		goto error;
	}

	DSP_DBG("New client access encountered.\n");
	tmp_client = kzalloc(sizeof(*tmp_client), GFP_KERNEL);
	if (!tmp_client) {
		DSP_ERROR("Failed to allocate new client.\n");
		ret = -ENOMEM;
		goto post_module_error;
	}

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto post_client_error;
	}

	list_add(&tmp_client->list, &dsp_manager_dev->client_list);
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	file->private_data = tmp_client;
	DSP_DBG("Client successfully opened.\n");
	return 0;

post_client_error:
	kfree(tmp_client);
post_module_error:
	module_put(THIS_MODULE);
error:
	return ret;
}

static ssize_t dsp_read(struct file *file, char __user *user_buffer,
			size_t size, loff_t *offset)
{
	DSP_DBG("Read is not implemented.\n");
	return 0;
	/* TBD */
}

int dsp_release(struct inode *inode, struct file *file)
{
	struct list_head *pos;
	struct dsp_resources_t *tmp_resource;
	struct dsp_client_t *client = (struct dsp_client_t *)file->private_data;
	int ret;

	DSP_DBG("Client closing connection.\n");
	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}
	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		tmp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (tmp_resource->client == client) {
			CVIP_EVTLOG(dsp_manager_dev->event_log, "->DSP", tmp_resource->dsp_id);
			tmp_resource->states->submit_mode = DSP_OFF;
			dsp_memory_deallocate_nolock(tmp_resource, true);
			dsp_helper_off(dsp_manager_dev, tmp_resource);
			tmp_resource->client = NULL;
		}
	}
	DSP_DBG("Client successfully closed.\n");
	list_del(&client->list);
	kfree(client);
	module_put(THIS_MODULE);
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
error:
	return ret;
}

static long dsp_mem_map(struct dsp_client_t *client, unsigned long arg, bool allocated)
{
	struct mem_alloc_t ioctl_param;
	struct list_head *pos;
	struct dsp_memory_t *dsp_memory;
	struct dsp_resources_t *dsp_resources = NULL;
	struct dma_buf *dma_buff_ret = NULL;
	struct dma_buf_attachment *dma_attach[DSPMEM_MAP_NUM] = {NULL};
	struct sg_table *sg_table[DSPMEM_MAP_NUM] = {NULL};
	struct iommu_domain *domain_ret[DSPMEM_MAP_NUM] = {NULL};
	struct iommu_domain *domain_0 = NULL;
	struct device *iommudev_ret = NULL;
	unsigned int mapped_size = 0;
	unsigned int mapped_size_0 = 0;
	int iommu_prot;
	int internal = 0;
	int ret;
	uint64_t phyaddr;
	unsigned int upperaddr;
	unsigned int loweraddr;
	void *vaddr = NULL;
	int mapcnt = 0;
	int i;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error;
	}

	if (copy_from_user(&ioctl_param, (struct mem_alloc_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error;
	}

	if (ioctl_param.dsp_id < DSP_Q6_0 || ioctl_param.dsp_id >= DSP_MAX) {
		DSP_ERROR("Invalid DSP ID: %d.\n",
			  ioctl_param.dsp_id);
		ret = -EINVAL;
		goto error;
	}

	CVIP_EVTLOG(dsp_manager_dev->event_log, "->DSP_MEM_MAP",
		    ioctl_param.dsp_id, 0x1111);
	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	DSP_DBG("IOCTL_MEM_MAP started.\n");
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}

	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error;
	}

	dsp_qreqn_pd_enable(dsp_manager_dev, dsp_resources->states);

	dsp_resources->states->bypass_pd =
		helper_get_pdbypass(dsp_resources->dsp_id);

	if (ioctl_param.sect_size == 0 && ioctl_param.mem_pool_id == NO_ION) {
		ioctl_param.fd = -1;
		CVIP_EVTLOG(dsp_manager_dev->event_log, "NO_ION");
		goto dsp_update_resource;
	}

	if (ioctl_param.fd < 0) {
		DSP_ERROR("Invalid FILE DESCRIPTOR: %d.\n",
			  ioctl_param.fd);
		ret = -EINVAL;
		goto post_lock_error;
	}

	dma_buff_ret = helper_get_dma_buf(ioctl_param.fd);

	if (IS_ERR(dma_buff_ret)) {
		DSP_ERROR("Failed to get shared memory dma buffer.\n");
		ret = PTR_ERR(dma_buff_ret);
		goto post_lock_error;
	}

	internal = is_mempool_internal(dsp_resources->dsp_id,
				       ioctl_param.mem_pool_id);
	if (internal < 0) {
		DSP_ERROR("Internal memory pool %d not matching dsp %d.\n",
			  ioctl_param.dsp_id, ioctl_param.mem_pool_id);
		ret = internal;
		goto post_dma_buf_error;
	}
	if (internal) {
		domain_ret[0] = NULL;
		domain_0 = NULL;
		mapped_size = ioctl_param.sect_size;
		CVIP_EVTLOG(dsp_manager_dev->event_log, "internal");
	} else {
		int access_attr = ioctl_param.access_attr & ~NOSHARE;

		while (access_attr && (mapcnt < DSPMEM_MAP_NUM)) {
			iommudev_ret =
				helper_get_iommu_device(dsp_resources->dsp_id,
							&access_attr);
			if (!iommudev_ret) {
				break;
			} else if (IS_ERR(iommudev_ret)) {
				DSP_ERROR("Failed to get device for DSP %d.\n",
					  dsp_resources->dsp_id);
				ret = PTR_ERR(iommudev_ret);
				goto post_dma_map_error;
			}

			domain_ret[mapcnt] =
				iommu_get_domain_for_dev(iommudev_ret);
			if (IS_ERR(domain_ret[mapcnt])) {
				DSP_ERROR("Failed to get domain for DSP %d.\n",
					  dsp_resources->dsp_id);
				ret = PTR_ERR(domain_ret[mapcnt]);
				goto post_dma_map_error;
			}

			dma_attach[mapcnt] =
				dma_buf_attach(dma_buff_ret, iommudev_ret);
			if (IS_ERR(dma_attach[mapcnt])) {
				DSP_ERROR("Failed to attach dsp to dmabuf.\n");
				goto post_dma_map_error;
			}

			sg_table[mapcnt] =
				dma_buf_map_attachment(dma_attach[mapcnt],
						       DMA_BIDIRECTIONAL);
			if (IS_ERR(sg_table[mapcnt])) {
				DSP_ERROR("Failed to get sg for dmabuf.\n");
				ret = PTR_ERR(sg_table[mapcnt]);
				goto post_dma_map_error;
			}

			iommu_prot = IOMMU_READ | IOMMU_WRITE;
			mapped_size =
				 iommu_map_sg(domain_ret[mapcnt],
					      (unsigned long)
					      ioctl_param.sect_addr,
					      sg_table[mapcnt]->sgl,
					      sg_table[mapcnt]->nents,
					      iommu_prot);
			if (mapped_size <= 0) {
				DSP_ERROR("Failed to map MMU with sg.\n");
				ret = -EFAULT;
				goto post_dma_map_error;
			}

			phyaddr = page_to_phys(sg_page(sg_table[mapcnt]->sgl));
			loweraddr = phyaddr & 0xFFFFFFFF;
			upperaddr = (phyaddr >> 32) & 0xFFFFFFFF;
			CVIP_EVTLOG(dsp_manager_dev->event_log,
				    "Mapped DSP and Address to DMA Address with size",
				    ioctl_param.dsp_id,
				    ioctl_param.sect_addr,
				    upperaddr,
				    loweraddr,
				    mapped_size);
			DSP_DBG("mapped DSP[%d] VA:0x%llX PA:0x%X%X sz:0x%X\n",
				ioctl_param.dsp_id, (u64)ioctl_param.sect_addr,
				upperaddr, loweraddr, mapped_size);

			mapcnt++;
		}

		vaddr = dma_buf_kmap(dma_buff_ret, 0);
		if (!vaddr) {
			DSP_ERROR("Failed kmap for dmabuf\n");
			ret = PTR_ERR(vaddr);
			goto post_dma_map_error;
		}

		if (ioctl_param.access_attr & NOSHARE) {
			DSP_DBG("Skip domain_0 iommu map for DSP#%d\n",
				ioctl_param.dsp_id);
			goto dsp_update_resource;
		}

		domain_0 = biommu_get_domain_byname("biommu!vdsp@0");
		if (IS_ERR(domain_0)) {
			DSP_ERROR("Failed to get iommu domain-0 for DSP %d\n",
				  dsp_resources->dsp_id);
			ret = PTR_ERR(domain_ret);
			goto post_iommu_error_0;
		}

		mapped_size_0 =
			iommu_map_sg(domain_0,
				     (unsigned long)ioctl_param.sect_addr,
				     sg_table[0]->sgl, sg_table[0]->nents,
				     iommu_prot);
		if (mapped_size_0 <= 0)
			DSP_ERROR("Duplicate iommu map in domain-0, ignore!\n");
	}

dsp_update_resource:
	dsp_memory = kzalloc(sizeof(*dsp_memory), GFP_KERNEL);
	if (!dsp_memory) {
		DSP_ERROR("Failed to allocate dsp memory structure.\n");
		ret = -EFAULT;
		goto post_iommu_error;
	}

	dsp_resources->client = client;

	dsp_memory->allocated = allocated;
	dsp_memory->fd = ioctl_param.fd;
	dsp_memory->mapcnt = mapcnt;
	dsp_memory->dmabuf = dma_buff_ret;
	memcpy(dsp_memory->attach, dma_attach, sizeof(dma_attach));
	dsp_memory->domain0 = domain_0;
	memcpy(dsp_memory->domain, domain_ret, sizeof(domain_ret));
	dsp_memory->mem_pool_id = ioctl_param.mem_pool_id;
	dsp_memory->cache_attr = ioctl_param.cache_attr;
	dsp_memory->addr = ioctl_param.sect_addr;
	dsp_memory->size = mapped_size;
	dsp_memory->size_0 = mapped_size_0;
	dsp_memory->access_attr = ioctl_param.access_attr;
	memcpy(dsp_memory->sg_table, sg_table, sizeof(sg_table));
	dsp_memory->vaddr = vaddr;
	list_add(&dsp_memory->list, &dsp_resources->memory_list);
	DSP_DBG("IOCTL_MEM_ALLOC successful.\n");
	mutex_unlock(&dsp_resources->dsp_resource_lock);
	CVIP_EVTLOG(dsp_manager_dev->event_log, "<-DSP_MEM_MAP",
		    ioctl_param.dsp_id, 0x9999);
	return 0;

post_iommu_error:
	if (!internal && mapped_size_0 > 0)
		iommu_unmap(domain_0, (unsigned long)ioctl_param.sect_addr,
			    mapped_size_0);
post_iommu_error_0:
	dma_buf_kunmap(dma_buff_ret, 0, vaddr);
post_dma_map_error:
	for (i = 0; i <= mapcnt; i++) {
		if (!internal && !IS_ERR_OR_NULL(domain_ret[i]))
			iommu_unmap(domain_ret[i],
				    (unsigned long)ioctl_param.sect_addr,
				    mapped_size);
		if (!IS_ERR_OR_NULL(dma_attach[i]) &&
		    !IS_ERR_OR_NULL(sg_table[i]))
			dma_buf_unmap_attachment(dma_attach[i],
						 sg_table[i],
						 DMA_BIDIRECTIONAL);
		if (!IS_ERR_OR_NULL(dma_attach[i]))
			dma_buf_detach(dma_buff_ret, dma_attach[i]);
	}
post_dma_buf_error:
	dma_buf_put(dma_buff_ret);
post_lock_error:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error:
	CVIP_EVTLOG(dsp_manager_dev->event_log, "<-DSP_MEM_MAP",
		    ioctl_param.dsp_id, 0xdead, -ret);
	return ret;
}

static long dsp_mem_unmap(struct dsp_client_t *client, unsigned long arg)
{
	struct mem_alloc_t ioctl_param;
	struct dsp_memory_t *memory;
	struct list_head *pos;
	struct list_head *mpos;
	struct list_head *q;
	struct dsp_resources_t *dsp_resources = NULL;
	int ret;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error;
	}

	if (copy_from_user(&ioctl_param, (struct mem_alloc_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error;
	}

	if (ioctl_param.fd < 0) {
		DSP_ERROR("Invalid ION FILE DESCRIPTOR: %d.\n",
			  ioctl_param.fd);
		ret = -EINVAL;
		goto error;
	}

	if (ioctl_param.dsp_id < DSP_Q6_0 || ioctl_param.dsp_id >= DSP_MAX) {
		DSP_ERROR("Invalid DSP ID: %d.\n",
			  ioctl_param.dsp_id);
		ret = -EINVAL;
		goto error;
	}

	CVIP_EVTLOG(dsp_manager_dev->event_log, "->DSP_MEM_UNMAP",
		    ioctl_param.dsp_id, 0x1111);
	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	DSP_DBG("IOCTL_MEM_UNMAP started.\n");
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto post_lock_error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id) {
			list_for_each_safe(mpos, q, &dsp_resources->memory_list) {
				memory = list_entry(mpos, struct dsp_memory_t, list);
				if (memory->fd == ioctl_param.fd) {
					dsp_memory_entry_deallocate_nolock(memory);
					list_del(mpos);
					kfree(memory);
				}
			}
		}
	}
	DSP_DBG("Client successfully unmap memory.\n");
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	CVIP_EVTLOG(dsp_manager_dev->event_log, "<-DSP_MEM_UNMAP",
		    ioctl_param.dsp_id, 0x9999);
	return 0;

post_lock_error:
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	CVIP_EVTLOG(dsp_manager_dev->event_log, "<-DSP_MEM_UNMAP",
		    ioctl_param.dsp_id, 0xdead, -ret);
error:
	return ret;
}

static long dsp_mem_alloc(struct dsp_client_t *client, unsigned long arg)
{
	struct mem_alloc_t ioctl_param;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	int fd;
	int ret;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error;
	}

	if (copy_from_user(&ioctl_param, (struct mem_alloc_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error;
	}

	if (ioctl_param.dsp_id < DSP_Q6_0 || ioctl_param.dsp_id >= DSP_MAX) {
		DSP_ERROR("Invalid DSP ID: %d.\n",
			  ioctl_param.dsp_id);
		ret = -EINVAL;
		goto error;
	}

	CVIP_EVTLOG(dsp_manager_dev->event_log, "->DSP_MEM_ALLOC",
		    ioctl_param.dsp_id, 0x1111);
	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	DSP_DBG("IOCTL_MEM_ALLOC started.\n");
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}

	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error;
	}

	/*
	 * allow bypassing memmory allocation if sect_size is 0, and mem_pool_id
	 * is -1. This is to support multiple DSP sharing the same allocation
	 * for streamID-0.
	 */
	if (ioctl_param.sect_size == 0 && ioctl_param.mem_pool_id == NO_ION) {
		DSP_DBG("Skipping memory alloc for DSP#%d\n",
			ioctl_param.dsp_id);
		fd = -1;
		goto dsp_mem_alloc_end;
	}

	DSP_DBG("IOCTL_MEM_ALLOC allocating memory.\n");
	fd = ion_alloc(ioctl_param.sect_size, ioctl_param.mem_pool_id,
		       ioctl_param.cache_attr);
	if (fd < 0) {
		DSP_ERROR("Failed to allocated ION shared memory.\n");
		ret = -ENOMEM;
		goto post_lock_error;
	}

dsp_mem_alloc_end:
	ioctl_param.fd = fd;
	if (copy_to_user((struct mem_alloc_t __user *)arg, &ioctl_param, sizeof(ioctl_param))) {
		DSP_ERROR("Unable to copy to user.\n");
		ret = -EFAULT;
		goto post_ion_error;
	}
	mutex_unlock(&dsp_resources->dsp_resource_lock);

	ret = dsp_mem_map(client, arg, true);
	if (ret) {
		DSP_ERROR("Failed to map memory.\n");
		ksys_close(fd);
		goto error;
	}

	DSP_DBG("IOCTL_MEM_ALLOC successful.\n");
	CVIP_EVTLOG(dsp_manager_dev->event_log, "<-DSP_MEM_ALLOC",
		    ioctl_param.dsp_id, 0x9999);
	return 0;

post_ion_error:
	ksys_close(fd);
post_lock_error:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error:
	CVIP_EVTLOG(dsp_manager_dev->event_log, "<-DSP_MEM_ALLOC",
		    ioctl_param.dsp_id, 0xdead, -ret);
	return ret;
}

static long dsp_mem_dealloc(struct dsp_client_t *client)
{
	struct list_head *pos;
	struct dsp_resources_t *tmp_resource;
	int ret;

	DSP_DBG("Client dealloc memory.\n");
	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error_dealloc;
	}
	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		tmp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (tmp_resource->client == client) {
			CVIP_EVTLOG(dsp_manager_dev->event_log, "dsp", tmp_resource->dsp_id);
			tmp_resource->states->submit_mode = DSP_OFF;
			dsp_memory_deallocate_nolock(tmp_resource, false);
			dsp_helper_off(dsp_manager_dev, tmp_resource);
		}
	}
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	DSP_DBG("Client successfully dealloc memory.\n");
error_dealloc:
	return ret;
}

static long dsp_set_mode(struct dsp_client_t *client, unsigned long arg)
{
	struct set_mode_t ioctl_param;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	int ret;
	int dspid = -1;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error;
	}

	if (copy_from_user(&ioctl_param, (struct set_mode_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error;
	}

	if (ioctl_param.dsp_id >= DSP_MAX || ioctl_param.dsp_id < DSP_Q6_0) {
		DSP_ERROR("Invalid DSP ID.\n");
		ret = -EINVAL;
		goto error;
	}
	dspid = ioctl_param.dsp_id;

	CVIP_EVTLOG(dsp_manager_dev->event_log, "->DSP_SET_MODE", dspid,
		    ioctl_param.dsp_mode, 0x1111);
	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	DSP_DBG("IOCTL_DSP_SET_MODE for dsp%d started.\n", dspid);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == dspid)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP%d not registered.\n", dspid);
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}

	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error;
	}

	if (!dsp_resources->states->clk) {
		dsp_resources->states->clk = helper_get_clk(dsp_resources->dsp_id);
		if (IS_ERR_OR_NULL(dsp_resources->states->clk)) {
			DSP_ERROR("Failed to get dsp%d clock.\n", dsp_resources->dsp_id);
			dsp_resources->states->clk = NULL;
		}
	}

	dsp_resources->states->submit_mode = ioctl_param.dsp_mode;
	switch (ioctl_param.dsp_mode) {
	case DSP_OFF:
		ret = dsp_helper_off(dsp_manager_dev, dsp_resources);
		break;
	case DSP_STOP:
		ret = dsp_helper_stop(dsp_manager_dev, dsp_resources->states,
				      dsp_resources->dsp_id);
		break;
	case DSP_PAUSE:
		ret = dsp_helper_pause(dsp_manager_dev, dsp_resources->states,
				       dsp_resources->dsp_id);
		break;
	case DSP_UNPAUSE:
		ret = dsp_helper_unpause(dsp_manager_dev, dsp_resources->states,
					 dsp_resources->dsp_id);
		break;
	case DSP_RUN:
		ret = dsp_helper_run(dsp_manager_dev, dsp_resources->states,
				     dsp_resources->dsp_id);
		break;
	case DSP_HALT_ON_RESET:
		ret = dsp_helper_haltonreset(dsp_manager_dev,
					     dsp_resources->states,
					     dsp_resources->dsp_id);
		break;
	case DSP_EXIT_OCD:
		ret = dsp_helper_exit_ocd(dsp_manager_dev,
					  dsp_resources->states,
					  dsp_resources->dsp_id);
		break;
	default:
		DSP_ERROR("Invalid DSP mode for set mode.\n");
		ret = -EINVAL;
	}
	if (ret)
		goto post_lock_error;
	mutex_unlock(&dsp_resources->dsp_resource_lock);
	DSP_DBG("IOCTL_DSP_SET_MODE for dsp%d successful.\n", dspid);
	CVIP_EVTLOG(dsp_manager_dev->event_log, "<-DSP_SET_MODE", dspid, 0x9999);
	return 0;
post_lock_error:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error:
	CVIP_EVTLOG(dsp_manager_dev->event_log, "<-DSP_SET_MODE",
		    ioctl_param.dsp_id, 0xdead, -ret);
	return ret;
}

static long dsp_get_state(struct dsp_client_t *client, unsigned long arg)
{
	struct get_state_t ioctl_param;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	int dsp_mode;
	int ret;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error;
	}

	if (copy_from_user(&ioctl_param, (struct get_state_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error;
	}

	if (ioctl_param.dsp_id >= DSP_MAX || ioctl_param.dsp_id < DSP_Q6_0) {
		DSP_ERROR("Invalid DSP ID.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	DSP_DBG("IOCTL_DSP_GET_STATE started.\n");
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}

	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error;
	}

	dsp_mode = dsp_helper_read(dsp_resources->states);
	ioctl_param.dsp_mode = dsp_mode;
	if (copy_to_user((struct get_state_t __user *)arg, &ioctl_param,
			 sizeof(ioctl_param))) {
		DSP_ERROR("Unable to copy to user.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}
	DSP_DBG("IOCTL_DSP_GET_STATE successful.\n");
post_lock_error:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error:
	return ret;
}

static inline int ocd_helper_hw_ibreak_hit(void __iomem *base, int dsp_id)
{
	u32 val;
	int ret;

	val = readl_relaxed(OCD_DSR(base));
	DSP_DBG("DSR:0x%x\n", val);
	if (!(val & OCD_DSR_STOPPED))
		return 0;

	if (DSP_IS_XEA3(dsp_id)) {
		val = val & OCD_DSR_STOPPEDCAUSE;
		val = val >> OCD_DSR_STOPPEDCAUSE_SHIFT;
		ret = (val == 2) ? 1 : 0;
	} else {
		ret = reg_get_sr(base, OCD_SR_DEBUGCAUSE, &val);
		DSP_DBG("DEBUGCASE:0x%x\n", val);
		ret = (!ret) ? (val & 0x2) : 0;
	}

	return ret;
}

static int ocd_helper_wait_dsr_set(void __iomem *base, u32 bits, u32 tout)
{
	int n;
	u32 val;

	for (n = 0; ;) {
		val = readl_relaxed(OCD_DSR(base));
		if ((val & bits) == bits)
			return OCD_ERR_OK;
		if (n++ == tout)
			return -OCD_ERR_TIMEOUT;
		udelay(1);
	}
}

static void ocd_helper_clear_dsr_status(void __iomem *base, u32 bits)
{
	writel_relaxed(bits, OCD_DSR(base));
	/* clear DSR.ExecDone */
	wmb();
}

static int ocd_helper_wait_exec_set(void __iomem *base, u32 tout)
{
	int n;
	u32 val;

	for (n = 0; ;) {
		val = readl_relaxed(OCD_DSR(base));
		DSP_DBG(" --> OCD_DSR:0x%x\n", val);
		if (val & OCD_DSR_EXECEXCEPTION)
			return -OCD_ERR_EXCEPTION;
		if (val & OCD_DSR_EXECDONE)
			return OCD_ERR_OK;
		if (n++ == tout)
			return -OCD_ERR_TIMEOUT;
		udelay(1);
	}
}

static int ocd_helper_load_sr2ar(void __iomem *base, u32 _val, u32 sr, u32 ar)
{
	u32 val;
	void __iomem *addr;
	int ret;

	DSP_DBG("start %x\n", _val);
	ocd_helper_clear_dsr_status(base, OCD_DSR_EXECDONE | OCD_DSR_COREREADDDR);

	addr = OCD_DIR(base, 0);
	val = OCD_SR_I_RSR(sr, ar);
	writel_relaxed(val, addr);
	/* make sure ocd register write updated */
	wmb();

	DSP_DBG("exec\n");
	addr = OCD_DDREXEC(base);
	val = _val;
	writel_relaxed(val, addr);
	/* make sure ocd register write updated */
	wmb();

	DSP_DBG("wait exec done\n");
	ret = ocd_helper_wait_exec_set(base, 100);
	if (ret != OCD_ERR_OK)
		return ret;

	DSP_DBG("wait CoreReadDDR\n");
	return ocd_helper_wait_dsr_set(base, OCD_DSR_COREREADDDR, 100);
}

static int ocd_helper_exec(void __iomem *base, u32 _val)
{
	u32 val;
	void __iomem *addr;

	DSP_DBG("start %x\n", _val);
	ocd_helper_clear_dsr_status(base, OCD_DSR_EXECDONE);

	addr = OCD_DIR0EXEC(base);
	val = _val;
	writel_relaxed(val, addr);

	/* make sure ocd register write updated */
	wmb();
	DSP_DBG("wait exec done\n");
	return ocd_helper_wait_exec_set(base, 100);
}

static int ocd_setup_ibreaka(void __iomem *base, int bindex, u32 baddr)
{
	int ret;
	u32 val;

	DSP_DBG("start, %x\n", baddr);
	ret = ocd_helper_load_sr2ar(base, baddr, OCD_SR_DDR, OCD_SR_AR);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("load ddr error %d\n", ret);
		return ret;
	}

	DSP_DBG("wsr\n");
	val = OCD_SR_I_WSR(OCD_SR_IBREAKA(bindex), OCD_SR_AR);
	ret = ocd_helper_exec(base, val);
	if (ret != OCD_ERR_OK)
		DSP_ERROR("exec error %d\n", ret);

	return ret;
}

static int ocd_enable_ibreak(void __iomem *base, int dsp_id, int bindex,
			     int enable)
{
	int ret;
	u32 val;
	int sr;
	int bit;

	DSP_DBG("start %x\n", enable);

	if (DSP_IS_XEA3(dsp_id)) {
		sr = OCD_SR_IBREAKC(bindex);
		bit = 31;
	} else {
		sr = OCD_SR_IBREAKA_EN;
		bit = bindex;
	}

	ret = reg_get_sr(base, sr, &val);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("get sr error %d\n", ret);
		return ret;
	}

	if (enable)
		val |= (1 << bit);
	else
		val &= ~(1 << bit);

	ret = reg_set_sr(base, sr, val);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("set sr error %d\n", ret);
		return ret;
	}

	return OCD_ERR_OK;
}

static int ocd_enter(void __iomem *base)
{
	u32 reg;

	DSP_DBG("dcrset 0x1\n");
	reg = readl_relaxed(OCD_DCRSET(base));
	reg |= (OCD_DCR_ENABLEOCD | OCD_DCR_DEBUGINTERRUPT);
	writel_relaxed(reg, OCD_DCRSET(base));
	/* make sure ocd register write updated */
	wmb();
	return ocd_helper_wait_dsr_set(base, OCD_DSR_STOPPED, 1000);
}

static int ocd_wait_ocddbgpower(void __iomem *base)
{
	DSP_DBG("wait DSR dbgpwr\n");
	return ocd_helper_wait_dsr_set(base, OCD_DSR_DBGPWR, 10);
}

static int ocd_wait_ocdenable(void __iomem *base)
{
	u32 val;
	int n;

	DSP_DBG("wait DCR enable ocd\n");
	for (n = 0; ;) {
		val = readl_relaxed(OCD_DCRSET(base));
		if (val & OCD_DCR_ENABLEOCD)
			break;
		if (n++ == 10)
			return -OCD_ERR_TIMEOUT;
		udelay(1);
	}

	return OCD_ERR_OK;
}

static int ocd_exit(void __iomem *base)
{
	DSP_DBG("dcrclr 0x1\n");
	writel_relaxed(OCD_DCR_ENABLEOCD, OCD_DCRCLR(base));
	/* make sure ocd register write updated */
	wmb();
	return OCD_ERR_OK;
}

static int ocd_helper_wait_ibreak(void __iomem *base, int dsp_id, int usec)
{
	u32 val;
	int n;

	val = readl_relaxed(OCD_DSR(base));
	DSP_DBG("wait !stopped %x\n", val);
	for (n = 0; ;) {
		val = readl_relaxed(OCD_DSR(base));
		if (!(val & OCD_DSR_STOPPED))
			break;
		else if (ocd_helper_hw_ibreak_hit(base, dsp_id))
			break;
		DSP_DBG("OCD_DSR=0x%x\n", val);
		if (n++ == usec)
			return -OCD_ERR_TIMEOUT;
		udelay(1);
	}

	return OCD_ERR_OK;
}

int ocd_unhalt_for_hwbrkpt(void __iomem *base, int dsp_id)
{
	u32 val;
	int ret;

	val = readl_relaxed(OCD_DSR(base));
	DSP_DBG("start, cur OCD_DSR=0x%x\n", val);

	val = OCD_SR_I_RFDO(OCD_SR_IMM4);
	ret = ocd_helper_exec(base, val);
	if (ret) {
		DSP_ERROR("exec error %d\n", ret);
		return ret;
	}
	usleep_range(500, 600);

	return ocd_helper_wait_ibreak(base, dsp_id, 100);
}

long dsp_set_brkpt(struct dsp_client_t *client, unsigned long arg)
{
	struct set_brkpt_t ioctl_param;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	void __iomem *base;
	int ret;
	u32 ar_save;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error;
	}

	if (copy_from_user(&ioctl_param, (struct set_brkpt_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error;
	}

	if (ioctl_param.dsp_id >= DSP_MAX || ioctl_param.dsp_id < DSP_Q6_0) {
		DSP_ERROR("Invalid DSP ID.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error;
	}

	DSP_DBG("BEGIN\n");

	/* OCD Interface not supported in simnow so skip setting breakpoint */
	dsp_resources->states->bypass_pd = helper_get_pdbypass(ioctl_param.dsp_id);
	if (dsp_resources->states->bypass_pd) {
		ret = 0;
		goto PD;
	}

	base = dsp_resources->states->ocd_register;
	ret = ocd_wait_ocddbgpower(base);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to wait ocddbgpower %d\n", ret);
		goto post_lock_error;
	}

	/*
	 * Save ar4 register, will restore at the end of SR register read
	 */
	ret = reg_get_ar(base, OCD_SR_AR, &ar_save);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to save ar4 %d\n", ret);
		goto post_lock_error;
	}

	ret = ocd_setup_ibreaka(base, ioctl_param.bindex, ioctl_param.dsp_addr);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to setup ibreaka %d\n", ret);
		goto restore_ar;
	}

	ret = ocd_enable_ibreak(base, dsp_resources->dsp_id,
				ioctl_param.bindex, 1);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to enable brkpt %d\n", ret);
		goto restore_ar;
	}

restore_ar:
	/*
	 * Restore ar4
	 */
	ret = reg_set_ar(base, OCD_SR_AR, ar_save);
	if (ret != OCD_ERR_OK)
		DSP_ERROR("failed to restore ar4 %d\n", ret);
PD:
	DSP_DBG("successful.\n");

post_lock_error:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error:
	return ret;
}

int dsp_wait_brkpt(struct dsp_client_t *client, unsigned long arg)
{
	struct set_brkpt_t ioctl_param;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	int ret;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error_wait;
	}

	if (copy_from_user(&ioctl_param, (struct set_brkpt_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error_wait;
	}

	if (ioctl_param.dsp_id >= DSP_MAX || ioctl_param.dsp_id < DSP_Q6_0) {
		DSP_ERROR("Invalid DSP ID.\n");
		ret = -EINVAL;
		goto error_wait;
	}

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error_wait;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error_wait;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error_wait;
		}
	}
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error_wait;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error_wait;
	}

	/* OCD Interface not supported in simnow so skip Waiting */
	dsp_resources->states->bypass_pd = helper_get_pdbypass(ioctl_param.dsp_id);
	if (dsp_resources->states->bypass_pd)
		goto PD;

	ret = ocd_helper_wait_ibreak(dsp_resources->states->ocd_register,
				     dsp_resources->dsp_id, 1000 * 1000);

	if (ret != OCD_ERR_OK) {
		DSP_ERROR("wait brkpt failed %d\n", ret);
		goto post_lock_error_wait;
	}
PD:
	DSP_DBG("breakpoint hit\n");

	mutex_unlock(&dsp_resources->dsp_resource_lock);
	DSP_DBG("successful.\n");
	return 0;

post_lock_error_wait:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error_wait:
	return ret;
}

int dsp_clr_brkpt(struct dsp_client_t *client, unsigned long arg)
{
	struct set_brkpt_t ioctl_param;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	void __iomem *base;
	int ret;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error_clr;
	}

	if (copy_from_user(&ioctl_param, (struct set_brkpt_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error_clr;
	}

	if (ioctl_param.dsp_id >= DSP_MAX || ioctl_param.dsp_id < DSP_Q6_0) {
		DSP_ERROR("Invalid DSP ID.\n");
		ret = -EINVAL;
		goto error_clr;
	}

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	DSP_ERROR("started.\n");
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error_clr;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error_clr;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error_clr;
		}
	}
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error_clr;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error_clr;
	}

	/* OCD Interface not supported in simnow so skip clearing breakpoint */
	dsp_resources->states->bypass_pd = helper_get_pdbypass(ioctl_param.dsp_id);
	if (dsp_resources->states->bypass_pd)
		goto PD;

	base = dsp_resources->states->ocd_register;
	ret = ocd_enable_ibreak(base, dsp_resources->dsp_id,
				ioctl_param.bindex, 0);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to disable ibreak %d\n", ret);
		goto post_lock_error_clr;
	}

PD:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
	DSP_DBG("successful.\n");
	return 0;

post_lock_error_clr:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error_clr:
	return ret;
}

static int reg_set_ar(void __iomem *base, int ar, unsigned int v)
{
	DSP_DBG("ar%d <-- %x\n", ar, v);
	return ocd_helper_load_sr2ar(base, v, OCD_SR_DDR, ar);
}

static int reg_get_ar(void __iomem *base, int ar, unsigned int *v)
{
	u32 val;
	int ret;

	DSP_DBG("ar%d\n", ar);
	ocd_helper_clear_dsr_status(base, OCD_DSR_EXECDONE | OCD_DSR_COREWROTEDDR);

	val = OCD_SR_I_WSR(OCD_SR_DDR, ar);
	writel_relaxed(val, OCD_DIR0EXEC(base));
	/* make sure ocd register write updated */
	wmb();

	DSP_DBG("wait exec done\n");
	ret = ocd_helper_wait_exec_set(base, 10);
	if (ret != OCD_ERR_OK)
		return ret;

	DSP_DBG("wait CoreWroteDDR\n");
	ret = ocd_helper_wait_dsr_set(base, OCD_DSR_COREWROTEDDR, 10);
	if (ret == OCD_ERR_OK)
		*v = readl_relaxed(OCD_DDR(base));

	return ret;
}

static int reg_set_sr(void __iomem *base, int sr, unsigned int v)
{
	u32 val, ar_save;
	int ret;

	DSP_DBG("sr%d <-- %x\n", sr, v);

	/*
	 * Save ar4 register, will restore at the end of SR register read
	 */
	ret = reg_get_ar(base, OCD_SR_AR, &ar_save);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to save ar4 %d\n", ret);
		return ret;
	}

	ret = ocd_helper_load_sr2ar(base, v, OCD_SR_DDR, OCD_SR_AR);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("load ddr error %d\n", ret);
		return ret;
	}

	DSP_DBG("wsr\n");
	val = OCD_SR_I_WSR(sr, OCD_SR_AR);
	ret = ocd_helper_exec(base, val);
	if (ret != OCD_ERR_OK)
		DSP_ERROR("exec error %d\n", ret);

	/*
	 * Restore ar4
	 */
	ret = reg_set_ar(base, OCD_SR_AR, ar_save);
	if (ret != OCD_ERR_OK)
		DSP_ERROR("failed to restore ar4 %d\n", ret);

	return OCD_ERR_OK;
}

static int reg_get_sr(void __iomem *base, int sr, unsigned int *v)
{
	u32 val, ar_save;
	int ret;

	DSP_DBG("read SR:%d\n", sr);

	/*
	 * Save ar4 register, will restore at the end of SR register read
	 */
	ret = reg_get_ar(base, OCD_SR_AR, &ar_save);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to save ar4 %d\n", ret);
		return ret;
	}

	/*
	 * Issue RSR ar4, sr
	 * write SR register value into ar4
	 */
	ocd_helper_clear_dsr_status(base, OCD_DSR_EXECDONE);

	val = OCD_SR_I_RSR(sr, OCD_SR_AR);
	writel_relaxed(val, OCD_DIR0EXEC(base));
	/* make sure ocd register write updated */
	wmb();

	DSP_DBG("wait rsr\n");
	ret = ocd_helper_wait_dsr_set(base, OCD_DSR_EXECDONE, 10);
	if (ret != OCD_ERR_OK)
		return ret;

	/*
	 * Issue WSR ar4, DDR
	 * Write ar4 register into DDR
	 */
	ocd_helper_clear_dsr_status(base, OCD_DSR_EXECDONE | OCD_DSR_COREWROTEDDR);

	val = OCD_SR_I_WSR(OCD_SR_DDR, OCD_SR_AR);
	writel_relaxed(val, OCD_DIR0EXEC(base));
	/* make sure ocd register write updated */
	wmb();

	DSP_DBG("wait wsr\n");
	ret = ocd_helper_wait_exec_set(base, 10);
	if (ret != OCD_ERR_OK)
		return ret;

	DSP_DBG("wait CoreWroteDDR\n");
	ret = ocd_helper_wait_dsr_set(base, OCD_DSR_COREWROTEDDR, 10);
	if (ret != OCD_ERR_OK)
		return ret;

	/*
	 * Read from DDR register to obtain the SR register value
	 */
	*v = readl_relaxed(OCD_DDR(base));
	/* wait for read complete before change the value of DDR register */
	mb();

	/*
	 * Restore ar4
	 */
	ret = reg_set_ar(base, OCD_SR_AR, ar_save);
	if (ret != OCD_ERR_OK)
		DSP_ERROR("failed to restore ar4 %d\n", ret);

	return OCD_ERR_OK;
}

static int dsp_set_reg(struct dsp_client_t *client, unsigned long arg)
{
	struct set_reg_t ioctl_param;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	void __iomem *base;
	int ret;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error;
	}

	if (copy_from_user(&ioctl_param, (struct set_reg_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error;
	}

	if (ioctl_param.dsp_id >= DSP_MAX || ioctl_param.dsp_id < DSP_Q6_0) {
		DSP_ERROR("Invalid DSP ID.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error;
	}

	DSP_DBG("BEGIN\n");

	/* OCD Interface not supported in simnow so skip setting reg */
	dsp_resources->states->bypass_pd = helper_get_pdbypass(ioctl_param.dsp_id);
	if (dsp_resources->states->bypass_pd)
		goto PD;

	base = dsp_resources->states->ocd_register;
	ret = ocd_wait_ocddbgpower(base);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to wait ocddbgpower %d\n", ret);
		goto post_lock_error;
	}

	switch (ioctl_param.reg_type) {
	case REG_AR:
		ret = reg_set_ar(base, ioctl_param.reg_idx, ioctl_param.val);
		break;
	case REG_SR:
	default:
		ret = -OCD_ERR_GENERAL;
	}

	if (ret != OCD_ERR_OK) {
		DSP_ERROR("set reg error %d\n", ret);
		goto post_lock_error;
	}
PD:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
	DSP_DBG("successful.\n");
	return 0;

post_lock_error:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error:
	return ret;
}

static int dsp_get_reg(struct dsp_client_t *client, unsigned long arg)
{
	struct get_reg_t ioctl_param;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	void __iomem *base;
	int ret;

	if (!arg) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EINVAL;
		goto error;
	}

	if (copy_from_user(&ioctl_param, (struct get_reg_t __user *)arg,
			   sizeof(ioctl_param))) {
		DSP_ERROR("Invalid IOCTL argument.\n");
		ret = -EFAULT;
		goto error;
	}

	if (ioctl_param.dsp_id >= DSP_MAX || ioctl_param.dsp_id < DSP_Q6_0) {
		DSP_ERROR("Invalid DSP ID.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == ioctl_param.dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error;
	}

	DSP_DBG("BEGIN\n");

	/* OCD Interface not supported in simnow so skip reading reg */
	dsp_resources->states->bypass_pd = helper_get_pdbypass(ioctl_param.dsp_id);
	if (dsp_resources->states->bypass_pd) {
		ioctl_param.val = 0x0;
		goto PD;
	}

	base = dsp_resources->states->ocd_register;
	ret = ocd_wait_ocddbgpower(base);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to wait ocddbgpower %d\n", ret);
		goto post_lock_error;
	}

	switch (ioctl_param.reg_type) {
	case REG_AR:
		ret = reg_get_ar(base, ioctl_param.reg_idx, &ioctl_param.val);
		break;
	case REG_SR:
		ret = reg_get_sr(base, ioctl_param.reg_idx, &ioctl_param.val);
		break;
	default:
		ret = -OCD_ERR_GENERAL;
	}

	if (ret != OCD_ERR_OK) {
		DSP_ERROR("get reg %s%d error %d\n",
			  (ioctl_param.reg_type == REG_AR) ? "ar" : "sr",
			  ioctl_param.reg_idx, ret);
		goto post_lock_error;
	}

PD:
	if (copy_to_user((struct get_reg_t __user *)arg, &ioctl_param,
			 sizeof(ioctl_param))) {
		DSP_ERROR("Unable to copy to user.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}

	mutex_unlock(&dsp_resources->dsp_resource_lock);
	DSP_DBG("successful. dsp-type-idx-val[%d-%d-%d-%x]\n",
		ioctl_param.dsp_id, ioctl_param.reg_type,
		ioctl_param.reg_idx, ioctl_param.val);
	return 0;

post_lock_error:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error:
	return ret;
}

int dsp_clr_excp(struct dsp_client_t *client, unsigned long arg)
{
	int dsp_id = arg;
	struct list_head *pos;
	struct dsp_resources_t *dsp_resources = NULL;
	void __iomem *base;
	int ret;

	if (dsp_id >= DSP_MAX || dsp_id < DSP_Q6_0) {
		DSP_ERROR("Invalid DSP ID.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp device.\n");
		goto error;
	}

	if (list_empty(&dsp_manager_dev->dsp_list)) {
		DSP_ERROR("DSP list is empty.\n");
		mutex_unlock(&dsp_manager_dev->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_resources = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resources->dsp_id == dsp_id)
			break;
		if (list_is_last(pos, &dsp_manager_dev->dsp_list)) {
			DSP_ERROR("DSP not registered.\n");
			mutex_unlock(&dsp_manager_dev->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resources->dsp_resource_lock);
	if (ret) {
		DSP_ERROR("Failed to lock dsp.\n");
		goto error;
	}

	if (is_dsp_inuse(dsp_resources, client)) {
		ret = -EBUSY;
		goto post_lock_error;
	}

	base = dsp_resources->states->ocd_register;
	ret = ocd_wait_ocddbgpower(base);
	if (ret != OCD_ERR_OK) {
		DSP_ERROR("failed to wait ocddbgpower %d\n", ret);
		goto post_lock_error;
	}

	writel_relaxed(OCD_DSR_EXECEXCEPTION, OCD_DSR(base));
	/* make sure ocd register write updated */
	wmb();

	mutex_unlock(&dsp_resources->dsp_resource_lock);
	DSP_DBG("successful\n");
	return 0;

post_lock_error:
	mutex_unlock(&dsp_resources->dsp_resource_lock);
error:
	return ret;
}

static long dsp_ioctl(struct file *file, unsigned int ioctl_num,
		      unsigned long arg)
{
	struct dsp_client_t *client;

	client = (struct dsp_client_t *)file->private_data;

	DSP_DBG("ioctl cmd=0x%x\n", ioctl_num);
	switch (ioctl_num) {
	case IOCTL_MEM_ALLOC:
		/* _IOWR
		 * Params: DSP_ID, MEM_POOL_ID, CACHE_ATTR, SECT_SIZE
		 * Return: ION_FD
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_MEM_ALLOC");
		return dsp_mem_alloc(client, arg);
	case IOCTL_MEM_DEALLOC:
		/* _IOWR
		 * Params: none
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_MEM_DEALLOC");
		return dsp_mem_dealloc(client);
	case IOCTL_MEM_MAP:
		/* _IOWR
		 * Params: ION_FD, DSP_ID, MEM_POOL_ID, CACHE_ATTR, SECT_ADDR, SECT_SIZE,
		 *	   ACCESS_ATTR
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_MEM_MAP");
		return dsp_mem_map(client, arg, false);
	case IOCTL_MEM_UNMAP:
		/* _IOWR
		 * Params: ION_FD, DSP_ID
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_MEM_UNMAP");
		return dsp_mem_unmap(client, arg);
	case IOCTL_DSP_SET_MODE:
		/* _IOWR
		 * Params: DSP_ID, DSP_STATE
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_DSP_SET_MODE");
		return dsp_set_mode(client, arg);
	case IOCTL_DSP_GET_STATE:
		/* _IOWR
		 * Params: DSP_ID
		 * Return: DSP_STATE
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_DSP_GET_STATE");
		return dsp_get_state(client, arg);
	case IOCTL_DSP_SET_BRKPT:
		/* _IOWR
		 * Params: DSP_ID, BREAKPOINT_ADDR
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_DSP_SET_BRKPT");
		return dsp_set_brkpt(client, arg);
	case IOCTL_DSP_CLR_BRKPT:
		/* _IOWR
		 * Params: DSP_ID, BREAKPOINT_ADDR
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_DSP_CLR_BRKPT");
		return dsp_clr_brkpt(client, arg);
	case IOCTL_DSP_WAIT_BRKPT:
		/* _IOWR
		 * Params: DSP_ID, BREAKPOINT_ADDR
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_DSP_WAIT_BRKPT");
		return dsp_wait_brkpt(client, arg);
	case IOCTL_DSP_SET_REG:
		/* _IOWR
		 * Params: DSP_ID, DSP_REG, REG_VALUE
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_DSP_SET_REG");
		return dsp_set_reg(client, arg);
	case IOCTL_DSP_GET_REG:
		/* _IOWR
		 * Params: DSP_ID, DSP_REG
		 * Return: REG_VALUE
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_DSP_GET_REG");
		return dsp_get_reg(client, arg);
	case IOCTL_DSP_CLR_EXCP:
		/* _IOW
		 * Params: DSP_ID
		 * Return: Pass or Fail
		 */
		CVIP_EVTLOG(dsp_manager_dev->event_log, "IOCTL_DSP_CLR_EXCP");
		return dsp_clr_excp(client, arg);
	default:
		DSP_ERROR("Invalid IOCTL command.\n");
		return -EINVAL;
	}
}

int dsp_suspend(void)
{
	int ret = 0;
	struct list_head *pos;
	struct dsp_resources_t *dsp_res;
	struct dsp_state_t *state;

	ret = mutex_lock_interruptible(&dsp_manager_dev->dsp_device_lock);
	if (ret) {
		DSP_ERROR("%s: Failed to lock!!\n", __func__);
		return -EFAULT;
	}

	if (list_empty(&dsp_manager_dev->dsp_list))
		goto suspend_exit;

	list_for_each(pos, &dsp_manager_dev->dsp_list) {
		dsp_res = list_entry(pos, struct dsp_resources_t, list);
		state = dsp_res->states;
		if (state->dsp_mode != DSP_OFF) {
			DSP_ERROR("DSP%d if not in OFF state,cannot suspend\n",
				  dsp_res->dsp_id);
			ret = -EINVAL;
		}
	}

suspend_exit:
	mutex_unlock(&dsp_manager_dev->dsp_device_lock);
	return ret;
}
EXPORT_SYMBOL(dsp_suspend);

int dsp_resume(void)
{
	return 0;
}
EXPORT_SYMBOL(dsp_resume);

MODULE_DESCRIPTION("A DSP Manager Daemon");

module_init(dsp_init);
module_exit(dsp_exit);

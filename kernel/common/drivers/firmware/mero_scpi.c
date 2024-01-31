/*
 * System Control and Power Interface (SCPI) Message Protocol driver
 *
 * SCPI Message Protocol is used between CVIP and SMU, SMU mailboxs are
 * used for inter-processor communication between CVIP and SMU.
 *
 * SCP offers control and management of the core/cluster power states,
 * various power domain DVFS including the core/cluster, certain system
 * clocks configuration, thermal sensors and many others.
 *
 * Copyright (C) 2010 ARM Ltd.
 * Modification Copyright (C) 2022, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/bitmap.h>
#include <linux/bitfield.h>
#include <linux/device.h>

#include <linux/debugfs.h>
#include <linux/fs.h>

#include <linux/err.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/printk.h>
#include <linux/pm_opp.h>
#include <linux/scpi_protocol.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/smu_protocol.h>
#include <linux/cvip_event_logger.h>
#include "arm_scpi.h"
#include "mero_scpi.h"
#include "mero_scpi_debugfs.h"
#include "ipc.h"
#include "mp_12_0_0_sh_mask.h"
#include "power.h"

#define CMD_ID_MASK		GENMASK(6, 0)
#define CMD_TOKEN_ID_MASK	GENMASK(15, 8)
#define CMD_DATA_SIZE_MASK	GENMASK(24, 16)
#define CMD_LEGACY_DATA_SIZE_MASK	GENMASK(28, 20)
#define PACK_SCPI_CMD(cmd_id, tx_sz)		\
	(FIELD_PREP(CMD_ID_MASK, cmd_id) |	\
	FIELD_PREP(CMD_DATA_SIZE_MASK, tx_sz))
#define PACK_LEGACY_SCPI_CMD(cmd_id, tx_sz)	\
	(FIELD_PREP(CMD_ID_MASK, cmd_id) |	\
	FIELD_PREP(CMD_LEGACY_DATA_SIZE_MASK, tx_sz))

#define CMD_SIZE(cmd)	FIELD_GET(CMD_DATA_SIZE_MASK, cmd)
#define CMD_UNIQ_MASK	(CMD_TOKEN_ID_MASK | CMD_ID_MASK)
#define CMD_XTRACT_UNIQ(cmd)	((cmd) & CMD_UNIQ_MASK)

#define SCPI_SLOT		0

#define MAX_DVFS_OPPS		4

#define PROTO_VERSION		0x10000
#define SMU_READY		0x59445253

#define MAX_RX_TIMEOUT		(msecs_to_jiffies(30))
#define SCPI_EVENT_LOG_SIZE	2048 /* max 2K log size */

static int scpi_std_commands[CMD_MAX_COUNT] = {
	SCPI_CMD_SCPI_CAPABILITIES,
	SCPI_CMD_GET_CLOCK_INFO,
	SCPI_CMD_GET_CLOCK_VALUE,
	SCPI_CMD_SET_CLOCK_VALUE,
	SCPI_CMD_GET_DVFS,
	SCPI_CMD_SET_DVFS,
	SCPI_CMD_GET_DVFS_INFO,
	SCPI_CMD_SENSOR_CAPABILITIES,
	SCPI_CMD_SENSOR_INFO,
	SCPI_CMD_SENSOR_VALUE,
	SCPI_CMD_SET_DEVICE_PWR_STATE,
	SCPI_CMD_GET_DEVICE_PWR_STATE,
};

DEFINE_MUTEX(clkreq_lock); /* to lock access to clkreq_list */

/*
 * The SCP firmware only executes in little-endian mode, so any buffers
 * shared through SCPI should have their contents converted to little-endian
 */
struct scpi_shared_mem {
	__le32 command;
	__le32 status;
	u8 payload[0];
} __packed;

struct legacy_scpi_shared_mem {
	__le32 status;
	u8 payload[0];
} __packed;

struct scp_capabilities {
	__le32 protocol_version;
	__le32 event_version;
	__le32 platform_version;
	__le32 commands[4];
} __packed;

struct clk_get_info {
	__le16 id;
	__le16 flags;
	__le32 min_rate;
	__le32 max_rate;
	u8 name[20];
} __packed;

struct clk_set_value {
	__le16 id;
	__le16 reserved;
	__le32 rate;
} __packed;

struct legacy_clk_set_value {
	__le32 rate;
	__le16 id;
	__le16 reserved;
} __packed;

struct dvfs_info {
	u8 domain;
	u8 opp_count;
	__le16 latency;
	struct {
		__le32 freq;
		__le32 m_volt;
	} opps[MAX_DVFS_OPPS];
} __packed;

struct dvfs_set {
	u8 domain;
	u8 index;
} __packed;

struct _scpi_sensor_info {
	__le16 sensor_id;
	u8 class;
	u8 trigger_type;
	char name[20];
};

struct dev_pstate_set {
	__le16 dev_id;
	u8 pstate;
} __packed;

static struct scpi_drvinfo *scpi_info;

static int scpi_linux_errmap[SCPI_ERR_MAX] = {
	/* better than switch case as long as return value is continuous */
	0, /* SCPI_SUCCESS */
	-EINVAL, /* SCPI_ERR_PARAM */
	-ENOEXEC, /* SCPI_ERR_ALIGN */
	-EMSGSIZE, /* SCPI_ERR_SIZE */
	-EINVAL, /* SCPI_ERR_HANDLER */
	-EACCES, /* SCPI_ERR_ACCESS */
	-ERANGE, /* SCPI_ERR_RANGE */
	-ETIMEDOUT, /* SCPI_ERR_TIMEOUT */
	-ENOMEM, /* SCPI_ERR_NOMEM */
	-EINVAL, /* SCPI_ERR_PWRSTATE */
	-EOPNOTSUPP, /* SCPI_ERR_SUPPORT */
	-EIO, /* SCPI_ERR_DEVICE */
	-EBUSY, /* SCPI_ERR_BUSY */
};

static inline int scpi_to_linux_errno(int errno)
{
	if (errno >= SCPI_SUCCESS && errno < SCPI_ERR_MAX)
		return scpi_linux_errmap[errno];
	return -EIO;
}

static void smu_apply_clkreq_nolock(void)
{
	struct scpi_clkreq *clkreq, *tmp;
	struct smu_msg msg;

	if (!scpi_info || list_empty(&scpi_info->clkreq_list))
		return;

	list_for_each_entry_safe_reverse(clkreq, tmp, &scpi_info->clkreq_list, list) {
		CVIP_EVTLOG(scpi_info->evtlogid, "smu_send_single_msg",
			    clkreq->mid, clkreq->arg);
		smu_msg(&msg, clkreq->mid, clkreq->arg);
		if (unlikely(!smu_send_single_msg(&msg)))
			pr_err("send smu msg failed on CLK%d\n", clkreq->mid);
		else
			pr_debug("CLK %d set as %d Mhz\n", clkreq->mid, msg.resp);

		list_del(&clkreq->list);
		kfree(clkreq);
	}
}

static int smu_dpm_feature_enabled;

int is_smu_dpm_ready(void)
{
	int smu_ready;

	mutex_lock(&clkreq_lock);
	smu_ready = smu_dpm_feature_enabled;
	if (smu_ready && scpi_info)
		writel_relaxed(SMU_READY, scpi_info->scpi_base);
	mutex_unlock(&clkreq_lock);

	return smu_ready;
}
EXPORT_SYMBOL(is_smu_dpm_ready);

void set_smu_dpm_ready(int state)
{
	mutex_lock(&clkreq_lock);

	smu_dpm_feature_enabled = (state ? 1 : 0);
	if (state) {
		if (scpi_info)
			writel_relaxed(SMU_READY, scpi_info->scpi_base);
		smu_apply_clkreq_nolock();
	}

	mutex_unlock(&clkreq_lock);

}
EXPORT_SYMBOL(set_smu_dpm_ready);

void set_smu_dpm_throttle(u32 event, u32 type)
{
	if (scpi_info)
		CVIP_EVTLOG(scpi_info->evtlogid, "smu DPM throttle", event, type);
}
EXPORT_SYMBOL(set_smu_dpm_throttle);

static void scpi_init_smuready(void)
{
	int smu_ready;

	smu_ready = readl_relaxed(scpi_info->scpi_base);
	if (smu_ready == SMU_READY)
		set_smu_dpm_ready(1);
}

static int clkid_msgid[MAX_CVIPCLKS] = {
	CVSMC_MSG_setsoftmina55clk,
	CVSMC_MSG_setsoftmina55twoclk,
	CVSMC_MSG_setsoftminc5clk,
	CVSMC_MSG_setsoftminq6clk,
	CVSMC_MSG_setsoftminnicclk,
	CVSMC_MSG_setcoresightclk,
	CVSMC_MSG_setdecmpressclk,
};

static enum smu_clk_id cvip_clk_id[MAX_CVIPCLKS] = {
	A55DSUCLK,
	A55TWOCLK,
	C5CLK_ID,
	Q6CLK_ID,
	NIC400CLK_ID,
	CORESIGHTCLK_ID,
	DECOMPRESSCLK_ID,
};

static const unsigned long vclk_dpm0[] = {400000000, 600000000, 400000000,
					 600000000, 400000000, 100000000,
					 200000000};

static int scpi_send_message(u8 idx, void *tx_buf, unsigned int tx_len,
			     void *rx_buf, unsigned int rx_len)
{
	int ret = SCPI_SUCCESS;
	u8 cmd;
	u32 mid, arg, resp;
	struct clk_set_value *clk;
	struct smu_msg msg;
	struct scpi_clkreq *clkreq;

	if (scpi_info->commands[idx] < 0)
		return -EOPNOTSUPP;

	cmd = scpi_info->commands[idx];

	/* put the following logic to a separated function */
	switch (cmd) {
	case SCPI_CMD_SET_CLOCK_VALUE:
		clk = (struct clk_set_value *)tx_buf;
		/* set the smu message id based on clk type */
		if (clk->id >= MAX_CVIPCLKS) {
			ret = SCPI_ERR_PARAM;
			goto scpi_ret;
		}
		mid = clkid_msgid[clk->id];
		if (clk->id == DECOMPRESSCLK) {
			if (clk->rate == CLOCK_OFF) {
				arg = CLOCK_OFF;
			} else if ((clk->rate / SCPI_1MHZ) ==
				  DECOMPRESS_FREQ_DPM0) {
				arg = DECOMPRESS_DPM0;
			} else if ((clk->rate / SCPI_1MHZ) ==
				  DECOMPRESS_FREQ_DPM1) {
				arg = DECOMPRESS_DPM1;
			} else {
				pr_err("Argument not supported\n");
				goto scpi_ret;
			}
		} else {
			arg = clk->rate / SCPI_1MHZ;
		}
		CVIP_EVTLOG(scpi_info->evtlogid, "SET_CLOCK_VALUE", mid, arg);

		if (!is_smu_dpm_ready()) {
			pr_debug("CLK = %d, freq is %d\n", mid, arg);
			ret = SCPI_SUCCESS;
		}
		break;
	case SCPI_CMD_GET_CLOCK_VALUE:
		arg = *((__le16 *)tx_buf);
		mid = CVSMC_MSG_getcurrentfreq;

		if (!is_smu_dpm_ready()) {
			resp = vclk_dpm0[arg];
			*(u32 *)rx_buf = resp / SCPI_1MHZ;
			pr_debug("CLK%d is %d Mhz\n", arg, *(u32 *)rx_buf);
		}

		if (arg >= MAX_CVIPCLKS) {
			ret = SCPI_ERR_PARAM;
			goto scpi_ret;
		}
		/* fixed rate for Coresight and decompress clks */
		if (arg == CORESIGHTCLK) {
			*(u32 *)rx_buf = CORESIGHTCLK_RATE;
			goto scpi_ret;
		}
		arg = cvip_clk_id[arg];
		CVIP_EVTLOG(scpi_info->evtlogid, "GET_CLOCK_VALUE", arg);
		break;
	default:
		pr_warn("unsupported scpi command received\n");
		ret = SCPI_ERR_PARAM;
		goto scpi_ret;
	};

	if (is_smu_dpm_ready()) {
		smu_msg(&msg, mid, arg);
		if (unlikely(!smu_send_single_msg(&msg))) {
			pr_err("%s: send smu msg failed\n", __func__);
			ret = SCPI_ERR_DEVICE;
			CVIP_EVTLOG(scpi_info->evtlogid, "smu_send_single_msg", 0xdead);
			goto scpi_ret;
		}
		CVIP_EVTLOG(scpi_info->evtlogid, "smu_send_single_msg done", mid, arg, msg.resp);

		switch (cmd) {
		case SCPI_CMD_SET_CLOCK_VALUE:
			/*SCPI_CMD_SET_CLOCK_VALUE does not care
			 * about the actual clock being set, put it
			 * into debug message
			 */
			pr_debug("CLK %d set as %d Mhz\n", mid, msg.resp);
			break;
		case SCPI_CMD_GET_CLOCK_VALUE:
			*(u32 *)rx_buf = msg.resp;
			pr_debug("CLK is %d Mhz\n", msg.resp);
			break;
		default:
			pr_warn("unsupported scpi command\n");
			ret = SCPI_ERR_PARAM;
		};
	} else {
		/* add SCPI_CMD_SET_CLOCK_VALUE to clkreq list */
		if (cmd == SCPI_CMD_SET_CLOCK_VALUE) {
			clkreq = kzalloc(sizeof(*clkreq), GFP_KERNEL);
			if (!clkreq)
				goto scpi_ret;

			clkreq->mid = mid;
			clkreq->arg = arg;
			mutex_lock(&clkreq_lock);
			list_add(&clkreq->list, &scpi_info->clkreq_list);
			mutex_unlock(&clkreq_lock);
			pr_debug("Store CLKREQ for CLK%d, freq=%d\n", mid, arg);
		}
	}

scpi_ret:
	/* SCPI error codes > 0, translate them to Linux scale */
	return ret > 0 ? scpi_to_linux_errno(ret) : ret;
}

static u32 scpi_get_version(void)
{
	return PROTO_VERSION;
}

static const unsigned long vclk_min[] = {0, 0, 0, 0, 0, 0, 0};
static const unsigned long vclk_max[] = {1133000000, 1700000000, 971000000,
					 1236000000, 850000000, 100000000,
					 600000000};

static int
scpi_clk_get_range(u16 clk_id, unsigned long *min, unsigned long *max)
{
	/* this is the interface for variable clks
	 * clk_id 0: A55DSUclk
	 *	  1: A55TwoClk
	 *	  2: c5clk
	 *	  3: q6clk
	 *	  4: nicclk
	 *	  5: coresightclk
	 *	  6: decompressclk
	 */
	if (clk_id >= ARRAY_SIZE(vclk_min)) {
		pr_err("unsupported scpi clk id = %d\n", clk_id);
		return -EINVAL;
	}
	*min = vclk_min[clk_id];
	*max = vclk_max[clk_id];
	return 0;
}

static unsigned long scpi_clk_get_val(u16 clk_id)
{
	int ret;
	__le32 rate;
	__le16 le_clk_id = cpu_to_le16(clk_id);

	ret = scpi_send_message(CMD_GET_CLOCK_VALUE, &le_clk_id,
				sizeof(le_clk_id), &rate, sizeof(rate));

	return ret ? ret : le32_to_cpu(rate);
}

static int scpi_clk_set_val(u16 clk_id, unsigned long rate)
{
	int stat;
	struct clk_set_value clk = {
		.id = cpu_to_le16(clk_id),
		.rate = cpu_to_le32(rate)
	};

	pr_debug("CLK id:%d is set %d Mhz\n", clk_id, (int)(rate / SCPI_1MHZ));

	return scpi_send_message(CMD_SET_CLOCK_VALUE, &clk, sizeof(clk),
				 &stat, sizeof(stat));
}

static int scpi_dvfs_get_idx(u8 domain)
{
	return scpi_info->dvfs_index[domain];
}

static int scpi_dvfs_set_idx(u8 domain, u8 index)
{
	struct clk_set_value clk;
	int stat;
	struct scpi_opp *opp;
	int ret = SCPI_SUCCESS;

	if (index != scpi_info->dvfs_index[domain]) {
		clk.id = domain;
		opp = scpi_info->dvfs[domain]->opps + index;
		clk.rate = opp->freq;
		ret = scpi_send_message(CMD_SET_CLOCK_VALUE, &clk, sizeof(clk),
				 &stat, sizeof(stat));
		if (ret == SCPI_SUCCESS)
			scpi_info->dvfs_index[domain] = index;
	}
	return ret;
}

static struct scpi_dvfs_info *scpi_dvfs_get_info(u8 domain)
{
	if (domain >= MAX_DVFS_DOMAINS)
		return ERR_PTR(-EINVAL);

	if (scpi_info->dvfs[domain])	/* data already populated */
		return scpi_info->dvfs[domain];
	else
		return ERR_PTR(-EINVAL);
}

static int scpi_dev_domain_id(struct device *dev)
{
	struct of_phandle_args clkspec;

	if (of_parse_phandle_with_args(dev->of_node, "clocks", "#clock-cells",
				       0, &clkspec))
		return -EINVAL;

	return clkspec.args[0];
}

static struct scpi_dvfs_info *scpi_dvfs_info(struct device *dev)
{
	int domain = scpi_dev_domain_id(dev);

	if (domain < 0)
		return ERR_PTR(domain);

	return scpi_dvfs_get_info(domain);
}

static int scpi_dvfs_get_transition_latency(struct device *dev)
{
	struct scpi_dvfs_info *info = scpi_dvfs_info(dev);

	if (IS_ERR(info))
		return PTR_ERR(info);

	return info->latency;
}

static int scpi_dvfs_add_opps_to_device(struct device *dev)
{
	int idx, ret;
	struct scpi_opp *opp;
	struct scpi_dvfs_info *info = scpi_dvfs_info(dev);

	if (IS_ERR(info))
		return PTR_ERR(info);

	if (!info->opps)
		return -EIO;

	for (opp = info->opps, idx = 0; idx < info->count; idx++, opp++) {
		ret = dev_pm_opp_add(dev, opp->freq, opp->m_volt * 1000);
		if (ret) {
			dev_warn(dev, "failed to add opp %uHz %umV\n",
				 opp->freq, opp->m_volt);
			while (idx-- > 0)
				dev_pm_opp_remove(dev, (--opp)->freq);
			return ret;
		}
	}
	return 0;
}

static int scpi_sensor_get_capability(u16 *sensors)
{
	/* TODO: to be implemented */
	return 0;
}

static int scpi_sensor_get_info(u16 sensor_id, struct scpi_sensor_info *info)
{
	/* TODO: to be implemented */
	return 0;
}

static int scpi_sensor_get_value(u16 sensor, u64 *val)
{
	/* TODO: to be implemented */
	return 0;
}

static int scpi_device_get_power_state(u16 dev_id)
{
	/* TODO: to be implemented */
	return 0;
}

static int scpi_device_set_power_state(u16 dev_id, u8 pstate)
{
	/* TODO: to be implemented */
	return 0;
}

static struct scpi_ops scpi_ops = {
	.get_version = scpi_get_version,
	.clk_get_range = scpi_clk_get_range,
	.clk_get_val = scpi_clk_get_val,
	.clk_set_val = scpi_clk_set_val,
	.dvfs_get_idx = scpi_dvfs_get_idx,
	.dvfs_set_idx = scpi_dvfs_set_idx,
	.dvfs_get_info = scpi_dvfs_get_info,
	.device_domain_id = scpi_dev_domain_id,
	.get_transition_latency = scpi_dvfs_get_transition_latency,
	.add_opps_to_device = scpi_dvfs_add_opps_to_device,
	.sensor_get_capability = scpi_sensor_get_capability,
	.sensor_get_info = scpi_sensor_get_info,
	.sensor_get_value = scpi_sensor_get_value,
	.device_get_power_state = scpi_device_get_power_state,
	.device_set_power_state = scpi_device_set_power_state,
};

struct scpi_ops *get_scpi_ops(void)
{
	return scpi_info ? scpi_info->scpi_ops : NULL;
}
EXPORT_SYMBOL_GPL(get_scpi_ops);

static int scpi_remove(struct platform_device *pdev)
{
	int i;
	struct scpi_drvinfo *info = platform_get_drvdata(pdev);

	scpi_info = NULL; /* stop exporting SCPI ops through get_scpi_ops */

	for (i = 0; i < MAX_DVFS_DOMAINS && info->dvfs[i]; i++) {
		kfree(info->dvfs[i]->opps);
		kfree(info->dvfs[i]);
	}
	mero_scpi_debugfs_exit();
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int scpi_suspend(struct device *dev)
{
	dev_dbg(dev, "Suspending scpi device\n");
	if (pm_test_level == TEST_NONE)
		set_smu_dpm_ready(0);
	return 0;
}

static int scpi_resume(struct device *dev)
{
	dev_dbg(dev, "Resuming scpi device\n");
	if (pm_test_level == TEST_NONE)
		smu_boot_handshake();
	return 0;
}

static const struct dev_pm_ops scpi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(scpi_suspend, scpi_resume)
};

#define SCPI_PM_OPS	(&scpi_pm_ops)
#else
#define SCPI_PM_OPS	NULL
#endif

const char *dvfs_compatible = "arm,scpi-dvfs-clocks";
const char *clk_names = "clock-output-names";

static int scpi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np, *clk_np, *dvfs_np, *dvfs, *opp_np;
	int count, idx;
	int val;
	const char *name;
	unsigned int latency, mvolt, opp_count;
	u64 rate;
	struct scpi_opp *opp;
	struct scpi_dvfs_info *info;
	struct resource *res;
	int ret;

	scpi_info = devm_kzalloc(dev, sizeof(*scpi_info), GFP_KERNEL);
	if (!scpi_info)
		return -ENOMEM;

	INIT_LIST_HEAD(&scpi_info->clkreq_list);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Error in get SMU aperture\n");
		return -EINVAL;
	}

	scpi_info->scpi_base = devm_ioremap(dev, res->start, resource_size(res));
	if (!scpi_info->scpi_base) {
		dev_err(dev, "ioremap failed\n");
		return -EADDRNOTAVAIL;
	}

	scpi_init_smuready();

	scpi_info->commands = scpi_std_commands;

	/* build opp table */
	np = dev->of_node;
	scpi_info->np = np;

	/* find the clocks node */
	clk_np = of_get_child_by_name(np, "clocks");
	if (!clk_np)
		goto opp_skip;
	for_each_child_of_node(clk_np, dvfs_np) {
		if (of_device_is_compatible(dvfs_np, dvfs_compatible)) {
			/* get the dvfs node */
			count = of_property_count_strings(dvfs_np, clk_names);
			if (count < 0) {
				dev_err(dev, "%s: invalid clock output count\n",
					np->name);
				return -EINVAL;
			}
			for (idx = 0; idx < count; idx++) {
				if (of_property_read_string_index(dvfs_np,
					clk_names, idx, &name)) {
					dev_err(dev, "invalid clock name @ %s\n",
						dvfs_np->name);
					return -EINVAL;
				}

				if (of_property_read_u32_index(dvfs_np,
					"clock-indices", idx, &val)) {
					dev_err(dev, "invalid clock index @ %s\n",
						dvfs_np->name);
					return -EINVAL;
				}
				/* dvfs domain is the val */
				/* populate the opp values */
				dvfs = of_parse_phandle(dvfs_np,
						"operating-points-v2", 0);
				opp_count = of_get_child_count(dvfs);
				info = kmalloc(sizeof(*info), GFP_KERNEL);
				if (!info)
					return -ENOMEM;

				info->count = opp_count;
				info->opps = kcalloc(info->count, sizeof(*opp),
						GFP_KERNEL);
				if (!info->opps) {
					kfree(info);
					return -ENOMEM;
				}
				opp = info->opps;
				for_each_available_child_of_node(dvfs, opp_np) {
					of_property_read_u32(opp_np,
						"clock-latency-ns", &latency);
					of_property_read_u32_index(opp_np,
						"opp-microvolt", 0, &mvolt);
					of_property_read_u64(opp_np, "opp-hz",
						 &rate);
					opp->freq = (u32)rate;
					opp->m_volt = mvolt;
					opp++;
				}
				info->latency = latency;
				scpi_info->dvfs[val] = info;
			}
		}
	}

opp_skip:
	platform_set_drvdata(pdev, scpi_info);

	scpi_info->scpi_ops = &scpi_ops;
	ret = mero_scpi_debugfs_init(scpi_info);
	if (ret < 0) {
		pr_err("Failed debugfs init function\n");
		return -EINVAL;
	}

	scpi_info->evtlogid = CVIP_EVTLOGINIT(SCPI_EVENT_LOG_SIZE);

	return devm_of_platform_populate(dev);
}

static const struct of_device_id scpi_of_match[] = {
	{.compatible = "amd,cvip-scpi"},
	{},
};

MODULE_DEVICE_TABLE(of, scpi_of_match);

static struct platform_driver scpi_driver = {
	.driver = {
		.name = "scpi_protocol",
		.of_match_table = scpi_of_match,
		.pm = SCPI_PM_OPS,
	},
	.probe = scpi_probe,
	.remove = scpi_remove,
};
module_platform_driver(scpi_driver);

MODULE_DESCRIPTION("Mero SCPI protocol driver");

// SPDX-License-Identifier: BSD-3-Clause-Clear
/**
 * Copyright (c) 2010 The Linux Foundation. All rights reserved.
 */

#include <linux/devcoredump.h>
#include <linux/dma-direction.h>
#include <linux/mhi.h>
#include "coredump.h"
#include "pci.h"
#include "debug.h"

static size_t cnss_get_remote_buf_len(struct fw_remote_mem *fw_mem)
{
	unsigned int i;
	size_t len = 0;

	for (i = 0; i < BHI_WLFW_MAX_NUM_MEM_SEG_V01; i++) {
		if (fw_mem[i].vaddr && fw_mem[i].size)
			len += fw_mem[i].size;
	}

	return len;
}

static int cnss_coredump_remote_dump(struct cnss_plat_data *plat_priv)
{
	struct fw_remote_crash_data *crash_data = &plat_priv->remote_crash_data;
	struct fw_remote_mem *fw_mem = plat_priv->remote_mem;
	u8 i;

	crash_data->remote_buf_len = cnss_get_remote_buf_len(fw_mem);
	cnss_pr_err("%s remote buffer len=%lu\n", __func__,
		    crash_data->remote_buf_len);

	crash_data->remote_buf = vzalloc(crash_data->remote_buf_len);
	if (!crash_data->remote_buf)
		return -ENOMEM;

	for (i = 0; i < BHI_WLFW_MAX_NUM_MEM_SEG_V01; i++) {
		if (fw_mem[i].vaddr && fw_mem[i].size) {
			cnss_pr_err("remote mem: 0x%p, size: 0x%lx\n",
				    fw_mem[i].vaddr,
				    fw_mem[i].size);
			memcpy(crash_data->remote_buf + i * fw_mem[i].size,
			       fw_mem[i].vaddr, fw_mem[i].size);
		}
	}
	return 0;
}

static int cnss_coredump_fw_rddm_dump(struct mhi_controller *mhi_cntrl)
{
	struct mhi_fw_crash_data *crash_data = &mhi_cntrl->fw_crash_data;
	struct image_info *img = mhi_cntrl->rddm_image;
	char *buf = NULL;
	unsigned int size = 0;
	int seg = 0;

	crash_data->ramdump_buf = vzalloc(crash_data->ramdump_buf_len);
	if (!crash_data->ramdump_buf)
		return -ENOMEM;

	for (seg = 0; seg < mhi_cntrl->rddm_vec_entry_num; seg++) {
		buf = img->mhi_buf[seg].buf;
		size = img->mhi_buf[seg].len;
		cnss_pr_err(
			    "write rddm memory: mem: 0x%p, size: 0x%x\n",
			    buf, size);
		memcpy(crash_data->ramdump_buf + seg * size, buf, size);
	}

	return 0;
}

static int cnss_coredump_fw_paging_dump(struct mhi_controller *mhi_cntrl)
{
	struct image_info *img = mhi_cntrl->fbc_image;
	struct mhi_fw_crash_data *crash_data = &mhi_cntrl->fw_crash_data;
	char *buf = NULL;
	unsigned int size = 0;
	int seg = 0;

	cnss_pr_dbg("fw_vec_entry_num=%d entries=%d\n",
		    mhi_cntrl->fw_vec_entry_num, img->entries);

	crash_data->fwdump_buf = vzalloc(crash_data->fwdump_buf_len);
	if (!crash_data->fwdump_buf)
		return -ENOMEM;

	for (seg = 0; seg < mhi_cntrl->fw_vec_entry_num; seg++) {
		buf = img->mhi_buf[seg].buf;
		size = img->mhi_buf[seg].len;
		memcpy(crash_data->fwdump_buf + seg * size, buf, size);
	}

	buf = crash_data->fwdump_buf + seg * size;
	size = img->mhi_buf[img->entries - 1].len;
	cnss_pr_info("to write last block: mem: 0x%p, size: 0x%x\n",
		    buf, size);
	memcpy(buf, img->mhi_buf[img->entries - 1].buf, size);

	return 0;
}

static struct cnss_dump_file_data *
cnss_coredump_build(struct mhi_fw_crash_data *crash_data,
		      struct fw_remote_crash_data *remote_crash_data)
{
	struct cnss_dump_file_data *dump_data;
	struct cnss_tlv_dump_data *dump_tlv;
	size_t hdr_len = sizeof(*dump_data);
	size_t len, sofar = 0;
	unsigned char *buf;
	struct timespec64 timestamp;

	len = hdr_len;

	len += sizeof(*dump_tlv) + crash_data->fwdump_buf_len;
	len += sizeof(*dump_tlv) + crash_data->ramdump_buf_len;
	len += sizeof(*dump_tlv) + remote_crash_data->remote_buf_len;

	sofar += hdr_len;

	/* This is going to get big when we start dumping FW RAM and such,
	 * so go ahead and use vmalloc.
	 */
	buf = vzalloc(len);
	if (!buf)
		return NULL;

	dump_data = (struct cnss_dump_file_data *)(buf);
	strlcpy(dump_data->df_magic, "ATH11K-FW-DUMP",
		sizeof(dump_data->df_magic));
	dump_data->len = cpu_to_le32(len);
	dump_data->version = cpu_to_le32(CNSS_FW_CRASH_DUMP_VERSION);
	guid_gen(&dump_data->guid);
	ktime_get_real_ts64(&timestamp);
	dump_data->tv_sec = cpu_to_le64(timestamp.tv_sec);
	dump_data->tv_nsec = cpu_to_le64(timestamp.tv_nsec);

	/* Gather FW paging dump */
	dump_tlv = (struct cnss_tlv_dump_data *)(buf + sofar);
	dump_tlv->type = cpu_to_le32(CNSS_FW_CRASH_PAGING_DATA);
	dump_tlv->tlv_len = cpu_to_le32(crash_data->fwdump_buf_len);
	memcpy(dump_tlv->tlv_data, crash_data->fwdump_buf,
	       crash_data->fwdump_buf_len);
	sofar += sizeof(*dump_tlv) + crash_data->fwdump_buf_len;

	/* Gather RDDM dump */
	dump_tlv = (struct cnss_tlv_dump_data *)(buf + sofar);
	dump_tlv->type = cpu_to_le32(CNSS_FW_CRASH_RDDM_DATA);
	dump_tlv->tlv_len = cpu_to_le32(crash_data->ramdump_buf_len);
	memcpy(dump_tlv->tlv_data, crash_data->ramdump_buf,
	       crash_data->ramdump_buf_len);
	sofar += sizeof(*dump_tlv) + crash_data->ramdump_buf_len;

	/* gather remote memory */
	dump_tlv = (struct cnss_tlv_dump_data *)(buf + sofar);
	dump_tlv->type = cpu_to_le32(CNSS_FW_REMOTE_MEM_DATA);
	dump_tlv->tlv_len = cpu_to_le32(remote_crash_data->remote_buf_len);
	memcpy(dump_tlv->tlv_data, remote_crash_data->remote_buf,
	       remote_crash_data->remote_buf_len);
	sofar += sizeof(*dump_tlv) + remote_crash_data->remote_buf_len;

	return dump_data;
}

static int cnss_coredump_submit(struct cnss_pci_data *pci_priv)
{
	struct cnss_dump_file_data *dump;

	dump = cnss_coredump_build(&pci_priv->mhi_ctrl->fw_crash_data,
				     &pci_priv->plat_priv->remote_crash_data);
	if (!dump)
		return -ENODATA;

	dev_coredumpv(pci_priv->mhi_ctrl->cntrl_dev, dump,
		      le32_to_cpu(dump->len), GFP_KERNEL);

	return 0;
}

static void cnss_coredump_buf_release(struct cnss_pci_data *pci_priv)
{
	struct fw_remote_crash_data *remote = &pci_priv->plat_priv->remote_crash_data;
	struct mhi_fw_crash_data *mhi = &pci_priv->mhi_ctrl->fw_crash_data;

	if (remote->remote_buf) {
		vfree(remote->remote_buf);
		remote->remote_buf = NULL;
	}

	if (mhi->ramdump_buf) {
		vfree(mhi->ramdump_buf);
		mhi->ramdump_buf = NULL;
	}

	if (mhi->fwdump_buf) {
		vfree(mhi->fwdump_buf);
		mhi->fwdump_buf = NULL;
	}
}

void cnss_mhi_pm_rddm_worker(struct work_struct *work)
{
	struct cnss_pci_data *pci_priv = container_of(work,
						 struct cnss_pci_data,
						 rddm_worker);

	mhi_download_rddm_img(pci_priv->mhi_ctrl, false);

	cnss_coredump_fw_rddm_dump(pci_priv->mhi_ctrl);
	cnss_coredump_fw_paging_dump(pci_priv->mhi_ctrl);
	cnss_coredump_remote_dump(pci_priv->plat_priv);

	cnss_coredump_submit(pci_priv);
	cnss_coredump_buf_release(pci_priv);
}
EXPORT_SYMBOL(cnss_mhi_pm_rddm_worker);

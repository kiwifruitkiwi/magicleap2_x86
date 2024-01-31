/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#if	!defined(ISP3_SILICON)
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include "isp_module_cfg.h"
#include "isp_common.h"
#include "isp_module_if.h"
#include "isp_module_if_imp.h"
#include "hal.h"
#include "log.h"
#include "i2c.h"
#include "isp_fw_if.h"
#include "isp_fpga.h"
#define AMD_VENDOR_ID 0x10ee
#define AMD_DEVICE_ID 0x0505
#define AMD_PCI_NAME "FPGAPCI"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][fpga_isp_module]"

struct device_private {
	struct pci_dev *amd_pdev;
	unsigned long ddrstart;
	unsigned long ddrlen;
	void __iomem *ddraddr;

	unsigned long regstart;
	unsigned long reglen;
	void __iomem *regaddr;

	int irq;

	/* struct isp_context ispContext; */

};
#define TOTAL_DDR_MEM_SIZE (512 * 1024 * 1024)
struct device_private fpga_pci_private;

int intRet = -1, probeRet = -1;


/********common implementation****/

uint64_t fpga_reg_base;
uint64_t fpga_mem_base;
struct mutex fpga_reg_access_mutex;
struct mutex fpga_mem_access_mutex;

struct cgs_device fpga_cgs_dev;
struct cgs_ops fpga_cgs_ops;




/********************************/
#ifndef os_read_reg32
#define os_read_reg32(address) (*((uint32_t *)address))
#define os_write_reg32(addr, val) (*((uint32_t *)addr) = val)
#endif

uint32_t fpga_read_reg32(uint64_t reg_addr)
{
	uint32_t *addr = (uint32_t *) reg_addr;

	return os_read_reg32(addr);
};

void fpga_write_reg32(uint64_t reg_addr, uint32_t value)
{
	uint32_t *addr = (uint32_t *) reg_addr;

	os_write_reg32(addr, value);
}
static struct isp_mc_addr_mgr l_isp_mc_addr_mgr;
void *isp_mc_addr_mgr_init(unsigned long long start,
	unsigned long long len)
{
	struct isp_mc_addr_node *buffer;

	memset(&l_isp_mc_addr_mgr, 0, sizeof(struct isp_mc_addr_mgr));
	isp_mutex_init(&l_isp_mc_addr_mgr.mutext);
	l_isp_mc_addr_mgr.start = start;
	l_isp_mc_addr_mgr.len = len;
	l_isp_mc_addr_mgr.head.start_addr = start - 1;
	l_isp_mc_addr_mgr.head.align_addr = start - 1;
	l_isp_mc_addr_mgr.head.end_addr = start - 1;
	l_isp_mc_addr_mgr.head.size = 0;
	l_isp_mc_addr_mgr.head.prev = NULL;
	l_isp_mc_addr_mgr.head.next = NULL;

	buffer = (struct isp_mc_addr_node  *)
		isp_sys_mem_alloc(sizeof(struct isp_mc_addr_node));
	if (buffer == NULL) {
		ISP_PR_ERR("fail for aloc buf\n");
		return NULL;
	}

	buffer->start_addr = start + len;
	buffer->align_addr = start + len;
	buffer->end_addr = start + len;
	buffer->size = 0;
	buffer->prev = &l_isp_mc_addr_mgr.head;
	buffer->next = NULL;
	l_isp_mc_addr_mgr.head.next = buffer;
	ISP_PR_INFO("suc\n");
	return &l_isp_mc_addr_mgr;
};

void isp_mc_addr_mgr_get_base_len(void *handle,
	unsigned long long *base_addr, unsigned long long *len)
{
	struct isp_mc_addr_mgr *mgr = (struct isp_mc_addr_mgr  *)handle;

	if (mgr) {
		if (base_addr)
			*base_addr = mgr->start;
		if (len)
			*len = mgr->len;
	}
}

void isp_mc_addr_mgr_destroy(void *handle)
{
	struct isp_mc_addr_mgr *mgr;
	struct isp_mc_addr_node *next = NULL;
	struct isp_mc_addr_node *prev = NULL;

	mgr = (struct isp_mc_addr_mgr  *)handle;
	if (mgr == NULL)
		mgr = &l_isp_mc_addr_mgr;
	if (mgr) {
		prev = l_isp_mc_addr_mgr.head.next;
		if (prev)
			next = prev->next;
		while (next) {
			isp_sys_mem_free(prev);
			prev = next;
			next = next->next;
		};
		if (prev)
			isp_sys_mem_free(prev);

		ISP_PR_INFO("suc\n");
	} else {
		ISP_PR_ERR("fail for NULL handle\n");
	}
}

int isp_mc_addr_alloc(void *handle,
	unsigned int byte_size,
	unsigned int align, unsigned long long *mc_addr)
{
	struct isp_mc_addr_mgr *mgr;
	struct isp_mc_addr_node *first = NULL;
	struct isp_mc_addr_node *second = NULL;
	struct isp_mc_addr_node *temp;
	unsigned long long size;
	int ret = RET_SUCCESS;

	mgr = (struct isp_mc_addr_mgr  *)handle;

	if (mgr == NULL)
		mgr = &l_isp_mc_addr_mgr;

	if ((mgr == NULL) || (byte_size == 0) || (mc_addr == NULL)) {
		ISP_PR_ERR("fail for bad para,size %u,mc %p\n",
			byte_size, mc_addr);
		return RET_INVALID_PARAM;
	}

	isp_mutex_lock(&mgr->mutext);

	first = &mgr->head;
	second = first->next;
	if (second == NULL) {
		ISP_PR_ERR("fail for second\n");
		ret = RET_FAILURE;
		goto quit;
	}

	while (1) {
		if (first->end_addr >= second->start_addr) {
			ISP_PR_ERR
				("fail FistEnd(0x%llx)>=secStart(0x%llx)\n",
				first->end_addr, second->start_addr);
		}

		size = second->start_addr - first->end_addr;
		if (size >= (byte_size + align)) {
			temp = (struct isp_mc_addr_node  *)
			isp_sys_mem_alloc(sizeof(struct isp_mc_addr_node));
			if (temp == NULL) {
				ISP_PR_ERR("fail for aloc tmp\n");
				ret = RET_FAILURE;
				goto quit;
			}
			temp->start_addr = first->end_addr + 1;
			temp->align_addr =
				ISP_ADDR_ALIGN_UP(temp->start_addr, align);
			temp->end_addr =
				temp->align_addr + byte_size - 1;// +(1 * 1024);
			temp->size = byte_size;
			temp->prev = first;
			temp->next = second;

			first->next = temp;
			second->prev = temp;
			*mc_addr = temp->align_addr;

			break;
		}
		first = second;
		second = second->next;
		if (second == NULL) {
			ISP_PR_ERR("fail for no enough MC,%u\n",
				byte_size);
			ret = RET_FAILURE;
			goto quit;
		}
	}

quit:

	isp_mutex_unlock(&mgr->mutext);

	return ret;
}

int isp_mc_addr_free(void *handle,
		unsigned long long base_addr)
{
	struct isp_mc_addr_mgr *mgr;
	struct isp_mc_addr_node *buffer;
	int ret = RET_SUCCESS;

	mgr = (struct isp_mc_addr_mgr  *)handle;
	if (mgr == NULL)
		mgr = &l_isp_mc_addr_mgr;

	if (mgr == NULL) {
		ISP_PR_ERR("fail for bad para\n");
		return RET_INVALID_PARAM;
	}

	isp_mutex_lock(&mgr->mutext);
	buffer = &mgr->head;
	while (buffer != NULL) {
		if (buffer->align_addr == base_addr) {
			buffer->prev->next = buffer->next;
			buffer->next->prev = buffer->prev;
			isp_sys_mem_free(buffer);
			ret = RET_SUCCESS;
			goto quit;
		} else {
			buffer = buffer->next;
		}
	}
	ret = RET_FAILURE;
	ISP_PR_ERR("fail for not find, for %llx:W\n", base_addr);

quit:
	isp_mutex_unlock(&mgr->mutext);
	return ret;
}

int cgs_alloc_gpu_mem_t_imp(void *cgs_device,
				   enum cgs_gpu_mem_type type,
				   uint64_t size, uint64_t align,
				   uint64_t min_offset, uint64_t max_offset,
				   cgs_handle_t *handle)
{
	int ret;
	uint64_t mc_addr;

	cgs_device = cgs_device;
	if (size == ISP_FW_WORK_BUF_SIZE) {
		if (handle)
			*handle = 0;
		return 0;
	}

	ret = isp_mc_addr_alloc(NULL, size, align, &mc_addr);

	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("fail by mc_alloc\n");
		return -1;
	};

	if (handle)
		*handle = (cgs_handle_t)mc_addr;
	return 0;
};



int cgs_free_gpu_mem_t_imp(void *cgs_device, cgs_handle_t handle)
{
	int ret;

	cgs_device = cgs_device;
	if (handle == 0)
		return 0;
	ret = isp_mc_addr_free(NULL, handle);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("fail by mc_free\n");
		return -1;
	};
	return 0;
};


int cgs_gmap_gpu_mem_t_imp(void *cgs_device,
		cgs_handle_t handle,
		uint64_t *mcaddr)
{
	cgs_device = cgs_device;
	if (mcaddr)
		*mcaddr = handle;
	return 0;

};

int cgs_gunmap_gpu_mem_t_imp(void *cgs_device,
		cgs_handle_t handle)
{
	cgs_device = cgs_device;
	handle = handle;
	return 0;
};


int cgs_kmap_gpu_mem_t_imp(void *cgs_device,
		cgs_handle_t handle,
		void **map)
{
	cgs_device = cgs_device;
	if (map)
		*map = (void *)(fpga_mem_base + handle);
	return 0;
};

int cgs_kunmap_gpu_mem_t_imp(void *cgs_device,
	cgs_handle_t handle)
{
	cgs_device = cgs_device;
	handle = handle;
	return 0;
};

uint32_t cgs_read_register_t_imp(void *cgs_device,
	unsigned int offset)
{
	uint64_t reg_addr_w;
	uint32_t ret_val;
	uint64_t final_reg_addr;

	cgs_device = cgs_device;
	reg_addr_w = ((offset) << 2);
	final_reg_addr = (fpga_reg_base + reg_addr_w);

	isp_mutex_lock(&fpga_reg_access_mutex);

	ret_val = fpga_read_reg32(final_reg_addr);

	isp_mutex_unlock(&fpga_reg_access_mutex);
	return ret_val;
}

void cgs_write_register_t_imp(void *cgs_device, unsigned int offset,
				uint32_t value)
{
	uint64_t reg_addr_w;
	uint64_t final_reg_addr;

	cgs_device = cgs_device;
	reg_addr_w = ((offset) << 2);
	final_reg_addr = (fpga_reg_base + reg_addr_w);

	isp_mutex_lock(&fpga_reg_access_mutex);

	fpga_write_reg32(final_reg_addr, value);

	isp_mutex_unlock(&fpga_reg_access_mutex);

};
//cdma related code referring to isp_fpga_cdma_reg.h,isp_fpga_cdma.h
//and isp_fpga_cdma.c
/****************************************/
//following are from isp_fpga_cdma_reg.h
//#define REG_CDMA_BASE       0x70000
//the original definition is above it is in the indirect reg accessing space
//but actually it is reg in the fpga board which is used by KMD in aidt,
//in kernel mode
//these registers should be accessed by direct way, so change it's definition
//to following
#define REG_CDMA_BASE       0x10070000

#define REG_CDMA_CDMACR     (REG_CDMA_BASE + 0x00) // CDMA control
#define REG_CDMA_CDMASR     (REG_CDMA_BASE + 0x04) // CDMA status
#define REG_CDMA_SRCADDR    (REG_CDMA_BASE + 0x18) // source address
#define REG_CDMA_DSTADDR    (REG_CDMA_BASE + 0x20) // destination address
#define REG_CDMA_BTT        (REG_CDMA_BASE + 0x28) // bytes to transfer

// AXI Base Address Translation Configuration Registers
// XILINX CDMA
//#define REG_CDMA_AXIBAR2PCIEBAR_BASE 0x71000
//same reason as REG_CDMA_BASE
#define REG_CDMA_AXIBAR2PCIEBAR_BASE 0x10071000

#define REG_CDMA_AXIBAR2PCIEBAR_0U   (REG_CDMA_AXIBAR2PCIEBAR_BASE + 0x208)
#define REG_CDMA_AXIBAR2PCIEBAR_0L   (REG_CDMA_AXIBAR2PCIEBAR_BASE + 0x20C)



struct _RegCdmaCr_t {
	union {
		uint32_t val;
		struct {
			uint32_t Reserved0 : 1;
			uint32_t TailPntrEn : 1;
			uint32_t Reset : 1;
			uint32_t SGMode : 1;
			uint32_t KeyHoleRead : 1;
			uint32_t KeyHoleWrite : 1;
			uint32_t Reserved1 : 6;
			uint32_t IOC_IrqEn : 1;
			uint32_t Dly_IrqEn : 1;
			uint32_t Err_IrqEn : 1;
			uint32_t Reserved2 : 1;
			uint32_t IRQThreshold : 8;
			uint32_t IRQDelay : 8;
		} field;
	};
};

struct _RegCdmaSr_t {
	union {
		uint32_t val;
		struct {
			uint32_t Reserved0 : 1;
			uint32_t Idle : 1;
			uint32_t Reserved1 : 1;
			uint32_t SGIncId : 1;
			uint32_t DMAIntErr : 1;
			uint32_t DMASlvErr : 1;
			uint32_t DMADecErr : 1;
			uint32_t Reserved2 : 1;
			uint32_t SGIntErr : 1;
			uint32_t SGSlvErr : 1;
			uint32_t SGDecErr : 1;
			uint32_t Reserved3 : 1;
			uint32_t IOC_Irq : 1;
			uint32_t Dly_Irq : 1;
			uint32_t Err_Irq : 1;
			uint32_t Reserved4 : 1;
			uint32_t IRQThresholdSts : 8;
			uint32_t IRQDlySts : 8;
		} field;
	};
};

struct _RegCdmaSrcAddr_t {
	union {
		uint32_t val;
		struct {
			uint32_t addr : 32;
		} field;
	};
};

struct _RegCdmaDstAddr_t {
	union {
		uint32_t val;
		struct {
			uint32_t addr : 32;
		} field;
	};
};

struct _RegCdmaBtt_t {
	union {
		uint32_t val;
		struct {
			uint32_t btt : 23;
			uint32_t reserved : 9;
		} field;
	};
};


/****************************************/

/**********************************/
#define CDMA_AXIBAR_MAXNUM  1
#define CDMA_AXIBAR_0       0xf0000000ULL
#define CDMA_AXI_HIGHADDR_0 0xffffffffULL


//for Altera Avalon-MM
#define AVMM_AXIBAR_MAXNUM  1
#define AVMM_AXIBAR_0       0x40000000ULL
#define AVMM_AXI_HIGHADDR_0 0x43ffffffULL


// **********************************

#define isp_write_reg(X, addr, value) isp_hw_reg_write32(addr, value)
#define isp_read_reg(X, addr) isp_hw_reg_read32(addr)

/****************************************/
//following are from isp_fpga_cdma.c



void cdma_init(void *pDrvData)
{
	cdma_reset(pDrvData);
}

void cdma_reset(const void *pDrvData)
{
	struct _RegCdmaCr_t      regCdmaCr;

	regCdmaCr.val = 0;
	regCdmaCr.field.Reset = 1;
	isp_write_reg(pDrvData, REG_CDMA_CDMACR, regCdmaCr.val);

	//udelay(10);

	regCdmaCr.val = 0;
	isp_write_reg(pDrvData, REG_CDMA_CDMACR, regCdmaCr.val);
}

int cdma_is_idle(const void *pDrvData)
{
	struct _RegCdmaSr_t      regCdmaSr;

	regCdmaSr.val = isp_read_reg(pDrvData, REG_CDMA_CDMASR);
	return (regCdmaSr.field.Idle == 1);
}

int cdma_has_error(const void *pDrvData)
{
	struct _RegCdmaSr_t      regCdmaSr;

	regCdmaSr.val = isp_read_reg(pDrvData, REG_CDMA_CDMASR);
	return (regCdmaSr.field.DMAIntErr
		|| regCdmaSr.field.DMASlvErr
		|| regCdmaSr.field.DMADecErr
		|| regCdmaSr.field.SGIntErr
		|| regCdmaSr.field.SGSlvErr
		|| regCdmaSr.field.SGDecErr);
}

// configure AXI2PCIE BAR register to
// map the PCIE space address to AXI space address
uint32_t map_pcie_to_axi(const void *pDrvData, uint64_t pcieAddr)
{
	uint32_t a2pAddr_lo = 0;
	uint32_t a2pAddr_hi = 0;
	uint32_t axiAddr = 0;

	a2pAddr_lo = (uint32_t)pcieAddr;
	a2pAddr_hi = (uint32_t)(pcieAddr >> 32);
	isp_write_reg(pDrvData, REG_CDMA_AXIBAR2PCIEBAR_0L, a2pAddr_lo);
	isp_write_reg(pDrvData, REG_CDMA_AXIBAR2PCIEBAR_0U, a2pAddr_hi);

	axiAddr = (pcieAddr & (CDMA_AXIBAR_0 ^ CDMA_AXI_HIGHADDR_0))
		| CDMA_AXIBAR_0;



	//ISP_PR_DBG("a2pAddr_lo = 0x%08x, a2pAddr_hi = 0x%08x\n",
	//a2pAddr_lo, a2pAddr_hi);
	//ISP_PR_DBG("mapped axiAddr = 0x%08x\n", axiAddr);

	return axiAddr;
}

int cdma_start(const void *pDrvData, uint64_t srcAddr,
		uint64_t dstAddr, uint32_t size, enum _DmaDirection_t direction)
{
	struct _RegCdmaCr_t      regCdmaCr;
	struct _RegCdmaSrcAddr_t regCdmaSrcAddr;
	struct _RegCdmaDstAddr_t regCdmaDstAddr;
	struct _RegCdmaBtt_t     regCdmaBtt;

	if ((size == 0) || (size > (CDMA_AXI_HIGHADDR_0 - CDMA_AXIBAR_0))) {
		ISP_PR_ERR("invalid DMA size\n");
		return false;
	}

	if (!cdma_is_idle(pDrvData)) {
		ISP_PR_ERR("cdma is busy\n");
		return false;
	}

	if (direction == DMA_DIRECTION_SYS_FROM_DEV) {
		regCdmaSrcAddr.val = 0;
		regCdmaSrcAddr.field.addr = (uint32_t)srcAddr;

		regCdmaDstAddr.val = 0;
		regCdmaDstAddr.field.addr = map_pcie_to_axi(pDrvData, dstAddr);
	} else if (direction == DMA_DIRECTION_SYS_TO_DEV) {
		regCdmaSrcAddr.val = 0;
		regCdmaSrcAddr.field.addr = map_pcie_to_axi(pDrvData, srcAddr);

		regCdmaDstAddr.val = 0;
		regCdmaDstAddr.field.addr = (uint32_t)dstAddr;
	} else if (direction == DMA_DIRECTION_DEV_TO_DEV) {
		regCdmaSrcAddr.val = 0;
		regCdmaSrcAddr.field.addr = (uint32_t)srcAddr;

		regCdmaDstAddr.val = 0;
		regCdmaDstAddr.field.addr = (uint32_t)dstAddr;
	} else {
		ISP_PR_ERR("unsupported DMA direction\n");
		return false;
	}

	regCdmaCr.val = 0;
	regCdmaCr.field.IOC_IrqEn = 1;
	regCdmaCr.field.Err_IrqEn = 1;
	isp_write_reg(pDrvData, REG_CDMA_CDMACR, regCdmaCr.val);

	isp_write_reg(pDrvData, REG_CDMA_SRCADDR, regCdmaSrcAddr.val);
	isp_write_reg(pDrvData, REG_CDMA_DSTADDR, regCdmaDstAddr.val);

	regCdmaBtt.val = 0;
	regCdmaBtt.field.btt = size;
	isp_write_reg(pDrvData, REG_CDMA_BTT, regCdmaBtt.val);//trig DMA start

	return true;
}

enum _IOCTL_RET_STATUS do_dma_once(void *pDrvData,
		uint64_t dmaSrcAddr,
		uint64_t dmaDstAddr,
		unsigned int length,
		enum _DmaDirection_t direct)
{
	enum _IOCTL_RET_STATUS retStatus = IOCTL_RET_STATUS_SUCCESS;
	unsigned int i;

//	ISP_PR_DBG("dmaSrcAddr = 0x%llx, dmaDstAddr = 0x%llx,
//	length = %d, direct = %d\n",
//		dmaSrcAddr, dmaDstAddr, length, direct);

	cdma_reset(pDrvData);
	if (!cdma_start(pDrvData, dmaSrcAddr, dmaDstAddr, length, direct)) {
		ISP_PR_ERR(
"cdma_start fail,dmaSrcAddr = 0x%llx, dmaDstAddr = 0x%llx, length = %d, direct = %d\n",
			dmaSrcAddr, dmaDstAddr, length, direct);
		retStatus = IOCTL_RET_STATUS_DMA_INTERNAL_ERR;
	}

	for (i = 0; i < 500; i++) {
		if (cdma_is_idle(pDrvData))
			break;

		msleep(20);//msleep(1);
	}

	ISP_PR_ERR("suc");
	return retStatus;
};


static enum _IOCTL_RET_STATUS wait_dma_finished(void *pDrvData)
{
	enum _IOCTL_RET_STATUS retStatus = IOCTL_RET_STATUS_SUCCESS;
	struct _RegCdmaSr_t  regCdmaSr;
	long long start, end;

	isp_get_cur_time_tick(&start);
	while (TRUE) {
		regCdmaSr.val = isp_read_reg(pDrvData, REG_CDMA_CDMASR);
		if (regCdmaSr.field.Idle == 1) {
			retStatus = IOCTL_RET_STATUS_SUCCESS;
			// clear irq
			isp_write_reg(pDrvData, REG_CDMA_CDMASR, regCdmaSr.val);
			break;
		} else if (regCdmaSr.field.Err_Irq
			|| regCdmaSr.field.DMAIntErr
			|| regCdmaSr.field.DMASlvErr
			|| regCdmaSr.field.DMADecErr
			|| regCdmaSr.field.SGIntErr
			|| regCdmaSr.field.SGSlvErr
			|| regCdmaSr.field.SGDecErr) {
			ISP_PR_ERR("DMA fail,regCdmaSr:0x%08x\n", regCdmaSr);
			// clear irq
			isp_write_reg(pDrvData, REG_CDMA_CDMASR, regCdmaSr.val);
			retStatus = IOCTL_RET_STATUS_DMA_INTERNAL_ERR;
			break;
		}

		isp_get_cur_time_tick(&end);
		if (isp_is_timeout(&start, &end, 10)) {
			retStatus = IOCTL_RET_STATUS_DMA_TIMEOUT;
			break;
		}
	}
	return retStatus;
}


enum _IOCTL_RET_STATUS do_dma_sg(void *pDrvData,
				uint64_t devAddr,
				struct physical_address *phy_sg_tbl,
				unsigned int sg_tbl_cnt,
				enum _DmaDirection_t direct,
				unsigned int lenInBytes)
{
	enum _IOCTL_RET_STATUS retStatus = IOCTL_RET_STATUS_SUCCESS;

	uint32_t i = 0;
	//uint32_t request_size = doDma.Length;
	uint32_t completed_size = 0;
	uint32_t trans_size = 0;
	uint64_t dmaSrcAddr = 0;
	uint64_t dmaDstAddr = 0;

	ISP_PR_DBG("sgcnt %u, %s, len %u\n", sg_tbl_cnt,
	(direct == DMA_DIRECTION_SYS_FROM_DEV) ? "devToSys" : "SysToDev",
	lenInBytes);
	if (direct == DMA_DIRECTION_SYS_FROM_DEV)
		dmaSrcAddr = devAddr;
	else if (direct == DMA_DIRECTION_SYS_TO_SYS)
		dmaSrcAddr = devAddr;
	else
		dmaDstAddr = devAddr;

	for (i = 0; i < sg_tbl_cnt; i++) {
		if (direct == DMA_DIRECTION_SYS_FROM_DEV) {
			dmaSrcAddr += trans_size;
			dmaDstAddr = phy_sg_tbl[i].phy_addr;
		} else if (direct == DMA_DIRECTION_SYS_TO_SYS) {
			dmaSrcAddr += trans_size;
			dmaDstAddr = phy_sg_tbl[i].phy_addr;
		} else {
			dmaDstAddr += trans_size;
			dmaSrcAddr = phy_sg_tbl[i].phy_addr;
		}

		trans_size =
			MIN(phy_sg_tbl[i].cnt, lenInBytes - completed_size);
		if (trans_size)
			retStatus = do_dma_once(pDrvData, dmaSrcAddr,
					dmaDstAddr, trans_size, direct);
		if (retStatus != IOCTL_RET_STATUS_SUCCESS) {
			if (direct == DMA_DIRECTION_SYS_FROM_DEV) {
				ISP_PR_ERR(
"[%d/%d]: dma_addr:0x%llx,dev_addr:0x%llx do_dma_once fail %u\n",
				i, sg_tbl_cnt, dmaDstAddr,
				dmaSrcAddr, retStatus);
			} else {
				ISP_PR_ERR(
"[%d/%d]: dma_addr:0x%llx,dev_addr:0x%llx do_dma_once fail %u\n",
				i, sg_tbl_cnt, dmaSrcAddr,
				dmaDstAddr, retStatus);
			}
			break;
		}

	    completed_size += trans_size;

		if ((completed_size < lenInBytes) && (i < sg_tbl_cnt - 1)) {
			retStatus = wait_dma_finished(pDrvData);
			if (retStatus != IOCTL_RET_STATUS_SUCCESS) {
				if (direct == DMA_DIRECTION_SYS_FROM_DEV) {
					ISP_PR_ERR(
"[%d/%d]: phy:0x%llx,devAddr:0x%llx,len %u wait_dma_finished fail %u\n",
					i, sg_tbl_cnt, dmaDstAddr, dmaSrcAddr,
					trans_size, retStatus);
				} else {
					ISP_PR_ERR(
"[%d/%d]: phy:0x%llx,devAddr:0x%llx,len %u wait_dma_finished fail %u\n",
					i, sg_tbl_cnt, dmaSrcAddr, dmaDstAddr,
					trans_size, retStatus);
				}
				break;
			} else if (1) {
				if (direct == DMA_DIRECTION_SYS_FROM_DEV) {
					ISP_PR_DBG(
"[%d/%d]: phy:0x%llx,devAddr:0x%llx,len %u done\n",
						i, sg_tbl_cnt, dmaDstAddr,
						dmaSrcAddr, trans_size);
				} else {
					ISP_PR_DBG(
"[%d/%d]: phy:0x%llx,devAddr:0x%llx,len %u\n done",
						i, sg_tbl_cnt, dmaSrcAddr,
						dmaDstAddr, trans_size);
				}
			}
		}
	}

	RET(retStatus);
	return retStatus;
};
void *mem_copy_virt_addr;
dma_addr_t mem_copy_phys_addr;


#define raw_buf_len		(1920*1080*2)

static int __init amd_pci_probe(struct pci_dev *dev,
			      const struct pci_device_id *ent)
{
	int retval = 0;
	unsigned long ddrstart, ddrend, ddrflags, ddrlen;
	unsigned long regstart, regend, regflags, reglen;

	void __iomem *ddraddr = NULL;
	void __iomem *regaddr = NULL;
	int32_t result;
	struct device_private *private = &fpga_pci_private;

	if (pci_enable_device(dev)) {
		ISP_PR_ERR("fpgapci:IO Error.\n");
		return -EIO;
	}

	memset(private, 0, sizeof(*private));

	private->amd_pdev = dev;

	private->irq = dev->irq;
	retval = pci_request_regions(dev, AMD_PCI_NAME);
	if (retval) {
		ISP_PR_ERR
		("fpgapci:cannot request_regions(%d), aborting.\n",
		       retval);
		goto err_out;
	}

	ddrstart = pci_resource_start(dev, 0);
	ddrend = pci_resource_end(dev, 0);
	ddrflags = pci_resource_flags(dev, 0);
	ddrlen = pci_resource_len(dev, 0);
	ISP_PR_INFO("fpgapci:ddrstart(len) is 0x%0x(%u),flasg:0x%x\n\n",
	       (uint32_t) ddrstart, (uint32_t) ddrlen, (uint32_t) ddrflags);
	if (!(ddrflags & IORESOURCE_MEM)) {
		ISP_PR_ERR
		("fpgapci:cannot find base address for 0, aborting.\n");
		retval = -ENODEV;
		goto err_out;
	}

	regstart = pci_resource_start(dev, 2);
	regend = pci_resource_end(dev, 2);
	regflags = pci_resource_flags(dev, 2);
	reglen = pci_resource_len(dev, 2);
	ISP_PR_INFO("fpgapci:regstart(len) is 0x%0x(%u),flasg:0x%x\n",
	       (uint32_t) regstart, (uint32_t) reglen, (uint32_t) regflags);
	if (!(regflags & IORESOURCE_MEM)) {
		ISP_PR_ERR
		("fpgapci:cannot find base address for 0, aborting.\n");
		retval = -ENODEV;
		goto err_out;
	}

	ddraddr = ioremap(ddrstart, ddrlen);
	if (ddraddr == NULL) {
		ISP_PR_INFO
		("fpgapci:%s:cannot remap ddrstart, aborting\n",
		       pci_name(dev));
		retval = -EIO;
		goto err_out;
	}
	ISP_PR_INFO("fpgapci:ddraddr =  0x%llx\n", ddraddr);

	regaddr = ioremap(regstart, reglen);
	if (regaddr == NULL) {
		ISP_PR_INFO
		("fpgapci:%s:cannot remap regstart, aborting\n",
		       pci_name(dev));
		retval = -EIO;
		goto err_out;
	}
	ISP_PR_INFO("fpgapci:regaddr =  0x%llx\n", regaddr);

	private->ddraddr = ddraddr;
	private->ddrstart = ddrstart;
	private->ddrlen = ddrlen;

	private->regaddr = regaddr;
	private->regstart = regstart;
	private->reglen = reglen;

	pci_set_master(dev);
	pci_set_drvdata(dev, private);

	ISP_PR_INFO("fpgapci:probe return 0\n");

	fpga_cgs_ops.read_register =
		(cgs_read_register_t)cgs_read_register_t_imp;
	fpga_cgs_ops.write_register =
		(cgs_write_register_t)cgs_write_register_t_imp;
	fpga_cgs_ops.alloc_gpu_mem =
		(cgs_alloc_gpu_mem_t)cgs_alloc_gpu_mem_t_imp;
	fpga_cgs_ops.free_gpu_mem =
		(cgs_free_gpu_mem_t)cgs_free_gpu_mem_t_imp;
	fpga_cgs_ops.gmap_gpu_mem =
		(cgs_gmap_gpu_mem_t)cgs_gmap_gpu_mem_t_imp;
	fpga_cgs_ops.gunmap_gpu_mem =
		(cgs_gunmap_gpu_mem_t)cgs_gunmap_gpu_mem_t_imp;
	fpga_cgs_ops.kmap_gpu_mem =
		(cgs_kmap_gpu_mem_t)cgs_kmap_gpu_mem_t_imp;
	fpga_cgs_ops.kunmap_gpu_mem =
		(cgs_kunmap_gpu_mem_t)cgs_kunmap_gpu_mem_t_imp;
	//fpga_cgs_ops.gmap_kmem = cgs_gmap_kmem_t_imp;
	//fpga_cgs_ops.gunmap_kmem = cgs_gunmap_kmem_t_imp;

	fpga_cgs_dev.ops = &fpga_cgs_ops;
	fpga_cgs_dev.os_ops = NULL;

	g_cgs_srv = &fpga_cgs_dev;

	isp_mutex_init(&fpga_reg_access_mutex);
	isp_mutex_init(&fpga_mem_access_mutex);
	fpga_reg_base = (uint64_t)private->regaddr;
	fpga_mem_base = (uint64_t)private->ddraddr;

	isp_mc_addr_mgr_init(ISP_FW_WORK_BUF_SIZE,
		TOTAL_DDR_MEM_SIZE - ISP_FW_WORK_BUF_SIZE);

	result = isp_init(&g_isp_context);
	if (result != 0) {
		ISP_PR_ERR("isp_init failed\n");
		retval = -EIO;
		goto err_out;

	} else {
		ISP_PR_ERR("isp_init suc\n");
	}

	probeRet = 0;

	mem_copy_virt_addr = (void *)pci_alloc_consistent(
			dev, raw_buf_len, &mem_copy_phys_addr);

	memset((unsigned char *)mem_copy_virt_addr, 0xff, raw_buf_len);

	return 0;

	//ALLOC PCI DMA
err_out:
	probeRet = retval;
	return retval;
}

static void amd_pci_remove(struct pci_dev *dev)
{
	struct device_private *private;

	ENTER();

	private = (struct device_private *)pci_get_drvdata(dev);
	unit_isp();
#ifndef USING_KMD_CGS
	isp_mc_addr_mgr_destroy(NULL);
#endif
	iounmap(private->regaddr);
	iounmap(private->ddraddr);
	pci_release_regions(dev);

	pci_set_drvdata(dev, NULL);
	pci_disable_device(dev);

	EXIT();
}

static struct pci_device_id amd_pci_table[] __initdata = {
	{
		AMD_VENDOR_ID,
		AMD_DEVICE_ID,
		PCI_ANY_ID,
		PCI_ANY_ID,
	},
	{0,},
};

MODULE_DEVICE_TABLE(pci, amd_pci_table);

static struct pci_driver amd_pci_driver_ops = {
	.name = AMD_PCI_NAME,
	.id_table = amd_pci_table,
	.probe = amd_pci_probe,
	.remove = amd_pci_remove
};

int amd_pci_init(void)
{
	int ret;

	ENTER();
	intRet = -1;
	probeRet = -1;

	ret = pci_register_driver(&amd_pci_driver_ops);
	if (ret < 0) {
		ISP_PR_ERR("fpgapci:Can't register driver!\n");
		return ret;
	}
	ISP_PR_INFO("fpgapci:The PCI driver is loaded successfully.\n");

	intRet = ret;

	RET(ret);
	return ret;
}

void amd_pci_exit(void)
{
	pci_unregister_driver(&amd_pci_driver_ops);
	intRet = -1;
	probeRet = -1;
}
#endif

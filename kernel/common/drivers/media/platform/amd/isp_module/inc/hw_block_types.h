/*
 *Copyright 2015 Advanced Micro Devices, Inc.
 *
 *Permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "Software"),
 *to deal in the Software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the Software, and to permit persons to whom the
 *Software is furnished to do so, subject to the following conditions:
 *
 *The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef HW_BLOCK_TYPES_H_
#define HW_BLOCK_TYPES_H_

enum hwblock_type {
	HWBLOCK__RESERVED,
	HWBLOCK_DONT_CARE__VBIOS_REQUEST,
	HWBLOCK_DONT_CARE__IP_DISCOVERY,
	HWBLOCK_DONT_CARE__LEGACY_BLOCK,
	HWBLOCK_MP1,
	HWBLOCK_MP2,
	HWBLOCK_THM,
	HWBLOCK_SMUIO,
	HWBLOCK_FUSE,
	HWBLOCK_CLKA,
	HWBLOCK_PWR,
	HWBLOCK_GC,
	HWBLOCK_UVD,
	HWBLOCK_AUDIO_AZ,
	HWBLOCK_ACP,
	HWBLOCK_DCI,
	HWBLOCK_DMU,
	HWBLOCK_DCO,
	HWBLOCK_DIO,
	HWBLOCK_XDMA,
	HWBLOCK_DCEAZ,
	HWBLOCK_DAZ,
	HWBLOCK_SDPMUX,
	HWBLOCK_NTB,
	HWBLOCK_IOHC,
	HWBLOCK_L2IMU,
	HWBLOCK_VCE,
	HWBLOCK_MMHUB,
	HWBLOCK_ATHUB,
	HWBLOCK_DBGU_NBIO,
	HWBLOCK_DFX,
	HWBLOCK_DBGU0,
	HWBLOCK_DBGU1,
	HWBLOCK_OSSSYS,
	HWBLOCK_HDP,
	HWBLOCK_SDMA0,
	HWBLOCK_SDMA1,
	HWBLOCK_ISP,
	HWBLOCK_DBGU_IO,
	HWBLOCK_DF,
	HWBLOCK_CLKB,
	HWBLOCK_FCH,
	HWBLOCK_DFX_DAP,
	HWBLOCK_L1IMU_PCIE,
	HWBLOCK_L1IMU_NBIF,
	HWBLOCK_L1IMU_IOAGR,
	HWBLOCK_L1IMU3,
	HWBLOCK_L1IMU4,
	HWBLOCK_L1IMU5,
	HWBLOCK_L1IMU6,
	HWBLOCK_L1IMU7,
	HWBLOCK_L1IMU8,
	HWBLOCK_L1IMU9,
	HWBLOCK_L1IMU10,
	HWBLOCK_L1IMU11,
	HWBLOCK_L1IMU12,
	HWBLOCK_L1IMU13,
	HWBLOCK_L1IMU14,
	HWBLOCK_L1IMU15,
	HWBLOCK_WAFLC,
	HWBLOCK_FCH_USB_PD,
	HWBLOCK_PCIE,
	HWBLOCK_PCS,
	HWBLOCK_DDCL,
	HWBLOCK_SST,
	HWBLOCK_IOAGR,
	HWBLOCK_NBIF,
	HWBLOCK_NBIO = HWBLOCK_NBIF,
	HWBLOCK_IOAPIC,
	HWBLOCK_SYSTEMHUB,
	HWBLOCK_NTBCCP,
	HWBLOCK_UMC,
	HWBLOCK_SATA,
	HWBLOCK_USB,
	HWBLOCK_CCXSEC,
	HWBLOCK_XGBE,
	HWBLOCK_MP0,
	HWBLOCK_VCN,

	HWBLOCK__COUNT,
};

enum swip_to_hwip_def {
	max_hwip_instance = 6,
	max_hwip_segment = 6,
};

struct hwip_version_info {
	uint32_t major;
	uint32_t minor;
	uint32_t revision;
};

struct hwip_discovery_info_instance {
	/*HW component version */
	struct hwip_version_info hc_version;

	/*SW interface version */
	struct hwip_version_info si_version;

	uint32_t seg_base[max_hwip_segment];
};

struct hwip_discovery_info {
	enum hwblock_type sw_hwip_id;
	uint32_t num_instances;
	struct hwip_discovery_info_instance
			hwip_instance_info[max_hwip_instance];
};

#endif //HW_BLOCK_TYPES_H_

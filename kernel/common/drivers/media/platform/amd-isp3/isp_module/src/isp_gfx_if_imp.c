/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "isp_module_if_imp.h"
#include "log.h"
#include "isp_soc_adpt.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][isp_gfx_if_imp]"

struct acpi_isp_info g_acpi_isp_info = { {0}, {0}, {0}, {0}, {0},
{0}, {0}, {0}, {0}, 0
};

enum irq_source_isp g_irq_src[] = {
	IRQ_SOURCE_ISP_RINGBUFFER_WPT9,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT10,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT11,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT12
};

struct cgs_device *g_cgs_srv;
EXPORT_SYMBOL(g_cgs_srv);

#if	defined(ISP_BRINGUP_WORKAROUND)
int isp_alloc_raw_buffer(struct isp_context *isp)
{
	struct sensor_info *sif;
	unsigned int i;
	unsigned int size;

	sif = &isp->sensor_info[CAMERA_ID_REAR];
	size = 4056*3040*2;

	sif->stream_tmp_buf[0] =
		isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_LFB);
	if (sif->stream_tmp_buf[0] == NULL) {
		ISP_PR_ERR("alloc raw buffer(fb) fail\n");
		return -1;
	}

	for (i = 1; i < STREAM_TMP_BUF_COUNT; i++) {
		sif->stream_tmp_buf[i] =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_LFB);

		if (sif->stream_tmp_buf[i] == NULL) {
			ISP_PR_ERR("aloc raw buffer(%u) fail\n", i);
			return -1;
		}
	}

	return 0;
}
#endif
int is_cgs_cos_srv_complete(void)
{
	if (g_cgs_srv->ops->alloc_gpu_mem == NULL) {
		ISP_PR_ERR("NULL alloc_gpu_mem\n");
		goto quit;
	};

	if (g_cgs_srv->ops->free_gpu_mem == NULL) {
		ISP_PR_ERR("NULL free_gpu_mem\n");
		goto quit;
	};

	if (g_cgs_srv->ops->gmap_gpu_mem == NULL) {
		ISP_PR_ERR("NULL gmap_gpu_mem\n");
		goto quit;
	};

	if (g_cgs_srv->ops->gunmap_gpu_mem == NULL) {
		ISP_PR_ERR("NULL gunmap_gpu_mem\n");
		goto quit;
	};

	if (g_cgs_srv->ops->kmap_gpu_mem == NULL) {
		ISP_PR_ERR("NULL kmap_gpu_mem\n");
		goto quit;
	};

	if (g_cgs_srv->ops->kunmap_gpu_mem == NULL) {
		ISP_PR_ERR("NULL kunmap_gpu_mem\n");
		goto quit;
	};

	if (g_cgs_srv->ops->read_register == NULL) {
		ISP_PR_ERR("NULL read_register\n");
		goto quit;
	};

	if (g_cgs_srv->ops->write_register == NULL) {
		ISP_PR_ERR("NULL write_register\n");
		goto quit;
	};

	return true;
 quit:
	return false;
}

enum isp_result isp_sw_init(struct cgs_device *cgs_dev,
	void **isp_handle, unsigned char *presence)
{
	if ((cgs_dev == NULL) || (cgs_dev == NULL)) {
		ISP_PR_ERR("fail for illegal param\n");
		return -1;
	}

	if (!is_cgs_cos_srv_complete())
		goto quit;

	ISP_PR_INFO("success\n");
	return 0;
quit:
	return -1;
}

int isp_init(struct cgs_device *cgs_dev, void *svr_in)
{
	void *isp_handle;
	struct swisp_services *svr = (struct swisp_services *)svr_in;

	isp_handle = &g_isp_context;
	if (isp_handle == NULL) {
		ISP_PR_ERR("fail for invalid para\n");
		return -1;
	}

	if (!svr) {
		ISP_PR_ERR("fail for swisp svr NULL\n");
		return -1;
	}

	if (cgs_dev == NULL) {
		ISP_PR_ERR("fail for cgs_dev NULL\n");
		return -1;
	}

	g_cgs_srv = cgs_dev;
	g_swisp_svr.cvip_set_gpuva = svr->cvip_set_gpuva;
	g_swisp_svr.get_cvip_buf = svr->get_cvip_buf;
	g_swisp_svr.set_isp_power = svr->set_isp_power;
	g_swisp_svr.set_isp_clock = svr->set_isp_clock;
	if (isp_sw_init(g_cgs_srv, &isp_handle, 0) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_sw_init\n");
		return -1;
	}

	if (ispm_sw_init(isp_handle, 0x446e6942, NULL) !=
		RET_SUCCESS) {
		ISP_PR_ERR("fail for ispm_sw_init\n");
		return ISP_ERROR_GENERIC;
	}

#if	defined(ISP_BRINGUP_WORKAROUND)
	if (isp_alloc_raw_buffer(isp_handle) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_alloc_raw_buffer\n");
		return ISP_ERROR_GENERIC;
	}
#endif
	ISP_PR_INFO("success\n");
	return 0;
}
EXPORT_SYMBOL(isp_init);

enum isp_result isp_hw_exit(void *isp_handle)
{
	if (isp_handle == NULL) {
		ISP_PR_ERR("fail for invalid para\n");
		return -1;
	}

	if (ispm_sw_uninit(isp_handle) != RET_SUCCESS) {
		ISP_PR_ERR("fail for ispm_sw_uninit\n");
		return -1;
	}

	ISP_PR_INFO("success\n");
	return 0;
}

enum isp_result isp_sw_exit(void __maybe_unused *isp_handle)
{
	memset(&g_acpi_isp_info, 0, sizeof(g_acpi_isp_info));
	memset(&g_cgs_srv, 0, sizeof(g_cgs_srv));
	ISP_PR_INFO("success\n");
	return 0;
}


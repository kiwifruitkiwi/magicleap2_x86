/**************************************************************************
 *copyright 2015~2016 advanced micro devices, inc.
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

#include "isp_common.h"
#include "isp_module_if_imp.h"
#include "log.h"
#include "isp_soc_adpt.h"

struct acpi_isp_info g_acpi_isp_info = { {0}, {0}, {0}, {0}, {0},
{0}, {0}, {0}, {0}, 0
};

enum irq_source_isp g_irq_src[] = {
	IRQ_SOURCE_ISP_RINGBUFFER_WPT5,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT6,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT7,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT8
};

struct cgs_device *g_cgs_srv;
EXPORT_SYMBOL(g_cgs_srv);

bool_t is_cgs_cos_srv_complete(void)
{
	if (g_cgs_srv->ops->alloc_gpu_mem == NULL) {
		isp_dbg_print_err("in %s,NULL alloc_gpu_mem\n", __func__);
		goto quit;
	};

	if (g_cgs_srv->ops->free_gpu_mem == NULL) {
		isp_dbg_print_err("in %s,NULL free_gpu_mem\n", __func__);
		goto quit;
	};

	if (g_cgs_srv->ops->gmap_gpu_mem == NULL) {
		isp_dbg_print_err("in %s,NULL gmap_gpu_mem\n", __func__);
		goto quit;
	};

	if (g_cgs_srv->ops->gunmap_gpu_mem == NULL) {
		isp_dbg_print_err("in %s,NULL gunmap_gpu_mem\n", __func__);
		goto quit;
	};

	if (g_cgs_srv->ops->kmap_gpu_mem == NULL) {
		isp_dbg_print_err("in %s,NULL kmap_gpu_mem\n", __func__);
		goto quit;
	};

	if (g_cgs_srv->ops->kunmap_gpu_mem == NULL) {
		isp_dbg_print_err("in %s,NULL kunmap_gpu_mem\n", __func__);
		goto quit;
	};

	if (g_cgs_srv->ops->read_register == NULL) {
		isp_dbg_print_err("in %s,NULL read_register\n", __func__);
		goto quit;
	};

	if (g_cgs_srv->ops->write_register == NULL) {
		isp_dbg_print_err("in %s,NULL write_register\n", __func__);
		goto quit;
	};

	return true;
 quit:
	return false;
}

enum isp_result isp_sw_init(struct cgs_device *cgs_dev,
	void **isp_handle, uint8 *presence)
{
	if ((cgs_dev == NULL) || (cgs_dev == NULL)) {
		isp_dbg_print_err("-><- %s fail for illegal param\n", __func__);
		return -1;
	}
	isp_dbg_print_info("-> %s\n", __func__);

	if (!is_cgs_cos_srv_complete())
		goto quit;

	isp_dbg_print_err("<- %s, succ\n", __func__);
	return 0;
quit:
	isp_dbg_print_err("<- %s, fail\n", __func__);
	return -1;
}

enum isp_result isp_init(struct cgs_device *cgs_dev)
{
	void *isp_handle;

	isp_handle = &g_isp_context;
	if ((isp_handle == NULL) /*|| (NULL == opninfo)*/) {
		isp_dbg_print_err("-><- %s, fail for invalid para\n", __func__);
		return -1;
	}

	isp_dbg_print_info("-> %s\n", __func__);
	if (cgs_dev == NULL) {
		isp_dbg_print_err("<- %s fail for cgs_dev NULL\n", __func__);
		return -1;
	}
	/*todo */
	g_cgs_srv = cgs_dev;
	if (isp_sw_init(g_cgs_srv, &isp_handle, 0) !=
		RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for isp_sw_init\n", __func__);
		return -1;
	}

	//test only
	if (ispm_sw_init(isp_handle, 0x446e6942, NULL) !=
		RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for ispm_sw_init\n", __func__);
		return ISP_ERROR_GENERIC;
	};

	isp_dbg_print_err("<- %s succ\n", __func__);
	return 0;
};
EXPORT_SYMBOL(isp_init);

enum isp_result isp_hw_exit(void *isp_handle)
{
	if (isp_handle == NULL) {
		isp_dbg_print_err("-><- %s, fail for invalid para\n", __func__);
		return -1;
	}
	isp_dbg_print_info("-> %s\n", __func__);
	if (ispm_sw_uninit(isp_handle) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for ispm_sw_uninit\n", __func__);
		return -1;
	}
	isp_dbg_print_info("<- %s succ\n", __func__);
	return 0;
}

enum isp_result isp_sw_exit(void *isp_handle)
{
	unreferenced_parameter(isp_handle);
	memset(&g_acpi_isp_info, 0, sizeof(g_acpi_isp_info));
	memset(&g_cgs_srv, 0, sizeof(g_cgs_srv));
	isp_dbg_print_info("-><- %s, succ\n", __func__);
	return 0;
}


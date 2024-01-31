/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _ISP_GFX_H
#define _ISP_GFX_H

#include "cgs_linux.h"
#include "hw_block_types.h"
//refer to //camip/isp/doc/design/isp3.0/MAS/IH/isp3.0_ih_mas.docx
//section 4.1
enum irq_source_isp {
	IRQ_SOURCE_ISP_RINGBUFFER_BASE9_CHANGED = 3,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT9 = 4,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE10_CHANGED = 5,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT10 = 6,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE11_CHANGED = 7,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT11 = 8,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE12_CHANGED = 9,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT12 = 10,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE13_HANGED = 11,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT13 = 12,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE14_CHANGED = 13,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT14 = 14,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE15_CHANGED = 15,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT15 = 16,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE16_CHANGED = 17,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT16 = 18,
	IRQ_SOURCE_ISP_END_OF_LIST
};

enum cgs_result {
	/*STATUS_SUCCESS*/
	CGS_RESULT__OK = 0,
	/*some unknown error happen*/
	CGS_RESULT__ERROR_GENERIC = 1,
	/*input or output parameter error*/
	CGS_RESULT__ERROR_INVALIDPARAMS = 2,
	/*service not avaialble yet*/
	CGS_RESULT__ERROR_FUNCTION_NOT_SUPPORT = 3
};

enum isp_result {
	ISP_OK = 0,		/*STATUS_SUCCESS */
	ISP_ERROR_GENERIC = 1,	/*some unknown error happen */
	ISP_ERROR_INVALIDPARAMS = 2,	/*input or output parameter error */
};

struct isp_runtime_service {
	unsigned int size;
	/*size of CGS_RUNTIME_SERVICE */
	unsigned int servicenum;
	/*number of IPS run time servie */
};

enum isp_result isp_sw_init(struct cgs_device *cgs_dev,
				void **isp_handle, unsigned char *presence);
enum isp_result isp_hw_exit(void *isp_handle);

enum isp_result isp_sw_exit(void *isp_handle);

#endif	/*_ISP_GFX_H */

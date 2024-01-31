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
 *authors:
 *bin bin.du@amd.com
 *************************************************************************
 */

#ifndef _ISP_GFX_H
#define _ISP_GFX_H

#include "cgs_linux.h"
#include "hw_block_types.h"

enum irq_source_isp {
	IRQ_SOURCE_ISP_RINGBUFFER_BASE5_CHANGED = 1,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE6_CHANGED,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE7_CHANGED,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE8_CHANGED,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT5,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT6,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT7,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT8,
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
	uint32 size;
	/*size of CGS_RUNTIME_SERVICE */
	uint32 servicenum;
	/*number of IPS run time servie */
};

enum isp_result isp_sw_init(struct cgs_device *cgs_dev,
				void **isp_handle, uint8 *presence);
enum isp_result isp_hw_exit(void *isp_handle);

enum isp_result isp_sw_exit(void *isp_handle);

#endif	/*_ISP_GFX_H */

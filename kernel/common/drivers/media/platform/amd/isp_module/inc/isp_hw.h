/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
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

#ifndef HAL_H
#define HAL_H

#include "os_base_type.h"
#include "buffer_mgr.h"

#define isp_hw_reg_read32(addr) \
	g_cgs_srv->ops->read_register(g_cgs_srv, ((addr) >> 2))
#define isp_hw_reg_write32(addr, value) \
	g_cgs_srv->ops->write_register(g_cgs_srv, ((addr) >> 2), value)

result_t isp_hw_mem_write_(uint64 mem_addr, pvoid p_write_buffer,
			uint32 byte_size);
result_t isp_hw_mem_read_(uint64 mem_addr, pvoid p_read_buffer,
			uint32 byte_size);

#endif

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

#ifndef DW_COMMON_IO_H
#define DW_COMMON_IO_H

#define DW_OUT32_32P(v, p)	\
	isp_hw_reg_write32((uint32)((uint64)&p), (uint32)v)

#define DW_OUT16P(v, p)	DW_OUT32_32P(v, p)

#define DW_IN32_32P(p)	\
	isp_hw_reg_read32((uint32)((uint64)&p))

#define DW_IN16P(p)	\
	((uint16)(isp_hw_reg_read32((uint32)((uint64)&p)) & 0xffff))

#define DW_IN8P(p)	\
	((uint8)(isp_hw_reg_read32((uint32)((uint64)&p)) & 0xff))

#define DW_INP(p)	DW_IN32_32P(p)

#endif				/*DW_COMMON_IO_H */

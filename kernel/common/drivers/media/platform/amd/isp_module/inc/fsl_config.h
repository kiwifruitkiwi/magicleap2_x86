/*
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
 */

#ifndef _FSL_CONFIG_H_
#define _FSL_CONFIG_H_

#define MAX_LABEL_NAME_LEN	(32)
#define MAX_FUNC_NAME_LEN	(MAX_LABEL_NAME_LEN)
#define MAX_ARG_NUM		(3)
#define MAX_LABELS		(40)
#define MAX_INSTRS		(200)
#define MAX_FUNCS		(40)
#define MAX_REGISTERS		(32)
#define MAX_MEM_SIZE		(16*1024)
#define MAX_MEMS		(2)

#define MAX_SCRIPT_ARG		(10)

#define MAX_LINE_LEN		(1024)
#define MAX_WORD_LEN		(32)
#define MAX_WORDS		(MAX_ARG_NUM + 1)

#define MAX_ERROR_INFO_LEN	(256)

#define MAX_LOG_BUF		(256)

#endif

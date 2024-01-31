/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
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

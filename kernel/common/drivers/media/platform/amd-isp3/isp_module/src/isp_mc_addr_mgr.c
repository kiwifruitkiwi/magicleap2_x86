/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "isp_mc_addr_mgr.h"
#include "log.h"
#include "isp_fw_if.h"
#include "isp_soc_adpt.h"
#include <linux/kmemleak.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][isp_mc_addr_mgr]"

struct isp_fw_cmd_pay_load_buf {
	unsigned long long sys_addr;
	unsigned long long mc_addr;
	struct isp_fw_cmd_pay_load_buf *next;
};

struct isp_fw_work_buf_mgr {
	unsigned long long sys_base;
	unsigned long long mc_base;
	unsigned int pay_load_pkg_size;
	unsigned int pay_load_num;
	struct mutex mutex;
	struct isp_fw_cmd_pay_load_buf *free_cmd_pl_list;
	struct isp_fw_cmd_pay_load_buf *used_cmd_pl_list;
};

struct isp_fw_work_buf_mgr l_isp_work_buf_mgr;

static void isp_fw_pl_list_destroy(struct isp_fw_cmd_pay_load_buf *head)
{
	struct isp_fw_cmd_pay_load_buf *temp;

	if (head == NULL)
		return;
	while (head != NULL) {
		temp = head;
		head = head->next;
		isp_sys_mem_free(temp);
	}
}

void print_fw_work_buf_info(struct isp_fw_work_buf_mgr *hdl)
{
	unsigned long long sys_addr;
	unsigned long long mc_addr;
	unsigned int len;

	if (hdl) {
		/*struct isp_fw_cmd_pay_load_buf *next; */
		isp_fw_buf_get_code_base(hdl, &sys_addr, &mc_addr, &len);
		ISP_PR_INFO
		("isp_fw_buf_get_code_base sys:%llx, mc:%llx,len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_fw_buf_get_stack_base(hdl, &sys_addr, &mc_addr, &len);
		ISP_PR_INFO
		("isp_fw_buf_get_stack_base sys:%llx, mc:%llx,len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_fw_buf_get_heap_base(hdl, &sys_addr, &mc_addr, &len);
		ISP_PR_INFO
		("isp_fw_buf_get_heap_base sys:%llx, mc:%llx,len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_fw_buf_get_trace_base(hdl, &sys_addr, &mc_addr, &len);
		ISP_PR_INFO
		("isp_fw_buf_get_trace_base:%llx, mc:%llx,len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_fw_buf_get_cmd_base(hdl, 0, &sys_addr, &mc_addr, &len);
		ISP_PR_INFO
		("isp_fw_buf_get_cmd_base:%llx, mc:%llx,len:%d\n",
			sys_addr, mc_addr, len);

		isp_fw_buf_get_resp_base(hdl, 0, &sys_addr, &mc_addr, &len);
		ISP_PR_INFO
		("isp_fw_buf_get_resp_cmd_base:%llx, mc:%llx,len:%d\n",
			sys_addr, mc_addr, len);

		isp_fw_buf_get_cmd_pl_base(hdl, &sys_addr, &mc_addr, &len);
		ISP_PR_INFO
		("isp_fw_buf_get_cmd_pl_base:%llx,mc:%llx,total len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		ISP_PR_INFO("cmd_pl num:%u, each size:%d\n",
			hdl->pay_load_num, hdl->pay_load_pkg_size);
	}
};

void *isp_fw_work_buf_init(
	unsigned long long sys_addr, unsigned long long mc_addr)
{
	unsigned int pkg_size;
	unsigned long long base_sys;
	unsigned long long base_mc;
	unsigned long long next_sys;
	unsigned long long next_mc;
	unsigned int pl_len;
	unsigned int i;
	struct isp_fw_cmd_pay_load_buf *new_buffer;
	struct isp_fw_cmd_pay_load_buf *tail_buffer;
	int pl_size;

	memset(&l_isp_work_buf_mgr, 0, sizeof(struct isp_fw_work_buf_mgr));
	l_isp_work_buf_mgr.sys_base = sys_addr;
	l_isp_work_buf_mgr.mc_base = mc_addr;
	l_isp_work_buf_mgr.pay_load_pkg_size = isp_get_cmd_pl_size();
	ISP_PR_DBG("sizeof struct _FrameInfo_t %lu\n",
			sizeof(struct _FrameInfo_t));
	isp_mutex_init(&l_isp_work_buf_mgr.mutex);

	pkg_size = l_isp_work_buf_mgr.pay_load_pkg_size;

	pl_size = ISP_FW_CMD_PAY_LOAD_BUF_SIZE;
	if ((pl_size <= 0) || ((pl_size / pkg_size) < 10)) {
		ISP_PR_ERR("fail pl_size %d\n", pl_size);
		return NULL;
	};

	isp_fw_buf_get_cmd_pl_base(&l_isp_work_buf_mgr, &base_sys, &base_mc,
			&pl_len);
	next_sys = base_sys;
	next_mc = base_mc;

	i = 0;
	while (true) {
		new_buffer =
		isp_sys_mem_alloc(sizeof(struct isp_fw_cmd_pay_load_buf));
		if (new_buffer == NULL) {
			ISP_PR_ERR("fail for alloc\n");
			goto fail;
		};
		kmemleak_not_leak(new_buffer);
		next_mc =
		ISP_ADDR_ALIGN_UP(next_mc, ISP_FW_CMD_PAY_LOAD_BUF_ALIGN);
		if ((next_mc + pkg_size - base_mc) >
			ISP_FW_CMD_PAY_LOAD_BUF_SIZE) {
			isp_sys_mem_free(new_buffer);
			break;
		}

		i++;
		next_sys = base_sys + (next_mc - base_mc);
		new_buffer->mc_addr = next_mc;
		new_buffer->sys_addr = next_sys;
		new_buffer->next = NULL;
		if (l_isp_work_buf_mgr.free_cmd_pl_list == NULL) {
			l_isp_work_buf_mgr.free_cmd_pl_list = new_buffer;
		} else {
			tail_buffer = l_isp_work_buf_mgr.free_cmd_pl_list;
			while (tail_buffer->next != NULL)
				tail_buffer = tail_buffer->next;
			tail_buffer->next = new_buffer;
		}
		next_mc += pkg_size;
	}
	l_isp_work_buf_mgr.pay_load_num = i;
	print_fw_work_buf_info(&l_isp_work_buf_mgr);
	return &l_isp_work_buf_mgr;

fail:

	isp_fw_pl_list_destroy(l_isp_work_buf_mgr.free_cmd_pl_list);
	l_isp_work_buf_mgr.free_cmd_pl_list = NULL;
	return NULL;
};

void isp_fw_work_buf_unit(void *handle)
{
	struct isp_fw_work_buf_mgr *mgr = (struct isp_fw_work_buf_mgr *)handle;

	if (mgr == NULL)
		return;
	isp_fw_pl_list_destroy(mgr->free_cmd_pl_list);
	mgr->free_cmd_pl_list = NULL;
	isp_fw_pl_list_destroy(mgr->used_cmd_pl_list);
	mgr->used_cmd_pl_list = NULL;
}

int isp_fw_get_nxt_cmd_pl(void *hdl,
			unsigned long long *sys_addr,
			unsigned long long *mc_addr, unsigned int *len)
{
	struct isp_fw_work_buf_mgr *mgr = (struct isp_fw_work_buf_mgr *)hdl;
	struct isp_fw_cmd_pay_load_buf *temp;
	struct isp_fw_cmd_pay_load_buf *tail;
	int ret;

	if (mgr == NULL) {
		ISP_PR_ERR("fail for bad para\n");
		return RET_INVALID_PARAM;
	}

	isp_mutex_lock(&mgr->mutex);
	if (mgr->free_cmd_pl_list == NULL) {
		ISP_PR_ERR("fail for no free\n");
		ret = RET_FAILURE;
		goto quit;
	} else {
		temp = mgr->free_cmd_pl_list;
		mgr->free_cmd_pl_list = temp->next;
		temp->next = NULL;
		if (sys_addr)
			*sys_addr = temp->sys_addr;
		if (mc_addr)
			*mc_addr = temp->mc_addr;
		if (len)
			*len = mgr->pay_load_pkg_size;
		if (mgr->used_cmd_pl_list == NULL) {
			mgr->used_cmd_pl_list = temp;
		} else {
			tail = mgr->used_cmd_pl_list;
			while (tail->next != NULL)
				tail = tail->next;
			tail->next = temp;
		};
		ret = RET_SUCCESS;
		goto quit;
	}
quit:
	isp_mutex_unlock(&mgr->mutex);
	return ret;
}

int isp_fw_ret_pl(void *hdl, unsigned long long mc_addr)
{
	struct isp_fw_work_buf_mgr *mgr = (struct isp_fw_work_buf_mgr *)hdl;
	struct isp_fw_cmd_pay_load_buf *temp = NULL;
	struct isp_fw_cmd_pay_load_buf *tail;
	int ret;

	if (mgr == NULL) {
		ISP_PR_ERR("fail for bad para\n");
		return RET_INVALID_PARAM;
	}
	if (mgr->used_cmd_pl_list == NULL) {
		ISP_PR_ERR("fail for no used list\n");
		return RET_FAILURE;
	}
	isp_mutex_lock(&mgr->mutex);
	if (mgr->used_cmd_pl_list->mc_addr == mc_addr) {
		temp = mgr->used_cmd_pl_list;
		mgr->used_cmd_pl_list = temp->next;
	} else {
		tail = mgr->used_cmd_pl_list;
		while (tail->next != NULL) {
			if (tail->next->mc_addr != mc_addr) {
				tail = tail->next;
			} else {
				temp = tail->next;
				tail->next = temp->next;
				break;
			};
		};
		if (temp == NULL) {
			ISP_PR_ERR("fail for no used found\n");
			ret = RET_FAILURE;
			goto quit;
		}
	};
	temp->next = mgr->free_cmd_pl_list;
	mgr->free_cmd_pl_list = temp;
	ret = RET_SUCCESS;
quit:
	isp_mutex_unlock(&mgr->mutex);
	return ret;
}

void isp_fw_buf_get_code_base(void *hdl,
			unsigned long long *sys_addr,
			unsigned long long *mc_addr, unsigned int *len)
{
	unsigned int offset;

	if (hdl == NULL)
		return;
	offset = 0;
	if (len)
		*len = ISP_FW_CODE_BUF_SIZE;
	if (sys_addr)
		*sys_addr =
			((struct isp_fw_work_buf_mgr *)hdl)->sys_base + offset;
	if (mc_addr)
		*mc_addr =
			((struct isp_fw_work_buf_mgr *)hdl)->mc_base + offset;
};

void isp_fw_buf_get_stack_base(void *hdl,
			 unsigned long long *sys_addr,
			 unsigned long long *mc_addr, unsigned int *len)
{
	unsigned int offset;

	if (hdl == NULL)
		return;
	offset = ISP_FW_CODE_BUF_SIZE;
	if (len)
		*len = ISP_FW_STACK_BUF_SIZE;
	if (sys_addr)
		*sys_addr =
			((struct isp_fw_work_buf_mgr *)hdl)->sys_base + offset;
	if (mc_addr)
		*mc_addr =
			((struct isp_fw_work_buf_mgr *)hdl)->mc_base + offset;
};

void isp_fw_buf_get_heap_base(void *hdl,
			unsigned long long *sys_addr,
			unsigned long long *mc_addr, unsigned int *len)
{
	unsigned int offset;

	if (hdl == NULL)
		return;
	offset = ISP_FW_CODE_BUF_SIZE + ISP_FW_STACK_BUF_SIZE;
	if (len)
		*len = ISP_FW_HEAP_BUF_SIZE;
	if (sys_addr)
		*sys_addr =
			((struct isp_fw_work_buf_mgr *)hdl)->sys_base + offset;
	if (mc_addr)
		*mc_addr =
			((struct isp_fw_work_buf_mgr *)hdl)->mc_base + offset;
};

void isp_fw_buf_get_trace_base(void *hdl,
			unsigned long long *sys_addr,
			unsigned long long *mc_addr, unsigned int *len)
{
	unsigned int offset;

	if (hdl == NULL)
		return;
	offset =
	ISP_FW_CODE_BUF_SIZE + ISP_FW_STACK_BUF_SIZE + ISP_FW_HEAP_BUF_SIZE;
	if (len)
		*len = ISP_FW_TRACE_BUF_SIZE;
	if (sys_addr)
		*sys_addr =
		((struct isp_fw_work_buf_mgr *)hdl)->sys_base + offset;
	if (mc_addr)
		*mc_addr =
		((struct isp_fw_work_buf_mgr *)hdl)->mc_base + offset;
};

void isp_fw_buf_get_cmd_base(void *hdl,
			enum fw_cmd_resp_stream_id id,
			unsigned long long *sys_addr,
			unsigned long long *mc_addr, unsigned int *len)
{
	unsigned int offset;
	unsigned int idx;

	if (hdl == NULL)
		return;
	switch (id) {
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		idx = 0;
		break;
	case FW_CMD_RESP_STREAM_ID_1:
		idx = 1;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		idx = 2;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		idx = 3;
		break;
	default:
		ISP_PR_ERR("invaled id %d\n", id);

		if (len)
			*len = 0;
		return;
	}

	offset =
		ISP_FW_CODE_BUF_SIZE + ISP_FW_STACK_BUF_SIZE +
		ISP_FW_HEAP_BUF_SIZE + ISP_FW_TRACE_BUF_SIZE +
		ISP_FW_CMD_BUF_SIZE * idx;

	if (len)
		*len = ISP_FW_CMD_BUF_SIZE;
	if (sys_addr)
		*sys_addr =
		((struct isp_fw_work_buf_mgr *)hdl)->sys_base + offset;
	if (mc_addr)
		*mc_addr =
		((struct isp_fw_work_buf_mgr *)hdl)->mc_base + offset;
};

void isp_fw_buf_get_resp_base(void *hdl,
			enum fw_cmd_resp_stream_id id,
			unsigned long long *sys_addr,
			unsigned long long *mc_addr, unsigned int *len)
{
	unsigned int offset;
	unsigned int idx;

	if (hdl == NULL)
		return;
	switch (id) {
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		idx = 0;
		break;
	case FW_CMD_RESP_STREAM_ID_1:
		idx = 1;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		idx = 2;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		idx = 3;
		break;
	default:
		ISP_PR_ERR("invaled id %d\n", id);

		if (len)
			*len = 0;
		return;
	}

	offset =
		ISP_FW_CODE_BUF_SIZE + ISP_FW_STACK_BUF_SIZE +
		ISP_FW_HEAP_BUF_SIZE + ISP_FW_TRACE_BUF_SIZE +
		ISP_FW_CMD_BUF_SIZE * ISP_FW_CMD_BUF_COUNT +
		ISP_FW_RESP_BUF_SIZE * idx;

	if (len)
		*len = ISP_FW_RESP_BUF_SIZE;	/*ISP_FW_RESP_BUF_SIZE; */
	if (sys_addr)
		*sys_addr =
		((struct isp_fw_work_buf_mgr *)hdl)->sys_base + offset;
	if (mc_addr)
		*mc_addr =
		((struct isp_fw_work_buf_mgr *)hdl)->mc_base + offset;
};

void isp_fw_buf_get_cmd_pl_base(void *hdl,
				unsigned long long *sys_addr,
				unsigned long long *mc_addr, unsigned int *len)
{
	unsigned int offset;

	if (hdl == NULL)
		return;
	offset =
		ISP_FW_CODE_BUF_SIZE + ISP_FW_STACK_BUF_SIZE +
		ISP_FW_HEAP_BUF_SIZE + ISP_FW_TRACE_BUF_SIZE +
		ISP_FW_CMD_BUF_SIZE * ISP_FW_CMD_BUF_COUNT +
		ISP_FW_RESP_BUF_SIZE * ISP_FW_RESP_BUF_COUNT;

	if (len)
		*len = ISP_FW_CMD_PAY_LOAD_BUF_SIZE;
	if (sys_addr)
		*sys_addr =
		((struct isp_fw_work_buf_mgr *)hdl)->sys_base + offset;
	if (mc_addr)
		*mc_addr =
		((struct isp_fw_work_buf_mgr *)hdl)->mc_base + offset;
};

unsigned long long isp_fw_buf_get_sys_from_mc(
	void *hdl, unsigned long long mc_addr)
{
	struct isp_fw_work_buf_mgr *mgr;
	unsigned long long offset;

	if (hdl == NULL)
		return 0;
	mgr = (struct isp_fw_work_buf_mgr *)hdl;
	if (mc_addr < mgr->mc_base)
		return 0;
	offset = mc_addr - mgr->mc_base;
	if (offset >= ISP_FW_WORK_BUF_SIZE)
		return 0;
	return mgr->sys_base + offset;
}

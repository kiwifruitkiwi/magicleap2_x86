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

#include "isp_common.h"
#include "isp_mc_addr_mgr.h"
#include "log.h"
#include "isp_fw_if.h"
#include "isp_soc_adpt.h"

struct isp_fw_cmd_pay_load_buf {
	uint64 sys_addr;
	uint64 mc_addr;
	struct isp_fw_cmd_pay_load_buf *next;
};

struct isp_fw_work_buf_mgr {
	uint64 sys_base;
	uint64 mc_base;
	uint32 pay_load_pkg_size;
	uint32 pay_load_num;
	isp_mutex_t mutex;
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
	uint64 sys_addr;
	uint64 mc_addr;
	uint32 len;

	if (hdl) {
		/*struct isp_fw_cmd_pay_load_buf *next; */
		isp_fw_buf_get_code_base(hdl, &sys_addr, &mc_addr, &len);
		isp_dbg_print_info
		("isp_fw_buf_get_code_base sys:%llx, mc:%llx,len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_fw_buf_get_stack_base(hdl, &sys_addr, &mc_addr, &len);
		isp_dbg_print_info
		("isp_fw_buf_get_stack_base sys:%llx, mc:%llx,len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_fw_buf_get_heap_base(hdl, &sys_addr, &mc_addr, &len);
		isp_dbg_print_info
		("isp_fw_buf_get_heap_base sys:%llx, mc:%llx,len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_fw_buf_get_trace_base(hdl, &sys_addr, &mc_addr, &len);
		isp_dbg_print_info
		("isp_fw_buf_get_trace_base:%llx, mc:%llx,len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_fw_buf_get_cmd_base(hdl, 0, &sys_addr, &mc_addr, &len);
		isp_dbg_print_info
		("isp_fw_buf_get_cmd_base:%llx, mc:%llx,len:%d\n",
			sys_addr, mc_addr, len);

		isp_fw_buf_get_resp_base(hdl, 0, &sys_addr, &mc_addr, &len);
		isp_dbg_print_info
		("isp_fw_buf_get_resp_cmd_base:%llx, mc:%llx,len:%d\n",
			sys_addr, mc_addr, len);

		isp_fw_buf_get_cmd_pl_base(hdl, &sys_addr, &mc_addr, &len);
		isp_dbg_print_info
		("isp_fw_buf_get_cmd_pl_base:%llx,mc:%llx,total len:%dk\n",
			sys_addr, mc_addr, len / 1024);
		isp_dbg_print_info("cmd_pl num:%u, each size:%d\n",
			hdl->pay_load_num, hdl->pay_load_pkg_size);
	}
};

isp_fw_work_buf_handle isp_fw_work_buf_init(uint64 sys_addr, uint64 mc_addr)
{
	uint32 pkg_size;
	uint64 base_sys;
	uint64 base_mc;
	uint64 next_sys;
	uint64 next_mc;
	uint32 pl_len;
	uint32 i;
	struct isp_fw_cmd_pay_load_buf *new_buffer;
	struct isp_fw_cmd_pay_load_buf *tail_buffer;
	int32 pl_size;

	memset(&l_isp_work_buf_mgr, 0, sizeof(struct isp_fw_work_buf_mgr));
	l_isp_work_buf_mgr.sys_base = sys_addr;
	l_isp_work_buf_mgr.mc_base = mc_addr;
	l_isp_work_buf_mgr.pay_load_pkg_size = isp_get_cmd_pl_size();
	isp_dbg_print_info("sizeof Mode3FrameInfo_t %lu\n",
			sizeof(Mode3FrameInfo_t));
	isp_mutex_init(&l_isp_work_buf_mgr.mutex);
	isp_dbg_print_info("-> %s\n", __func__);
	pkg_size = l_isp_work_buf_mgr.pay_load_pkg_size;

	pl_size = ISP_FW_CMD_PAY_LOAD_BUF_SIZE;
	if ((pl_size <= 0) || ((pl_size / pkg_size) < 10)) {
		isp_dbg_print_info
			("<- %s fail pl_size %i\n", __func__, pl_size);
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
			isp_dbg_print_err
				("-><- %s fail for alloc\n", __func__);
			goto fail;
		};

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
	isp_dbg_print_info("-> %s\n", __func__);
	print_fw_work_buf_info(&l_isp_work_buf_mgr);
	isp_dbg_print_info("<- %s suc\n", __func__);
	return &l_isp_work_buf_mgr;

fail:

	isp_fw_pl_list_destroy(l_isp_work_buf_mgr.free_cmd_pl_list);
	l_isp_work_buf_mgr.free_cmd_pl_list = NULL;
	return NULL;
};

void isp_fw_work_buf_unit(isp_fw_work_buf_handle handle)
{
	struct isp_fw_work_buf_mgr *mgr = (struct isp_fw_work_buf_mgr *)handle;

	if (mgr == NULL)
		return;
	isp_fw_pl_list_destroy(mgr->free_cmd_pl_list);
	mgr->free_cmd_pl_list = NULL;
	isp_fw_pl_list_destroy(mgr->used_cmd_pl_list);
	mgr->used_cmd_pl_list = NULL;
}

result_t isp_fw_get_nxt_cmd_pl(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len)
{
	struct isp_fw_work_buf_mgr *mgr = (struct isp_fw_work_buf_mgr *)hdl;
	struct isp_fw_cmd_pay_load_buf *temp;
	struct isp_fw_cmd_pay_load_buf *tail;
	result_t ret;

	if (mgr == NULL) {
		isp_dbg_print_err
		("-><- %s fail for bad para\n", __func__);
		return RET_INVALID_PARAM;
	}

	isp_mutex_lock(&mgr->mutex);
	if (mgr->free_cmd_pl_list == NULL) {
		isp_dbg_print_err
		("-><- %s fail for no free\n", __func__);
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

result_t isp_fw_ret_pl(isp_fw_work_buf_handle hdl, uint64 mc_addr)
{
	struct isp_fw_work_buf_mgr *mgr = (struct isp_fw_work_buf_mgr *)hdl;
	struct isp_fw_cmd_pay_load_buf *temp = NULL;
	struct isp_fw_cmd_pay_load_buf *tail;
	result_t ret;

	if (mgr == NULL) {
		isp_dbg_print_err("-><- %s fail for bad para\n", __func__);
		return RET_INVALID_PARAM;
	}
	if (mgr->used_cmd_pl_list == NULL) {
		isp_dbg_print_err("-><- %s fail for no used list\n", __func__);
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
			isp_dbg_print_err
			("-><- %s fail for no used found\n", __func__);
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

void isp_fw_buf_get_code_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len)
{
	uint32 offset;

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

void isp_fw_buf_get_stack_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			 uint64 *mc_addr, uint32 *len)
{
	uint32 offset;

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

void isp_fw_buf_get_heap_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len)
{
	uint32 offset;

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

void isp_fw_buf_get_trace_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len)
{
	uint32 offset;

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

void isp_fw_buf_get_cmd_base(isp_fw_work_buf_handle hdl,
			enum fw_cmd_resp_stream_id id,
			uint64 *sys_addr, uint64 *mc_addr, uint32 *len)
{
	uint32 offset;
	uint32 idx;

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
		isp_dbg_print_err
		("-><- %s fail, bad id %d\n", __func__, id);
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

void isp_fw_buf_get_resp_base(isp_fw_work_buf_handle hdl,
			enum fw_cmd_resp_stream_id id,
			uint64 *sys_addr, uint64 *mc_addr, uint32 *len)
{
	uint32 offset;
	uint32 idx;

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
		isp_dbg_print_err
		("-><- %s fail, bad id %d\n", __func__, id);
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

void isp_fw_buf_get_cmd_pl_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
				uint64 *mc_addr, uint32 *len)
{
	uint32 offset;

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

uint64 isp_fw_buf_get_sys_from_mc(isp_fw_work_buf_handle hdl, uint64 mc_addr)
{
	struct isp_fw_work_buf_mgr *mgr;
	uint64 offset;

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

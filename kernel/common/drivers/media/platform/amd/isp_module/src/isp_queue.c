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
 *************************************************************************
 */

#include "isp_common.h"
#include "isp_queue.h"
#include "log.h"
#include "isp_soc_adpt.h"

result_t isp_list_init(struct isp_list *list)
{
	if (list == NULL)
		return RET_FAILURE;
	isp_mutex_init(&list->mutext);
	list->front =
	(struct list_node *)isp_sys_mem_alloc(sizeof(struct list_node));
	if (list->front == NULL)
		return RET_FAILURE;

	list->rear = list->front;
	list->front->next = NULL;
	list->count = 0;
	return RET_SUCCESS;
}

result_t isp_list_destroy(struct isp_list *list, func_node_process func)
{
	struct list_node *p;

	if (list == NULL)
		return RET_FAILURE;
	isp_mutex_lock(&list->mutext);
	while (list->front != list->rear) {
		p = list->front->next;
		list->front->next = p->next;
		if (list->rear == p)
			list->rear = list->front;
		if (func)
			func(p);
	}
	isp_sys_mem_free(list->front);
	list->front = NULL;
	list->rear = NULL;
	isp_mutex_unlock(&list->mutext);

	isp_mutex_destroy(&list->mutext);

	return RET_SUCCESS;
};

result_t isp_list_insert_tail(struct isp_list *list, struct list_node *p)
{
	if (list == NULL)
		return RET_FAILURE;
	isp_mutex_lock(&list->mutext);

	list->rear->next = p;
	list->rear = p;
	p->next = NULL;
	list->count++;
	isp_mutex_unlock(&list->mutext);
	return RET_SUCCESS;
}

struct list_node *isp_list_get_first(struct isp_list *list)
{
	struct list_node *p;

	if (list == NULL)
		return NULL;

	isp_mutex_lock(&list->mutext);
	if (list->front == list->rear) {
		if (list->count)
			isp_dbg_print_err
			("%s fail bad count %u\n",
			 __func__, list->count);
		isp_mutex_unlock(&list->mutext);
		return NULL;
	}

	p = list->front->next;
	list->front->next = p->next;
	if (list->rear == p)
		list->rear = list->front;
	if (list->count)
		list->count--;
	else
		isp_dbg_print_err("%s fail bad 0 count\n", __func__);
	isp_mutex_unlock(&list->mutext);
	return p;
}

struct list_node *isp_list_get_first_without_rm(struct isp_list *list)
{
	struct list_node *p;

	if (list == NULL)
		return NULL;

	isp_mutex_lock(&list->mutext);
	if (list->front == list->rear) {
		isp_mutex_unlock(&list->mutext);
		return NULL;
	}

	p = list->front->next;
	isp_mutex_unlock(&list->mutext);
	return p;
}

void isp_list_rm_node(struct isp_list *list, struct list_node *node)
{
	struct list_node *p;
	struct list_node *pre;

	if ((list == NULL) || (node == NULL))
		return;
	isp_mutex_lock(&list->mutext);
	if (list->front == list->rear) {
		isp_mutex_unlock(&list->mutext);
		return;
	}

	p = list->front->next;
	if (p == NULL) {
		isp_dbg_print_err
		("-><- %s, fail cannot find node1\n", __func__);
		return;
	}
	if (p == node) {
		list->front->next = p->next;
		if (list->rear == p)
			list->rear = list->front;
	} else {
		pre = p;
		p = p->next;
		while (p != NULL) {
			if (p == node) {
				pre->next = p->next;
				if (list->rear == p)
					list->rear = pre;
				break;
			}
		}
		if (p == NULL) {
			isp_dbg_print_err
			("-><- %s, fail cannot find node\n", __func__);
		}
	}

	isp_mutex_unlock(&list->mutext);
}

uint32 isp_list_get_cnt(struct isp_list *list)
{
	int i = 0;
	struct list_node *p;

	if (list == NULL)
		return 0;
	isp_mutex_lock(&list->mutext);
	p = list->front;
	while (p != list->rear) {
		++i;
		p = p->next;
	}
	isp_mutex_unlock(&list->mutext);
	return i;
}

result_t isp_spin_list_init(struct isp_spin_list *list)
{
	if (list == NULL)
		return RET_FAILURE;

	//isp_spin_lock_init(&list->lock);

	list->front =
	(struct list_node *)isp_sys_mem_alloc(sizeof(struct list_node));
	if (list->front == NULL)
		return RET_FAILURE;

	list->rear = list->front;
	list->front->next = NULL;

	return RET_SUCCESS;
}

result_t isp_spin_list_destroy(struct isp_spin_list *list,
			 func_node_process func)
{
	struct list_node *p;

	if (list == NULL)
		return RET_FAILURE;
	isp_spin_lock_lock((list->lock));
	while (list->front != list->rear) {
		p = list->front->next;
		list->front->next = p->next;
		if (list->rear == p)
			list->rear = list->front;
		if (func)
			func(p);
	}
	isp_sys_mem_free(list->front);
	list->front = NULL;
	list->rear = NULL;
	isp_spin_lock_unlock(list->lock);

	return RET_SUCCESS;
};

result_t isp_spin_list_insert_tail(struct isp_spin_list *list,
				 struct list_node *p)
{
	if (list == NULL)
		return RET_FAILURE;
	isp_spin_lock_lock(list->lock);

	list->rear->next = p;
	list->rear = p;
	p->next = NULL;
	isp_spin_lock_unlock(list->lock);
	return RET_SUCCESS;
}

struct list_node *isp_spin_list_get_first(struct isp_spin_list *list)
{
	struct list_node *p;

	if (list == NULL)
		return NULL;

	isp_spin_lock_lock(list->lock);
	if (list->front == list->rear) {
		isp_spin_lock_unlock(list->lock);
		return NULL;
	}

	p = list->front->next;
	list->front->next = p->next;
	if (list->rear == p)
		list->rear = list->front;
	isp_spin_lock_unlock(list->lock);
	return p;
}

void isp_spin_list_rm_node(struct isp_spin_list *list, struct list_node *node)
{
	struct list_node *p;
	struct list_node *pre;

	if ((list == NULL) || (node == NULL))
		return;
	isp_spin_lock_lock(list->lock);
	if (list->front == list->rear) {
		isp_spin_lock_unlock(list->lock);
		return;
	}

	p = list->front->next;
	if (p == node) {
		list->front->next = p->next;
		if (list->rear == p)
			list->rear = list->front;
	} else {
		pre = p;
		p = p->next;
		while (p != NULL) {
			if (p == node) {
				pre->next = p->next;
				if (list->rear == p)
					list->rear = pre;
				break;
			}
		}
		if (p == NULL) {
			isp_dbg_print_err
			("-><- %s, cannot find node\n", __func__);
		}
	}

	isp_spin_lock_unlock(list->lock);
}

uint32 isp_spin_list_get_cnt(struct isp_spin_list *list)
{
	int i = 0;
	struct list_node *p;

	if (list == NULL)
		return 0;
	isp_spin_lock_lock(list->lock);
	p = list->front;
	while (p != list->rear) {
		++i;
		p = p->next;
	}
	isp_spin_lock_unlock(list->lock);
	return i;
}

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

#ifndef ISP_QUEUE_H
#define ISP_QUEUE_H

struct list_node {
	struct list_node *next;
};

struct isp_list {
	struct list_node *front;
	struct list_node *rear;
	isp_mutex_t mutext;
	uint32 count;
};

struct isp_spin_list {
	struct list_node *front;
	struct list_node *rear;
	struct isp_spin_lock lock;
};

typedef void (*func_node_process) (struct list_node *pnode);

result_t isp_list_init(struct isp_list *list);
result_t isp_list_destroy(struct isp_list *list, func_node_process func);
result_t isp_list_insert_tail(struct isp_list *list, struct list_node *node);
void isp_list_rm_node(struct isp_list *list, struct list_node *node);
struct list_node *isp_list_get_first(struct isp_list *list);
struct list_node *isp_list_get_first_without_rm(struct isp_list *list);
uint32 isp_list_get_cnt(struct isp_list *list);

result_t isp_spin_list_init(struct isp_spin_list *list);
result_t isp_spin_list_destroy(struct isp_spin_list *list,
			func_node_process func);
result_t isp_spin_list_insert_tail(struct isp_spin_list *list,
			struct list_node *node);
void isp_spin_list_rm_node(struct isp_spin_list *list, struct list_node *node);
struct list_node *isp_spin_list_get_first(struct isp_spin_list *list);
uint32 isp_spin_list_get_cnt(struct isp_spin_list *list);

#endif

/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef ISP_QUEUE_H
#define ISP_QUEUE_H

struct list_node {
	struct list_node *next;
};

struct isp_list {
	struct list_node *front;
	struct list_node *rear;
	struct mutex mutext;
	unsigned int count;
};

struct isp_spin_list {
	struct list_node *front;
	struct list_node *rear;
	struct isp_spin_lock lock;
};

typedef void (*func_node_process) (struct list_node *pnode);

int isp_list_init(struct isp_list *list);
int isp_list_destroy(struct isp_list *list, func_node_process func);
int isp_list_insert_tail(struct isp_list *list, struct list_node *node);
void isp_list_rm_node(struct isp_list *list, struct list_node *node);
struct list_node *isp_list_get_first(struct isp_list *list);
struct list_node *isp_list_get_first_without_rm(struct isp_list *list);
unsigned int isp_list_get_cnt(struct isp_list *list);

int isp_spin_list_init(struct isp_spin_list *list);
int isp_spin_list_destroy(struct isp_spin_list *list,
			func_node_process func);
int isp_spin_list_insert_tail(struct isp_spin_list *list,
			struct list_node *node);
void isp_spin_list_rm_node(struct isp_spin_list *list, struct list_node *node);
struct list_node *isp_spin_list_get_first(struct isp_spin_list *list);
unsigned int isp_spin_list_get_cnt(struct isp_spin_list *list);

#endif

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

#ifndef DW_COMMON_LIST_H
#define DW_COMMON_LIST_H

/*INLINE definition for the ARM C compiler*/
/*#define INLINE inline*/
/*#define INLINE inline*/
#define INLINE

/****h*include.list
 *NAME
 *dw_list.h -- simple double linked list implementation
 *DESCRIPTION
 *some of the internal functions ("__dw_xxx") are useful when
 *manipulating whole lists rather than single entries, as
 *sometimes we already know the next/prev entries and we can
 *generate better code by using them directly rather than
 *using the generic single-entry routines.
 ***/

/****is*include.dw_list_head
 *DESCRIPTION
 *this is the structure used for managing linked lists.
 *SOURCE
 */
struct dw_list_head {
	struct dw_list_head *next, *prev;
};
/*****/

#define DW_LIST_HEAD_INIT(name) { &(name), &(name) }

#define DW_LIST_HEAD(name) \
	struct dw_list_head name = DW_LIST_HEAD_INIT(name)

#define DW_INIT_LIST_HEAD(ptr)	\
do {	\
	(ptr)->next = (ptr); (ptr)->prev = (ptr);	\
} while (0)

	/****if*include.list/__dw_list_add
	 *DESCRIPTION
	 *insert a new entry between two known consecutive entries.
	 *this is only for internal list manipulation where we know
	 *the prev/next entries already!
	 *ARGUMENTS
	 *new	element to insert
	 *prev	previous entry
	 *next	next entry
	 *SOURCE
	 */
/*
 *static INLINE void __dw_list_add(struct dw_list_head *new_node, struct
 *	dw_list_head *prev, struct dw_list_head *next)
 *{
 *	next->prev = new_node;
 *	new_node->next = next;
 *	new_node->prev = prev;
 *	prev->next = new_node;
 *}
 */
/*****/

	/****f*include.list/dw_list_add
	 *DESCRIPTION
	 *insert a new entry after the specified head.
	 *this is good for implementing stacks.
	 *ARGUMENTS
	 *new	 new entry to be added
	 *head	list head to add it after
	 *SOURCE
	 */
/*
 *static INLINE void dw_list_add(struct dw_list_head *new_node, struct
 *dw_list_head *head)
 *{
 *	__dw_list_add(new_node, head, head->next);
 *}
 */
/*****/

	/****f*include.list/dw_list_add_tail
	 *DESCRIPTION
	 *insert a new entry before the specified head.
	 *this is useful for implementing queues.
	 *ARGUMENTS
	 *new	new entry to be added
	 *head	list head to add it before
	 *SOURCE
	 */
/*
 *static INLINE void dw_list_add_tail
 *(struct dw_list_head *new_node, struct
 *dw_list_head *head)
 *{
 *	__dw_list_add(new_node, head->prev, head);
 *}
 */
/*****/

	/****if*include.list/__dw_list_del
	 *DESCRIPTION
	 *delete a list entry by making the prev/next entries point to each
	 *other.this is only for internal list manipulation where we know
	 *the prev/next entries already!
	 *ARGUMENTS
	 *prev	previous entry
	 *next	next entry
	 *SOURCE
	 */
/*
 *static INLINE void __dw_list_del(struct dw_list_head *prev, struct
 *dw_list_head *next)
 *{
 *	next->prev = prev;
 *	prev->next = next;
 *}
 */
/*****/

	/****f*include.list/dw_list_del
	 *DESCRIPTION
	 *deletes entry from list.
	 *ARGUMENTS
	 *entry the element to delete from the list
	 *NOTES
	 *list_empty on entry does not return true after this, the entry
	 *is in an undefined state.
	 *SOURCE
	 */
/*
 *static INLINE void dw_list_del(struct dw_list_head *entry)
 *{
 *	__dw_list_del(entry->prev, entry->next);
 *}
 */
/*****/
	/****f*include.list/dw_list_del_init
	 *DESCRIPTION
	 *deletes entry from list and reinitializes it.
	 *ARGUMENTS
	 *entry the element to delete from the list
	 *SOURCE
	 */
/*
 *static INLINE void dw_list_del_init(struct dw_list_head *entry)
 *{
 *	__dw_list_del(entry->prev, entry->next);
 *	DW_INIT_LIST_HEAD(entry);
 *}
 */
/*****/

	/****f*include.list/dw_list_empty
	 *DESCRIPTION
	 *tests whether a list is empty.
	 *ARGUMENTS
	 *head	the list to test
	 *SOURCE
	 */
/*
 *static INLINE int dw_list_empty(struct dw_list_head *head)
 *{
 *	return head->next == head;
 *}
 */
/*****/

	/****f*include.list/dw_list_splice
	 *DESCRIPTION
	 *join two lists.
	 *ARGUMENTS
	 *list	the new list to add
	 *head	the place to add it in the first list
	 *SOURCE
	 */
/*
 *static INLINE void dw_list_splice
 *	(struct dw_list_head *list, struct dw_list_head *head)
 *{
.*	struct dw_list_head *first = list->next;
 *
 *	if (first != list) {
 *	struct dw_list_head *last = list->prev;
 *	struct dw_list_head *at = head->next;
 *
 *	first->prev = head;
 *	head->next = first;
 *
 *	last->next = at;
 *	at->prev = last;
 *	}
 *}
 */
/*****/

	/****d*include.list/dw_list_entry
	 *DESCRIPTION
	 *get the struct for this entry.
	 *ARGUMENTS
	 *ptr	 the &struct dw_list_head pointer
	 *type	the type of the struct this is embedded in
	 *memberthe name of the list_struct within the struct
	 *SOURCE
	 */
#define DW_LIST_ENTRY(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))
/*****/

	/****d*include.list/dw_list_for_each
	 *DESCRIPTION
	 *iterate over a list.
	 *ARGUMENTS
	 *pos	 the &struct dw_list_head to use as a loop counter
	 *head	the head for your list
	 *SOURCE
	 */
#define DW_LIST_FOR_EACH(pos, head)	\
	for (pos = (head)->next; pos != (head); pos = pos->next)
/*****/

	/****d*include.list/dw_list_for_each_safe
	 *SYNOPSIS
	 *list_for_each_safe(pos, head)
	 *DESCRIPTION
	 *iterate over a list safe against removal of list entry.
	 *ARGUMENTS
	 *pos	 the &struct dw_list_head to use as a loop counter
	 *n	 another &struct dw_list_head to use as temporary storage
	 *head	the head for your list
	 *SOURCE
	 */
#define DW_LIST_FOR_EACH_SAFE(pos, n, head)	\
	for (pos = (head)->next, n = pos->next; pos != (head);	\
	pos = n, n = pos->next)
/*****/

	/****d*include.list/dw_list_for_each_prev
	 *DESCRIPTION
	 *iterate over a list in reverse order.
	 *ARGUMENTS
	 *pos		the &struct dw_list_head to use as a loop counter
	 *head		the head for your list
	 *SOURCE
	 */
#define DW_LIST_FOR_EACH_PREV(pos, head)	\
	for (pos = (head)->prev; pos != (head); pos = pos->prev)
/*****/

#endif				/*DW_COMMON_LIST_H */

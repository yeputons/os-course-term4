#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>
#include "util.h"

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)
#define LIST_ENTRY(ptr, type, member) CONTAINER_OF(ptr, type, member)

void list_init(struct list_head *head);
void list_add(struct list_head *new, struct list_head *head);
void list_add_tail(struct list_head *new, struct list_head *head);
void list_del(struct list_head *entry);
void list_splice(struct list_head *list, struct list_head *head);
bool list_empty(const struct list_head *head);
struct list_head *list_first(struct list_head *head);
size_t list_size(const struct list_head *head);

#endif /*__LIST_H__*/

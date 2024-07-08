#pragma once

#include <jp/types.h>
#include <jp/assert.h>

#define member_offset(type, member) ((u32)&(((type*)0)->member))
#define container_of(type, member, ptr) (type *)((u32)(ptr)-member_offset(type, member))

#define list_for_each(iter, head, member) for(iter=container_of(typeof(*iter), member, (head)->next); &((iter)->member) !=(head);iter=container_of(typeof(*iter), member, (iter)->member.next))

typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
} list_node_t;

#define LIST_INIT(head) (list_node_t){ \
    .prev = &head, \
    .next = &head, \
}

static inline void list_init(list_node_t *head)
{
    head->next = head->prev = head;
}

static bool list_empty(list_node_t *head)
{
    return head->prev == head;
}

static inline void list_insert_before(list_node_t *anchor, list_node_t *node)
{
    anchor->prev->next = node;
    node->prev = anchor->prev;
    node->next = anchor;
    anchor->prev = node;
}

static inline void list_insert_after(list_node_t *anchor, list_node_t *node)
{
    anchor->next->prev = node;
    node->next = anchor->next;
    node->prev = anchor;
    anchor->next = node;
}

static inline void list_add(list_node_t* head, list_node_t *node)
{
    if (list_empty(head)) {
        head->next = node;
        head->prev = node;
        node->prev = head;
        node->next = head;
        return;
    }
    list_insert_after(head, node);    
}


static inline void list_del(list_node_t *node)
{
    assert(node != node->next);
    node->prev->next = node->next;
    node->next->prev = node->prev;
    list_init(node);
}


static inline list_node_t *list_pop(list_node_t *head)
{
    if (list_empty(head)) {
        return NULL;
    }
    list_node_t *ret = head->next;
    list_del(ret);
    return ret;
}

static inline void list_tail_add(list_node_t *head, list_node_t *node)
{    
    if (list_empty(head)) {
        head->next = node;
        head->prev = node;
        node->prev = head;
        node->next = head;
        return;
    }
    list_insert_before(head, node);
}

static inline list_node_t *list_tail_pop(list_node_t *head)
{
    if (list_empty(head)) {
        return NULL;
    }
    list_node_t *ret = head->prev;
    list_del(ret);
    return ret;
}

static inline bool list_search(list_node_t *head, list_node_t *t)
{
    list_node_t *now = head->next;
    while(now != head) {
        if (now == t)
            return true;
        now = now->next;
    }
    return false;
}

static u32 list_size(list_node_t *head)
{
    u32 size = 0;
    list_node_t *now = head->next;
    while(now != head) {
        size++;
        now = now->next;
    }
    return size;
}
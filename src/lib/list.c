#include <jp/list.h>
#include <jp/memory.h>
#include <jp/debug.h>
void list_test(void)
{
    list_node_t head = LIST_INIT(&head);
    u32 count=3;
    list_node_t *node;

    while (count--)
    {
        node = (list_node_t*)alloc_kpage(1);
        list_add(&head, node);
    }

    while(!list_empty(&head)) {
        node = list_pop(&head);
        free_kpage((u32)node, 1);
    }

    count=3;
    while (count--)
    {
        /* code */
        node = (list_node_t*)alloc_kpage(1);
        list_tail_add(&head, node);
    }
    DEBUGK("list size %d\n", list_size(&head));

    while (!list_empty(&head))
    {
        node = list_tail_pop(&head);
        free_kpage((u32)node, 1);
    }
    
    struct test {
        list_node_t node;
        int a;
    } node2 = {
        .a = 0x55aa55aa,
        .node = LIST_INIT(&node2.node),
    };
    list_tail_add(&head, &node2.node);

    DEBUGK("search node 0x%p -> %d\n", &node2.node, list_search(&head, &node2.node));
    DEBUGK("search node 0x%p -> %d\n", 0, list_search(&head, 0));

    struct test *t = container_of(struct test, node, &node2.node);
    list_for_each(t, &head, node) {
        DEBUGK("t->a = 0x%x\n", t->a);
    }
    list_del(&node2.node);
}
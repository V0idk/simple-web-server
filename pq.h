#ifndef _PQ_H_
#define _PQ_H_


#include <sys/types.h>
typedef int (*pq_compare)(void *node1, void *node2);

typedef struct {
    void **pq;
    size_t item_num;
    size_t capacity;
    pq_compare compare;
} priority_queue_t;

int pq_init(priority_queue_t *pq, pq_compare comp, size_t size);
int pq_is_empty(priority_queue_t *pq);
size_t pq_size(priority_queue_t *pq);
void *pq_top(priority_queue_t *pq);
int pq_pop(priority_queue_t *pq);
int pq_insert(priority_queue_t *pq, void *item);
int pq_sink(priority_queue_t *pq, size_t i);


#endif


#include "pq.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

int pq_init(priority_queue_t *pq, pq_compare compare, size_t size) {
    pq->pq = malloc( sizeof(void *) * (size+1) );//索引0不使用,代码方便
    if (!pq->pq) {
        return -1;
    }
    pq->item_num = 0;
    pq->capacity = size + 1;
    pq->compare = compare;
    return 0;
}

int pq_is_empty(priority_queue_t *pq) {
    return (pq->item_num == 0)? 1: 0;
}

size_t pq_size(priority_queue_t *pq) {
    return pq->item_num;
}

void *pq_top(priority_queue_t *pq) {
    if (pq_is_empty(pq)) {
        return NULL;
    }
    return pq->pq[1];
}

static int pq_resize(priority_queue_t *pq, size_t new_size) {
    if (new_size <= pq->item_num) {
        return -1;
    }
    void **new_pq = malloc(sizeof(void *) * new_size);
    if (!new_pq) {
        return -1;
    }
    memcpy(new_pq, pq->pq, sizeof(void *) * (pq->item_num + 1));
    free(pq->pq);
    pq->pq = new_pq;
    pq->capacity = new_size;
    return 0;
}

//交换
static void pq_swap(priority_queue_t *pq, size_t i, size_t j) {
    void *tmp = pq->pq[i];
    pq->pq[i] = pq->pq[j];
    pq->pq[j] = tmp;
}


static void pq_swim(priority_queue_t *pq, size_t k) {
    //例:如小顶堆, 若子节点较小,则上升.
    while (k > 1 && pq->compare(pq->pq[k], pq->pq[k/2])) {
        pq_swap(pq, k, k/2);
        k /= 2;
    }
}


static size_t pq_sink_k(priority_queue_t *pq, size_t k) {
    size_t j;
    while (2*k <= pq->item_num) {
        j = 2*k;
        //另j等于左右子节点较小者
        if (j+1 <= pq->item_num && pq->compare(pq->pq[j+1], pq->pq[j])) j++;
        //若已满足则退出
        if (pq->compare(pq->pq[k],pq->pq[j])) break;
        //否则交换节点
        pq_swap(pq, j, k);
        //下一个节点
        k = j;
    }
    return k;
}

int pq_pop(priority_queue_t *pq) {
    if (pq_is_empty(pq)) {
        return 0;
    }
    //交换尾部
    pq_swap(pq, 1, pq->item_num);
    pq->item_num--;
    //下沉尾部
    pq_sink_k(pq, 1);
    //适度缩减容量
    if (pq->item_num > 0 && pq->item_num <= (pq->capacity - 1)/4) {
        if (pq_resize(pq, pq->capacity / 2) < 0) {
            return -1;
        }
    }
    return 0;
}

int pq_insert(priority_queue_t *pq, void *item) {
    if (pq->item_num + 1 == pq->capacity) {
        if (pq_resize(pq, pq->capacity * 2) < 0) {
            return -1;
        }
    }
    //插入尾部上浮即可
    pq->pq[++pq->item_num] = item;
    pq_swim(pq, pq->item_num);
    return 0;
}


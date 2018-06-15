
#ifndef _THPOOL_
#define _THPOOL_

typedef struct thpool* threadpool;
threadpool thpool_init(int num_threads);
int thpool_add_task(threadpool, void (*function)(void*), void* arg);
#endif
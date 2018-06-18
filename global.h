#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <sys/epoll.h>
#include "pq.h"

extern struct epoll_event *events;
extern priority_queue_t server_timer;
extern int port;
extern char *dir;
extern int worker_num;

#endif

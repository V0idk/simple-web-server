#include "global.h"


struct epoll_event *events;
priority_queue_t server_timer;
int port = 3000;
char dir[1024] = "./";
int worker_num = 4;
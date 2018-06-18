#ifndef _FD_H_
#define _FD_H_

#include <sys/epoll.h>

#define MAXEVENTS 1024
int create_listenfd();
int creat_epollfd();
void epoll_add(int epfd, int fd, struct epoll_event *event);

int make_socket_non_blocking(int fd);
#endif

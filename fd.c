
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>


#include "fd.h"
#include "global.h"
#include "util.h"

typedef struct event_data_s{
    int fd;
}event_data_t;


//O_NONBLOCK, SO_REUSEADDR
int create_listenfd(){
    int listenfd;
    int optval=1;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        log_err("socket\n");

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        log_err("setsockopt\n");

    struct sockaddr_in serveraddr;
    //bzero非标准不赞成使用
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    if(bind(listenfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)))
        log_err("bind\n");

    //nginx 默认#define NGX_LISTEN_BACKLOG  511, 这里我们设置1023
    if (listen(listenfd, 1023) < 0)
        log_err("listen\n");

    int flags = fcntl(listenfd, F_GETFL);
    if (flags == -1)
        log_err("fcntl_get\n");
    if (fcntl(listenfd, F_SETFL, flags | O_NONBLOCK))
        log_err("fcntl_set\n");
    return listenfd;
}

int creat_epollfd(){
    //epoll_create(size) 这个函数的size参数已经器用。更推荐使用的是epoll_create1(0)来代替普通的用法
    int epollfd = epoll_create1(0);
    if(epollfd < 0)
        log_err("epoll_create1\n");
    /*设置一次性返回最大数量*/
    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    if(events == NULL)
        log_err("malloc");
    return  epollfd;
}

void epoll_add(int epfd, int fd, struct epoll_event *event) {
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event)!=0)
        log_err("epoll_add");
    return;
}

int make_socket_non_blocking(int fd) {
    int flags, s;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        log_err("fcntl");
        return -1;
    }
    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
        log_err("fcntl");
        return -1;
    }

    return 0;
}



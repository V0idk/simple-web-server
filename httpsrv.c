
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "fd.h"
#include "global.h"
#include "handle_request.h"
#include "http_request.h"
#include "picohttpparser.h"
#include "thpool.h"
#include "timer.h"
#include "util.h"


static void check_option(int argc, char *argv[]);
static void set_sig_handler(int sig,void (*func)(int));
static void epoll_add_fd(int epfd,int fd,int events, http_request_t *request );
static void clean_request(http_request_t *request);
static int worker_loop(void *ptr);

int main(int argc, char *argv[])
{
    //参数设置
    check_option(argc,argv);
    //屏蔽SIGPIPE,意外断开的客户端写入将产生SIGPIPE导致服务端进程终止
    set_sig_handler(SIGPIPE,SIG_IGN);
    //创建监听描述符:SO_REUSEADDR,非阻塞
    int listenfd = create_listenfd();
    //创建epoll并添加监听描述符
    int epfd = creat_epollfd();
    //添加监听描述符到epfd
    http_request_t *request = (http_request_t *)malloc(sizeof(http_request_t));
    epoll_add_fd(epfd,listenfd, EPOLLIN | EPOLLET,request);
    //线程池: 默认4 worker
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    clilen = sizeof(struct sockaddr_in);
    threadpool thpool = thpool_init(worker_num);
    //初始化timer
    timer_init();
    //事件循环
    for (;;)
    {
        size_t timeout = get_timewait();
        int n = epoll_wait(epfd, events, MAXEVENTS,timeout);
        //若存在可读描述符, 遍历描述符
        for (int i = 0; i < n; i++)
        {
            http_request_t *r = (http_request_t *)events[i].data.ptr;
            int fd = r->fd;
            /* 如果是监听描述符中来了新的连接,则一一连接,并将连接描述符添加到epoll中;          
            */
            if (listenfd == fd)
            {
                int infd;
                while (1)
                {
                    infd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                    if (infd < 0)
                    {
                        //EAGAIN,EWOULDBLOCK属于本应阻塞错误, 可重试.
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))  break;
                        log_warns("accept");//too many open files
                        break;
                    }
                    make_socket_non_blocking(infd);
                    http_request_t *request = (http_request_t *)malloc(sizeof(http_request_t));
                    epoll_add_fd(epfd,infd, EPOLLIN | EPOLLET | EPOLLONESHOT, request);
                    add_timer(request, TIMEOUT_DEFAULT, http_expire_handler);
                } // end of while of accept
            }
            /*  若是已连接描述符有了可读事件, 则将任务添加到任务队列中 */
            else
            {
                
                if ((events[i].events & EPOLLERR) || //发生错误
                    (events[i].events & EPOLLHUP) || //
                    (!(events[i].events & EPOLLIN)))
                {
                    log_warns("epoll error fd: %d", r->fd);
                    Close(fd);
                    continue;
                }
                del_timer(events[i].data.ptr);
                thpool_add_task(thpool, (void (*)(void *))worker_loop, events[i].data.ptr);
            }
        } 
    }
    return 0;
}

static void check_option(int argc, char *argv[]){
    int opt;
    struct option long_option[] = {
            {"help", no_argument, NULL, 'h'},
            {"port", required_argument,NULL,'p'},
            {NULL, 0, NULL, 0}};

    while ((opt = getopt_long(argc, argv, "hp:d:t:", long_option, NULL)) != -1)
    {
        switch (opt)
        {
            case 'h':
                printf("\033[31m USAGE: \033[0m\n" 
                       "\t-p PORT\n"
                       "-d DIR"
                );
                exit(1);
            case 'd':
                printf("working dir:%s\n",optarg);
                dir = optarg+1;
                // printf("%s;%ld\n",dir,strlen(dir));
                break;
            case 'p':
                printf("setting port:%s\n",optarg);
                port = atoi(optarg);
                break;
            case 't':
                printf("worker number:%s\n",optarg);
                worker_num = atoi(optarg);
                break;
            default:
                exit(1);
        }
    }
    if (optind < argc)
    {
        printf("non-option: ");
        while (optind < argc)
        {
            printf("%s", argv[optind++]);
        }
        putchar ('\n');
        exit(1);
    }
}

static void set_sig_handler(int sig,void (*func)(int)){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = func;
    sa.sa_flags = 0;
    if (sigaction(sig, &sa, NULL))
        log_err("err sigaction\n");
}


static void epoll_add_fd(int epfd,int fd,int events, http_request_t *request ){
    struct epoll_event event_temp;
    if (request == NULL)
    {
        log_err("malloc(sizeof(hs_http_request_t))");
    }
    request->fd = fd;
    request->keeplive = 0;
    request->epfd = epfd;
    event_temp.data.ptr = (void *)request;
    event_temp.events = events;
    //添加该listenfd到epoll
    epoll_add(epfd,fd,&event_temp);
    return;
}

void clean_request(http_request_t *request){
    Close(request->fd);
    free(request->timer);
    free(request);    
}



/*************处理函数******************/

static int worker_loop(void *ptr){

#ifndef NDEBUG
    printf("threadID: %ld\n",(unsigned long int)pthread_self());
#endif

    http_request_t *request = (http_request_t *)ptr;
    request->buflen=0;
    request->prevbuflen=0;
    request->finish = 0;
    ssize_t rret;

    while (1) {
        /* 读取该请求 */
        while ((rret = read(request->fd, 
            request->buf + request->buflen, 
            sizeof(request->buf) - request->buflen)) == -1 && errno == EINTR
        )  
            continue;

        if (rret == 0){ //正常关闭
            clean_request(request);
            return 0;
        }
        if (rret < 0){//读取发生错误
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)){//可重试错误
                //处理完毕
                if(request->keeplive){
                    //重新注册
                    struct epoll_event event;
                    event.data.ptr = ptr;
                    request->finish = 0;
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    epoll_ctl(request->epfd, EPOLL_CTL_MOD, request->fd, &event);
                    add_timer(request, TIMEOUT_DEFAULT, http_expire_handler);
                    return 0;
                }else{
                    clean_request(request);
                    return 0;
                }
            }
            else
            {
                clean_request(request);
                return -1;
            }
        }
        //正常读取
        request->prevbuflen = request->buflen;
        request->buflen += rret;
        //解析请求
        request->num_headers = sizeof(request->headers) / sizeof(request->headers[0]);
        request->pret = phr_parse_request(request->buf, request->buflen, &request->method,
                    &request->method_len, &request->request_url, &request->request_url_len,
                &request->minor_version, request->headers, &request->num_headers, 
                request->prevbuflen
        );
        if (request->pret > 0){//成功解析
            //处理header
            http_handle_header(ptr);
            //处理URL
            http_handle_url(ptr);
#ifndef NDEBUG
            printf("request is %d bytes long\n", request->pret);
            printf("method is %.*s\n", (int)request->method_len, request->method);
            printf("path is %.*s\n", (int)request->path_len, request->path);
            printf("HTTP version is 1.%d\n", request->minor_version);
            printf("headers:\n");
            for (size_t i = 0; i != request->num_headers; ++i) {
                printf("%.*s: %.*s\n", (int)request->headers[i].name_len,request->headers[i].name,
                    (int)request->headers[i].value_len, request->headers[i].value);
            }
            printf("\033[31m ------------- \033[0m\n");
#endif
            //判断文件类型
            // file_type * request_file_type = get_file_type_from_path(request->path); php?待完成

            //静态处理
            response_static_file(request);
            // break;
        }
        else if (request->pret == -1){//解析发生错误
            clean_request(request);
            return -1;
        }
        if (request->buflen == sizeof(request->buf)){
            strncpy(request->buf, request->buf+request->prevbuflen,request->buflen-request->prevbuflen);
            request->buflen=request->buflen-request->prevbuflen;
            request->prevbuflen=0;
        //    return -1;
        }
    }
    // clean_request(request);
    return 0;
}


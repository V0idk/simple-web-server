#include "timer.h"


#include <stdlib.h>
#include <sys/time.h>


#include "global.h"
#include "http_request.h"
#include "pq.h"
#include "util.h"

#define PQ_INIT_SIZE 100
typedef struct{
    size_t expire_time;
    int deleted;
    expire_handler handler;
    http_request_t *request;
} timer_node_t;

static int timer_comp_small(void *node1, void *node2) {
    //小顶堆,故小于才返回1
    timer_node_t *time1 = (timer_node_t *)node1;
    timer_node_t *time2 = (timer_node_t *)node2;
    return (time1->expire_time < time2->expire_time) ? 1 : 0;
}

int timer_init() {
    pq_init(&server_timer, timer_comp_small, PQ_INIT_SIZE);
    return 0;
}

void add_timer(http_request_t *request,  size_t timeout, expire_handler handler) {
    //优先队列管理了内存,因此request结构体只保留指针
    timer_node_t *timer_node = (timer_node_t *)malloc(sizeof(timer_node_t));
    request->timer = timer_node;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timer_node->expire_time = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;
    timer_node->deleted = 0;
    timer_node->handler = handler;
    timer_node->request = request;
    pq_insert(&server_timer, timer_node);
}


void del_timer(http_request_t *request){
    timer_node_t *timer_node = request->timer;
    timer_node->deleted = 1;
}

static void handle_expire_time(){
    timer_node_t *timer_node;
    while(!pq_is_empty(&server_timer)){
        timer_node = pq_top(&server_timer);
        if(timer_node -> deleted){
            pq_pop(&server_timer); 
            continue;
        }
        //未超时
        struct timeval tv;
        gettimeofday(&tv, NULL);
        if((size_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000) < timer_node->expire_time)
            return;
        //超时,处理
        //删除定时器
        pq_pop(&server_timer); 
        if (timer_node->handler) {
            timer_node->handler(timer_node->request);
        }
    }
}


//等待时间
int get_timewait() {
    timer_node_t *timer_node;
    int timeout = -1;  //队列空则无限阻塞
    while (!pq_is_empty(&server_timer)) {
        //处理过期时间
        handle_expire_time();
        if(pq_is_empty(&server_timer)){
            break;
        }
        timer_node = pq_top(&server_timer);
        if (timer_node->deleted) {
            continue;
        }else{
            //获取当前时间
            struct timeval tv;
            gettimeofday(&tv, NULL);
            timeout = timer_node->expire_time - (tv.tv_sec * 1000 + tv.tv_usec / 1000);
            timeout = (timeout > 0? timeout: 0);
            break;
        }
    }
    return timeout;
}

int http_expire_handler(http_request_t *request) {
    //描述符关闭会导致epoll移除fd
    Close(request->fd);
    free(request->timer);
    free(request);
    return 0;
}
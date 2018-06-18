#ifndef _TIMER_H_
#define _TIMER_H_

#include "http_request.h"

#define TIMEOUT_DEFAULT 500

typedef int (*expire_handler)(http_request_t *request);


int  timer_init();

void add_timer(http_request_t *request,  size_t timeout, expire_handler handler) ;
void del_timer(http_request_t *rq);


int get_timewait();
int http_expire_handler(http_request_t *request);

#endif

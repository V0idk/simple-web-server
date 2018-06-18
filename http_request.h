#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <sys/types.h>
#include "picohttpparser.h"

typedef struct {
    int fd;     //已连接描述符
    int epfd;   //对于epollfd
    char buf[4096]; 
    int minor_version;
    int pret;
    const char *method;
    const char * path;
    struct phr_header headers[100];
    size_t buflen;
    size_t prevbuflen;
    size_t method_len;
    size_t path_len;
    size_t num_headers;
    int finish;
    int keeplive;
    const char * param;
    size_t param_len;
    
    const char *request_url;
    size_t request_url_len;
    const char *content;
    size_t content_length;
     void *timer;
} http_request_t;



#endif
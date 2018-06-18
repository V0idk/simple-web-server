#include "handle_request.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "global.h"
#include "util.h"




typedef int (*http_handle)(http_request_t *r, 
    int index);

typedef struct {
    const char *name;
    http_handle handler;
} header_handle_func_t;



static int header_ignore(http_request_t *r, int i){
    return 0;
}

static int header_connection(http_request_t *r, int i){
    (void) i;
    if (strncasecmp("Keep-Alive",r->headers[i].value, r->headers[i].value_len) == 0) {
        r->keeplive = 1;
    }
    return 0;
}


static int header_content_length(http_request_t *r, int i){
    r->content_length = r->buflen - r->pret;
    r->content = r->buf + r->pret;
    return 0;
}

header_handle_func_t header_handle_func[] = {
    {"Host", header_ignore},
    {"Connection", header_connection},
    {"If-Modified-Since", header_ignore},
    {"Content-Length",header_content_length},
    {"", header_ignore}
};



void http_handle_header(void *ptr){
    http_request_t *request = (http_request_t *)ptr;
    header_handle_func_t *header_fun = header_handle_func;
    size_t num_func = sizeof(header_handle_func)/sizeof(header_handle_func_t);
    struct phr_header *header = request->headers;
    for(size_t i=0;i<request->num_headers;i++){
        for(size_t j=0;j<num_func;j++){
            if(!strncmp(header[i].name,header_fun[j].name,header[i].name_len)){
                (*header_fun[j].handler)(request,i);
                break;
            }
        }
    }
}

void request_error(int fd, char *errnum, char *shortmsg)
{
    char header[8192], body[8192];

    sprintf(body, "<html><title>Server Error</title>");
    sprintf(body, "%s<body>\n", body);
    sprintf(body, "%s%s: %s\n", body, errnum, shortmsg);
    sprintf(body, "%s\n</body></html>", body);

    sprintf(header, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    sprintf(header, "%sServer: Zaver\r\n", header);
    sprintf(header, "%sContent-type: text/html\r\n", header);
    sprintf(header, "%sConnection: close\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));
    rio_writen(fd, header, strlen(header));
    rio_writen(fd, body, strlen(body));
    return;
}

file_type http_file_type[] = 
{
    {"html", "text/html"},
    {"xml", "text/xml"},
    {"xhtml", "application/xhtml+xml"},
    {"txt", "text/plain"},
    {"pdf", "application/pdf"},
    {"word", "application/msword"},
    {"png", "image/png"},
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"mpeg", "video/mpeg"},
    {"mpg", "video/mpeg"},
    {"avi", "video/x-msvideo"},
    {"gz", "application/x-gzip"},
    {"tar", "application/x-tar"},
    {"css", "text/css"},
    {"php","text/html;"},
    {NULL ,"text/plain"}
};


//不支持url编码

file_type* get_file_type_from_path(const char *filename)
{
    const char *type = strchr(filename, '.');
    if (!type) {
        return &http_file_type[sizeof(http_file_type)/sizeof(file_type)];
    }
    type++;
    int i;
    for (i = 0; http_file_type[i].type != NULL; ++i) {
        if (strncmp(type, http_file_type[i].type,strlen(http_file_type[i].type) )== 0)
            return &http_file_type[i];
    }
    return &http_file_type[i];
}


int response_static_file(http_request_t *request){
    char path[1024] = {0};
    struct stat sbuf;
    memset(&sbuf,0,sizeof(sbuf));
    int len = strlen(dir);
    strncpy(path,dir,len);
    const char * aa =request->path;
    strncpy(path+len,request->path,request->path_len);
    if(stat(path, &sbuf) < 0) {
        request_error(request->fd, "404", "Not Found");
        return -1;
    }
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
        request_error(request->fd, "403", "Forbidden");
        return -1;
    }
    char header[8192]={0};
    sprintf(header, "HTTP/1.%d %d %s\r\n",request->minor_version, 200, "OK");
    if (request->keeplive) {
        sprintf(header, "%sConnection: keep-alive\r\n", header);
    }
    sprintf(header, "%sContent-type: %s\r\n", header, get_file_type_from_path(request->path)->value);
    sprintf(header, "%sContent-length: %zu\r\n", header, sbuf.st_size);
    sprintf(header, "%s\r\n", header);
    rio_writen(request->fd, header, strlen(header));
     //读写文件
    int filefd = open(path, O_RDONLY, 0);
    //https://www.ibm.com/developerworks/cn/linux/l-cn-zerocopy2/index.html
    //sendfile或mmap避免用户缓冲区
    off_t offset = 0;
    while(sendfile(request->fd,filefd,&offset,sbuf.st_size-offset) > 0);
    return 0;
}

//url解析
int http_handle_url(http_request_t *request){
    // /path/index.html?a=1&b=2#test
    const char * ptr = request->request_url;
    request->path=request->request_url;
    request->path_len = request->request_url_len;
    size_t i, question_mark_po=0,pound_mark_pos=0;
    //path
    for(i=0;i<request->request_url_len;i++){
        if(ptr[i] == '#'){
            request->path_len = i;
            break;
        }
        if(ptr[i] == '?'){
            request->path_len = i;
            if(i < request->request_url_len-1){
                request->param = ptr+i+1;
            }
            for(;i<request->request_url_len;i++){
                if(ptr[i] == '#'){
                    break;
                }
            }
            request->param_len = i-request->path_len -1;
            break;
        }
    }
    return 0;
}
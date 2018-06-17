#ifndef _UTIL_H_
#define _UTIL_H_



#include <netinet/in.h>
#include <string.h>

#define log_err(M, ...) {fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n",\
                                __FILE__,\
                                __LINE__,\
                                (errno == 0 ? "None" : strerror(errno)), ##__VA_ARGS__);\
                         exit(1);}
#define log_warns(M, ...) {fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n",\
                                __FILE__,\
                                __LINE__,\
                                (errno == 0 ? "None" : strerror(errno)), ##__VA_ARGS__);\
                                }
void Close(int fd);

ssize_t rio_writen(int fd, void *usrbuf, size_t n) ;
       
#endif

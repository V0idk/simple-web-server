#include "util.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>


void Close(int fd)
{
    if (close(fd) == -1){
        log_err("close");
    }
}

ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char *)usrbuf;
        
    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR)  /* interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            else {
                log_err("errno == %d\n", errno);
                return -1;       /* errorno set by write() */
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    
    return n;
}


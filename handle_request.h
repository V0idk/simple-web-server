#ifndef _HANDLE_REQUEST_H_
#define _HANDLE_REQUEST_H_

#include "http_request.h"


typedef struct {
	const char *type;
	const char *value;
} file_type;

extern void http_handle_header(void *ptr);
extern int response_static_file(http_request_t *request);
extern int http_handle_url(http_request_t *request);

extern file_type* get_file_type_from_path(const char *filename);
extern void request_error(int fd, char *errnum, char *shortmsg);

#endif

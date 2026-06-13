#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

bool net_http_get(const char * url, char * out_buf, size_t buf_size);

#endif

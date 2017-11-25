#ifndef __PROXY_COMMON
#define __PROXY_COMMON

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <assert.h>

#define success(x) assert((x) >= 0)

#define logv(fmt, ...) fprintf(stderr, "[%s:%d]\t" fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif

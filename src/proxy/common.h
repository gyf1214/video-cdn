#ifndef __PROXY_COMMON
#define __PROXY_COMMON

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <netdb.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>

#define success(x) assert((x) >= 0)

#define __stringify(x) #x
#define __tostring(x)  __stringify(x)
#define __at           "[" __FILE__ ":" __tostring(__LINE__) "]"
#define logv(fmt, ...) fprintf(stderr, "%-30s" fmt "\n", __at, ##__VA_ARGS__)

#define min(x, y)      ((x) < (y) ? (x) : (y))
#define max(x, y)      ((x) > (y) ? (x) : (y))

#endif

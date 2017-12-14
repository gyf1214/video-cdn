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
#define __at           "[" __FILE__ ":" __stringify(__LINE__) "]"
#define logv(fmt, ...) fprintf(stderr, "%-20s" fmt "\n", __at, ##__VA_ARGS__)

#endif

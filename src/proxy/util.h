#ifndef __PROXY_UTIL
#define __PROXY_UTIL

#include "common.h"
#include "buffer.h"

extern struct Util {
    void (*init)(void);
    const char *(*itoa)(int);
    double (*interval)(const struct timespec *begin, const struct timespec *end);
    double (*recordTput)(int, double);
    double (*estimateTput)(void);

    void (*parseList)(char *);
} util;

#endif

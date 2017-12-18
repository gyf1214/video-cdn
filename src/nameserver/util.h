#ifndef __DNS_UTIL
#define __DNS_UTIL

#include "common.h"

extern struct Util {
    void (*init)(void);

    uint32_t (*resolve)(uint32_t);
} util;

#endif

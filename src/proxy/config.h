#ifndef __PROXY_CONFIG
#define __PROXY_CONFIG

#include "common.h"

extern struct Config {
    // parse args
    void (*parse)(int, char **);

    // data
    float alpha;
    struct sockaddr listen;
    const char *backend;
    uint16_t backendPort;
    struct sockaddr dns;
} config;

#endif

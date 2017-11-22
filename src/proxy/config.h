#ifndef __PROXY_CONFIG
#define __PROXY_CONFIG

#include "common.h"

extern struct Config {
    // parse args
    void (*parse)(int, char **);

    // data
    FILE *logging;
    float alpha;
    // listen addr
    struct sockaddr listen;
    // local bind addr
    struct sockaddr local;
    const char *backendHost;
    in_addr_t backendIP;
    // backend port
    uint16_t backendPort;
    // dns addr
    struct sockaddr dns;
} config;

#endif

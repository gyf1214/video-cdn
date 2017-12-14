#ifndef __PROXY_CONFIG
#define __PROXY_CONFIG

#include "common.h"

#define ConfigBackendPort 8080
#define ConfigHost        "video.pku.edu.cn"

extern struct Config {
    // parse args
    void (*parse)(int, char **);

    // config data
    FILE *logging;
    float alpha;
    // listen addr
    struct sockaddr_in listen;
    // local bind addr
    struct sockaddr_in local;
    const char *backendHost;
    in_addr_t backendIP;
    // backend port
    uint16_t backendPort;
    // dns addr
    struct sockaddr_in dns;
} config;

#endif

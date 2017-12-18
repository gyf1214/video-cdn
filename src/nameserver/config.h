#ifndef __DNS_CONFIG
#define __DNS_CONFIG

#include "common.h"

extern struct Config {
    void (*parse)(int, char **);

    FILE *logging;
    struct sockaddr_in local;
    FILE *servers, *lsa;
    int round;
} config;

#endif

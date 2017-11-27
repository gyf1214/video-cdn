#ifndef __PROXY_SERVER
#define __PROXY_SERVER

#include "common.h"

extern struct Server {
    void (*create)(struct sockaddr *);
} server;

#endif

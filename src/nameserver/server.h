#ifndef __DNS_SERVER
#define __DNS_SERVER

#include "common.h"

extern struct Server {
    void (*create)(struct sockaddr_in *);

    void (*loop)(void);
} server;

#endif

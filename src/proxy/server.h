#ifndef __PROXY_SERVER
#define __PROXY_SERVER

#include "common.h"
#include "io.h"
#include "buffer.h"

extern struct Server {
    // create proxy server with listen address
    void (*create)(struct sockaddr_in *);

    // kill connection
    void (*disconnect)(Socket *);

    // called when proxy connection established
    void (*linkProxy)(Socket *, Chunk *);
} server;

#endif

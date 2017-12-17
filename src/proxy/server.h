#ifndef __PROXY_SERVER
#define __PROXY_SERVER

#include "common.h"
#include "io.h"
#include "buffer.h"

#define MaxRequestLine 2560

extern struct Server {
    // create proxy server with listen address
    void (*create)(struct sockaddr_in *);

    // kill connection
    void (*release)(Socket *);

    // link with forward socket
    void (*link)(Socket *, Socket *, Chunk *);
} server;

#endif

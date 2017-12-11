#ifndef __PROXY_CLIENT
#define __PROXY_CLIENT

#include "common.h"
#include "io.h"
#include "buffer.h"

extern struct Client {
    Socket *(*createClient)(Socket *, Chunk *, const struct sockaddr_in *);
} client;

#endif

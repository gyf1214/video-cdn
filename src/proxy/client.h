#ifndef __PROXY_CLIENT
#define __PROXY_CLIENT

#include "common.h"
#include "io.h"
#include "buffer.h"

extern struct Client {
    void (*createClient)(Socket *, Chunk *);

    void (*release)(Socket *);

    int (*eof)(Socket *);
} client;

#endif

#ifndef __PROXY_IO
#define __PROXY_IO

#include "common.h"

#define IOWaitRead  0x01
#define IOWaitWrite 0x02
#define IOWaitLoop  0x04

#define IOMaxListen 1024
#define IOMaxSocket FD_SETSIZE

#define IOTimeout   10

typedef struct Socket Socket;
typedef void (*SocketCallback)(Socket *, int);

struct Socket {
    int fd;
    SocketCallback callback;
    void *data;
    int flags;
};

extern struct IO {
    void (*init)(void);
    void (*loop)(void);
    Socket *(*socket)(int, const struct sockaddr *);
    void (*close)(Socket *);
    void (*listen)(Socket *, SocketCallback);
    Socket *(*accept)(Socket *);
} io;

#endif

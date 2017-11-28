#ifndef __PROXY_IO
#define __PROXY_IO

#include "common.h"

#define IOWaitRead  0x01
#define IOWaitWrite 0x02

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
    // call before using io loop
    void (*init)(void);

    // io main loop
    void (*loop)(void);

    // new socket from binding address
    Socket *(*socket)(int, const struct sockaddr_in *);

    // close socket and release handler
    void (*close)(Socket *);

    // listen on socket and set handler
    void (*listen)(Socket *, SocketCallback);

    // accept and return the new Socket & peer address
    Socket *(*accept)(Socket *, struct sockaddr_in *);

    // set socket wait flags
    void (*wait)(Socket *, int);

    // unset socket wait flags
    void (*block)(Socket *, int);
} io;

#endif

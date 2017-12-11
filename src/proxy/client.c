#include "client.h"

typedef struct {
    struct sockaddr_in peer;
    Chunk buf;
    Socket *proxy;
    Chunk *proxyBuf;
} Conn;

static void readHandler(Socket *s, Conn *c) {
    // TODO
    (void) s, (void) c;
}

static void writeHandler(Socket *s, Conn *c) {
    // TODO
    (void) s, (void) c;
}

static void connHandler(Socket *s, int flag) {
    Conn *c = (Conn *)s->data;
    if (flag & IOWaitRead) {
        logv("handle proxy read %d", s->fd);
        readHandler(s, c);
    } else if (flag & IOWaitWrite) {
        logv("handle proxy write %d", s->fd);
        writeHandler(s, c);
    }
}

static Socket *createClient(Socket *s, Chunk *buf, const struct sockaddr_in *addr) {
    Socket *cs = io.socket(SOCK_STREAM, addr);

    Conn *c = malloc(sizeof(Conn));
    c->peer = *addr;
    buffer.init(&c->buf, cs->fd);
    c->proxy = s;
    c->proxyBuf = buf;

    cs->data = c;
    cs->callback = connHandler;

    io.connect(cs, addr);
    logv("connect %s:%hu with %d", inet_ntoa(addr->sin_addr),
        ntohs(addr->sin_port), cs->fd);
    return cs;
}

struct Client client = {
    createClient
};

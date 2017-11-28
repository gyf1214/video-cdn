#include "server.h"
#include "io.h"
#include "buffer.h"

static struct {
    Socket *listen;
} local;

typedef struct {
    struct sockaddr_in peer;
    Socket *proxy;
    Chunk buf;
} Conn;

static void readHandler(Socket *s, Conn *c) {
    int n = buffer.fill(&c->buf);
    // reset on fail
    if (n <= 0) {
        logv("read from %d failed, close", s->fd);
        io.close(s);
        free(c);
        return;
    }

    // if chunk full, clear read handler
    if (BufferFull(&c->buf)) {
        io.block(s, IOWaitRead);
    }

    // if chunk have data, notify write
    if (buffer.flush(&c->buf)) {
        io.wait(s, IOWaitWrite);
    }
}

static void writeHandler(Socket *s, Conn *c) {
    char *data = buffer.flush(&c->buf);
    int len = c->buf.seek - c->buf.head;

    int n = write(s->fd, data, len);
    // reset on fail
    if (n <= 0) {
        logv("write to %d failed, close", s->fd);
        io.close(s);
        free(c);
        return;
    }

    buffer.consume(&c->buf, n);
    // if chunk empty, clear write notify
    if (!BufferReady(&c->buf)) {
        io.block(s, IOWaitWrite);
    }
}

static void connHandler(Socket *s, int flag) {
    Conn *c = (Conn *)s->data;
    if (flag & IOWaitRead) {
        logv("handle connection read %d", s->fd);
        readHandler(s, c);
    } else if (flag & IOWaitWrite) {
        logv("handle connection write %d", s->fd);
        writeHandler(s, c);
    }
}

static void listenHandler(Socket *s, int flag) {
    assert(flag == IOWaitRead);
    logv("new connection");

    Conn *conn = malloc(sizeof(Conn));
    conn->proxy = NULL;

    Socket *cs = io.accept(s, &conn->peer);
    buffer.init(&conn->buf, cs->fd);
    cs->callback = connHandler;
    cs->data = conn;
}

static void createServer(struct sockaddr_in *addr) {
    local.listen = io.socket(SOCK_STREAM, addr);
    io.listen(local.listen, listenHandler);
}

struct Server server = {
    createServer
};

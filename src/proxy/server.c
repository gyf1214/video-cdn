#include "server.h"
#include "config.h"
#include "client.h"

static struct {
    Socket *listen;
} local;

typedef struct {
    struct sockaddr_in peer;
    Chunk buf;
    char line[MaxRequestLine], output[MaxRequestLine];
    Socket *proxy;
    Chunk *proxyBuf;
} Conn;

static void release(Socket *s) {
    Conn *c = (Conn *)s->data;
    if (c->proxy) {
        client.release(c->proxy);
    }

    logv("close connection %d", s->fd);
    io.close(s);
    free(c);
}

static void readHandler(Socket *s, Conn *c) {
    int n = buffer.fill(&c->buf);
    // reset on fail || buffer full
    // client should not close before server
    if (n <= 0 || BufferFull(&c->buf)) {
        logv("request read error");
        release(s);
        return;
    }

    char *pch = strstr(BufferHead(&c->buf), "\r\n\r\n");
    if (pch) {
        *pch = 0;
        client.createClient(s, &c->buf);
        io.block(s, IOWaitRead);
    }
}

static void writeHandler(Socket *s, Conn *c) {
    if (client.eof(c->proxy) && BufferEmpty(c->proxyBuf)) {
        // client eof && buffer empty -> connection finish
        logv("connection finish");
        // TODO : calculate bitrate
        release(s);
        return;
    }

    char *data = BufferHead(c->proxyBuf);
    int len = BufferLen(c->proxyBuf);

    int n = write(s->fd, data, len);
    // reset on fail
    if (n < 0) {
        logv("forward write error");
        release(s);
        return;
    }

    buffer.consume(c->proxyBuf, n);
    // if chunk empty, clear write wait
    if (BufferEmpty(c->proxyBuf) && !client.eof(c->proxy)) {
        logv("forward buffer empty");
        io.block(s, IOWaitWrite);
    }
    io.wait(c->proxy, IOWaitRead);
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

    Conn *c = malloc(sizeof(Conn));
    c->proxy = NULL;
    c->proxyBuf = NULL;

    Socket *cs = io.accept(s, &c->peer);
    buffer.init(&c->buf, cs->fd);
    cs->callback = connHandler;
    cs->data = c;

    io.wait(cs, IOWaitRead);
    logv("new connection %d from %s:%hu", cs->fd,
        inet_ntoa(c->peer.sin_addr), ntohs(c->peer.sin_port));
}

static void createServer(struct sockaddr_in *addr) {
    local.listen = io.socket(SOCK_STREAM, addr);
    io.listen(local.listen, listenHandler);
}

static void linkForward(Socket *s, Socket *cs, Chunk *buf) {
    Conn *c = (Conn *)s->data;
    assert(!c->proxy);

    c->proxy = cs;
    c->proxyBuf = buf;
    logv("link forward %d->%d", cs->fd, s->fd);
}

struct Server server = {
    createServer, release, linkForward
};

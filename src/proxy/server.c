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

static void openConnection(Socket *s, Conn *c) {
    if (config.backendIP) {
        struct sockaddr_in peer;
        peer.sin_family = AF_INET;
        peer.sin_addr.s_addr = config.backendIP;
        peer.sin_port = config.backendPort;

        Socket *cs = client.createClient(s, &c->buf, &peer);

        c->proxy = cs;
        c->proxyBuf = client.getBuffer(cs);
    } else {
        // TODO: request DNS
        assert(0);
    }
}

static void readHandler(Socket *s, Conn *c) {
    int n = buffer.fill(&c->buf);
    // reset on fail
    // client should not close before server
    if (n <= 0) {
        logv("read from %d failed", s->fd);
        release(s);
        return;
    }

    // if chunk full, clear io wait
    if (BufferFull(&c->buf)) {
        io.block(s, IOWaitRead);
    }

    if (!c->proxy) {
        openConnection(s, c);
    }
    io.wait(c->proxy, IOWaitWrite);
}

static void writeHandler(Socket *s, Conn *c) {
    char *data = BufferHead(c->proxyBuf);
    int len = buffer.flush(c->proxyBuf);

    int n = write(s->fd, data, len);
    // reset on fail
    if (n < 0) {
        logv("write to %d failed, close", s->fd);
        release(s);
        return;
    }

    buffer.consume(c->proxyBuf, n);
    // if chunk empty, clear write wait
    if (BufferEmpty(c->proxyBuf)) {
        if (client.eof(s)) {
            // client eof && buffer empty -> connection finish
            logv("connection %d finish", s->fd);
            release(s);
        } else {
            io.block(s, IOWaitWrite);
        }
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

    Conn *conn = malloc(sizeof(Conn));
    conn->proxy = NULL;
    conn->proxyBuf = NULL;

    Socket *cs = io.accept(s, &conn->peer);
    buffer.init(&conn->buf, cs->fd);
    cs->callback = connHandler;
    cs->data = conn;

    io.wait(cs, IOWaitRead);
    logv("new connection %d from %s:%hu", cs->fd,
        inet_ntoa(conn->peer.sin_addr), ntohs(conn->peer.sin_port));
}

static void createServer(struct sockaddr_in *addr) {
    local.listen = io.socket(SOCK_STREAM, addr);
    io.listen(local.listen, listenHandler);
}

struct Server server = {
    createServer, release
};

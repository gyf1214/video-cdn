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

static void releaseSocket(Socket *s) {
    Conn *c = (Conn *)s->data;
    logv("close connection %d", s->fd);
    io.close(s);
    free(c);
}

// returns line length
// -1 on error / no new line
static int readLine(Socket *s, Conn *c) {
    int len = buffer.readline(&c->buf);

    if (len >= MaxRequestLine) {
        logv("max line exceeded");
        releaseSocket(s);
        return -1;
    }

    if (len < 0) {
        logv("no new line");
        return -1;
    }

    for (int i = 0; i < len; ++i) {
        c->line[i] = BufferChar(&c->buf, i);
    }
    c->line[len] = 0;

    // consume the line and try recover io wait
    buffer.consume(&c->buf, BufferSeek(&c->buf));
    if (BufferReady(&c->buf)) {
        io.wait(s, IOWaitRead);
    }

    return len;
}

static void openConnection(Socket *s, Conn *c) {
    if (config.backendIP) {
        struct sockaddr_in peer;
        peer.sin_family = AF_INET;
        peer.sin_addr.s_addr = config.backendIP;
        peer.sin_port = config.backendPort;

        client.createClient(s, &c->buf, &peer);
        io.connect(s, &peer);


    } else {
        // TODO: request DNS
        assert(0);
    }
}

// returns 1 if succ
// returns 0 only on parse error
static int parseRequest(Socket *s, Conn *c) {
    // read a line
    int len = readLine(s, c);
    if (len < 0) return 1;

    char *src = c->line;
    char *dst = c->output;

    logv("http request: %s", src);

    // 1. HTTP method
    while (*src && *src != ' ') {
        *dst++ = *src++;
    }
    if (!*src) return 0;
    *dst++ = *src++;

    // 2. find uri, ignore 2 /
    while (*src && *src != ' ' && (*src != '/' ||
    (*(src + 1) == '/' || *(src - 1) == '/'))) {
        ++src;
    }
    if (!*src) return 0;

    // no uri, use /
    if (*src == ' ') {
        *dst++ = '/';
    }

    while (*src && *src != ' ') {
        *dst++ = *src++;
    }
    if (!*src) return 0;

    // 3. protocol
    while (*src) {
        *dst++ = *src++;
    }
    *dst = 0;

    logv("proxy request: %s", c->output);
    return 1;
}

static void readHandler(Socket *s, Conn *c) {
    int n = buffer.fill(&c->buf);
    // reset on fail
    if (n <= 0) {
        logv("read from %d failed", s->fd);
        releaseSocket(s);
        return;
    }

    // if chunk full, clear io wait
    if (!BufferReady(&c->buf)) {
        io.block(s, IOWaitRead);
    }

    // if no proxy connection
    // try parse request line and connect
    if (!c->proxy) {
        if (!parseRequest(s, c)) {
            // error on request line
            // close
            logv("request error");
            releaseSocket(s);
            return;
        }
    }
    if (c->proxy) {
        // if there is a line
        // set proxy wait write
        int len = buffer.readline(&c->buf);
        if (len >= 0) {
            io.wait(c->proxy, IOWaitWrite);
        }
    }
}

static void writeHandler(Socket *s, Conn *c) {
    char *data = BufferHead(&c->buf);
    int len = BufferSeek(&c->buf);

    int n = write(s->fd, data, len);
    // reset on fail
    if (n <= 0) {
        logv("write to %d failed, close", s->fd);
        releaseSocket(s);
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
    createServer, releaseSocket
};

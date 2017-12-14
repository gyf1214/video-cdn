#include "client.h"
#include "server.h"
#include "config.h"

#define StateRequest  1
#define StateHeader   2
#define StateBody     3
#define StateEof      4

const char *ignoreHeaders[] = {
    "Host", "Proxy-Connection", "Connection"
};

const char *defaultHeaders =
    "Host: " ConfigHost "\r\n"
    "Connection: close\r\n";

typedef struct {
    struct sockaddr_in peer;
    Chunk buf;
    Chunk out;
    Socket *proxy;
    Chunk *proxyBuf;
    int state;
} Conn;

static void release(Socket *s) {
    Conn *c = (Conn *)s->data;
    logv("close proxy %d", s->fd);
    io.close(s);
    free(c);
}

static void readHandler(Socket *s, Conn *c) {
    int n = buffer.fill(&c->buf);
    if (n < 0) {
        logv("read from proxy %d failed", s->fd);
        server.release(c->proxy);
        return;
    } else if (!n) {
        logv("proxy %d finish", s->fd);
        io.block(s, IOWaitRead);
        // n == 0 indicates peer close
        // set eof
        c->state = StateEof;
    } else if (BufferFull(&c->buf)) {
        io.block(s, IOWaitRead);
    }
    io.wait(c->proxy, IOWaitWrite);
}

// 0  -> no request
// 1  -> succ
// -1 -> error
static int parseRequest(Chunk *c, Chunk *o) {
    int len = buffer.readline(c);
    if (len < 0) return 0;

    int src = 0;
    char ch;

    // 1. method
    while (src < len && (ch = BufferChar(c, src)) != ' ') {
        BufferAppend(o, ch);
        ++src;
    }
    if (src >= len) return -1;
    BufferAppend(o, ' ');
    ++src;

    // 2. uri
    // ignore continuous '//'
    // find first '/'
    while (src < len && BufferChar(c, src) != ' ' &&
          (BufferChar(c, src) != '/' || BufferChar(c, src - 1) == '/' ||
           BufferChar(c, src + 1) == '/')) {
        ++src;
    }
    if (src >= len) return -1;

    if (BufferChar(c, src) == ' ') {
        BufferAppend(o, '/');
    }
    while (src < len && (ch = BufferChar(c, src)) != ' ') {
        BufferAppend(o, ch);
        ++src;
    }
    BufferAppend(o, ' ');
    ++src;

    // 3. protocol
    while (src < len) {
        BufferAppend(o, BufferChar(c, src++));
    }

    c->data[c->tail] = 0;
    logv("proxy request: %s", c->data);
    BufferAppend(o, '\r');
    BufferAppend(o, '\n');

    buffer.consumeEOL(c);

    return 1;
}

// 0  -> nothing
// 1  -> succ
// 2  -> end of header (empty line)
// -1 -> error
static int parseHeader(Chunk *c, Chunk *o) {
    int len = buffer.readline(c);
    if (len < 0) return 0;

    if (!len) {
        buffer.consumeEOL(c);
        return 2;
    }

    char ch;

    int src = 0;
    while (src < len && BufferChar(c, src) != ':') {
        ++src;
    }
    if (src >= len) return -1;

}

static void writeHandler(Socket *s, Conn *c) {
    int stop = 0;
    int ret;

    while (!stop) {
        switch (c->state) {
            case StateRequest:
            ret = parseRequest(c->proxyBuf, &c->out);
            if (ret < 0) {
                server.release(c->proxy);
                return;
            } else if (!ret) {
                stop = 1;
            } else {
                c->state = StateHeader;
            }
            break;
            default:
            logv("error parse request");
            server.release(c->proxy);
            return;
        }
    }
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
    buffer.init(&c->out, cs->fd);
    c->proxy = s;
    c->proxyBuf = buf;
    c->state = StateRequest;

    cs->data = c;
    cs->callback = connHandler;

    io.connect(cs, addr);
    logv("connect %s:%hu with %d", inet_ntoa(addr->sin_addr),
        ntohs(addr->sin_port), cs->fd);
    return cs;
}

static Chunk *getBuffer(Socket *s) {
    Conn *c = (Conn *)s->data;
    return c->proxyBuf;
}

static int getEof(Socket *s) {
    Conn *c = (Conn *)s->data;
    return c->state == StateEof;
}

struct Client client = {
    createClient, getBuffer,
    release, getEof
};

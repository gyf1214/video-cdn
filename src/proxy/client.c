#include "client.h"
#include "server.h"
#include "config.h"
#include "util.h"

#define StateDNS      1
#define StateRequest  2
#define StateList     3
#define StateForward  4
#define StateEof      5
#define StateError    -1

#define IgnoreSize    3
#define MaxNameSize   64

static const char *ignoreHeaders[IgnoreSize] = {
    "Host", "Proxy-Connection", "Connection"
};

static const char *defaultHeaders =
    "\r\nHost: " ConfigHost
    "\r\nConnection: close\r\n";

typedef struct {
    struct sockaddr_in peer;
    Chunk buf0, buf1;
    Socket *proxy;
    Chunk *proxyBuf;
    int state;
    int chunkSize;
    struct timespec begin, end;
    char chunkName[MaxNameSize];
    int bitrate;
} Conn;

static void connHandler(Socket *s, int flag);

static void release(Socket *s) {
    Conn *c = (Conn *)s->data;
    logv("close proxy %d", s->fd);
    io.close(s);
    free(c);
}

static Socket *connectSocket(Conn *c) {
    Socket *cs = io.socket(SOCK_STREAM, &config.local);
    cs->data = c;
    cs->callback = connHandler;

    io.connect(cs, &c->peer);
    io.wait(cs, IOWaitWrite);
    logv("connect %s:%hu with %d", inet_ntoa(c->peer.sin_addr),
        ntohs(c->peer.sin_port), cs->fd);
    return cs;
}

static void forwardHandler(Socket *s, Conn *c) {
    int n = buffer.fill(&c->buf0);
    if (n < 0) {
        logv("forward read error");
        server.release(c->proxy);
        return;
    }
    if (!n) {
        logv("forward read eof");
        c->state = StateEof;
        io.block(s, IOWaitRead);

        if (c->bitrate) {
            clock_gettime(CLOCK_REALTIME, &c->end);
            double dur = util.interval(&c->begin, &c->end);
            double tput = util.recordTput(c->chunkSize, dur);
            double estTput = util.estimateTput();

            fprintf(config.logging, "%ld %lf %.0lf %.0lf %d %s %s\n",
                    c->begin.tv_sec, dur, tput, estTput, c->bitrate,
                    inet_ntoa(c->peer.sin_addr), c->chunkName);
        }
    }

    c->chunkSize += n;
    if (BufferFull(&c->buf0)) {
        logv("forward buffer full");
        io.block(s, IOWaitRead);
    }
    io.wait(c->proxy, IOWaitWrite);
}

static void listHandler(Socket *s, Conn *c) {
    int n = buffer.fill(&c->buf1);
    if (n < 0 || BufferFull(&c->buf1)) {
        logv("list read error");
        server.release(c->proxy);
        release(s);
        return;
    }

    if (!n) {
        logv("list finish: %s", c->buf1.data);
        util.parseList(c->buf1.data);

        io.close(s);
        connectSocket(c);

        c->state = StateForward;
    }
}

static int parseRequest(char *str, Conn *c) {
    char *method = strtok(str, " ");
    char *uri = strtok(NULL, " ");
    char *version = strtok(NULL, " ");
    if (!version) return 0;

    // get uri from url
    char *uri0 = strstr(uri, "://");
    if (uri0) {
        uri = uri0 + strlen("://");
    }
    uri += strcspn(uri, "/");
    if (!*uri) return 0;
    logv("request uri: %s", uri);

    buffer.append(&c->buf0, method);
    buffer.append(&c->buf0, " ");

    // rewrite
    char *seg = strstr(uri, "Seg");
    char *f4m = strstr(uri, ".f4m");

    if (seg) {
        char *end = strrchr(uri, '/');
        if (!end) return 0;
        *end = 0;
        buffer.append(&c->buf0, uri);
        buffer.append(&c->buf0, "/");

        char *name = BufferTail(&c->buf0);
        // TODO : modify this
        c->bitrate = 500;
        buffer.append(&c->buf0, util.itoa(c->bitrate));
        buffer.append(&c->buf0, seg);
        strncpy(c->chunkName, name, MaxNameSize);

        c->state = StateForward;
    } else if (f4m) {
        buffer.append(&c->buf1, method);
        buffer.append(&c->buf1, uri);
        buffer.append(&c->buf1, " ");
        buffer.append(&c->buf1, version);
        logv("list request: %s", c->buf1.data);
        buffer.append(&c->buf1, defaultHeaders);

        *f4m = 0;
        buffer.append(&c->buf0, uri);
        buffer.append(&c->buf0, "_nolist.f4m");

        c->state = StateList;
    } else {
        buffer.append(&c->buf0, uri);
        c->state = StateForward;
    }
    buffer.append(&c->buf0, " ");
    buffer.append(&c->buf0, version);
    logv("forward request: %s", c->buf0.data);
    buffer.append(&c->buf0, defaultHeaders);

    return 1;
}

static int parseHeader(char *str, Conn *c) {
    char *field = strtok(str, ": ");
    if (!field) return 0;

    int i;
    for (i = 0; i < IgnoreSize; ++i) {
        if (!strcmp(field, ignoreHeaders[i])) {
            logv("ignore header: %s", field);
            return 1;
        }
    }

    char *line = BufferTail(&c->buf0);
    buffer.append(&c->buf0, field);
    buffer.append(&c->buf0, ": ");
    char *val = strtok(NULL, "");
    if (val) {
        buffer.append(&c->buf0, val);
    }
    buffer.append(&c->buf0, "\r\n");

    logv("forward header: %s", field);
    if (c->state == StateList) {
        buffer.append(&c->buf1, line);
    }

    return 1;
}

static int tryFlush(Socket *s, Chunk *buf) {
    if (!BufferEmpty(buf)) {
        buffer.write(buf);
        return 0;
    } else {
        io.block(s, IOWaitWrite);

        buffer.init(buf, s->fd);
        io.wait(s, IOWaitRead);
        return 1;
    }
}

static void writeHandler(Socket *s, Conn *c) {
    // parse request
    if (c->state == StateRequest) {
        char *req = BufferHead(c->proxyBuf);
        logv("request full: %s", req);
        char *line = strstr(req, "\r\n");
        *line = 0;

        if (!line || !parseRequest(req, c)) {
            server.release(c->proxy);
            release(s);
            return;
        }

        line += 2;
        for (;;) {
            char *next = strstr(line, "\r\n");
            if (next) *next = 0;

            if (!parseHeader(line, c)) {
                server.release(c->proxy);
                release(s);
                return;
            }

            if (!next) break;
            line = next + 2;
        }
        logv("forward request full: %s", c->buf0.data);
        buffer.append(&c->buf0, "\r\n");
        if (c->state == StateList) {
            logv("list request full: %s", c->buf1.data);
            buffer.append(&c->buf1, "\r\n");
        }
    }

    if (c->state == StateList) {
        // request original .f4m list
        if (tryFlush(s, &c->buf1)) {
            logv("finish list request");
        }
    } else if (c->state == StateForward) {
        // forward response from remote
        if (tryFlush(s, &c->buf0)) {
            logv("finish forward request");
            server.link(c->proxy, s, &c->buf0);

            clock_gettime(CLOCK_REALTIME, &c->begin);
            c->chunkSize = 0;
        }
    }
}

static void connHandler(Socket *s, int flag) {
    Conn *c = (Conn *)s->data;
    if (flag & IOWaitRead) {
        logv("handle proxy read %d", s->fd);
        if (c->state == StateList) {
            logv("response from list");
            listHandler(s, c);
        } else if (c->state == StateForward) {
            logv("response from forward");
            forwardHandler(s, c);
        } else {
            logv("response from unknown state");
        }
    } else if (flag & IOWaitWrite) {
        logv("handle proxy write %d", s->fd);
        writeHandler(s, c);
    }
}

static void createClient(Socket *s, Chunk *buf) {
    Conn *c = malloc(sizeof(Conn));
    c->proxy = s;
    c->proxyBuf = buf;
    c->peer.sin_family = AF_INET;
    c->peer.sin_port = config.backendPort;
    c->bitrate = 0;

    if (config.backendIP) {
        c->peer.sin_addr.s_addr = config.backendIP;
        c->state = StateRequest;
        Socket *cs = connectSocket(c);
        buffer.init(&c->buf0, cs->fd);
        buffer.init(&c->buf1, cs->fd);
    }
}

static int getEof(Socket *s) {
    Conn *c = (Conn *)s->data;
    return c->state == StateEof;
}

struct Client client = {
    createClient, release, getEof
};

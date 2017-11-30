#include "buffer.h"

static void reset(Chunk *c) {
    c->head = c->tail = c->seek = 0;
    logv("reset buffer %d", c->fd);
}

static void init(Chunk *c, int fd) {
    c->fd = fd;
    reset(c);
}

static int fill(Chunk *c) {
    int n = read(c->fd, &c->data[c->tail], BufferMaxSize - c->tail - 1);
    if (n > 0) {
        c->tail += n;
        logv("fill %d with %d bytes", c->fd, n);
    }
    return n;
}

static void consume(Chunk *c, int x) {
    assert(x <= c->seek - c->head);
    c->head += x;
    logv("consume %d bytes from %d", x, c->fd);

    if (c->tail == c->head) {
        reset(c);
    }
}

static char *flush(Chunk *c) {
    if (c->head >= c->tail) {
        return NULL;
    }

    c->seek = c->tail;
    c->data[c->seek] = 0;

    return BufferHead(c);
}

static char *readline(Chunk *c) {
    while (c->seek < c->tail && c->data[c->seek] != 0 &&
           c->data[c->seek] != '\r' && c->data[c->seek] != '\n') {
        ++c->seek;
    }

    if (c->seek >= c->tail) {
        return NULL;
    }

    c->data[c->seek] = 0;
    if (c->seek + 1 < c->tail && c->data[c->seek] == '\r' &&
        c->data[c->seek + 1] == '\n') {
        ++c->seek;
    }
    ++c->seek;

    return BufferHead(c);
}

struct Buffer buffer = {
    init, fill, consume,
    flush, readline
};

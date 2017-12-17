#include "buffer.h"

static void reset(Chunk *c) {
    c->head = c->tail = 0;
    logv("reset buffer %d", c->fd);
}

static void init(Chunk *c, int fd) {
    c->fd = fd;
    reset(c);
}

static int fill(Chunk *c) {
    assert(!BufferFull(c));

    int len = BufferCap(c);
    int len0 = BufferMaxSize - c->tail;
    len = min(len, len0);

    int n = read(c->fd, &c->data[c->tail], len);
    if (n > 0) {
        c->tail = (c->tail + n) % BufferMaxSize;
        // make it like a string
        c->data[c->tail] = 0;
        logv("fill %d with %d bytes", c->fd, n);
    }
    return n;
}

static void consume(Chunk *c, int x) {
    assert(!BufferEmpty(c));
    assert(x > 0 && x <= BufferLen(c));

    c->head = (c->head + x) % BufferMaxSize;
    logv("consume %d bytes from %d", x, c->fd);
}

static int writeBuffer(Chunk *c) {
    assert(!BufferEmpty(c));

    int len = BufferLen(c);
    int len0 = BufferMaxSize - c->head;
    len = min(len, len0);

    int n = write(c->fd, BufferHead(c), len);
    if (n > 0) {
        consume(c, n);
        logv("write %d bytes to %d", n, c->fd);
    }

    return n;
}

static int appendBuffer(Chunk *c, const char *s) {
    int len = strlen(s);
    int len0 = BufferCap(c);
    len = min(len, len0);
    len0 = BufferMaxSize - c->tail;
    len0 = min(len, len0);

    memcpy(BufferTail(c), s, len0);
    if (len > len0) {
        memcpy(c->data, s + len0, len - len0);
    }
    c->tail = (c->tail + len) % BufferMaxSize;
    c->data[c->tail] = 0;
    logv("append %d data to %d from %ld chars", len, c->fd, strlen(s));

    return len;
}

struct Buffer buffer = {
    init, fill, writeBuffer, consume, appendBuffer
};

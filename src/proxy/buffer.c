#include "buffer.h"

static void reset(Chunk *c) {
    c->head = c->tail = c->seek = 0;
    c->lastOp = 0;
    logv("reset buffer %d", c->fd);
}

static void init(Chunk *c, int fd) {
    c->fd = fd;
    reset(c);
}

static int fill(Chunk *c) {
    assert(!BufferFull(c));

    int len = BufferTail(c);
    int n = read(c->fd, &c->data[c->tail], len);
    if (n > 0) {
        c->tail = (c->tail + n) % BufferMaxSize;
        c->lastOp = 1;
        logv("fill %d with %d bytes", c->fd, n);
    }
    return n;
}

static void consume(Chunk *c, int x) {
    assert(!BufferEmpty(c));
    assert(x > 0 && x <= BufferLen(c));

    int seek = BufferSeek(c);
    if (x > seek) {
        c->seek = (c->seek + x - seek) % BufferMaxSize;
    }
    c->head = (c->head + x) % BufferMaxSize;
    c->lastOp = 0;
    logv("consume %d bytes from %d", x, c->fd);
}

static void consumeCRLF(Chunk *c) {
    assert(!BufferEmpty(c));

    int len = 0;
    char ch = BufferChar(c, 0);
    if (ch == '\r' || ch == '\n') {
        ++len;
        if (BufferLen(c) >= 2) {
            char ch1 = BufferChar(c, 1);
            if (ch == '\r' && ch1 == '\n') {
                ++len;
            }
        }
    }

    if (len > 0) consume(c, len);
}

static int flush(Chunk *c) {
    assert(!BufferEmpty(c));
    c->seek = c->tail > c->head ? c->tail : 0;

    return BufferSeek(c);
}

static int readline(Chunk *c) {
    assert(!BufferEmpty(c));

    int len = 0;
    while ((c->seek != c->tail || !len) && c->data[c->seek] != 0 &&
            c->data[c->seek] != '\r' && c->data[c->seek] != '\n') {
        c->seek = (c->seek + 1) % BufferMaxSize;
        ++len;
    }

    if (c->seek == c->tail) {
        return -1;
    }

    return BufferSeek(c);
}

static int writeBuffer(Chunk *c) {
    int len = BufferSeek(c);
    assert(len > 0);

    int n = write(c->fd, BufferHead(c), len);
    if (n > 0) {
        consume(c, n);
        logv("write %d bytes to %d", n, c->fd);
    }

    return n;
}

struct Buffer buffer = {
    init, fill, consume, consumeCRLF,
    flush, readline, writeBuffer
};

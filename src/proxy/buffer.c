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
    }
    logv("fill %d with return %d", c->fd, n);
    return n;
}

static void consume(Chunk *c, int x) {
    assert(x <= c->seek - c->head);
    c->head += x;
    logv("consume %d from %d", x, c->fd);

    if (c->tail == c->head) {
        reset(c);
    }
}

static char *flush(Chunk *c) {
    if (c->head <= c->tail) {
        return NULL;
    }

    c->seek = c->tail;
    c->data[c->seek] = 0;

    return &c->data[c->head];
}

struct Buffer buffer = {
    init, fill, consume,
    flush
};

#include "buffer.h"

static void reset(Chunk *c) {
    c->head = c->tail = c->seek = 0;
    logv("reset buffer %d", c->fd);
}

static void init(Chunk *c, int fd) {
    c->fd = fd;
    reset(c);
}

// assure buffer is not full
// treat c->head == c->tail as empty
static int fill(Chunk *c) {
    int len = BufferTail(c);
    int n = read(c->fd, &c->data[c->tail], len);
    if (n > 0) {
        c->tail = (c->tail + n) % BufferMaxSize;
        logv("fill %d with %d bytes", c->fd, n);
    }
    return n;
}

// assure buffer is not empty
// treat c->head == c->tail as full
static void consume(Chunk *c, int x) {
    assert(x <= BufferSeek(c));
    c->head = (c->head + x) % BufferMaxSize;
    logv("consume %d bytes from %d", x, c->fd);
}

// assure buffer is not empty
// treat c->head == c->tail as full
static int flush(Chunk *c) {
    c->seek = c->tail;

    return BufferSeek(c);
}

// assure buffer is not empty
// treat c->head == c->tail as full
static int readline(Chunk *c) {
    while ((c->seek != c->tail || c->tail == c->head) && c->data[c->seek] != 0 &&
            c->data[c->seek] != '\r' && c->data[c->seek] != '\n') {
        c->seek = (c->seek + 1) % BufferMaxSize;
    }

    if (c->seek == c->tail) {
        return -1;
    }

    c->data[c->seek] = 0;
    int len = BufferSeek(c);
    int next = (c->seek + 1) % BufferMaxSize;
    if (next != c->tail && c->data[c->seek] == '\r' &&
        c->data[next] == '\n') {
        c->seek = next;
    }
    c->data[c->seek] = 0;
    c->seek = (c->seek + 1) % BufferMaxSize;

    return len;
}

struct Buffer buffer = {
    init, fill, consume,
    flush, readline
};

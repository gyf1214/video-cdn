#include "buffer.h"

static void init(Chunk *c, int fd) {
    c->fd = fd;
    c->head = c->tail = c->seek = 0;
}

struct Buffer buffer = {
    init
};

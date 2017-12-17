#ifndef __PROXY_BUFFER
#define __PROXY_BUFFER

#include "common.h"

#define BufferMaxSize 65536

typedef struct Chunk {
    int fd, head, tail;
    char data[BufferMaxSize];
    // consume = 0, fill = 1
} Chunk;

#define BufferFull(c)      (((c)->tail + 1) % BufferMaxSize == (c)->head)
#define BufferEmpty(c)     ((c)->head == (c)->tail)
#define BufferHead(c)      (&(c)->data[(c)->head])
#define BufferTail(c)      (&(c)->data[(c)->tail])
#define BufferLen(c)       (((c)->tail - (c)->head + BufferMaxSize) % BufferMaxSize)
#define BufferCap(c)       (BufferMaxSize - 1 - BufferLen(c))

extern struct Buffer {
    // init a chunk based on fd
    void (*init)(Chunk *, int);

    // try read from fd and fill the chunk
    int (*fill)(Chunk *);

    // write to fd
    int (*write)(Chunk *);

    // consume the buffered data
    void (*consume)(Chunk *, int);

    // append at end
    int (*append)(Chunk *, const char *);
} buffer;

#endif

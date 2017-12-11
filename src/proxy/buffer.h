#ifndef __PROXY_BUFFER
#define __PROXY_BUFFER

#include "common.h"

#define BufferMaxSize 65536

typedef struct Chunk {
    int fd, head, tail, seek;
    char data[BufferMaxSize];
} Chunk;

#define BufferReady(c)   ((c)->head != (c)->tail)
#define BufferHead(c)    (&(c)->data[(c)->head])
#define BufferTail(c)    ((c)->tail >= (c)->head ? BufferMaxSize - (c)->tail : (c)->head - (c)->tail)
#define BufferSeek(c)    ((c)->seek > (c)->head ? (c)->seek - (c)->head : BufferMaxSize + (c)->seek - (c)->head)
#define BufferChar(c, x) ((c)->data[((c)->head + x) % BufferMaxSize])

extern struct Buffer {
    // init a chunk based on fd
    void (*init)(Chunk *, int);

    // try read from fd and fill the chunk
    int (*fill)(Chunk *);

    // consume the buffered data
    void (*consume)(Chunk *, int);

    // seek to tail
    int (*flush)(Chunk *);

    // seek '\n' & '\r' and return the line
    int (*readline)(Chunk *);
} buffer;

#endif

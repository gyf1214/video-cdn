#ifndef __PROXY_BUFFER
#define __PROXY_BUFFER

#include "common.h"

#define BufferMaxSize 65536

typedef struct Chunk {
    int fd, head, tail, seek;
    char data[BufferMaxSize];
} Chunk;

#define BufferReady(c)   ((c)->seek > (c)->head)
#define BufferFull(c)    ((c)->tail >= BufferMaxSize - 1)
#define BufferSeekLen(c) ((c)->seek - (c)->head)
#define BufferHead(c)    (&(c)->data[(c)->head])

extern struct Buffer {
    // init a chunk based on fd
    void (*init)(Chunk *, int);

    // try read from fd and fill the chunk
    int (*fill)(Chunk *);

    // consume the buffered data
    void (*consume)(Chunk *, int);

    // seek to tail
    char *(*flush)(Chunk *);

    // seek '\n' & '\r' and return the line
    char *(*readline)(Chunk *);
} buffer;

#endif

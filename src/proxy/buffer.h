#ifndef __PROXY_BUFFER
#define __PROXY_BUFFER

#include "common.h"

#define BufferMaxSize 65536

typedef struct Chunk {
    int fd, head, tail, seek;
    char data[BufferMaxSize];
    // consume = 0, fill = 1
    int lastOp;
} Chunk;

#define BufferReady(c)     ((c)->head != (c)->tail)
#define BufferFull(c)      ((c)->head == (c)->tail && (c)->lastOp)
#define BufferEmpty(c)     ((c)->head == (c)->tail && !(c)->lastOp)
#define BufferHead(c)      (&(c)->data[(c)->head])
#define BufferType(c, x)   ((c)->x > (c)->head || BufferEmpty(c))
#define BufferTail(c)      (BufferType(c, tail) ? BufferMaxSize - (c)->tail :\
                           (c)->head - (c)->tail)
#define BufferLen(c)       (BufferType(c, tail) ? (c)->tail - (c)->head :\
                           (c)->tail - (c)->head + BufferMaxSize)
#define BufferSeek(c)      (BufferType(c, seek) ? (c)->seek - (c)->head :\
                           (c)->seek - (c)->head + BufferMaxSize)
#define BufferChar(c, x)   ((c)->data[((c)->head + x) % BufferMaxSize])
#define BufferAppend(c, x) ((c)->data[(c)->tail] = x, (c)->lastOp = 1,\
                           (c)->tail = ((c)->tail + 1) % BufferMaxSize)

extern struct Buffer {
    // init a chunk based on fd
    void (*init)(Chunk *, int);

    // try read from fd and fill the chunk
    int (*fill)(Chunk *);

    // consume the buffered data
    void (*consume)(Chunk *, int);

    // consume end of line
    void (*consumeEOL)(Chunk *);

    // seek to tail
    int (*flush)(Chunk *);

    // seek '\n' & '\r' and return the line
    int (*readline)(Chunk *);

    // write to fd
    int (*write)(Chunk *);
} buffer;

#endif

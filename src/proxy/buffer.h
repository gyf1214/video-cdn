#ifndef __PROXY_BUFFER
#define __PROXY_BUFFER

#define BufferMaxSize 65536

typedef struct Chunk {
    int fd, head, tail, seek;
    char data[BufferMaxSize];
} Chunk;

extern struct Buffer {
    void (*init)(Chunk *, int);
    void (*fill)(void);
    char *(*readline)(void);
    char *(*flush)(void);
} buffer;

#endif

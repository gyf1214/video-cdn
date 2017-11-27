#include "server.h"
#include "io.h"
#include "buffer.h"

static struct {
    Socket *listen;
} local;

typedef struct {
    struct sockaddr_in peer;
    Socket *proxy;
    Chunk buf;
} Conn;

static void listenHandler(Socket *s, int flag) {
    assert(flag == IOWaitRead);
    logv("new connection");

    Conn *conn = malloc(sizeof(Conn));
    conn->proxy = NULL;

    Socket *cs = io.accept(s, &conn->peer);
    buffer.init(&conn->buf, cs->fd);
}

static void createServer(struct sockaddr_in *addr) {
    local.listen = io.socket(SOCK_STREAM, addr);
    io.listen(local.listen, listenHandler);
}

struct Server server = {
    createServer
};

#include "server.h"
#include "io.h"

static struct {
    Socket *listen;
} local;

static void listenHandler(Socket *s, int flag) {
    assert(flag == IOWaitRead);

    Socket *r = io.accept(s);
    io.close(r);
}

static void createServer(struct sockaddr *addr) {
    local.listen = io.socket(SOCK_STREAM, addr);
    io.listen(local.listen, listenHandler);
}

struct Server server = {
    createServer
};

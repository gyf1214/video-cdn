#include "config.h"
#include "io.h"

void serverHandler(Socket *s, int flag) {
    assert(flag == IOWaitRead);

    struct sockaddr_storage nothing;
    socklen_t len = sizeof(struct sockaddr_storage);
    int fd = accept(s->fd, (struct sockaddr *)&nothing, &len);
    success(fd);

    logv("accept %d", fd);

    success(close(fd));
}

int main(int argc, char **argv) {
    config.parse(argc, argv);
    io.init();

    Socket *server = io.socket(SOCK_STREAM, &config.listen);
    io.listen(server, serverHandler);

    return 0;
}

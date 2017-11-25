#include "io.h"

static struct {
    Socket *sock[IOMaxSocket];
    int nfds;
    fd_set waitRead, selectRead;
    fd_set waitWrite, selectWrite;
    int stop;
    struct timeval timeout;
} local;

static void waitSet(int fd, fd_set *set, int flag) {
    assert(!FD_ISSET(fd, set));
    FD_SET(fd, set);
    local.sock[fd]->flags |= flag;

    if (fd >= local.nfds) local.nfds = fd + 1;
}

static void clearSet(int fd, fd_set *set, int flag) {
    assert(FD_ISSET(fd, set));
    FD_CLR(fd, set);
    local.sock[fd]->flags &= ~flag;

    while (!local.sock[local.nfds - 1]->flags) --local.nfds;
}

#define waitRead(fd)   waitSet(fd, &local.waitRead, IOWaitRead)
#define waitWrite(fd)  waitSet(fd, &local.waitWrite, IOWaitWrite)
#define clearRead(fd)  clearSet(fd, &local.waitRead, IOWaitRead)
#define clearWrite(fd) clearSet(fd, &local.waitWrite, IOWaitWrite)

static void init() {
    memset(local.sock, 0, IOMaxSocket * sizeof(Socket *));
    local.nfds = 0;
    FD_ZERO(&local.waitRead);
    FD_ZERO(&local.waitWrite);
    local.stop = 0;
    local.timeout.tv_sec = IOTimeout;
    local.timeout.tv_usec = 0;
}

static Socket *newSocket(int fd) {
    Socket *sock = malloc(sizeof(Socket));
    sock->fd = fd;
    sock->flags = 0;
    sock->callback = NULL;
    sock->data = NULL;
    local.sock[fd] = sock;

    return sock;
}

static Socket *bindSocket(int type, const struct sockaddr* addr) {
    int fd = socket(PF_INET, type, 0);
    assert(fd >= 0 && fd < IOMaxSocket);
    success(bind(fd, addr, sizeof(struct sockaddr_in)));

    return newSocket(fd);
}

static void closeSocket(Socket *s) {
    assert(!s->flags);

    int fd = s->fd;
    local.sock[fd] = NULL;
    success(close(fd));

    free(s);
}

static void listenSocket(Socket *s, SocketCallback cb) {
    success(listen(s->fd, IOMaxListen));

    waitRead(s->fd);
    s->flags |= IOWaitLoop;
    s->callback = cb;
}

static void loop() {
    while (!local.stop) {
        FD_COPY(&local.waitRead, &local.selectRead);
        FD_COPY(&local.waitWrite, &local.selectWrite);
        int ret = select(local.nfds, &local.selectRead,
            &local.selectWrite, NULL, &local.timeout);

        success(ret);
        if (!ret) {
            logv("select timeout, nothing");
        } else {
            for (int i = 0; i < local.nfds; ++i) {
                Socket *sock = local.sock[i];
                if (FD_ISSET(i, &local.selectRead)) {
                    logv("read %d", i);
                    if (!(sock->flags & IOWaitLoop)) {
                        clearRead(i);
                    }
                    sock->callback(sock, IOWaitRead);
                } else if (FD_ISSET(i, &local.selectWrite)) {
                    logv("write %d", i);
                    if (!(sock->flags & IOWaitLoop)) {
                        clearWrite(i);
                    }
                    sock->callback(sock, IOWaitWrite);
                }
            }
        }
    }
}

struct IO io = {
    init, loop,
    bindSocket, closeSocket,
    listenSocket
};

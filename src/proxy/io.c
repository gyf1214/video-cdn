#include "io.h"

static struct {
    Socket *sock[IOMaxSocket];
    int nfds;
    fd_set waitRead, selectRead;
    fd_set waitWrite, selectWrite;
    int stop;
    struct timeval timeout, wait;
} local;

static void waitSet(Socket *sock, int flag) {
    if (!flag) return;

    sock->flags |= flag;
    if (flag & IOWaitRead) {
        FD_SET(sock->fd, &local.waitRead);
    }
    if (flag & IOWaitWrite) {
        FD_SET(sock->fd, &local.waitWrite);
    }

    if (sock->fd >= local.nfds) local.nfds = sock->fd + 1;
}

static void waitClear(Socket *sock, int flag) {
    if (!flag) return;

    sock->flags &= ~flag;
    if (flag & IOWaitRead) {
        FD_CLR(sock->fd, &local.waitRead);
    }
    if (flag & IOWaitWrite) {
        FD_CLR(sock->fd, &local.waitWrite);
    }

    while (!local.sock[local.nfds - 1] ||
           !local.sock[local.nfds - 1]->flags) --local.nfds;
}

static void setNonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    assert(flags != -1);
    assert(fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

static Socket *newSocket(int fd) {
    Socket *sock = malloc(sizeof(Socket));
    sock->fd = fd;
    sock->flags = 0;
    sock->callback = NULL;
    sock->data = NULL;
    local.sock[fd] = sock;
    setNonblock(fd);

    return sock;
}

static Socket *bindSocket(int type, const struct sockaddr_in* addr) {
    int fd = socket(PF_INET, type, 0);
    assert(fd >= 0 && fd < IOMaxSocket);
    success(bind(fd, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)));

    logv("bind %d to %s:%hu", fd, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    return newSocket(fd);
}

static void closeSocket(Socket *s) {
    waitClear(s, IOWaitRead | IOWaitWrite);

    int fd = s->fd;
    local.sock[fd] = NULL;
    success(close(fd));

    free(s);
    logv("close %d", fd);
}

static void listenSocket(Socket *s, SocketCallback cb) {
    success(listen(s->fd, IOMaxListen));

    waitSet(s, IOWaitRead);
    s->callback = cb;
    logv("listen %d", s->fd);
}

static void connectSocket(Socket *s, const struct sockaddr_in *addr) {
    int ret = connect(s->fd, (const struct sockaddr *)addr,
        sizeof(struct sockaddr_in));

    assert(!ret || errno == EINPROGRESS);
}

static void stopHandler(int sig) {
    logv("signal %d caught", sig);
    assert(sig == SIGINT || sig == SIGTERM);
    local.stop = 1;
    local.nfds = 0;
    int i;
    for (i = 0; i < IOMaxSocket; ++i) {
        if (local.sock[i]) {
            closeSocket(local.sock[i]);
        }
    }
}

static void init() {
    memset(local.sock, 0, IOMaxSocket * sizeof(Socket *));
    local.nfds = 0;
    FD_ZERO(&local.waitRead);
    FD_ZERO(&local.waitWrite);
    local.stop = 0;
    local.timeout.tv_sec = IOTimeout;
    local.timeout.tv_usec = 0;
    assert(signal(SIGINT, stopHandler) != SIG_ERR);
    assert(signal(SIGTERM, stopHandler) != SIG_ERR);
    // ignore SIGPIPE
    assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR);
}

static void loop() {
    while (1) {
        local.selectRead = local.waitRead;
        local.selectWrite = local.waitWrite;
        local.wait = local.timeout;
        int ret = select(local.nfds, &local.selectRead,
            &local.selectWrite, NULL, &local.wait);

        if (local.stop) break;

        success(ret);
        if (!ret) {
            logv("select timeout, nothing");
        } else {
            int i;
            for (i = 0; i < local.nfds; ++i) {
                Socket *sock = local.sock[i];
                if (!sock) continue;
                if (FD_ISSET(i, &local.selectRead)) {
                    logv("read signal from %d", i);
                    sock->callback(sock, IOWaitRead);
                } else if (FD_ISSET(i, &local.selectWrite)) {
                    logv("write signal from %d", i);
                    sock->callback(sock, IOWaitWrite);
                }
            }
        }
    }

    logv("loop end");
}

static Socket *acceptSocket(Socket *listen, struct sockaddr_in *addr) {
    struct sockaddr_in tmp;
    socklen_t len = sizeof(struct sockaddr_in);
    if (!addr) addr = &tmp;

    int fd = accept(listen->fd, (struct sockaddr *)addr, &len);
    success(fd);
    assert(len == sizeof(struct sockaddr_in));

    logv("accept %d from %s:%hu", fd, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    return newSocket(fd);
}

struct IO io = {
    init, loop,
    bindSocket, closeSocket,
    listenSocket, connectSocket, acceptSocket,
    waitSet, waitClear,
};

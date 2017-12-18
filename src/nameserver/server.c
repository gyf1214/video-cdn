#include "server.h"

#define BufferSize 512

static struct {
    int fd;
    struct sockaddr_in peer;
    char buf[BufferSize];
    DNSResponse resp;
    DNSError err;
    int stop;
} local;

static void stopHandler(int sig) {
    logv("caught signal %d", sig);
    local.stop = 1;
    if (local.fd >= 0) {
        close(local.fd);
    }
}

static void create(struct sockaddr_in *listen) {
    memcpy(local.resp.magic, ResponseMagic, ResponseSize);
    memcpy(local.err.magic, ErrorMagic, ErrorSize);
    local.stop = 0;
    local.fd = -1;
    assert(signal(SIGINT , stopHandler) != SIG_ERR);
    assert(signal(SIGTERM, stopHandler) != SIG_ERR);
    assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR);

    local.fd = socket(PF_INET, SOCK_DGRAM, 0);
    success(local.fd);

    success(bind(local.fd, (struct sockaddr *)listen, sizeof(struct sockaddr_in)));
    logv("bind %d to %s:%hu", local.fd,
         inet_ntoa(listen->sin_addr), ntohs(listen->sin_port));
}

static void loop() {
    while (1) {
        socklen_t len = sizeof(struct sockaddr_in);
        recvfrom(local.fd, local.buf, BufferSize, 0,
                 (struct sockaddr *)&local.peer, &len);

        if (local.stop) break;

        logv("recv from %s:%hu", inet_ntoa(local.peer.sin_addr),
             ntohs(local.peer.sin_port));
        DNSRequest *req = (DNSRequest *)local.buf;
        char *pat = req->magic + QueryHeader;
        const char *magic = QueryMagic + QueryHeader;

        if (memcmp(pat, magic, QuerySize - QueryHeader)) {
            local.err.id = req->id;
            sendto(local.fd, &local.err, sizeof(DNSError), 0,
                  (struct sockaddr *)&local.peer, len);
        } else {
            local.resp.id = req->id;

            // TODO
            local.resp.addr = inet_addr("1.2.3.4");

            sendto(local.fd, &local.resp, sizeof(DNSResponse), 0,
                  (struct sockaddr *)&local.peer, len);
        }
    }

    logv("loop end");
}

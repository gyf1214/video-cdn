#include "server.h"
#include "config.h"
#include "util.h"

static struct {
    int fd;
    struct sockaddr_in peer;
    char buf[MaxDNSSize];
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
        recvfrom(local.fd, local.buf, MaxDNSSize, 0,
                 (struct sockaddr *)&local.peer, &len);

        if (local.stop) break;

        logv("recv from %s:%hu", inet_ntoa(local.peer.sin_addr),
             ntohs(local.peer.sin_port));
        DNSRequest *req = (DNSRequest *)local.buf;
        char *pat = req->magic + QueryHeader;
        const char *magic = QueryMagic + QueryHeader;

        if (memcmp(pat, magic, QuerySize - QueryHeader)) {
            logv("dns request error");
            local.err.id = req->id;
            sendto(local.fd, &local.err, sizeof(DNSError), 0,
                  (struct sockaddr *)&local.peer, len);
        } else {
            logv("handle dns request");
            local.resp.id = req->id;

            local.resp.addr = util.resolve(local.peer.sin_addr.s_addr);
            struct in_addr addr = { local.resp.addr };

            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            fprintf(config.logging, "%ld.%06ld %s ",
                    now.tv_sec, now.tv_nsec / 1000,
                    inet_ntoa(local.peer.sin_addr));
            fprintf(config.logging, "%s %s\n",
                    BackendHost, inet_ntoa(addr));

            sendto(local.fd, &local.resp, sizeof(DNSResponse), 0,
                  (struct sockaddr *)&local.peer, len);
        }
    }

    logv("loop end");
}

struct Server server = {
    create, loop
};

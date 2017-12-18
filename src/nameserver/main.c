#include "common.h"

#define BufferSize 512

static char buf[BufferSize];
static DNSResponse response;

int main(int argc, char **argv) {
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    success(sock);

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(5353);

    success(bind(sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in)));

    response = getDNSResponse(0, 0);
    for (;;) {
        struct sockaddr_in peer;
        socklen_t len = sizeof(struct sockaddr_in);

        recvfrom(sock, buf, BufferSize, 0, (struct sockaddr *)&peer, &len);
        logv("recv from %s:%hu", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));

        uint16_t id = ((DNSRequest *)buf)->id;
        response.id = id;
        response.addr = inet_addr("1.2.3.4");

        sendto(sock, &response, sizeof(DNSResponse), 0, (struct sockaddr *)&peer, len);
    }
}

#include "config.h"

static void parse(int argc, char **argv) {
    assert(argc > 5);

    config.round = argc > 6 ? 1 : 0;
    int k = argc > 6 ? 1 : 0;

    config.logging = fopen(argv[k + 1], "w");
    assert(config.logging);

    uint16_t port;
    assert(sscanf(argv[k + 3], "%hu", &port) == 1);
    struct sockaddr_in *addr = &config.local;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(argv[k + 2]);
    addr->sin_port = htons(port);

    config.servers = fopen(argv[k + 4], "r");
    config.lsa = fopen(argv[k + 5], "r");
    assert(config.servers && config.lsa);
}

struct Config config = { parse };

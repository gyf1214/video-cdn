#include "config.h"

static void parse(int argc, char **argv) {
    assert(argc > 6);

    config.logging = fopen(argv[1], "w");
    assert(sscanf(argv[2], "%f", &config.alpha) == 1);

    uint16_t port;
    assert(sscanf(argv[3], "%hu", &port) == 1);
    struct sockaddr_in *addr = &config.listen;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);

    addr = &config.local;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(argv[4]);
    addr->sin_port = htons(0);

    assert(sscanf(argv[6], "%hu", &port) == 1);
    addr = &config.dns;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(argv[5]);
    addr->sin_port = htons(port);

    config.backendPort = htons(ConfigBackendPort);
    config.backendHost = ConfigHost;

    if (argc > 7) {
        config.backendIP = inet_addr(argv[7]);
    } else {
        config.backendIP = 0;
    }
}

struct Config config = { parse };

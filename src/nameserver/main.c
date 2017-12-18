#include "common.h"
#include "config.h"
#include "server.h"

int main(int argc, char **argv) {
    config.parse(argc, argv);

    server.create(&config.local);
    server.loop();
    fclose(config.logging);

    return 0;
}

#include "config.h"
#include "server.h"
#include "io.h"

int main(int argc, char **argv) {
    config.parse(argc, argv);

    io.init();
    server.create(&config.listen);
    io.loop();

    return 0;
}

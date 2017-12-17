#include "config.h"
#include "server.h"
#include "io.h"
#include "util.h"

int main(int argc, char **argv) {
    config.parse(argc, argv);
    util.init();
    io.init();
    server.create(&config.listen);

    io.loop();

    fclose(config.logging);
    return 0;
}

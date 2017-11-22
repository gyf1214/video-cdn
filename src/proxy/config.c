#include "config.h"

static void parse(int argc, char **argv) {
    printf("hello world\n");
}

struct Config config = {
    parse
};

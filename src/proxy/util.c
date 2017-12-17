#include "util.h"
#include "config.h"

#define MaxNumberSize 64

static struct {
    double avgTput;
} local;

static void init() {
    local.avgTput = 0.0;
}

static const char *itoaImp(int x) {
    static char buf[MaxNumberSize];

    sprintf(buf, "%d", x);
    return buf;
}

static double timeInterval(const struct timespec *begin,
                           const struct timespec *end) {
    return (double)(end->tv_sec  - begin->tv_sec)  +
           (double)(end->tv_nsec - begin->tv_nsec) * 1e-9;
}

static double recordTput(int size, double t) {
    double tput = (double)size / 125.0 / t;
    local.avgTput = config.alpha * tput + local.avgTput * (1.0 - config.alpha);

    return tput;
}

static double estimateTput() {
    return local.avgTput;
}

struct Util util = {
    init, itoaImp, timeInterval, recordTput, estimateTput
};

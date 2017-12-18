#include "util.h"
#include "config.h"
#include "dns.h"
#include "io.h"

#define MaxNumberSize 64
#define MaxListSize   128
#define MaxDNSSize    512

static struct {
    double avgTput;
    int listSize;
    int list[MaxListSize];
    int minRate;

    char encodedHost[MaxListSize];
    char dns[MaxDNSSize];
} local;

static void init() {
    local.avgTput = 0.0;
    local.listSize = 0;
    local.minRate = 0;

    char *str = local.encodedHost;
    char *now = (char *)config.backendHost;
    for (;;) {
        char *next = strchr(now, '.');
        if (!next) {
            break;
        }

        int size = next - now;
        *str++ = size;
        memcpy(str, now, size);
        now = next + 1;
    }
    *str = 0;
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

static void parseList(char *str) {
    const char *pattern = "bitrate=\"";
    int patLen = strlen(pattern);
    local.listSize = 0;

    for (;;) {
        str = strstr(str, pattern);
        if (!str) break;

        str += patLen;
        char *end = strchr(str, '\"');
        *end = 0;

        int rate = atoi(str);
        logv("get bitrate: %d", rate);
        local.list[local.listSize++] = rate;
        if (!local.minRate || local.minRate > rate) {
            local.minRate = rate;
        }

        str = end + 1;
    }
}

static int findBitrate() {
    int i;
    int best = local.minRate;
    for (i = 0; i < local.listSize; ++i) {
        int rate = local.list[i];
        if (rate <= local.avgTput / 1.5 && rate > best) {
            best = rate;
        }
    }
    return best;
}

// static void queryDNS() {
//     Socket *sock = io.socket(SOCK_DGRAM, &config.local);
//
//     int size = 0;
//
// }

struct Util util = {
    init, itoaImp, timeInterval,
    recordTput, estimateTput,
    parseList, findBitrate
};

#include "util.h"
#include "config.h"

#define MaxNodes    16
#define MaxNameSize 32
#define MaxLine     512

static struct {
    char node[MaxNodes][MaxNameSize];
    int dist[MaxNodes][MaxNodes];
    struct {
        int id;
        uint32_t ip;
    } server[MaxNodes], host[MaxNodes];
    int n, ns, nh, current;
} local;

static int findNode(const char *name, int insert) {
    int i;
    for (i = 0; i < local.n; ++i) {
        if (!strcmp(name, local.node[i])) {
            return i;
        }
    }

    if (insert) {
        strncpy(local.node[local.n], name, MaxNameSize);

        struct in_addr ip;
        if (inet_aton(name, &ip)) {
            local.host[local.nh].id = local.n;
            local.host[local.nh].ip = ip.s_addr;
            ++local.nh;
            logv("host %d: %s", local.n, name);
        } else {
            logv("node %d: %s", local.n, name);
        }

        return local.n++;
    }

    return -1;
}

static void init() {
    srand(time(NULL));
    local.n = local.ns = local.nh = 0;
    memset(local.dist, 0, sizeof(local.dist));

    char name[MaxNameSize], buf[MaxLine];
    int nothing;

    logv("load lsa");
    while (fscanf(config.lsa, "%s%d%s", name, &nothing, buf) != EOF) {
        int k = findNode(name, 1);

        char *tok = strtok(buf, ",");
        for (; tok; tok = strtok(NULL, ",")) {
            int m = findNode(tok, 1);
            if (!local.dist[k][m]) {
                logv("connect node %d <-> %d", k, m);
                local.dist[k][m] = local.dist[m][k] = 1;
            }
        }
    }
    fclose(config.lsa);

    logv("load servers");
    while (fscanf(config.servers, "%s", name) != EOF) {
        int k = findNode(name, 1);
        local.server[local.ns].id = k;
        local.server[local.ns].ip = inet_addr(name);
        logv("server %s at node %d", name, k);
    }
    fclose(config.servers);

    logv("calculate floyd");
    int i, j, k;
    for (i = 0; i < local.n; ++i) {
        for (j = 0; j < local.n; ++j) {
            local.dist[i][j] = i == j ? 0 : (local.dist[i][j] ? 1 : MaxNodes);
        }
    }
    for (k = 0; k < local.n; ++k) {
        for (i = 0; i < local.n; ++i) {
            for (j = 0; j < local.n; ++j) {
                int best = local.dist[i][k] + local.dist[k][j];
                local.dist[i][j] = min(local.dist[i][j], best);
            }
        }
    }

    local.current = 0;
}

static int findRandom() {
    return rand() % local.ns;
}

static int findRound() {
    int ret = local.current;
    local.current = (ret + 1) % local.ns;
    return ret;
}

static int findNearest(uint32_t src) {
    int i;
    for (i = 0; i < local.nh; ++i) {
        if (src == local.host[i].ip) {
            break;
        }
    }
    if (i >= local.ns) return findRandom();

    int k = local.host[i].id;
    int best = MaxNodes;
    int m = -1;
    for (i = 0; i < local.ns; ++i) {
        int id = local.server[i].id;
        if (local.dist[k][id] < best) {
            best = local.dist[k][id];
            m = i;
        }
    }

    return m >= 0 ? m : findRandom();
}

static uint32_t resolve(uint32_t src) {
    int k = config.round ? findRound() : findNearest(src);
    uint32_t ret = local.server[k].ip;
    struct in_addr addr = { ret };

    logv("resolve host as %s", inet_ntoa(addr));
    return ret;
}

struct Util util = {
    init, resolve
};

#ifndef __COMMON_INC_DNS
#define __COMMON_INC_DNS

#include <stdint.h>
#include <string.h>

#define BackendHost     "video.pku.edu.cn"
#define QuerySize       32
#define QueryMagic      "\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00"\
                        "\x05video\x03pku\x03""edu\x02cn\x00"\
                        "\x00\x01\x00\x01"
#define ResponseSize    60
#define ResponseMagic   "\x84\x00\x00\x01\x00\x01\x00\x00\x00\x00"\
                        "\x05video\x03pku\x03""edu\x02cn\x00"\
                        "\x00\x01\x00\x01"\
                        "\x05video\x03pku\x03""edu\x02cn\x00"\
                        "\x00\x01\x00\x01"\
                        "\x00\x00\x00\x00"\
                        "\x00\x04"

typedef struct DNSRequest {
    uint16_t id;
    char magic[QuerySize];
} DNSRequest;

typedef struct DNSResponse {
    uint16_t id;
    char magic[ResponseSize];
    uint32_t addr;
} DNSResponse;

inline DNSRequest getDNSRequest(uint16_t id) {
    DNSRequest req;
    req.id = id;
    memcpy(req.magic, QueryMagic, QuerySize);

    return req;
}

inline DNSResponse getDNSResponse(uint16_t id, uint32_t addr) {
    DNSResponse resp;
    resp.id = id;
    resp.addr = addr;
    memcpy(resp.magic, ResponseMagic, ResponseSize);

    return resp;
}

#endif

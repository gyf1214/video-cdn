#ifndef __COMMON_INC_DNS
#define __COMMON_INC_DNS

#include <stdint.h>
#include <string.h>

#define BackendHost     "video.pku.edu.cn"
#define QuerySize       32
#define QueryHeader     10
#define QueryMagic      "\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00"\
                        "\x05""video\x03""pku\x03""edu\x02""cn\x00"\
                        "\x00\x01\x00\x01"
#define ResponseSize    60
#define ResponseMagic   "\x84\x00\x00\x01\x00\x01\x00\x00\x00\x00"\
                        "\x05""video\x03""pku\x03""edu\x02""cn\x00"\
                        "\x00\x01\x00\x01"\
                        "\x05""video\x03""pku\x03""edu\x02""cn\x00"\
                        "\x00\x01\x00\x01"\
                        "\x00\x00\x00\x00"\
                        "\x00\x04"
#define ErrorSize       10
#define ErrorMagic      "\x84\x03\x00\x00\x00\x00\x00\x00\x00\x00"

#define Packed          __attribute__((__packed__))

typedef struct Packed DNSRequest {
    uint16_t id;
    char magic[QuerySize];
} DNSRequest;

typedef struct Packed DNSResponse {
    uint16_t id;
    char magic[ResponseSize];
    uint32_t addr;
} DNSResponse;

typedef struct Packed DNSError {
    uint16_t id;
    char magic[ErrorSize];
} DNSError;

#endif

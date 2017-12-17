#ifndef __COMMON_INC_DNS
#define __COMMON_INC_DNS

#include <stdint.h>

typedef struct DNSHeader {
    uint16_t id, flags;
    uint16_t qdcount, ancount;
    uint16_t nscount, arcount;
    char data[0];
} DNSHeader;

typedef struct DNSRecord {
    uint32_t typeClass;
    char request[0];
    uint32_t ttl;
    uint16_t length;
    char response[0];
} DNSRecord;

#endif

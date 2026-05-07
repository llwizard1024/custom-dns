#pragma once

#include <cstdint>

struct DnsHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

struct DnsQuestion {
    uint16_t qtype;
    uint16_t qclass;
};

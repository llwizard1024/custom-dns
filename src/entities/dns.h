#pragma once

#include <cstdint>
#include <netinet/in.h>

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

struct PendingQuery {
    uint16_t query_id;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    size_t question_len;
};

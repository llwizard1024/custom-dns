#pragma once

#include "entities/dns.h"

#include <cstdint>
#include <string>
#include <cstring>

std::string parse_dns_name(const uint8_t* buffer, size_t buffer_len, size_t& offset);
size_t build_response(
    const uint8_t* request_buffer,
    const DnsHeader& header,
    size_t question_end_offset,
    uint32_t ip_addr,
    uint8_t* response_buffer
);

uint16_t generate_dns_id();

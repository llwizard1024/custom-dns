#pragma once

#include "entities/dns.h"

#include <cstdint>
#include <cstring>

size_t build_response(
    const uint8_t* request_buffer,
    const dns_header& header,
    size_t question_end_offset,
    uint32_t ip_addr,
    uint8_t* response_buffer
);
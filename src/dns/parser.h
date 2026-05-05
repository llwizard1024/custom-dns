#pragma once

#include "entities/dns.h"

#include <string>
#include <cstdint>

std::string parse_dns_name(const uint8_t* buffer, size_t buffer_len, size_t& offset);
dns_header parse_header(const uint8_t* buffer, size_t& offset);
dns_question parse_question(const uint8_t* buffer, size_t& offset);
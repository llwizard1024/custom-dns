#include "dns/parser.h"

#include <cstring>
#include <netinet/in.h>

std::string parse_dns_name(const uint8_t* buffer, size_t buffer_len, size_t& offset) {
    std::string result;

    if (offset >= buffer_len) {
        return "";
    }

    while (true) {
        uint8_t len = buffer[offset];
        if (len == 0) {
            offset++;

            if (!result.empty()) result.pop_back(); // Delete last dot

            return result;
        }

        // Set offset pos to symbols length
        offset += 1;

        do {
            result.push_back(buffer[offset]);
            offset++;
            len--;
        } while (len != 0);

        result.push_back('.');
    }
}

dns_header parse_header(const uint8_t* buffer, size_t& offset) {
    dns_header headers;

    std::memcpy(&headers.id, &buffer[offset], sizeof(headers.id)); offset += 2;
    std::memcpy(&headers.flags, &buffer[offset], sizeof(headers.flags)); offset += 2;
    std::memcpy(&headers.qdcount, &buffer[offset], sizeof(headers.qdcount)); offset += 2;
    std::memcpy(&headers.ancount, &buffer[offset], sizeof(headers.ancount)); offset += 2;
    std::memcpy(&headers.nscount, &buffer[offset], sizeof(headers.nscount)); offset += 2;
    std::memcpy(&headers.arcount, &buffer[offset], sizeof(headers.arcount)); offset += 2;

    return headers;
}

dns_question parse_question(const uint8_t* buffer, size_t& offset) {
    dns_question question;

    std::memcpy(&question.qtype, &buffer[offset], sizeof(question.qtype)); offset += 2;
    question.qtype = ntohs(question.qtype);
    std::memcpy(&question.qclass, &buffer[offset], sizeof(question.qclass)); offset += 2;
    question.qclass = ntohs(question.qclass);

    return question;
}
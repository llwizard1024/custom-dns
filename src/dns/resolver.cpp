#include "dns/resolver.h"

#include "entities/dns.h"
#include "network/udp_socket.h"
#include "utils/parser.h"

#include <optional>
#include <random>

Resolver::Resolver() {
    client_fd_ = create_socket(0);
    
    if (client_fd_ == -1) {
        _exit(1);
    }
    
    struct in_addr bin_addr;
    const char* global_dns_ip = "1.1.1.1";
    int convert_result = inet_pton(AF_INET, global_dns_ip, &bin_addr);
    
    if (convert_result != 1) {
        _exit(1);
    }
    
    std::memset(&upstream_, 0, sizeof(upstream_));
    upstream_.sin_port = htons(53);
    upstream_.sin_family = AF_INET;
    upstream_.sin_addr.s_addr = bin_addr.s_addr;
}

void Resolver::send_query(const uint8_t* request_data, size_t request_len, const sockaddr_in& client_addr, socklen_t addr_len) {
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<uint16_t> dis(0, 65535); 
    
    uint16_t query_id = dis(gen);
    
    uint16_t network_id = htons(query_id);
    uint16_t qdcount = htons(1);
    
    uint8_t query[512] = {};
    size_t question_len = request_len - 12;
    
    std::memcpy(&query, &network_id, 2);
    std::memcpy(&query[4], &qdcount, 2);
    std::memcpy(&query[12], request_data + 12, question_len);
    
    if (sendto(client_fd_, query, 12 + question_len, 0, (struct sockaddr*)&upstream_, sizeof(upstream_)) == -1) {
        return;
    }
    
    PendingQuery pending_query = {query_id, client_addr, addr_len, question_len};
    std::memcpy(pending_query.request_buffer, request_data, 12 + question_len);
    pending_query.question_end_offset = 12 + question_len;
    
    pending_[query_id] = pending_query;
}

std::optional<std::pair<uint32_t, PendingQuery>> Resolver::handle_response(const uint8_t* response_data, ssize_t response_len) {
    if (response_len < 12) return std::nullopt;

    uint16_t response_id;
    std::memcpy(&response_id, response_data, 2);
    response_id = ntohs(response_id);

    auto it = pending_.find(response_id);
    if (it == pending_.end()) return std::nullopt;

    size_t offset = 12; // skip header

    parse_dns_name(response_data, response_len, offset); // QNAME
    offset += 4; // QTYPE + QCLASS

    while (offset < static_cast<size_t>(response_len)) {
        parse_dns_name(response_data, response_len, offset);

        if (offset + 10 > static_cast<size_t>(response_len))
            break;

        uint16_t type, rdlength;
        std::memcpy(&type, &response_data[offset], 2);
        type = ntohs(type);
        offset += 2;
        offset += 2; // class skip
        offset += 4; // ttl skip

        std::memcpy(&rdlength, &response_data[offset], 2);
        rdlength = ntohs(rdlength);
        offset += 2;

        if (offset + rdlength > static_cast<size_t>(response_len)) break;

        if (type == 1 && rdlength == 4) {
            uint32_t ip_addr;
            std::memcpy(&ip_addr, &response_data[offset], 4);
            return std::make_pair(ip_addr, it->second);
        }

        offset += rdlength;
    }

    pending_.erase(it);
    return std::nullopt;
}

int Resolver::get_client_fd() const {
    return client_fd_;
}

void Resolver::remove_query(uint16_t id) {
    pending_.erase(id);
}

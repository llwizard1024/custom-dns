#pragma once

#include "entities/dns.h"

#include <unordered_map>
#include <unistd.h>
#include <optional>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <string>
#include <unordered_set>


class Resolver {
    int client_fd_;
    struct sockaddr_in upstream_;
    std::unordered_map<uint16_t, PendingQuery> pending_;
    std::unordered_set<std::string> blocked_;
public:
    Resolver();
    ~Resolver() { if (client_fd_ != -1) close(client_fd_); }
    
    void send_query(const uint8_t* request_data, size_t request_len, const sockaddr_in& client_addr, socklen_t addr_len);
    std::optional<std::pair<uint32_t, PendingQuery>> handle_response(const uint8_t* response_data, ssize_t response_len);
    void remove_query(uint16_t id);
    
    void load_blocklist(const std::string& file_path);
    bool is_blocked(const std::string domain_name);
    
    int get_client_fd() const;
};

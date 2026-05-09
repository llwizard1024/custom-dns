#include "network/udp_socket.h"
#include "entities/dns.h"
#include "utils/parser.h"

#include <cerrno>
#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <random>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <iostream>

constexpr size_t DNS_BUFFER_SIZE = 512;
constexpr uint16_t DNS_PORT = 5353;

struct epoll_event server_ev, client_ev, events[64];

uint16_t generate_dns_id() {
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<uint16_t> dis(0, 65535); 
    
    return dis(gen);
}

int main() {
    int server_fd = create_socket(DNS_PORT); 
    int client_fd = create_socket(0);

    if (server_fd == -1 || client_fd == -1) {
        return -1;
    }

    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        close(server_fd);
        close(client_fd);
        return -1;
    }
    
    server_ev.events = EPOLLIN | EPOLLET;
    server_ev.data.fd = server_fd;

    client_ev.events = EPOLLIN | EPOLLET;
    client_ev.data.fd = client_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_ev)) {
        close(server_fd);
        close(client_fd);
        return -1;
    }

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev)) {
        close(server_fd);
        close(client_fd);
        return -1;
    }
    
    uint8_t server_buffer[512];
    uint8_t client_buffer[512];

    std::unordered_map<uint16_t, PendingQuery> pending;
    
    while(true) {
        int nfds = epoll_wait(epoll_fd, events, 64, -1);

        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }

            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                
                ssize_t n = recvfrom(server_fd, server_buffer, sizeof(server_buffer), 0,
                            (struct sockaddr*)&client_addr, &addr_len);
                
                if (n == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }
                    
                    return -1;
                }
                
                // Get data from server
                size_t offset = 12;
                
                DnsHeader header;
                DnsQuestion question;
                
                std::memcpy(&header, server_buffer, offset);
                std::string dns_name = parse_dns_name(server_buffer, sizeof(server_buffer), offset);
                std::memcpy(&question, &server_buffer[offset], 4); offset += 4;
                
                // Data for query to global DNS server
                // Header section (fill only id and qdcount)
                uint16_t query_id = generate_dns_id();
                uint16_t network_id = htons(query_id);
                
                uint8_t query[DNS_BUFFER_SIZE] = {};
                std::memcpy(&query, &network_id, 2);
                
                uint16_t qdcount = htons(1);
                std::memcpy(&query[4], &qdcount, 2);
                
                // Question section
                size_t question_len = offset - 12;
                std::memcpy(&query[12], &server_buffer[12], question_len);
                
                // Send
                struct in_addr bin_addr;
                const char* global_dns_ip = "1.1.1.1";
                int convert_result = inet_pton(AF_INET, global_dns_ip, &bin_addr);
                
                if (convert_result != 1) {
                    std::cout << "Error global dns ip convert\n";
                    exit(1);
                }
                
                struct sockaddr_in upstream;
                upstream.sin_port = htons(53);
                upstream.sin_family = AF_INET;
                upstream.sin_addr.s_addr = bin_addr.s_addr;
                
                sendto(client_fd, query, 12 + question_len, 0, (struct sockaddr*)&upstream, sizeof(upstream));
                
                // Save waited query
                PendingQuery pending_query = {query_id, client_addr, addr_len, question_len};
                pending[query_id] = pending_query;
                continue;
            } else if (events[i].data.fd == client_fd) {
                struct sockaddr_in upstream_response;
                socklen_t upstream_len = sizeof(upstream_response);
                
                // Get response from global dns
                ssize_t n = recvfrom(client_fd, client_buffer, sizeof(client_buffer), 0,
                         (struct sockaddr*)&upstream_response, &upstream_len);
                
                if (n == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }
                    
                    return -1;
                }
                
                uint16_t response_id;
                std::memcpy(&response_id, &client_buffer[0], 2);
                response_id = ntohs(response_id);
                
                auto it = pending.find(response_id);
                
                if (it == pending.end()) {
                    continue;
                }
                // Get ip from global dns
                uint32_t ip_addr;
                std::memcpy(&ip_addr, &client_buffer[12 + it->second.question_len + 12], 4);
                
                uint8_t response[DNS_BUFFER_SIZE];
                DnsHeader header;
                std::memcpy(&header, server_buffer, 12);
                size_t response_len = build_response(server_buffer, header,
                                                it->second.question_len + 12, ip_addr, response);
                
                sendto(server_fd, response, response_len, 0,
                       (struct sockaddr*)&it->second.client_addr,
                       it->second.client_addr_len);
                
                pending.erase(it);
            }
        }
    }
    
    return 0;
}

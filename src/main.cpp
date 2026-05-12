#include "network/udp_socket.h"
#include "entities/dns.h"
#include "utils/parser.h"
#include "dns/resolver.h"

#include <cerrno>
#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

constexpr size_t DNS_BUFFER_SIZE = 512;
constexpr uint16_t DNS_PORT = 5353;

struct epoll_event server_ev, client_ev, events[64];

int main() {
    int server_fd = create_socket(DNS_PORT); 
    Resolver resolver;
    resolver.load_blocklist("config/blocked_domains.txt");

    if (server_fd == -1 || resolver.get_client_fd() == -1) {
        return -1;
    }

    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        close(server_fd);
        return -1;
    }
    
    server_ev.events = EPOLLIN | EPOLLET;
    server_ev.data.fd = server_fd;

    client_ev.events = EPOLLIN | EPOLLET;
    client_ev.data.fd = resolver.get_client_fd();
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_ev)) {
        close(server_fd);
        return -1;
    }

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, resolver.get_client_fd(), &client_ev)) {
        close(server_fd);
        return -1;
    }
    
    uint8_t server_buffer[512];
    uint8_t client_buffer[512];
    
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
                std::string domain_name = parse_dns_name(server_buffer, sizeof(server_buffer), offset); offset += 4;
                
                if (resolver.is_blocked(domain_name)) {
                    DnsHeader header;
                    std::memcpy(&header, server_buffer, 12);
                    uint8_t response[DNS_BUFFER_SIZE];
                    
                    size_t response_len = build_response(server_buffer, header, offset, INADDR_ANY, response);
                    sendto(server_fd, response, response_len, 0,(struct sockaddr*)&client_addr, addr_len);
                    continue;
                }
                
                resolver.send_query(server_buffer, offset, client_addr, addr_len);
                continue;
            } else if (events[i].data.fd == resolver.get_client_fd()) {
                struct sockaddr_in upstream_response;
                socklen_t upstream_len = sizeof(upstream_response);
                
                // Get response from global dns
                ssize_t n = recvfrom(resolver.get_client_fd(), client_buffer, sizeof(client_buffer), 0,
                         (struct sockaddr*)&upstream_response, &upstream_len);
                
                if (n == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }
                    
                    return -1;
                }
                
                auto result = resolver.handle_response(client_buffer, n);
                
                if (result == std::nullopt) {
                    continue;
                }
                
                uint32_t ip_addr = result.value().first;
                
                auto& pq = result.value().second;
                
                uint8_t response[DNS_BUFFER_SIZE];
                DnsHeader header;
                std::memcpy(&header, pq.request_buffer, 12);
                size_t response_len = build_response(pq.request_buffer, header,
                                                pq.question_end_offset, ip_addr, response);

                sendto(server_fd, response, response_len, 0,
                       (struct sockaddr*)&result.value().second.client_addr,
                       result.value().second.client_addr_len);
                
                resolver.remove_query(result.value().second.query_id);
            }
        }
    }
    
    return 0;
}

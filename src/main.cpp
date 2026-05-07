#include "network/udp_socket.h"
#include "entities/dns.h"

#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <iostream>
#include <random>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unordered_map>

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

    std::unordered_map<uint16_t, PendingQuery> pending;
    
    while(true) {
        int nfds = epoll_wait(epoll_fd, events, 64, -1);
        for (int i = 0; i < nfds; ++i) {
            
        }
    }
    
    return 0;
}

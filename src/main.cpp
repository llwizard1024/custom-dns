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

constexpr size_t DNS_BUFFER_SIZE = 512;
constexpr uint16_t DNS_PORT = 5353;


uint16_t generate_dns_id() {
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<uint16_t> dis(0, 65535); 
    
    return dis(gen);
}

int main() {
    int fd = create_socket(DNS_PORT); 

    if (fd == -1) {
        return -1;
    }

    return 0;
}

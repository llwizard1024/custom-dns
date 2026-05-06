#include "network/udp_socket.h"
#include "dns/parser.h"
#include "dns/response.h"
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

    uint8_t buff[DNS_BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    
    while (true) {
        ssize_t n = recvfrom(fd, buff, sizeof(buff), 0, (struct sockaddr*)&client_addr, &addr_len);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            close(fd);
            return -1;
        }
        size_t offset = 0;
        struct dns_header header = parse_header(buff, offset);
        std::string domain_name = parse_dns_name(buff, sizeof(buff), offset);
        struct dns_question question = parse_question(buff, offset);        

        std::cout << "Domain: " << domain_name << ", QTYPE: " << question.qtype << ", QCLASS: " << question.qclass << std::endl;
        
        uint16_t rnd_dns_id = generate_dns_id();

        int fd_query = create_socket(0);
        if (fd_query == -1) {
            return -1;
        }
        uint8_t query[DNS_BUFFER_SIZE] = {};
        std::memcpy(&query[12], &buff[12], offset - 12);
        std::memset(query, 0, 12);
        uint16_t qdcount_query = 1;
        std::memcpy(&query[4], &qdcount_query, 2);

        struct sockaddr_in global_dns_addr;
        global_dns_addr.sin_port = htons(53);
        global_dns_addr.sin_family = AF_INET;
        if (inet_pton(AF_INET, "1.1.1.1", &global_dns_addr.sin_addr) != 1) {
            close(fd);
            close(fd_query);
            exit(1);
        }

        socklen_t global_dns_addr_len = sizeof(global_dns_addr);

        sendto(fd_query, query, offset, 0, (struct sockaddr*)&global_dns_addr, global_dns_addr_len);
        
        uint8_t query_response_buff[DNS_BUFFER_SIZE];
        ssize_t query_response = recvfrom(fd_query, query_response_buff, sizeof(query_response_buff), 0, (struct sockaddr*)&global_dns_addr, &global_dns_addr_len);

        if (query_response == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "CORRECT!!!!" << std::endl;
            }

            close(fd_query);
            return -1;
        }

        uint32_t ip_from_global_dns;
        std::memcpy(&ip_from_global_dns, &query_response_buff[28], 4);

        std::cout << ip_from_global_dns << std::endl;


        uint8_t response[DNS_BUFFER_SIZE] = {};
        size_t response_len = build_response(buff, header, offset, 0, response);

        sendto(fd, response, response_len, 0, (struct sockaddr*)&client_addr, addr_len);
    }

    return 0;
}

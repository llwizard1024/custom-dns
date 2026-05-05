#include "network/udp_socket.h"
#include "dns/parser.h"
#include "entities/dns.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <iostream>

constexpr size_t DNS_BUFFER_SIZE = 512;
constexpr uint16_t DNS_PORT = 5353;

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
        struct dns_header headers = parse_header(buff, offset);
        std::string domain_name = parse_dns_name(buff, sizeof(buff), offset);

        struct dns_question question = parse_question(buff, offset);        

        std::cout << "Domain: " << domain_name << ", QTYPE: " << question.qtype << ", QCLASS: " << question.qclass << std::endl;

        uint8_t response[DNS_BUFFER_SIZE] = {};
        std::memcpy(&response, &buff, 12);

        std::memcpy(&headers.flags, &response[2], sizeof(headers.flags));
        headers.flags = headers.flags | 0x8000;
        std::memcpy(&response[2], &headers.flags, sizeof(headers.flags));

        size_t question_len = offset - 12;
        std::memcpy(&response[12], &buff[12], question_len);

        size_t pos = 12 + question_len;
        uint16_t name_ptr = htons(0xC00C);
        std::memcpy(&response[pos], &name_ptr, sizeof(name_ptr)); pos += 2;

        uint16_t type = htons(1);
        std::memcpy(&response[pos], &type, sizeof(type)); pos += 2;
        
        uint16_t class_val = htons(1);
        std::memcpy(&response[pos], &class_val, sizeof(class_val)); pos += 2;

        uint32_t ttl = htonl(300);
        std::memcpy(&response[pos], &ttl, sizeof(ttl)); pos += 4;

        uint16_t rdlength = htons(4);
        std::memcpy(&response[pos], &rdlength, sizeof(rdlength)); pos += 2;

        uint32_t ip_addr = 0; // 0.0.0.0
        std::memcpy(&response[pos], &ip_addr, sizeof(ip_addr)); pos += 4;

        uint16_t ancount = 1;
        ancount = htons(ancount);
        std::memcpy(&response[6], &ancount, sizeof(ancount));

        sendto(fd, response, pos, 0, (struct sockaddr*)&client_addr, addr_len);
    }

    return 0;
}

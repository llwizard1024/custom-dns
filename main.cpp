#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>

bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

std::string parse_dns_name(const uint8_t* buffer, size_t buffer_len, size_t& offset) {
    std::string result;

    while (true) {
        uint8_t len = buffer[offset];
        if (len == 0) {
            offset++;
            result.pop_back(); // Delete last dot
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

int main() 
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd == -1) {
        return -1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(fd);
        return -1;
    }

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(5353);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        close(fd);
        return -1;
    }

    if (!set_nonblocking(fd)) {
        close(fd);
        return -1;
    }

    char buff[1024];
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

        uint16_t id, flags, qdcount, ancount, nscount, arcount;
        
        std::memcpy(&id, &buff[offset], sizeof(id));
        offset += 2;

        std::memcpy(&flags, &buff[offset], sizeof(flags));
        offset += 2;
        std::cout << "ID: " << ntohs(*(uint16_t*)&buff[0]) << std::endl;
        std::cout << "Flags: " << ntohs(*(uint16_t*)&buff[2]) << std::endl;
        std::cout << "qDcount: " << ntohs(*(uint16_t*)&buff[4]) << std::endl;
        std::cout << "aNcount: " << ntohs(*(uint16_t*)&buff[6]) << std::endl;
        std::cout << "nScount: " << ntohs(*(uint16_t*)&buff[8]) << std::endl;
        std::cout << "aRcount: " << ntohs(*(uint16_t*)&buff[10]) << std::endl;

        std::string domain_name = parse_dns_name((uint8_t*)buff, sizeof(buff), offset);

        std::cout << "QTYPE: " << ntohs(*(uint16_t*)&buff[offset]) << std::endl;
        offset += 2;
        std::cout << "QCLASS: " << ntohs(*(uint16_t*)&buff[offset]) << std::endl;
        offset += 2;

    }

    return 0;
}
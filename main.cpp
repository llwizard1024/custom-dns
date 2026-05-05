#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>

constexpr size_t DNS_BUFFER_SIZE = 512;
constexpr uint16_t DNS_PORT = 5353;

bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

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
    address.sin_port = htons(DNS_PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        close(fd);
        return -1;
    }

    if (!set_nonblocking(fd)) {
        close(fd);
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

        uint16_t id, flags, qdcount, ancount, nscount, arcount;
        
        std::memcpy(&id, &buff[offset], sizeof(id)); offset += 2;
        std::memcpy(&flags, &buff[offset], sizeof(flags)); offset += 2;
        std::memcpy(&qdcount, &buff[offset], sizeof(qdcount)); offset += 2;
        std::memcpy(&ancount, &buff[offset], sizeof(ancount)); offset += 2;
        std::memcpy(&nscount, &buff[offset], sizeof(nscount)); offset += 2;
        std::memcpy(&arcount, &buff[offset], sizeof(arcount)); offset += 2;

        std::string domain_name = parse_dns_name(buff, sizeof(buff), offset);

        uint16_t qtype, qclass;
        
        std::memcpy(&qtype, &buff[offset], sizeof(qtype));
        offset += 2;
        qtype = htons(qtype);
        std::memcpy(&qclass, &buff[offset], sizeof(qclass));
        offset += 2;
        qclass = htons(qclass);

        std::cout << "Domain: " << domain_name << ", QTYPE: " << qtype << ", QCLASS: " << qclass << std::endl;

        uint8_t response[DNS_BUFFER_SIZE] = {};
        std::memcpy(&response, &buff, 12);

        std::memcpy(&flags, &response[2], sizeof(flags));
        flags = flags | 0x8000;
        std::memcpy(&response[2], &flags, sizeof(flags));

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

        ancount = 1;
        ancount = htons(ancount);
        std::memcpy(&response[6], &ancount, sizeof(ancount));

        sendto(fd, response, pos, 0, (struct sockaddr*)&client_addr, addr_len);
    }

    return 0;
}

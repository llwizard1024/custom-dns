#include "utils/parser.h"

std::string parse_dns_name(const uint8_t* buffer, size_t buffer_len, size_t& offset) {
    std::string result;
    bool jumped = false;
    size_t jump_offset = 0;
    size_t current = offset;
    
    while (true) {
        if (current >= buffer_len) return "";
            
        uint8_t len = buffer[current];
        
        if ((len & 0xC0) == 0xC0) {
            if (current + 1 >= buffer_len)
                return "";
            
            uint16_t pointer = ((len & 0x3F) << 8) | buffer[current + 1];
            if (pointer >= buffer_len)
                return "";
            
            if (!jumped) {
                jump_offset = current + 2;
                jumped = true;
            }
            
            current = pointer;
            continue;
        }
        
        if (len == 0) {
            ++current;
            break;
        }

        ++current;
        if (current + len > buffer_len) return "";
        
        result.append(reinterpret_cast<const char*>(&buffer[current]), len);
        result += '.';
        current += len;
    }
    
    if (!result.empty())
        result.pop_back();

    offset = jumped ? jump_offset : current;
    return result;
}

size_t build_response(
    const uint8_t* request_buffer,
    const DnsHeader& header,
    size_t question_end_offset,
    uint32_t ip_addr,
    uint8_t* response_buffer
) {
    std::memcpy(response_buffer, request_buffer, 12);
    
    uint16_t header_flags = header.flags | 0x8000;
    std::memcpy(&response_buffer[2], &header_flags, sizeof(header_flags));
    
    size_t question_len = question_end_offset - 12;
    std::memcpy(&response_buffer[12], &request_buffer[12], question_len);
    
    size_t pos = 12 + question_len;
    uint16_t name_ptr = htons(0xC00C);
    std::memcpy(&response_buffer[pos], &name_ptr, sizeof(name_ptr)); pos += 2;
    
    uint16_t type = htons(1);
    std::memcpy(&response_buffer[pos], &type, sizeof(type)); pos += 2;
    
    uint16_t class_val = htons(1);
    std::memcpy(&response_buffer[pos], &class_val, sizeof(class_val)); pos += 2;
    
    uint32_t ttl = htonl(300);
    std::memcpy(&response_buffer[pos], &ttl, sizeof(ttl)); pos += 4;
    
    uint16_t rdlength = htons(4);
    std::memcpy(&response_buffer[pos], &rdlength, sizeof(rdlength)); pos += 2;
    
    std::memcpy(&response_buffer[pos], &ip_addr, sizeof(ip_addr)); pos += 4;
    
    uint16_t ancount = 1;
    ancount = htons(ancount);
    std::memcpy(&response_buffer[6], &ancount, sizeof(ancount));
    
    return pos;
} 

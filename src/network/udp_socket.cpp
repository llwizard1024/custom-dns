#include "network/udp_socket.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>

bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int create_socket(uint16_t port) {
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
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        close(fd);
        return -1;
    }

    if (!set_nonblocking(fd)) {
        close(fd);
        return -1;
    }

    return fd;
}

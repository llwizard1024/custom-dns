#pragma once

#include "network/udp_socket.h"

class Server {
    int server_fd_;
    int epoll_fd_;
public:
    Server() : server_fd_(create_socket(5353)), epoll_fd_(epoll_create1(0)) {}
    void run();
};

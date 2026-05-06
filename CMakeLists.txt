cmake_minimum_required(VERSION 3.16)
project(custom-dns VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dns_lib STATIC
    src/network/udp_socket.cpp
    src/dns/parser.cpp
    src/dns/response.cpp
)

target_include_directories(dns_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(dns_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(custom_dns src/main.cpp)
target_link_libraries(custom_dns PRIVATE dns_lib)
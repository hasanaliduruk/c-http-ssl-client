cmake_minimum_required(VERSION 4.2)
project(http_client C)

set(CMAKE_C_STANDARD 99)

add_executable(http_client main.c bytebuffer.c
        url_parser.c
        http_parser.c
        network.c
        http_client.c)
target_link_libraries(http_client ssl crypto)

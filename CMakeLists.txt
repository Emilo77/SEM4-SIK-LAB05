cmake_minimum_required(VERSION 3.1)

project(MIMUW_SIK_TCP_SERVERS)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-g -Wall -Wextra")

add_executable(file-client-tcp.c common.h err.h)
add_executable(file-server-tcp.c common.h err.h)
target_link_libraries(thread_server pthread)
add_executable(proc_server proc_server.c common.h err.h)
add_executable(reuse_server reuse_server.c common.h err.h)
add_executable(preforked_server preforked_server.c common.h err.h)

install(TARGETS DESTINATION .)

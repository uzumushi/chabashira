cmake_minimum_required(VERSION 2.8)
project(master)

find_package(Threads)

set(CMAKE_CXX_STANDARD 11)

add_executable(master master.cpp client_socket.cpp)
target_link_libraries (master ${CMAKE_THREAD_LIBS_INIT})

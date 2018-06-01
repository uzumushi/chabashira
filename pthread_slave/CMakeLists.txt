cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)
find_package(Threads)

add_executable(slave pthread_slave.cpp client_socket.cpp)
target_link_libraries (slave ${CMAKE_THREAD_LIBS_INIT})

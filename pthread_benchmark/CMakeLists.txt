cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)
find_package(Threads)

add_executable(bench pthread_bench.cpp )
target_link_libraries (bench ${CMAKE_THREAD_LIBS_INIT})

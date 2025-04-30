cmake_minimum_required(VERSION 3.20)
project(http_epoll_server)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加include目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(epoll)
add_subdirectory(io_uring)
add_subdirectory(test)
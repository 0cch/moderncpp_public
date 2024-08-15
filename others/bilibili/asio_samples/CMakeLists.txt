cmake_minimum_required(VERSION 3.20)

project(asio_samples)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories("path/to/asio/include")

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_subdirectory(sample1)
add_subdirectory(sample2)
add_subdirectory(sample3)
add_subdirectory(sample4)


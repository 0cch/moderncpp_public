# 查找 liburing 库
find_library(LIBURING_LIBRARY uring)

# 添加源文件
set(SOURCES
    ../src/main.cc
    http_server.cc
    http_connection.cc
    ../src/http_request.cc
    ../src/http_response.cc
)

# 添加头文件
set(HEADERS
    http_server.h
    http_connection.h
    ../src/http_request.h
    ../src/http_response.h
)

# 创建可执行文件
add_executable(http_server_io_uring ${SOURCES} ${HEADERS})

# 添加include目录
target_include_directories(http_server_io_uring PRIVATE .)

# 链接必要的库
target_link_libraries(http_server_io_uring pthread ${LIBURING_LIBRARY})
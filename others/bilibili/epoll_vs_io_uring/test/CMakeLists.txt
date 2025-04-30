# 添加可执行文件
add_executable(http_benchmark http_benchmark.cc)

# 链接必要的库
target_link_libraries(http_benchmark pthread)
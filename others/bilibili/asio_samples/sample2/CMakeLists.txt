add_executable(timer timer.cpp)

add_executable(async_httpsclient async_httpsclient.cpp)
target_link_libraries(async_httpsclient ${OPENSSL_LIBRARIES} Crypt32.lib)
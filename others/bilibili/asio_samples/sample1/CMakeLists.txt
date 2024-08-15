add_executable(httpclient httpclient.cpp)

add_executable(httpsclient httpsclient.cpp)
target_link_libraries(httpsclient ${OPENSSL_LIBRARIES} Crypt32.lib)
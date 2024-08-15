
include_directories("path/to/llhttp/include")

file(GLOB LLHTTP_SRC
     "path/to/llhttp/src/*.c"
)

add_executable(httpserver3 main.cpp httpserver.cpp ${LLHTTP_SRC})

add_executable(httpserver4 main2.cpp httpserver2.h ${LLHTTP_SRC})
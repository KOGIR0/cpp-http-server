CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
set (CMAKE_CXX_STANDARD 17)

project(server)

add_executable(server 
    src/main.cpp
    src/Server.cpp
    src/Socket.cpp
    src/File.cpp
    src/Request.cpp
    src/Response.cpp
    src/html.cpp
    )
target_include_directories(server PRIVATE include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)


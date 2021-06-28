CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

project(server)
add_executable(server main.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)


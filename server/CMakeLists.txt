cmake_minimum_required(VERSION 3.16)
project(server)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(server server.cpp)

target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}/utils)

target_link_libraries(server Qt6::Core Qt6::Network)
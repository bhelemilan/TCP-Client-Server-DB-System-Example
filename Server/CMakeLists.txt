cmake_minimum_required(VERSION 3.8)
project(Server)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES server.c)
add_executable(Server ${SOURCE_FILES})
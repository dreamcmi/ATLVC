cmake_minimum_required(VERSION 4.0)

set(CMAKE_C_STANDARD 11)

add_library(libatlvc atlvc.c atlvc_check.c)

target_include_directories(libatlvc PUBLIC .)

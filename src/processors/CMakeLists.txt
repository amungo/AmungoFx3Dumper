cmake_minimum_required(VERSION 2.4)
project(processors)
include_directories(${AMGDMP_ROOT})

add_library(processors STATIC
    streamdumper.cpp streamdumper.h
)

target_link_libraries(processors)

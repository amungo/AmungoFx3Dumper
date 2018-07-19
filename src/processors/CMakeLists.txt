cmake_minimum_required(VERSION 2.4)
project(processors)
include_directories(${AMGDMP_ROOT})

add_library(processors STATIC
    dumpers_ifce.h
    streamdumper.cpp streamdumper.h
    socketdumper.cpp socketdumper.h
)

target_link_libraries(processors net)

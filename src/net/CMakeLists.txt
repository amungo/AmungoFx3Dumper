cmake_minimum_required(VERSION 2.4)
project(net)
include_directories(${AMGDMP_ROOT})

if(WIN32)
    #link with winsock2 library
    set(SOCK_LIBNAME "ws2_32")
else()
    set(SOCK_LIBNAME "")
endif()

add_library(net STATIC
    TCPCommon.cpp TCPCommon.h
)

target_link_libraries(net ${SOCK_LIBNAME})


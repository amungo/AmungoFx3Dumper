cmake_minimum_required(VERSION 2.4)
project(hwfx3)
include_directories(${AMGDMP_ROOT})

add_library(hwfx3 STATIC
    fx3dev.cpp fx3dev.h
    fx3devdebuginfo.h
    fx3deverr.cpp fx3deverr.h
    fx3devifce.h
    fx3fwparser.cpp fx3fwparser.h
    HexParser.cpp HexParser.h
    host_commands.h
    libusb.h
    pointdrawer.cpp
    pointdrawer.h
)


target_link_libraries(hwfx3)


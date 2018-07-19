cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)

project(amgdmp)

set(AMGDMP_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_C_FLAGS   "-O2 -g -march=core2 -std=c99   -mfpmath=sse -funroll-loops -fopenmp -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-O2 -g -march=core2 -std=c++11 -mfpmath=sse -funroll-loops -fopenmp -Wall -Wextra")

if (WIN32)
    link_directories("./libs/libusb/MinGW32/static")
    set(LIBUSB_LIBNAME "libusb-1.0.a")
else()
    set(LIBUSB_LIBNAME "libusb-1.0")
endif()


include_directories(${AMGDMP_ROOT})

set(SOURCE_FILES main.cpp )

add_subdirectory("hwfx3")
add_subdirectory("processors")
add_subdirectory("sys")
add_subdirectory("net")

add_executable(amgdmp ${SOURCE_FILES})
target_link_libraries(
    amgdmp
    hwfx3
    processors
    sys
    net
    ${LIBUSB_LIBNAME}
)

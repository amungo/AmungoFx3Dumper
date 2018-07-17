cmake_minimum_required(VERSION 2.4)
project(sys)
include_directories(${AMGDMP_ROOT})

add_library(sys STATIC
    Runnable.cpp Runnable.h
    BlockQueue.h
    AsyncQueueHandler.h
    MessageReceiverIfce.h
    NamedClass.cpp NamedClass.h
    SingleEvent.h
)

target_link_libraries(sys)

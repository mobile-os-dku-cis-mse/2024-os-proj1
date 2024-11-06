cmake_minimum_required(VERSION 3.29)
project(2024_os_proj1 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(scheduler
        src/main.c
        src/scheduler.c
        src/util/process.c
        src/util/process.h
        src/child.c
        src/child.h
)

cmake_minimum_required(VERSION 3.29)
project(2024_os_proj1 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(scheduler
        src/main.c
        src/entity/scheduler.c
        src/util/process.c
        src/util/process.h
        src/entity/child.c
        src/entity/child.h
        src/util/pid_queue.c
        src/util/pid_queue.h
        src/util/timer.c
        src/util/timer.h
        src/util/messge_queue.c
        src/util/messge_queue.h
        src/entity/child.c
)

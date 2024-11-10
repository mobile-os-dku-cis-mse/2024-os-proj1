#pragma once

#include "common.h"

typedef enum{
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} ProcessState;

typedef struct{
    int pid;
    
    int original_cpu_burst;
    int original_io_burst;

    int cpu_burst;
    int io_burst;
    int remaining_time;

    double arrival_time;
    double start_time;
    
    int flag;
    int finish_flag;

    ProcessState state;
} Process;

Process* create_process(int pid, int cpu_burst, int io_burst, int time_tick);
void update_process_state(Process *process, ProcessState new_state);
void terminate_process(Process *process);

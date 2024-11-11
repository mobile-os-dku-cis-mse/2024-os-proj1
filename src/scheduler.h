#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/types.h>

#define NUM_CHILDREN 10
#define TIME_QUANTUM 2
#define CPU_BURST_MAX 5
#define IO_BURST_MAX 5
#define MAX_TICKS 10000

extern struct Scheduler *global_scheduler;

struct my_msgbuf {
    long mtype;
    int pid;
    int io_time;
};

struct Process {
    pid_t pid;
    int cpu_burst;
    int io_burst;
    int remaining_time;
    int in_io;
};

struct Scheduler {
    struct Process processes[NUM_CHILDREN];
    int run_queue[NUM_CHILDREN];
    int wait_queue[NUM_CHILDREN];
    int run_queue_size;
    int wait_queue_size;
    int msgid;
    int current_process;
    int time_ticks;
    int fd;
};

void initialize_timer();
void create_child_processes(struct Scheduler *scheduler);
void alarm_handler(int sig);
void log_scheduling_operation(const struct Scheduler *scheduler, const char *message);
void dump_queues(const struct Scheduler *scheduler);

#endif

#ifndef CHILD_H
#define CHILD_H

#define CPU_BURST_MAX 5
#define IO_BURST_MAX 5

struct my_msgbuf {
    long mtype;
    int pid;
    int io_time;
};

void child_process();

#endif

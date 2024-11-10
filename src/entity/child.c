//
// Created by hochacha on 24. 11. 6.
//

#include "child.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/msg.h>

#include "src/util/message_queue.h"
#ifndef TIME_QUANTUAM
#define TIME_QUANTUAM 5
#endif

int cpu_burst = 0;  // the time for cpu execute at one time quantum
int io_burst = 0;   // the time for io execute at one request
int cpu_time = 0;   // total CPU executing time
int io_time = 0;    // IO로 요청하는 시간
int total_exec_time = 0;
// 따라서, CPU Time 전부 소진하면, IO Time 만큼 IO 수행 요청

int process_state = 0;
int EOP = 0; // End of Process


void child_msgsnd(int io_time) {
    io_msg msg = {MESSAGE_TYPE_IO_REQ, getpid(), io_time};
    if(msgsnd(msg_queue_id, &io_time, sizeof(int), IPC_NOWAIT) == -1) {
        perror("child_msgsnd");
        exit(EXIT_FAILURE);
    }
}


// time tick flow
// job: decrement 1 for cpu burst
void child_SIGALRM(int sig) {
    if(process_state == PROCESS_RUNNING && !EOP) {
        cpu_time--;
    }
    if(cpu_time <= 0) {
        process_state = PROCESS_BLOCKED;
        total_exec_time+= io_time;
        cpu_time = 5;

    }

}

// schedule in
// job: set the cpu_burst for this process
void child_SIGUSR1(int sig) {
    cpu_burst = TIME_QUANTUAM;
    process_state = PROCESS_RUNNING;
    printf("[Child::%d] scheduled in\n", getpid());
}

// schedule out
// job: stop the process
void child_SIGUSR2(int sig) {
    process_state = PROCESS_READY;
    printf("[Child::%d] scheduled out\n", getpid())
}

void init_child() {
    // enroll the SIGNAL Handler
    if(signal(SIGALRM, child_SIGALRM) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if(signal(SIGUSR1, child_SIGUSR1) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if(signal(SIGUSR2, child_SIGUSR2) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    srand(time(NULL));
    cpu_time = rand() % 10 + 1;
    io_time = rand() % 10 + 1;

    if(init_msg_queue() == -1) {
        perror("init_msg_queue");
        exit(1);
    }

    printf("[do_work_child] init child process\n");
}

int child_main(int argc, char **argv) {
    init_child();
#ifdef DEBUG
    printf("[Child]");
#endif
    while(1) {
        //printf("[Child] %d spinning\n", getpid());
    }
    exit(0);
}
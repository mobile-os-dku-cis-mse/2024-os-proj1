//
// Created by hochacha on 24. 11. 6.
//

#include "child.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef TIME_QUANTUAM
#define TIME_QUANTUAM 5
#endif


int cpu_burst = 0;
int io_burst = 0;
int run_flag = 0;
int io_burst_flag = 0;

// time tick flow
// job: decrement 1 for cpu burst
void child_SIGALRM(int sig) {
    if (run_flag) {
        cpu_burst--;
    }
}

// schedule in
// job: set the cpu_burst for this process
void child_SIGUSR1(int sig) {
    cpu_burst = TIME_QUANTUAM;
    run_flag = 1;
}

// schedule out
// job: stop the process
void child_SIGUSR2(int sig) {
    run_flag = 0;
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

    printf("[do_work_child] init child process");

    child_main(0, NULL);
}

int child_main(int argc, char **argv) {

    /* some of the works gonna held */
    if(io_burst_flag) {
        /* do IO stuff work - msgsnd to kernel (parent process) */
    }
}

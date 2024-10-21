#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include "timer.h"
#include "child_process.h"

#define TIME_QUANTUM 1 // time quantum in seconds

void setup_timer()
{
    struct itimerval timer;
    timer.it_value.tv_sec = TIME_QUANTUM;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = TIME_QUANTUM;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL); // Set up periodic timer
}

void alarm_handler(int sig)
{
    // Handle context switch or other actions when timer triggers
    switch_to_next_child();
}

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>

#define MAX_PROCESSES 10

extern int counter;
extern int wait_proc_count;
extern int turn_proc_count;
extern int burst_limit;

extern int time_count;

extern int time_tick;
extern int time_quantum;

extern double wait_time;
extern double turnaround_time;
extern double total_wait_time;

extern int schedule_policy;
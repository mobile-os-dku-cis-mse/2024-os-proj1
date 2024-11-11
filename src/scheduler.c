#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <string.h>

struct Scheduler *global_scheduler = NULL;

void log_scheduling_operation(const struct Scheduler *scheduler, const char *message) {
    write(scheduler->fd, message, strlen(message));
}

void dump_queues(const struct Scheduler *scheduler) {
    char buffer[256];
    sprintf(buffer, "Time %d - Run Queue: [", scheduler->time_ticks);
    for (int i = 0; i < scheduler->run_queue_size; i++) {
        sprintf(buffer + strlen(buffer), " %d ", scheduler->processes[scheduler->run_queue[i]].pid);
    }
    sprintf(buffer + strlen(buffer), "] Wait Queue: [");
    for (int i = 0; i < scheduler->wait_queue_size; i++) {
        sprintf(buffer + strlen(buffer), " %d ", scheduler->processes[scheduler->wait_queue[i]].pid);
    }
    sprintf(buffer + strlen(buffer), "]\n");
    log_scheduling_operation(scheduler, buffer);
}

void alarm_handler(int sig) {
    struct Scheduler *scheduler = global_scheduler;
    scheduler->time_ticks++;

    if (scheduler->time_ticks > MAX_TICKS) {
        exit(0);
    }

    struct my_msgbuf msg;
    const struct Process *current_proc = &scheduler->processes[scheduler->run_queue[scheduler->current_process]];

    if (current_proc->remaining_time > 0) {
        msg.mtype = current_proc->pid;
        msg.pid = current_proc->pid;
        msg.io_time = current_proc->io_burst;

        if (msgsnd(scheduler->msgid, &msg, sizeof(msg), IPC_NOWAIT) == -1) {
            perror("msgsnd failed");
        }

        char buffer[128];
        sprintf(buffer, "At time %d, process %d gets CPU time, remaining CPU burst %d\n",
                scheduler->time_ticks, current_proc->pid, current_proc->remaining_time);
        log_scheduling_operation(scheduler, buffer);
    }

    for (int i = 0; i < scheduler->wait_queue_size; i++) {
        scheduler->processes[scheduler->wait_queue[i]].io_burst--;
        if (scheduler->processes[scheduler->wait_queue[i]].io_burst <= 0) {
            scheduler->run_queue[scheduler->run_queue_size++] = scheduler->wait_queue[i];
            for (int j = i; j < scheduler->wait_queue_size - 1; j++) {
                scheduler->wait_queue[j] = scheduler->wait_queue[j + 1];
            }
            scheduler->wait_queue_size--;
            i--;
        }
    }

    scheduler->current_process = (scheduler->current_process + 1) % scheduler->run_queue_size;
    dump_queues(scheduler);
}

void initialize_timer() {
    struct itimerval timer;
    struct sigaction sa;

    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }

    timer.it_interval.tv_sec = TIME_QUANTUM;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = TIME_QUANTUM;
    timer.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer failed");
        exit(1);
    }
}

void create_child_processes(struct Scheduler *scheduler) {
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("./child", "./child", NULL);
        } else if (pid > 0) {
            scheduler->processes[i].pid = pid;
            scheduler->processes[i].cpu_burst = rand() % CPU_BURST_MAX + 1;
            scheduler->processes[i].io_burst = rand() % IO_BURST_MAX + 1;
            scheduler->processes[i].remaining_time = scheduler->processes[i].cpu_burst;
            scheduler->processes[i].in_io = 0;
            scheduler->run_queue[i] = i;
        } else {
            perror("fork failed");
            exit(1);
        }
    }
}

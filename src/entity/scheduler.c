//
// Created by hochacha on 24. 11. 6.
//


#include "scheduler.h"

#include <signal.h>


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "src/util/pid_queue.h"
#include "../util/timer.h"
#include "../util/messge_queue.h"

#define DEBUG

unsigned int current_time = 0;
pcb_queue* ready_queue = NULL;
pcb_queue* waiting_queue = NULL;
pcb_t* current_process = NULL;

void alarm_handler(int sig) {
    current_time++;  // global time increment

    // currently time decrement happens only CPU scheduling,
    // it's seem to implement the I/O schedule
    if(current_process != NULL && current_process->state == PROCESS_RUNNING \
        && current_process->remain_time > 0) {
        // Time tick pass
        kill(current_process->pid, SIGALRM);
        current_process->remain_time--;

#ifdef DEBUG
        printf("[Tick :: %d] Process %d: remain time = %d",
            current_time, current_process->pid, current_process->remain_time);
#endif
        // if time out -> schedule out
        if(current_process->remain_time == 0) {
            // schedule out
            kill(current_process->pid, SIGUSR2);
            current_process->state = PROCESS_READY;
        }

        enqueue_pcb(ready_queue, current_process);
    }else {
        // if there is no running process
        if(is_queue_empty(ready_queue) != 1) {
            // schedule in the child process
            current_process = dequeue_pcb(ready_queue);
            kill(current_process->pid, SIGUSR1);
        } else {
            // ready queue empty
            // in this situation, I/O queue full or child process have done for work
            if(is_queue_empty(waiting_queue) != 1) {
                // wait for IO Queue done
            } else {
                // child processes have done
                exit(0);
            }
        }
    }

    // check I/O schedule request
    // just polling the message


}

void io_schedule_handler(int sig) {
    // not implemented

}

void scheduler_init(pcb_queue* ready_queue) {
    if(signal(SIGALRM, alarm_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    // wait! for scheduler, SIGUSR2 is work for I/O scheduler
    if(signal(SIGUSR2, io_schedule_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    if(init_msg_queue() == -1) {
        perror("init_msg_queue");
    }

    setup_timer();

    ready_queue = ready_queue;
    waiting_queue = create_pcb_queue();
    
#ifdef DEBUG
    printf("[Tick :: %d] scheduler lunch", current_time);
#endif
}

void scheduler_run(pcb_queue* ready_queue, int n_process) {
    scheduler_init(ready_queue);

    start_scheduler();

    // just work with handler function
    while(1);
}

void start_scheduler() {
    current_process = dequeue_pcb(waiting_queue);
    current_process->remain_time = PROCESS_RUNNING;
    // notify child scheduled
    kill(current_process->pid, SIGUSR1);
}

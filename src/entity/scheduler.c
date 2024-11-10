//
// Created by hochacha on 24. 11. 6.
//


#include "scheduler.h"

#include <signal.h>


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include "src/util/pid_queue.h"
#include "../util/timer.h"
#include "../util/message_queue.h"

#define DEBUG

unsigned int current_time = 0;
pcb_queue* ready_queue_schd = NULL;
pcb_queue* waiting_queue_schd = NULL;
pcb_t* current_process = NULL;

void sigint_handler(int sig) {
    if(sig == SIGINT) {
        pcb_t* current_process = dequeue_pcb(ready_queue_schd);
        while(current_process != NULL) {
            kill(current_process->pid, SIGTERM);
            waitpid(current_process->pid, NULL, 0);
            current_process = dequeue_pcb(ready_queue_schd);
        }
        current_process = dequeue_pcb(waiting_queue_schd);
        while(current_process != NULL) {
            kill(current_process->pid, SIGTERM);
            waitpid(current_process->pid, NULL, 0);
            current_process = dequeue_pcb(waiting_queue_schd);
        }
    }
    exit(0);
}

void alarm_handler(int sig) {
    current_time++;  // global time increment

    // currently time decrement happens only CPU scheduling,
    // it's seem to implement the I/O schedule
    if(current_process != NULL && current_process->state == PROCESS_RUNNING \
        && current_process->remaining_time > 0) {
        // Time tick pass
        kill(current_process->pid, SIGALRM);
        current_process->remaining_time--;

#ifdef DEBUG
        printf("[Tick :: %d] Process %d: remain time = %d\n",
            current_time, current_process->pid, current_process->remaining_time);
#endif
        // if time out -> schedule out
        if(current_process->remaining_time == 0) {
            // schedule out
            kill(current_process->pid, SIGUSR2);
            current_process->state = PROCESS_READY;
        }

        enqueue_pcb(ready_queue_schd, current_process);
    }else {
        // if there is no running process
        if(is_queue_empty(ready_queue_schd) != 1) {
            // schedule in the child process
            current_process = dequeue_pcb(ready_queue_schd);
            kill(current_process->pid, SIGUSR1);
        } else {
            // ready queue empty
            // in this situation, I/O queue full or child process have done for work
            if(is_queue_empty(waiting_queue_schd) != 1) {
                // wait for IO Queue done
            } else {
                // child processes have done
#ifdef DEBUG
                printf("\n======================================================\n");
                printf("[:::TERMINATION::::] The program has been terminated");
                printf("\n======================================================\n");
#endif
                exit(0);
            }
        }
    }
    // check I/O schedule (FIFO) request
    // just polling the message
    /* IO burst */
}

// when the child request for IO job
void io_schedule_handler(int sig) {

    // get io scheduling message
    io_msg msg = {0};
    if(msgrcv(msg_queue_id, &msg, sizeof(io_msg),1,0) == -1) {
        perror("msgrcv");
        exit(1);
    }

}

void scheduler_init(pcb_queue* ready_queue) {

    // wait! for scheduler, SIGUSR2 is work for I/O scheduler
    if(signal(SIGUSR2, io_schedule_handler) == SIG_ERR) {
        perror("signal");
        sigint_handler(SIGINT);
        exit(1);
    }

    if(signal(SIGUSR1, sigint_handler) == SIG_ERR) {
        perror("signal");
        sigint_handler(SIGINT);
        exit(1);
    }

    if(init_msg_queue() == -1) {
        perror("init_msg_queue");
        sigint_handler(SIGINT);
        exit(1);
    }
    if(signal(SIGALRM, alarm_handler) == SIG_ERR) {
        perror("signal");
        sigint_handler(SIGINT);
        exit(1);
    }
    ready_queue_schd = ready_queue;
    waiting_queue_schd = create_pcb_queue();
    setup_timer();

#ifdef DEBUG
    printf("[Tick :: %d] scheduler lunch\n", current_time);
#endif

    current_process = dequeue_pcb(ready_queue);
    current_process->state = PROCESS_RUNNING;
    // notify child scheduled
    kill(current_process->pid, SIGUSR1);

#ifdef DEBUG
    printf("[Tick :: %d] child process starts being scheduled", current_time);
#endif
}

void scheduler_run(pcb_queue* ready_queue, int n_process) {
    scheduler_init(ready_queue);

    // just work with handler function
    while(1) {
        printf("[scheduler_run] Waiting for process to schedule\n");
    };
}
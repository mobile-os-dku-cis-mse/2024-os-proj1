//
// Created by hochacha on 24. 11. 6.
//

#include "scheduler.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
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

volatile sig_atomic_t io_request_received = 0;

void sigint_handler(int sig) {
    if (sig == SIGINT) {
        pcb_t* process;
        while ((process = dequeue_pcb(ready_queue_schd)) != NULL) {
            kill(process->pid, SIGTERM);
            waitpid(process->pid, NULL, 0);
        }
        while ((process = dequeue_pcb(waiting_queue_schd)) != NULL) {
            kill(process->pid, SIGTERM);
            waitpid(process->pid, NULL, 0);
        }
        if (current_process != NULL) {
            kill(current_process->pid, SIGTERM);
            waitpid(current_process->pid, NULL, 0);
        }
        msgctl(msg_queue_id, IPC_RMID, NULL);  // 메시지 큐 삭제
        exit(0);
    }
}

void alarm_handler(int sig) {
    current_time++;  // global time increment

    // case: currently time decrement happens only CPU scheduling,
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
        if(current_process->remaining_time <= 0) {
            // schedule out
            kill(current_process->pid, SIGUSR2);

            // get back to the queue
            current_process->state = PROCESS_READY;
            enqueue_pcb(ready_queue_schd, current_process);
            current_process = NULL;
        }
    }else {
        // case: if there is no running process
        if(is_queue_empty(ready_queue_schd) != 1) {
            // schedule in the child process
            current_process = dequeue_pcb(ready_queue_schd);
            current_process->state = PROCESS_RUNNING;
            current_process->remaining_time = TIME_QUANTUAM;

            time_alloc_msg t_msg;
            t_msg.mtype = current_process->pid;
            t_msg.time_alloc = TIME_QUANTUAM;
            if(msgsnd(msg_queue_id, &t_msg, sizeof(time_alloc_msg) - sizeof(long), 0) == -1) {
                perror("msgsnd");
            }
#ifdef DEBUG
            printf("=======================================\n");
            printf("[Tick :: %d] Child %d scheduled in\n", current_time,current_process->pid);
            printf("=======================================\n");
#endif

            // SIGUSR1 just change the state of the process
            kill(current_process->pid, SIGUSR1);
            kill(current_process->pid, SIGALRM);
        } else {
            // ready queue empty
            // in this situation, I/O queue full or child process have done for work
            if(is_queue_empty(waiting_queue_schd) != 1) {
                // wait for IO Queue done


            } else {
                // child processes have done
#ifdef DEBUG
                printf("\n======================================================\n");
                printf("[:::TERMINATION::::] The scheduler program has been terminated");
                printf("\n======================================================\n");
#endif
                sigint_handler(SIGINT);
                exit(0);
            }
        }
    }

    // check I/O schedule (FIFO) request
    // just polling the message
    /* IO burst */
    int waiting_queue_size = get_queue_size(waiting_queue_schd);
    for(int i = 0; i < waiting_queue_size; i++) {
        pcb_t * process = dequeue_pcb(waiting_queue_schd);
        process->io_time--;

        // IO 처리가 끝난 프로세스
        if(process->io_time <= 0) {
            // IO 완료 처리
            process->state = PROCESS_READY;
            enqueue_pcb(ready_queue_schd, process);
            kill(process->pid, SIGUSR2);
#ifdef DEBUG
            printf("[Tick:: %d] process %d IO completed, move to ready queue\n", current_time, process->pid);
#endif
        }else {
            enqueue_pcb(waiting_queue_schd, process);
        }
    }
}

// when the child request for IO job
void io_schedule_handler(int sig) {
    printf("SIGUSR2 RECEVIED !!!!!!!!!!!!!!!!!\n");
    // get io scheduling message
    io_request_received = 1;
    current_process->state = PROCESS_BLOCKED;
}

void scheduler_init(pcb_queue* ready_queue) {
#ifdef DEBUG
    printf("[Tick :: %d] child process starts being scheduled\n", current_time);
#endif

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
}

void scheduler_run(pcb_queue* ready_queue, int n_process) {
    scheduler_init(ready_queue);

    // just work with handler function
    while(1) {
        if(io_request_received) {
            io_request_received = 0;
            io_msg t_msg = {0, };
            if(msgrcv(msg_queue_id, &t_msg, sizeof(io_msg) - sizeof(long),
                                            MESSAGE_TYPE_IO_REQ, IPC_NOWAIT) == -1) {
                perror("[Sched :: IO] msgrcv");
            }else {
                if(current_process != NULL && current_process->pid == t_msg.pid) {
                    current_process->io_time = t_msg.io_time;
                    enqueue_pcb(waiting_queue_schd, current_process);
                    current_process = NULL;
#ifdef DEBUG
                    printf("[Tick :: %d] Process %d moved to waiting queue for IO of %d ticks\n",
                                            current_time, t_msg.pid, t_msg.io_time);
#endif
                }
            }

        }
    };
}
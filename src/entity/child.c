//
// Created by hochacha on 24. 11. 6.
//

#include "child.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <asm-generic/errno-base.h>
#include <sys/msg.h>

#include "src/util/message_queue.h"

#define DEBUG
int cpu_burst = 0;  // 한 프로그램이 실행되는데 필요한 CPU 시간
int io_burst = 0;   // 한 프로그램이 요청하는 IO 작업의 시간
int cpu_time = 0;   // 스케줄로 부여 받는 시간
int io_time = 0;    // IO로 요청하는 시간
int total_exec_time = 0;
// 따라서, CPU Time 전부 소진하면, IO Time 만큼 IO 수행 요청

int process_state = 0;
int EOP = 0; // End of Process

volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t sigusr2_received = 0;

void request_io_msgsnd(int io_time) {
    io_msg msg = {MESSAGE_TYPE_IO_REQ, getpid(), io_time};
    if(msgsnd(msg_queue_id, &msg, sizeof(int), IPC_NOWAIT) == -1) {
        perror("child_msgsnd");
        exit(EXIT_FAILURE);
    }
}

// SIGALRM 핸들러: 타이머 틱마다 호출되어 CPU 버스트를 감소시킴
void child_SIGALRM(int sig) {
    if (process_state == PROCESS_RUNNING && !EOP) {
        cpu_time--;
        cpu_burst--;
        total_exec_time++;
#ifdef DEBUG
        printf("[Child::%d] CPU burst remaining: %d\n", getpid(), cpu_burst);
#endif
    }

    // CPU 버스트가 종료되면 IO 버스트 요청
    if (cpu_burst <= 0 && process_state == PROCESS_RUNNING) {
        process_state = PROCESS_BLOCKED;
#ifdef DEBUG
        printf("[Child::%d] CPU burst completed. Requesting IO.\n", getpid());
#endif
        request_io_msgsnd(io_burst);            // IO 요청 메시지 전송
        kill(getppid(), SIGUSR2);          // 부모 프로세스에 신호 전송
    }
}

// 스케줄 알림
void child_SIGUSR1(int sig) {
    sigusr1_received = 1;
}

// 스케줄 아웃 처리
void child_SIGUSR2(int sig) {
    sigusr2_received = 1;
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
    cpu_burst = rand() % 10 + 1;
    io_burst = rand() % 10 + 1;

#ifdef DEBUG
    printf("[Child::%d] cpu_burst = %d, io_burst = %d\n", getpid(),cpu_burst, io_burst);
#endif

    if(init_msg_queue() == -1) {
        perror("init_msg_queue");
        exit(1);
    }

    printf("[do_work_child] init child process\n");
}

int child_main(int argc, char **argv) {
    init_child();
#ifdef DEBUG
    printf("[Child::%d] ready to run\n", getpid());
#endif
    while(1) {
        while (!EOP) {

            /* 스케줄 인 처리*/

            if (sigusr1_received) {
                sigusr1_received = 0;
                // 메시지 수신 및 처리
                time_alloc_msg t_msg = {0,};
                if (msgrcv(msg_queue_id, &t_msg, sizeof(time_alloc_msg) - sizeof(long), getpid(), 0) == -1) {
                    if (errno != EINTR) {
                        perror("[child_msgrcv in child]");
                        exit(1);
                    }
                    // EINTR인 경우에는 시스템 호출을 재시도하거나 적절히 처리
                }
                cpu_time = t_msg.time_alloc;
                process_state = PROCESS_RUNNING;
#ifdef DEBUG
                printf("[Child::%d]msg rcv: message type = %ld, allocate time = %d\n",
                        getpid(), t_msg.mtype, t_msg.time_alloc);
                printf("CPU burst: %d\nCPU Time: %d\n", cpu_burst, cpu_time);
#endif
            }

            /* 스케줄 아웃 또는 IO 완료 처리 */

            if (sigusr2_received) {
                sigusr2_received = 0;

                if (process_state == PROCESS_BLOCKED) {
                    cpu_burst = rand() % 10 + 1;
                    io_burst = rand() % 10 + 1;

                    printf("[Child::%d] IO completed. New CPU burst: %d, IO Burst: %d\n",
                        getpid(), cpu_burst, io_burst);

                    process_state = PROCESS_READY;
                }
                if(process_state == PROCESS_RUNNING) {
                    process_state = PROCESS_READY;
#ifdef DEBUG
                    printf("[Child::%d] Scheduled out\n", getpid());
#endif
                }
            }
            // CPU 사용을 줄이기 위해 잠시 대기
            usleep(1000); // 1밀리초 대기
        }
    }
    exit(0);
}
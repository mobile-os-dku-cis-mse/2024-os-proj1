#ifndef PROC_H
#define PROC_H

#include "queue.h"
#include "ipc_msg.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <string.h>


#define NUM_PROCESSES 10

extern int TIME_QUANTUM;
extern int msgid;
extern int activeProcesses;
extern pid_t child_pids[NUM_PROCESSES];
extern Queue *readyQueue;
extern Queue *waitQueue;
FILE* fp;


void initQueues();
void initMessageQueue();
void handle_sigcont(int sig);
void createChildProcesses();   // 자식 프로세스 생성 함수
void scheduleProcesses();      // 스케줄링 관리 함수
void terminateChildProcesses(); // 모든 자식 프로세스 종료 함수
void childProcessLoop();       // 자식의 작업 루프 (부모의 신호에 의해 작동)

void alrm_handler(int signum);
void setup_timer();

#endif // PROC_H

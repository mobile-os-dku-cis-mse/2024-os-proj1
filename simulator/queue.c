#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <fcntl.h>
#include "msg.h"

#define QUEUE_SIZE 10
#define RUN_TIME 50

// 큐 초기화 함수
void initQueue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

// 큐가 비었는지 확인하는 함수
int isEmpty(Queue* q) {
    return q->size == 0;
}

// 큐가 꽉 찼는지 확인하는 함수
int isFull(Queue* q) {
    return q->size == QUEUE_SIZE;
}

// 큐에 프로세스를 추가하는 함수
void enqueue(Queue* q, Process* process) {
    if (isFull(q)) {
        printf("Queue is full\n");
        return;
    }
    q->rear = (q->rear + 1) % QUEUE_SIZE;
    q->processes[q->rear] = *process;
    q->size++;
}

// 큐에서 프로세스를 제거하는 함수
Process* dequeue(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return NULL;
    }
    Process* process = &q->processes[q->front];
    q->front = (q->front + 1) % QUEUE_SIZE;
    q->size--;
    return process;
}



Process* getProcessAtIndex(Queue* q, int index) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return NULL;
    }
    if (index < 0 || index >= q->size) {
        printf("idx: %d, max: %d\n", index, q->size);
        printf("Invalid index\n");
        return NULL;
    }

    int actualIndex = (q->front + index) % QUEUE_SIZE;
    return &q->processes[actualIndex];
}


void print_qstate(int rtime, Queue* runq, Queue* waitq) {
    // 상단 구분선 출력
    printf("\n\n\n┌───────────────────────────────────────────────────────────── System Status at Time %5d ─────────────────────────────────────────────────────────────┐\n\n", rtime);

    // Run Queue 상태 출력
    printf("  RUN QUEUE  │ ");
    if (isEmpty(runq)) {
        printf("Empty");
    } else {
        for (int i = 0; i < runq->size; i++) {
            Process* rproc = getProcessAtIndex(runq, i);
            printf("P%d[CPU:%2d]", rproc->idx, rproc->remain_cpu_burst);
            if (i < runq->size - 1) printf(" → ");
        }
    }
    printf("\n");

    // Wait Queue 상태 출력
    printf("  WAIT QUEUE │ ");
    if (isEmpty(waitq)) {
        printf("Empty");
    } else {
        for (int i = 0; i < waitq->size; i++) {
            Process* wproc = getProcessAtIndex(waitq, i);
            printf("P%d[I/O:%2d]", wproc->idx, wproc->remain_io_burst);
            if (i < waitq->size - 1) printf(" → ");
        }
    }
    printf("\n");

    // 하단 구분선 출력
    printf("\n└───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘\n");
}

void writeToFile(FILE *file, int rtime, int count, Queue* runq, Queue* waitq) {
    if (file == NULL) {
        perror("File pointer is NULL");
        return;
    }

    // 시간 정보 및 구분선 출력
    fprintf(file, "\n====== Time: %3d (Quantum Count: %d) ======\n", rtime, count);
    

    // 현재 실행 중인 프로세스 정보 출력
    if (!isEmpty(runq)) {
        Process* current = &runq->processes[runq->front];
        fprintf(file, "Current Process: P%d\n", current->idx);
        fprintf(file, "├── CPU Burst Remaining: %d\n", current->remain_cpu_burst);
        fprintf(file, "└── I/O Burst Total: %d\n", current->io_burst);
    } else {
        fprintf(file, "No process currently running\n");
    }

    // Run Queue 상태 출력
    fprintf(file, "\nRun Queue Status:\n");
    if (isEmpty(runq)) {
        fprintf(file, "└── Empty\n");
    } else {
        for (int i = 0; i < runq->size; i++) {
            Process* proc = getProcessAtIndex(runq, i);
            if (i == runq->size - 1) {
                fprintf(file, "└── P%d (CPU: %d)\n", proc->idx, proc->remain_cpu_burst);
            } else {
                fprintf(file, "├── P%d (CPU: %d)\n", proc->idx, proc->remain_cpu_burst);
            }
        }
    }

    // Wait Queue 상태 출력
    fprintf(file, "\nWait Queue Status:\n");
    if (isEmpty(waitq)) {
        fprintf(file, "└── Empty\n");
    } else {
        for (int i = 0; i < waitq->size; i++) {
            Process* proc = getProcessAtIndex(waitq, i);
            if (i == waitq->size - 1) {
                fprintf(file, "└── P%d (I/O: %d)\n", proc->idx, proc->remain_io_burst);
            } else {
                fprintf(file, "├── P%d (I/O: %d)\n", proc->idx, proc->remain_io_burst);
            }
        }
    }

    fprintf(file, "\n");
}


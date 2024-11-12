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

// typedef struct Process {
//     pid_t pid;
//     int cpu_burst;
//     int io_burst;
// } Process;

// typedef struct Queue {
//     Process processes[QUEUE_SIZE];
//     int front;
//     int rear;
//     int size;
// } Queue;

int initMsgQueue(int key) {
    int msgq = msgget(key, IPC_CREAT | 0666);
    if (msgq == -1) {
        perror("msgget failed");
        return -1;
    }
    return msgq;
}

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



// 메인 함수
// int main() {
//     Queue queue;
//     initQueue(&queue);

//     // 프로세스 생성 및 큐에 추가
//     for (int i = 0; i < QUEUE_SIZE; i++) {
//         Process newProcess;
//         newProcess.pid = i + 1;  // PID를 1부터 시작하도록 설정
//         newProcess.cpu_burst = (rand() % 10) + 1;  // 1부터 10 사이의 랜덤 CPU 버스트 시간
//         newProcess.io_burst = (rand() % 10) + 1;   // 1부터 10 사이의 랜덤 IO 버스트 시간

//         enqueue(&queue, &newProcess); // 큐에 프로세스 추가
//     }

//     // 큐에서 프로세스 제거 및 출력
//     printf("Dequeuing processes:\n");
//     while (!isEmpty(&queue)) {
//         Process* process = dequeue(&queue);
//         if (process != NULL) {
//             printf("Process PID: %d, CPU Burst: %d, IO Burst: %d\n", process->pid, process->cpu_burst, process->io_burst);
//         }
//     }

//     return 0;
// }

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

void print_qstate(int rtime, Queue* runq, Queue* waitq){
    printf("--------------------------------\n");
    printf("Run time left: %d\n", rtime);
    printf("run queue > ");
    if(!isEmpty(runq)){
        for(int i=0;i<runq->size;i++){
            Process* rproc = getProcessAtIndex(runq, i);
            printf("%d: %d(cpu: %d) | ", rproc->idx, rproc->pid, rproc->remain_cpu_burst);
        }
    }

    if(!isEmpty(waitq)){
        printf("\nwait queue > ");
        for(int i=0;i<waitq->size;i++){
            Process* wproc = getProcessAtIndex(waitq, i);
            printf("%d: %d(io: %d) | ", wproc->idx, wproc->pid, wproc->remain_io_burst);
        }
    }
    printf("\n--------------------------------\n");
}

// 파일에 메시지를 출력하는 함수
void writeToFile(FILE *file, int rtime, int count, Queue* runq, Queue* waitq) {
    if (file == NULL) {
        perror("File pointer is NULL");
        return;
    }

    // 큐가 비어 있지 않은지 확인
    if (isEmpty(runq)) {
        fprintf(file, "No process in runq\n");
        return;
    }
    
    fprintf(file, "-----------------------------------\n");
    fprintf(file, " %5d (count: %d)", rtime, count);

    // Process* current_proc = getProcessAtIndex(runq, runq->front);
    fprintf(file, "\n [runq] curent_process[%d] %d(cpu:%d, io: %d) - idx(cpu_burst)\n",  runq->processes[runq->front].idx, runq->processes[runq->front].pid, 
        runq->processes[runq->front].remain_cpu_burst, runq->processes[runq->front].remain_io_burst);
    for(int j=0;j<runq->size;j++){
        Process* proc = getProcessAtIndex(runq, j);
        if (proc != NULL) {
            if (j == runq->size - 1) { // 마지막 항목에는 "->" 없이 출력
                fprintf(file, " %d(%d)", proc->idx, proc->remain_cpu_burst);
            } else {
                fprintf(file, " %d(%d) ->", proc->idx, proc->remain_cpu_burst);
            }
        }
    }
    fprintf(file, "\n [waitq] - idx(io_burst)\n");
    for(int k=0;k<waitq->size;k++){
        Process* proc = getProcessAtIndex(waitq, k);
        if (proc != NULL) {
            if (k == waitq->size - 1) { // 마지막 항목에는 "->" 없이 출력
                fprintf(file, " %d(%d)", proc->idx, proc->remain_io_burst);
            } else {
                fprintf(file, " %d(%d) ->", proc->idx, proc->remain_io_burst);
            }
        }
    }
    fprintf(file, "\n");
}



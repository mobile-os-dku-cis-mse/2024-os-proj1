// msg.h
#ifndef MSG_H
#define QUEUE_SIZE 10

typedef struct msgbuf {
    long mtype;         // 메시지의 타입

	int pid;
    int idx;
	int io_burst;
	int cpu_burst;
} msgbuf;

typedef struct ipcbuf{
    long mtype;

    int count;
} ipcbuf;


typedef struct Process {
    pid_t pid;
    int idx;
    int cpu_burst;
    int io_burst;

    int remain_cpu_burst;
    int remain_io_burst;
} Process;

typedef struct Queue {
    Process processes[QUEUE_SIZE];
    int front;
    int rear;
    int size;
} Queue;

// 함수 선언들
int initMsgQueue(int key);
void initQueue(Queue* q);
int isFull(Queue* q);
int isEmpty(Queue* q);
void enqueue(Queue* q, Process* process);
Process* dequeue(Queue* q);
// int msggets(Process* proc, int msgq);
// int msgrcvs(struct msgbuf* msg, int msgq);
int msggets(int msgid, struct msgbuf *msg);
int msgrcvs(int msgid, struct msgbuf *msg, long msgtype);
Process* getProcessAtIndex(Queue* q, int index);
void print_qstate(int rtime, Queue* runq, Queue* waitq);
void writeToFile(FILE *file, int rtime, int count, Queue* runq, Queue* waitq);
void waitq_burst();
// void child_process(Process* proc);
// void parent_process();

#endif
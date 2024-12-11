#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>


#define TIME_QUANTUM 2
#define NUM_PROCESSES 10
#define MAX_BURST 10

typedef struct Node {
    int pid;
    int CPUburstRemain;
    int burstIO;
    struct Node* next;
} Node;
typedef struct Queue {
    Node* front;
    Node* back;
} Queue;
void queueINIT(Queue* q) {
    q->front = q->back = NULL;
}
int empty(Queue* q) {
    return q->front == NULL;
}
void enqueue(Queue* q, int pid, int remaining_cpu_burst, int io_burst) {
    Node* temp = (Node*)malloc(sizeof(Node));
    if (!temp) {
        perror("malloc failed");
        exit(1);
    }
    temp->pid = pid;
    temp->CPUburstRemain = remaining_cpu_burst;
    temp->burstIO = io_burst;
    temp->next = NULL;
    if (q->back) {
        q->back->next = temp;
    }
    q->back = temp;
    if (!q->front) {
        q->front = temp;
    }
}
Node* dequeue(Queue* q) {
    if (empty(q)) {
        fprintf(stderr, "Queue underflow\n");
        return NULL;
    }
    Node* temp = q->front;
    q->front = q->front->next;
    if (!q->front) {
        q->back = NULL;
    }
    return temp;
}
struct my_msgbuf {
    long typeM;
    int pid;
    int timeIO;
};
Queue queueRUN;
Queue queueWAIT;
int msgq;
FILE* fileLOG;
void SignalHandling(int signo);
void queueLOG(const char* label, Queue* q);

int main() {
    
    queueINIT(&queueRUN);
    queueINIT(&queueWAIT);

    int key = 0x12345;
    msgq = msgget(key, IPC_CREAT | 0666);
    if (msgq == -1) {
        perror("msgget failed");
        exit(1);
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();
        if (pid == 0) {
          
            execl("./child", "child", NULL); 
            perror("execl failed");
            exit(1);
        }
        else if (pid > 0) {
            enqueue(&queueRUN, pid, rand() % MAX_BURST + 1, 0); 
        }
        else {
            perror("fork failed!");
            exit(1);
        }
    }

    fileLOG = fopen("schedule_dump.txt", "w");
    if (!fileLOG) {
        perror("fopen failed");
        exit(1);
    }
   
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandling;
    sigaction(SIGALRM, &sa, NULL);
   
    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 100000; // 100ms
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100000;
    setitimer(ITIMER_REAL, &timer, NULL);
    
    while (1) {
       
        struct my_msgbuf msg;
        while (msgrcv(msgq, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) != -1) {
            if (msg.timeIO > 0) {
               
                enqueue(&queueWAIT, msg.pid, 0, msg.timeIO);
                fprintf(fileLOG, "Process %d moved to wait queue for I/O burst %d\n", msg.pid, msg.timeIO);
                fflush(fileLOG);
            }
        }
        pause();
    }

    fclose(fileLOG);
    return 0;
}

void SignalHandling(int signo) {
    static int time_tick = 0;

    Node* prev = NULL, * curr = queueWAIT.front;
    while (curr) {
        curr->burstIO--;
        if (curr->burstIO <= 0) {
         
            enqueue(&queueRUN, curr->pid, rand() % MAX_BURST + 1, 0);
            fprintf(fileLOG, "Process %d finished I/O and moved to run queue\n", curr->pid);
            fflush(fileLOG);
          
            if (prev) {
                prev->next = curr->next;
            }
            else {
                queueWAIT.front = curr->next;
            }
            Node* temp = curr;
            curr = curr->next;
            free(temp);
            continue;
        }
        prev = curr;
        curr = curr->next;
    }
   
    if (!empty(&queueRUN)) {
        Node* current_process = dequeue(&queueRUN);

        fprintf(fileLOG, "At time %d, process %d gets CPU time (remaining burst: %d)\n",
            time_tick, current_process->pid, current_process->CPUburstRemain);
        fflush(fileLOG);

        current_process->CPUburstRemain -= TIME_QUANTUM;
        if (current_process->CPUburstRemain > 0) {
            enqueue(&queueRUN, current_process->pid, current_process->CPUburstRemain, 0);
        }
        else {
            fprintf(fileLOG, "Process %d finished its CPU Burst\n", current_process->pid);
            fflush(fileLOG);
        }
        free(current_process);
    }
    
    queueLOG("Queue Run", &queueRUN);
    queueLOG("Queue Wait", &queueWAIT);

    time_tick++;
}
void queueLOG(const char* label, Queue* q) {
    fprintf(fileLOG, "%s: ", label);
    Node* temp = q->front;
    while (temp) {
        fprintf(fileLOG, "[PID: %d, CPU: %d, IO: %d] ", temp->pid, temp->CPUburstRemain, temp->burstIO);
        temp = temp->next;
    }
    fprintf(fileLOG, "\n");
    fflush(fileLOG);
}


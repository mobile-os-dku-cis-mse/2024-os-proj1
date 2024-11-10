#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>

// PCB 구조체 정의
// pid: 프로세스 ID, exe_time: 실행 시간, wait_time: 대기 시간
typedef struct PCB {
    int pid;
    int exe_time;
    int wait_time;
} PCB;

// 큐 노드 구조체 정의
// pcb: 큐의 원소(PCB 객체), next: 다음 노드를 가리키는 포인터
typedef struct Node {
    PCB *pcb;
    struct Node *next;
} Node;

// 큐 구조체 정의
// count: 큐에 있는 원소의 개수, head: 큐의 첫 번째 노드, tail: 큐의 마지막 노드
typedef struct Queue {
    int count;
    Node *head;
    Node *tail;
} Queue;

// 함수 프로토타입
Queue* createQueue();           // 큐 생성 함수
Node* createNode(PCB *pcb);     // 노드 생성 함수
void enqueue(Queue *queue, PCB *pcb);  // 큐에 원소 추가 함수
PCB* dequeue(Queue *queue);     // 큐에서 원소 제거 함수
void printQueue(Queue *queue);  // 큐 출력 함수
void destroyQueue(Queue *queue);  // 큐 메모리 해제 함수
void enqueueSortedByExeTime(Queue *queue, PCB *pcb);

#endif // QUEUE_H

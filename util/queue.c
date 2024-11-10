#include "queue.h"

// 큐를 생성하는 함수
Queue* createQueue() {
    Queue *queue = (Queue*)malloc(sizeof(Queue));
    queue->count = 0;
    queue->head = queue->tail = NULL;
    return queue;
}

// 새로운 노드를 생성하는 함수
Node* createNode(PCB *pcb) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->pcb = pcb;
    node->next = NULL;
    return node;
}

// 실행 시간을 기준으로 PCB를 큐에 정렬하여 추가하는 함수
void enqueueSortedByExeTime(Queue *queue, PCB *pcb) {
    Node *node = createNode(pcb);

    // 큐가 비어 있는 경우 - 처음 노드로 삽입
    if (queue->count == 0) {
        queue->head = queue->tail = node;
    } else {
        // 실행 시간이 가장 짧은 위치를 찾아 삽입
        Node *current = queue->head;
        Node *previous = NULL;

        // PCB의 exe_time이 더 작을 때 삽입 위치를 찾음
        while (current != NULL && current->pcb->exe_time <= pcb->exe_time) {
            previous = current;
            current = current->next;
        }

        // 첫 번째 위치에 삽입할 경우
        if (previous == NULL) {
            node->next = queue->head;
            queue->head = node;
        } else {
            // 중간 또는 끝에 삽입할 경우
            previous->next = node;
            node->next = current;
        }

        // 큐의 tail을 업데이트 (맨 끝에 삽입할 경우)
        if (node->next == NULL) {
            queue->tail = node;
        }
    }

    queue->count++;  // 카운트 증가를 함수 끝에서 한 번만 호출
}


// 기본적인 큐에 PCB를 추가하는 함수 (FCFS나 RR 용으로 사용 가능)
void enqueue(Queue *queue, PCB *pcb) {
    Node *node = createNode(pcb);
    if (queue->tail) queue->tail->next = node;
    else queue->head = node;
    queue->tail = node;
    queue->count++;
}

// 큐에서 PCB를 제거하고 반환하는 함수
PCB* dequeue(Queue *queue) {
    if (queue->count == 0) return NULL;
    Node *node = queue->head;
    PCB *pcb = node->pcb;
    queue->head = node->next;
    if (!queue->head) queue->tail = NULL;
    free(node);
    queue->count--;
    return pcb;
}

// 큐의 내용을 출력하는 함수
void printQueue(Queue *queue) {
    Node *current = queue->head;
    while (current) {
        printf("PID: %d, CPU Burst: %d, IO Burst: %d\n",
               current->pcb->pid, current->pcb->exe_time, current->pcb->wait_time);
        current = current->next;
    }
}

// 큐의 메모리를 해제하는 함수
void destroyQueue(Queue *queue) {
    while (queue->head) {
        Node *node = queue->head;
        queue->head = node->next;
        free(node);
    }
    free(queue);
}

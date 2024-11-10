#include "ipc_msg.h"

// 새로운 메시지 큐를 생성하는 함수
// 메시지 큐는 IPC_PRIVATE를 사용하여 고유한 큐를 생성
int createMessageQueue()
{

    key_t key = ftok("msgqueue", 65);  // 고유한 메시지 큐 키 생성
    // msgget 함수로 메시지 큐를 생성하거나 기존 큐에 접근
    // IPC_PRIVATE는 새로 고유한 큐를 생성하도록 지시
    // 0666: 모든 사용자에게 읽기/쓰기 권한 부여
    // IPC_CREAT: 큐가 없으면 생성
    return msgget(key, 0666 | IPC_CREAT);
}

// 메시지 전송 함수
void sendMessage(int msgid, Message *msg) {

    msgsnd(msgid, msg, sizeof(Message) - sizeof(long), 0);
    //printf("Message sent with PID, msgid: %d // %d, exe_time: %d, wait_time: %d bytes : %ld\n",
     //          msg->pid, msgid, msg->exe_time, msg->wait_time, sizeof(Message) - sizeof(long));

}

// 메시지 큐에서 특정 타입의 메시지를 동기적으로 수신하는 함수
void receiveMessage(int msgid, Message *msg, int type)
{
    // msgrcv 함수로 메시지를 D큐에서 받음 (type에 따라 특정 메시지만 수신)
    // msgid: 큐 식별자, msg: 수신할 메시지를 저장할 구조체
    // sizeof(Message) - sizeof(long): 수신할 메시지 크기
    // type: 수신하려는 메시지 타입, 특정 PID 값에 해당하는 메시지
    // if (msgrcv(msgid, msg, sizeof(Message) - sizeof(long), type, 0) == -1)
    // {
    //     perror("msgrcv failed"); // 수신 실패 시 에러 메시지 출력
    // }
    msgrcv(msgid, msg, sizeof(Message) - sizeof(long), type, IPC_NOWAIT);
    //printf("Receiving message with PID and msgid: (%d // %d), exe_time: %d, wait_time: %d\n", msg->pid, msgid, msg->exe_time, msg->wait_time);


    /*
    while (msgrcv(msgid, msg, sizeof(Message) - sizeof(long), type, IPC_NOWAIT) != -1) {
        if (errno == EINTR) {
            // 인터럽트에 의해 호출이 중단되었을 경우 재시도
            continue;
        } else {
            perror("msgrcv failed"); // 다른 오류 시 에러 메시지 출력
            break;
        }
    }
    */

}

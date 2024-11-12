#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MSGSZ 128

struct msgbuf {
    long mtype;                // 메시지 유형
    char mtext[MSGSZ];         // 메시지 내용
};

int main() {
    key_t key;
    int msgid;
    struct msgbuf message;

    // 메시지 큐 키 생성
    key = ftok("progfile", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);

    if (fork() == 0) {
        // 자식 프로세스
        message.mtype = 1;
        snprintf(message.mtext, sizeof(message.mtext), "I/O request from child");

        // 메시지 전송
        msgsnd(msgid, &message, sizeof(message.mtext), 0);
        printf("Child: I/O request sent to parent\n");
        exit(0);
    } else {
        // 부모 프로세스
        sleep(1); // 자식이 메시지를 보낼 시간을 줌
        msgrcv(msgid, &message, sizeof(message.mtext), 1, 0);
        printf("Parent received: %s\n", message.mtext);

        // 메시지 큐 제거
        msgctl(msgid, IPC_RMID, NULL);
    }

    return 0;
}

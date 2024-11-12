// #include <stdio.h>
// #include <sys/types.h>
// #include <sys/ipc.h>
// #include <sys/msg.h>
// #include "msg.h"
// #include <string.h>
// #include <unistd.h>

// int main()
// {
// 	int msgq;
// 	int ret;
// 	int key = 0x12345;
// 	msgq = msgget( key, IPC_CREAT | 0666);
// 	printf("msgq id: %d\n", msgq);

// 	struct my_msgbuf msg;
// 	memset(&msg, 0, sizeof(msg));
// 	msg.mtype = 1;
// 	msg.pid = getpid();
// 	msg.io_time = 10;
// 	ret = msgsnd(msgq, &msg, sizeof(msg) - sizeof(long), 0);
// 	printf("msgsnd ret: %d\n", ret);

// 	return 0;
// }

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "msg.h"
#include <string.h>
#include <unistd.h>

int main()
{
	int msgq;
	int ret;
	int key = 0x12345;

	// 기존 메시지 큐 삭제
	msgq = msgget(key, 0666);
	if (msgq != -1) {
		if (msgctl(msgq, IPC_RMID, NULL) == -1) {
			perror("msgctl(IPC_RMID) failed");
			return 1;
		}
		printf("Existing message queue deleted.\n");
	}

	// 새로운 메시지 큐 생성
	msgq = msgget(key, IPC_CREAT | 0666);
	printf("msgq id: %d\n", msgq);

	// 메시지 전송
	struct my_msgbuf msg;
	memset(&msg, 0, sizeof(msg));
	msg.mtype = 1;
	msg.pid = getpid();
	msg.io_time = 10;
	ret = msgsnd(msgq, &msg, sizeof(msg) - sizeof(long), 0);
	printf("msgsnd ret: %d\n", ret);

	return 0;
}

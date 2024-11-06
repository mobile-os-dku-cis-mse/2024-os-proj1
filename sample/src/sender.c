#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define BUFFER_SIZE 1024

typedef struct{
  long msgtype;
  int value;
  char buf[BUFFER_SIZE];
}msgbuf;

int main() {
  int cnt = 0;
  int key_id;
  msgbuf msg;
  msg.msgtype = 1;

  key_id = msgget((key_t) 1234, 0666 | IPC_CREAT);

  if (key_id == -1) {
    printf("Message Get Failed!\n");
    exit(1);
  }
  while(1) {
    msg.value == ++cnt;
    if(cnt >= 10) {
      printf("Message Sending Finished!\n");
      break;
    }

    if(msgsnd(key_id, &msg, sizeof(msg), IPC_NOWAIT) == -1) {
      printf("Message Send Failed!\n");
      exit(0);
    }

    printf("value: %d\n", msg.value);
    sleep(1);
  }
  exit(0);
}
#ifndef IPC_MSG_H
#define IPC_MSG_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct Message {
    long type;      
    int pid;
    int exe_time;
    int wait_time;
    int cpu_io;         // msg type : from child process = 0, cpu = 1, i/O = 2
} Message;

int createMessageQueue();
void sendMessage(int msgid, Message *msg);
void receiveMessage(int msgid, Message *msg, int type);

#endif // IPC_MSG_H

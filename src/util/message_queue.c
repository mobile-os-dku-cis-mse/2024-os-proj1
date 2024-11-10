//
// Created by hochacha on 24. 11. 7.
//

#include "message_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>

int msg_queue_id;
#define MSG_KEY 12345

int init_msg_queue() {
    msg_queue_id = msgget(MSG_KEY, IPC_CREAT | 0666);
    if(msg_queue_id == -1) {
        perror("msgget");
        return -1;
    }
    return 0;
}

void close_msg_queue() {
    if(msgctl(msg_queue_id, IPC_RMID, 0) == -1) {
        perror("msgctl");
    }
}

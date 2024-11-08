//
// Created by hochacha on 24. 11. 7.
//

#ifndef MESSGE_QUEUE_H
#define MESSGE_QUEUE_H
#include "pid_queue.h"

extern int msg_queue_id;

typedef struct{
    long msg_t;
    pid_t pid;
    int io_time;
}io_msg;
int init_msg_queue();
#endif //MESSGE_QUEUE_H

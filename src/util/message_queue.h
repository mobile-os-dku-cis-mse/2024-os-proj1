//
// Created by hochacha on 24. 11. 7.
//

#ifndef MESSGE_QUEUE_H
#define MESSGE_QUEUE_H
#include "pid_queue.h"

#define MESSAGE_TYPE_IO_FIN 0
#define MESSAGE_TYPE_IO_REQ 1
#define MESSAGE_TYPE_SCHD_TIME 2

extern int msg_queue_id;

typedef struct{
    int msg_t;
    pid_t pid;
    unsigned int io_time;
    int is_finished;
}io_msg;

typedef struct {
    long mtype;
    int time_alloc;
}time_alloc_msg;

int init_msg_queue();
#endif //MESSGE_QUEUE_H

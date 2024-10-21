#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

struct message
{
    long mtype;
    int cpu_burst;
};

#endif // MESSAGE_QUEUE_H
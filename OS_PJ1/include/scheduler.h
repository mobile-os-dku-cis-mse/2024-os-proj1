#ifndef SCHEDULER_H
#define SCHEDULER_H

#define NUM_CHILDREN 10
#define TIME_QUANTUM 1

extern pid_t child_pids[NUM_CHILDREN];
extern int remaining_quantum[NUM_CHILDREN];
extern int current_child;
extern int msgid;

#endif // SCHEDULER_H
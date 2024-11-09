#ifndef __SIG_H
#define __SIG_H

#include <sys/types.h>

int my_sigaction(int, void(*)(int));
int my_msgsnd(int, pid_t, int);
ssize_t my_msgrcv(int, int*);

#endif

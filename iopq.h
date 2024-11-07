#ifndef __IOPQ_H
#define __IOPQ_H

// io priority queue entry; (burst, pid)
struct iopq_pair
{
	int burst;
	pid_t pid;
};

typedef struct iopq_pair iopq_pair;

// io priority queue.
struct iopq
{
	iopq_pair *mem;
	int sz, cap;
};

typedef struct iopq iopq;

void iopq_init(iopq*, int);
void iopq_destroy(iopq*);
int iopq_empty(iopq*);
void iopq_push(iopq*, iopq_pair);
iopq_pair iopq_pop(iopq*);

#endif

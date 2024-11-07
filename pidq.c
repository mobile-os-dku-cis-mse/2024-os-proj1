#include <stdlib.h>
#include <sys/types.h>
#include "pidq.h"

void pidq_init(pidq *q, int cap)
{
	q->mem = malloc(sizeof(pid_t) * cap);
	q->cap = cap;
	q->sz = 0;
	q->front = 0;
	q->back = -1;
}

void pidq_destroy(pidq *q)
{
	free(q->mem);
}

int pidq_empty(pidq *q)
{
	return q->sz == 0;
}

void pidq_push(pidq *q, pid_t val)
{
	q->mem[(++q->back) % q->cap] = val;
	q->sz++;
}

pid_t pidq_pop(pidq *q)
{
	q->sz--;
	return q->mem[(q->front++) % q->cap];
}

pid_t pidq_at(pidq *q, int off)
{
	return q->mem[(q->front + off) % q->cap];
}

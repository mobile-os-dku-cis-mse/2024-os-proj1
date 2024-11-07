#include <stdlib.h>
#include "iopq.h"

static inline void __iopq_pair_swap(iopq_pair *p1, iopq_pair *p2)
{
	iopq_pair temp = *p1;
	*p1 = *p2;
	*p2 = temp;
}

static inline int __iopq_pair_cmp(iopq_pair *p1, iopq_pair *p2)
{
	return p1->burst - p2->burst;
}

void iopq_init(iopq *q, int cap)
{
	q->mem = malloc(sizeof(iopq_pair) * cap);
	q->sz = 0;
	q->cap = cap;
}

void iopq_destroy(iopq *q)
{
	free(q->mem);
}

static void __iopq_sift_up(iopq *q, int from)
{
	if (from == 0)
		return;

	int next = (from-1)/2;
	if (q->mem[next].burst > q->mem[from].burst)
	{
		__iopq_pair_swap(&q->mem[from], &q->mem[next]);
		__iopq_sift_up(q, next);
	}
}

static void __iopq_sift_down(iopq *q, int from)
{
	if (from >= q->sz)
		return;

	int left = from*2 + 1;
	int right = from*2 + 2;
	int next;

	if (left >= q->sz)
		return;

	if (right >= q->sz)
		next = left;
	else
	{
		if (q->mem[left].burst < q->mem[right].burst)
			next = left;
		else
			next = right;
	}

	if (q->mem[next].burst < q->mem[from].burst)
	{
		__iopq_pair_swap(&q->mem[from], &q->mem[next]);
		__iopq_sift_down(q, next);
	}
}

int iopq_empty(iopq *q)
{
	return q->sz == 0;
}

void iopq_push(iopq *q, iopq_pair elem)
{
	q->mem[q->sz++] = elem;
	__iopq_sift_up(q, q->sz-1);
}

iopq_pair iopq_pop(iopq *q)
{
	iopq_pair elem = q->mem[0];
	q->mem[0] = q->mem[--q->sz];
	__iopq_sift_down(q, 0);

	return elem;
}

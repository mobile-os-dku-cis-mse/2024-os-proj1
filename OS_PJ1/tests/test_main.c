#include <stdio.h>
#include <assert.h>
#include "scheduler.h"

void test_scheduler()
{
    // Initial tests to check if global variables are set correctly
    assert(NUM_CHILDREN == 10);
    assert(TIME_QUANTUM == 1);
}

#include <stdio.h>
#include <assert.h>
#include "child_process.h"

void test_create_children()
{
    // Assuming we have created children successfully
    create_children(NUM_CHILDREN, msgid);
    // Check if the number of children is correct
    assert(child_pids[NUM_CHILDREN - 1] != 0);
}

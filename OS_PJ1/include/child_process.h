#ifndef CHILD_PROCESS_H
#define CHILD_PROCESS_H

void create_children(int num_children, int msgid);
void switch_to_next_child();

#endif // CHILD_PROCESS_H
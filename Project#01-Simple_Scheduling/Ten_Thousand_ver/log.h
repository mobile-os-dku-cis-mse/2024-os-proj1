#pragma once

#include "common.h"
#include "process.h"
#include "queue.h"

void initialize_log(const char *filename);
void cal_wait_time(Node *current_node);
void cal_turn_time(Node *current_node);
const char* get_timestamp();
void log_process_event(const Process *process, const char *event);
void log_queue_state(const char *queue_name, const Queue *queue);
void close_log();
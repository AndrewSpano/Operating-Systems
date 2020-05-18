#ifndef __UTIL__
#define __UTIL__

#include "shared_memory.h"

void *MallocOrDie(size_t MemSize);
void free_and_exit(shared_segment_t* shared_memory, int shmid, char** file_name);
int get_nth_string(char* str, const char buf[], int n);
unsigned long waiting_time(struct tm start_time, struct tm end_time);
void print_time_from_seconds(unsigned long seconds, char* activity);


#endif

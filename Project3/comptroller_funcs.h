#ifndef __COMPTROLLER__
#define __COMPTROLLER__

#define COMP_ARGS 7


#include "shared_memory.h"

int get_comptroller_input(char* argv[], double* time, double* stattimes, key_t* shmid_key);
void print_state(shared_segment_t* shared_memory);
void print_statistics(shared_segment_t* shared_memory);

#endif

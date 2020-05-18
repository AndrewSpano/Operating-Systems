#ifndef __STATION_MANAGER__
#define __STATION_MANAGER__

#define STAT_MAN_ARGS 3

#include "shared_memory.h"


int get_station_manager_input(char* argv[], key_t* shmid_key);
int findSpot(shared_segment_t* shared_memory, Destination dest);


#endif

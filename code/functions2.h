#ifndef __FUNCTIONS2__
#define __FUNCTIONS2__

#include "structs.h"
#include "macros.h"
#include "list.h"

off_t find_hole(hole_map* holes, size_t my_size);
int cfs_mkdir(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, char* name);

#endif

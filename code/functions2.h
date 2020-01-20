#ifndef __FUNCTIONS2__
#define __FUNCTIONS2__

#include "structs.h"
#include "macros.h"
#include "list.h"

off_t find_hole(char* filename, int fd, size_t my_size);
int cfs_mkdir(char * name, uint bs, uint fns, uint cfs, uint mdfn, int fd, Stack_List* list);

#endif

#ifndef __FUNCTIONS2__
#define __FUNCTIONS2__

#include "structs.h"
#include "macros.h"
#include "list.h"


int get_nth_pair(MDS* mds, char** name, off_t* offset, int fd, int n);

int cfs_ls(int fd, off_t offset);



#endif

#ifndef __FUNCTIONS__
#define __FUNCTIONS__

#include "structs.h"

int cfs_create(char* cfs_filename, uint bs, uint fns, uint cfs, uint mdfn);
int cfs_read(int fd);

#endif

#ifndef __FUNCTIONS__
#define __FUNCTIONS__

#include "structs.h"
#include "macros.h"
#include "list.h"



int cfs_create(char* cfs_filename, size_t bs, size_t fns, size_t cfs, uint mdfn);
int cfs_workwith(char* cfs_filename, Stack_List** list);
int cfs_touch(const char buffer[], int fd);
int cfs_read(char* cfs_filename, int fd);

#endif

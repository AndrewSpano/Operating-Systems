#ifndef __FUNCTIONS__
#define __FUNCTIONS__

#include "structs.h"
#include "macros.h"
#include "list.h"



int cfs_create(char* cfs_filename, size_t bs, size_t fns, size_t cfs, uint mdfn);
int cfs_workwith(char* cfs_filename, superblock** my_superblock, hole_map** holes, Stack_List** list);
int cfs_mkdir(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, MDS* current_directory, off_t parent_offset, char* insert_name);
int cfs_touch(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, MDS* current_directory, off_t parent_offset, char* insert_name, off_t file_offset, int flag_a, int flag_m);
int cfs_cd(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, const char path[]);

int cfs_read(char* cfs_filename, int fd);

#endif

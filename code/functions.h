#ifndef __FUNCTIONS__
#define __FUNCTIONS__

#include "structs.h"
#include "macros.h"
#include "list.h"



int cfs_create(char* cfs_filename, size_t bs, size_t fns, size_t cfs, uint mdfn);
int cfs_workwith(char* cfs_filename, superblock** my_superblock, hole_map** holes, Stack_List** list);
int cfs_mkdir(int fd, superblock* my_superblock, hole_map* holes, MDS* current_directory, off_t parent_offset, char* insert_name);
int cfs_touch(int fd, superblock* my_superblock, hole_map* holes, MDS* current_directory, off_t parent_offset, char* insert_name, off_t file_offset, int flag_a, int flag_m);
int cfs_pwd(int fd, superblock* my_superblock, Stack_List* list);
int cfs_cd(int fd, superblock* my_superblock, Stack_List* list, const char path[]);
int cfs_cp(int fd, superblock* my_superblock, hole_map* holes, MDS* source, char* source_name, MDS* destination_directory, off_t destination_offset, int flag_R, int flag_i, int flag_r, char* source_path, char* destination_path);
int cfs_cat(int fd, superblock* my_superblock, hole_map* holes, MDS* destination_file, off_t destination_file_offset, MDS* source_file);
int cfs_ln(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, char source_file_path[], char output_file_path[]);
int cfs_rm(int fd, superblock* my_superblock, hole_map* holes, MDS* remove_entity, off_t remove_offset, MDS* parent_entity, off_t parent_offset, int flag_i, int flag_r, char* remove_path);
int cfs_import(int fd, superblock* my_superblock, hole_map* holes, MDS* destination_directory, off_t destination_offset, char* linux_path_name);
int cfs_export(int fd, superblock* my_superblock, MDS* source, char* linux_path_name);

int cfs_read(char* cfs_filename, int fd);

#endif

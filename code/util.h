#ifndef __UTIL__
#define __UTIL__

#define MAX_BUFFER_SIZE 512

#include "structs.h"
#include "macros.h"


/* useful functions */
int get_nth_string(char* str, const char buf[], int n);
int get_option(const char buffer[]);
int char_exists_in_string(const char* str, char x);
int is_parameter(const char* str);
int is_positive_integer(const char* str);
int contains_unknown_flag(const char* str, const char* flags);
int path_is_absolute(const char path[]);
int extract_last_entity_from_path(char path[], char* destination_string);
int get_approval(char* source, char* destination, char* operation);

/* get the parameters of functions */
int get_cfs_touch_parameters(const char buffer[], int* flag_a, int* flag_m);
int get_cfs_ls_parameters(const char buffer[], int* flag_a, int* flag_r, int* flag_l, int* flag_u, int* flag_d, int* flag_h);
int get_cfs_cp_parameters(const char buffer[], int* flag_R, int* flag_i, int* flag_r);
int get_cfs_mv_parameters(const char buffer[], int* flag_i);
int get_cfs_rm_parameters(const char buffer[], int* flag_i, int* flag_R);
int get_cfs_create_parameters(const char buffer[], size_t* bs, size_t* fns, size_t* cfs, uint* mdfn, char* cfs_filename);

/* navigation functions */
off_t* pointer_to_offset(char* pointer, size_t fns);
char* pointer_to_next_name(char* pointer, size_t fns);

/* initialization functions */
void initialize_superblock(superblock* my_superblock, char* cfs_filename, int fd, off_t root_directory_offset, size_t current_size, size_t bs, size_t fns, size_t cfs, uint mdfn);
void initialize_holes(hole_map* holes, uint n, uint current_holes, off_t hole_start);
void initialize_MDS(MDS* mds, uint id, uint type, uint number_of_hard_links, uint blocks_using, size_t size, off_t parent_offset, off_t first_block);
void initialize_data_Block(Block* block, size_t block_size);
void initialize_Directory_Data_Block(Block* block, size_t fns, off_t self_offset, off_t parent_offset);

/* fast access functions */
superblock* get_superblock(int fd);
hole_map* get_hole_map(int fd);
MDS* get_MDS(int fd, off_t offset);
Block* get_Block(int fd, size_t block_size, off_t offset);

/* fast set functions */
int set_superblock(superblock* superblock, int fd);
int set_hole_map(hole_map* holes, int fd);
int set_MDS(MDS* mds, int fd, off_t offset);
int set_Block(Block* block, int fd, size_t block_size, off_t offset);

/* printing functions */
void print_superblock(superblock* my_superblock);
void print_hole_table(hole_map* holes);
void print_MDS(MDS* mds);
void print_Directory_Data_Block(Block* block, size_t fns);


#endif

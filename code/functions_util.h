#ifndef __FUNCTIONS_UTIL__
#define __FUNCTIONS_UTIL__

#include "structs.h"
#include "util.h"

void insert_pair(Block* block, char* insert_name, off_t insert_offset, size_t fns);
void shift_pairs_to_the_left(char* name, uint remaining_pairs, size_t size_of_pair, size_t fns);
int remove_pair(Block* block, char* remove_name, size_t fns);
int directory_data_block_Is_Full(Block* block, size_t block_size, size_t fns);
off_t get_offset(Block* block, char* target_name, size_t fns);

#endif

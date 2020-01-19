#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functions_util.h"
#include "util.h"



/* inserts a pair: <name, offset> to the data block of a directory */
void insert_pair(Block* block, char* insert_name, off_t insert_offset, size_t fns)
{
  char* name = (char *) block->data;
  name += block->bytes_used;

  /* this part may cause segmentation error, if we write past the block size */
  strcpy(name, insert_name);
  off_t* offset = pointer_to_offset(name, fns);
  *offset = insert_offset;

  size_t pair_size = fns + sizeof(off_t);
  block->bytes_used += pair_size;
}


/* shift the pairs of the data block of a directory, one place to the left */
int shift_pairs_to_the_left(char* name, uint remaining_pairs, size_t size_of_pair, size_t fns)
{
  size_t size_of_remaining_pairs = remaining_pairs * size_of_pair;
  /* temp array used to store the pairs */
  void* temp_pairs = NULL;
  MALLOC_OR_DIE_3(temp_pairs, size_of_remaining_pairs);


  char* next_name = pointer_to_next_name(name, fns);

  /* copy to the temporary space */
  memcpy(temp_pairs, next_name, size_of_remaining_pairs);
  /* "delete" the previous values */
  memset(next_name, 0, size_of_remaining_pairs);
  /* copy in the right place */
  memcpy(name, temp_pairs, size_of_remaining_pairs);

  free(temp_pairs);

  return 1;
}


/* remove a pair: <name, offset> from the data block of a directory, while
   also checking for existing errors. Returns 1 on success; -1 on failure */
int remove_pair(Block* block, char* remove_name, size_t fns)
{
  /* if the data is less than the minimum data for a directory, or if we want
     to remove a key directory, then we have an error */
  if (block->bytes_used <= (2 *(fns + sizeof(size_t))) || !strcmp(remove_name, "./") || !strcmp(remove_name, "../"))
  {
    return -1;
  }

  size_t size_of_pair = fns + sizeof(off_t);
  uint pairs = block->bytes_used / size_of_pair;

  char* name = (char *) block->data;

  int i = 1;
  /* skip the pairs until we arrive to the wanted pair */
  while (strcmp(name, remove_name))
  {
    name = pointer_to_next_name(name, fns);
    i++;

    /* if we iterated through the whole data and did not find the given name,
       then it is invalid and therefore we have an error */
    if (i > pairs)
    {
      return -1;
    }
  }

  /* set data to 0 <=> remove pair */
  memset(name, 0, fns);
  off_t* offset = pointer_to_offset(name, fns);
  *offset = 0;

  if (pairs - i > 0)
  {
    /* shift the pairs to the left */
    uint remaining_pairs = pairs - i;
    shift_pairs_to_the_left(name, remaining_pairs, size_of_pair, fns);
  }

  return 1;
}


/* returns 1 if the directory data block is full; else returns 0 */
int directory_data_block_Is_Full(Block* block, size_t block_size, size_t fns)
{
  size_t size_of_struct_variables = sizeof(Block);
  size_t size_for_pairs = block_size - size_of_struct_variables;

  size_t size_remaining_for_pairs = size_for_pairs - block->bytes_used;
  size_t size_of_pair = fns + sizeof(off_t);

  /* if at least one pair can't fit, then the block is full */
  if (size_remaining_for_pairs < size_of_pair)
  {
    return 1;
  }

  return 0;
}


/* returns non-zero value if the directory data block is empty;
   else, if its not empty, it returns 0 */
int directory_data_block_Is_Empty(Block* block)
{
  return block->bytes_used == 0;
}


/* returns the offset that corresponds to the target_name name given upon success;
   on failure, returns 0 */
off_t get_offset(Block* block, char* target_name, size_t fns)
{
  size_t size_of_pair = fns + sizeof(off_t);
  uint pairs = block->bytes_used / size_of_pair;

  char* name = (char *) block->data;

  int i = 0;
  for (; i < pairs; i++)
  {
    if (!strcmp(name, target_name))
    {
      off_t* address_of_offset = pointer_to_offset(name, fns);
      off_t value = *address_of_offset;

      return value;
    }

    name = pointer_to_next_name(name , fns);
  }

  return 0;
}





/* function that shifts the holes of the hole table 1 position to the left */
int shift_holes_to_the_left(hole_map* holes, uint hole_position)
{
  uint holes_remaining = holes->current_hole_number - hole_position - 1;
  size_t size_of_remaining_holes = holes_remaining * sizeof(hole);

  void* temp_holes = NULL;
  MALLOC_OR_DIE_3(temp_holes, size_of_remaining_holes);

  memcpy(temp_holes, &(holes->holes_table[hole_position + 1]), size_of_remaining_holes);

  memset(&(holes->holes_table[hole_position + 1]), 0, size_of_remaining_holes);

  memcpy(&(holes->holes_table[hole_position ]), temp_holes, size_of_remaining_holes);

  free(temp_holes);

  return 1;
}


/* function that shifts the holes of the hole table 1 position to the left */
int shift_holes_to_the_right(hole_map* holes, uint hole_position)
{
  uint holes_remaining = holes->current_hole_number - hole_position;
  size_t size_of_remaining_holes = holes_remaining * sizeof(hole);

  void* temp_holes = NULL;
  MALLOC_OR_DIE_3(temp_holes, size_of_remaining_holes);

  memcpy(temp_holes, &(holes->holes_table[hole_position]), size_of_remaining_holes);

  memset(&(holes->holes_table[hole_position]), 0, size_of_remaining_holes);

  memcpy(&(holes->holes_table[hole_position + 1]), temp_holes, size_of_remaining_holes);

  free(temp_holes);

  return 1;
}

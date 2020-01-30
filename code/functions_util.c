#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functions_util.h"
#include "functions.h"
#include "util.h"



/* inserts a pair: <name, offset> to the data block of a directory */
void insert_pair_into_block(Block* block, char* insert_name, off_t insert_offset, size_t fns)
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


/* inserts a pair: <name, offset> to a directory */
int insert_pair(int fd, hole_map* holes, MDS* mds, char* insert_name, off_t insert_offset, size_t block_size, size_t fns)
{
  off_t block_position = mds->first_block;
  Block* block = get_Block(fd, block_size, block_position);
  DIE_IF_NULL(block);

  int new_block_needed = 0;

  while (directory_data_block_Is_Full(block, block_size, fns))
  {
    if (block->next_block == 0)
    {
      new_block_needed = 1;
      break;
    }

    block_position = block->next_block;
    free(block);
    block = get_Block(fd, block_size, block_position);
    DIE_IF_NULL(block);
  }

  if (!new_block_needed)
  {
    insert_pair_into_block(block, insert_name, insert_offset, fns);
    int retval = set_Block(block, fd, block_size, block_position);
    if (retval == 0)
    {
      printf("Error in set_Block() when called from insert_pair().\n");
      free(block);

      return 0;
    }

    free(block);
  }
  else
  {
    off_t new_block_position = find_hole(holes, block_size);
    if (new_block_position == 0)
    {
      printf("No hole was found that could fit a new block. Make the hole map bigger.\n");
      return 0;
    }

    block->next_block = new_block_position;
    int retval = set_Block(block, fd, block_size, block_position);
    if (retval == 0)
    {
      printf("Error in set_Block() when called from insert_pair().\n");
      free(block);

      return 0;
    }

    free(block);

    Block* new_block = NULL;
    MALLOC_OR_DIE_3(new_block, block_size);

    new_block->next_block = 0;
    new_block->bytes_used = 0;
    insert_pair_into_block(block, insert_name, insert_offset, fns);

    retval = set_Block(new_block, fd, block_size, new_block_position);
    if (retval == 0)
    {
      printf("Error in set_Block() when called from insert_pair().\n");
      free(new_block);

      return 0;
    }

    free(new_block);
  }

  if (new_block_needed)
  {
    return 1;
  }
  else
  {
    return -1;
  }
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




/* implements first fit algorithm */
off_t find_hole(hole_map* holes, size_t my_size)
{
  /* the position that will be returned */
  off_t offset_to_return = 0;

  int i = 0;
  for (; i < MAX_HOLES; i++)
  {
    /* if we reach the last hole (which is the biggest), use it */
    if (holes->holes_table[i].end == 0)
    {
      offset_to_return = holes->holes_table[i].start;
      /* make the hole smaller because it will host a new entity */
      holes->holes_table[i].start += my_size;

      return offset_to_return;
    }

    size_t available_size = holes->holes_table[i].end - holes->holes_table[i].start;

    /* if it fits exactly */
    if (available_size == my_size)
    {
      offset_to_return = holes->holes_table[i].start;
      /* the entity fits exactly, so the hole will disappear, and therefore
         we must shift the previous holes one position to the left to fill it */
      shift_holes_to_the_left(holes, i);
      /* the hole disappeared, so decrement the counter by 1 */
      holes->current_hole_number--;

      return offset_to_return;
    }
    /* else, if the entity has a smaller size than the hole, then the hole will
       keep existing, but become smaller */
    else if (available_size > my_size)
    {
      offset_to_return = holes->holes_table[i].start;
      /* shrink the whole by the size of the entity to be inserted */
      holes->holes_table[i].start += my_size;

      return offset_to_return;
    }
  }

  /* if no hole is found, return 0 */
  return (off_t) 0;
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

  memcpy(&(holes->holes_table[hole_position]), temp_holes, size_of_remaining_holes);

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


/* returns the number of sub-entites that the directory has */
uint number_of_sub_entities_in_directory(MDS* current_directory, size_t fns)
{
  size_t directory_data_size = current_directory->size;
  size_t size_of_pair = fns + sizeof(off_t);

  uint total_pairs = directory_data_size / size_of_pair;
  return total_pairs;
}


/* returns the offset of the name if the name already exists in the directory
   as a sub-entity; else returns 0 */
off_t directory_get_offset(int fd, MDS* directory, size_t block_size, size_t fns, char* target_name)
{
  off_t block_position = directory->first_block;
  size_t size_of_pair = fns + sizeof(off_t);

  /* check all the blocks of a directory */
  while (block_position != 0)
  {
    /* get the block that is being pointed at */
    Block* block = get_Block(fd, block_size, block_position);
    if (block == NULL)
    {
      perror("malloc() error in directory_get_offset()");
      return (off_t) 0;
    }

    uint number_of_pairs = block->bytes_used / size_of_pair;
    char* name = (char *) block->data;

    int i = 0;
    /* check all the names inside the data block of the directory */
    for (; i < number_of_pairs; i++)
    {
      /* if we find a same name, return 1 */
      if (!strcmp(name, target_name))
      {
        off_t* ptr = pointer_to_offset(name, fns);
        off_t return_offset = *ptr;
        free(block);
        return return_offset;
      }

      /* if not, point to the next name */
      name = pointer_to_next_name(name, fns);
    }

    /* get the position of the next block */
    block_position = block->next_block;
    /* free the current block */
    free(block);
  }

  /* return 0 if no same name is found after scanning all the directory blocks */
  return (off_t) -1;
}





/* return the offset of the last entity from a given path */
off_t get_offset_from_path(int fd, superblock* my_superblock, Stack_List* list, char original_path[])
{
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;

  char path[MAX_BUFFER_SIZE] = {0};
  strcpy(path, original_path);

  char last_entity_name[MAX_BUFFER_SIZE] = {0};
  extract_last_entity_from_path(path, last_entity_name);

  Stack_List* file_path_list = copy_List(list);
  if (file_path_list == NULL)
  {
    return (off_t) 0;
  }

  /* check if we are to stay in the current directory. if not, go */
  if (path[0] != 0)
  {
    int retval = cfs_cd(fd, my_superblock, file_path_list, path);
    /* check if the operation failed */
    if (retval == 0)
    {
      Stack_List_Destroy(&file_path_list);
      return (off_t) 0;
    }
    else if (retval == -1)
    {
      return (off_t) 0;
    }
  }

  /* get the offset of the current directory, in order to get the directory */
  off_t destination_directory_offset = Stack_List_Peek_offset(file_path_list);
  if (destination_directory_offset == (off_t) 0)
  {
    Stack_List_Destroy(&file_path_list);
    printf("Error in Stack_List_Peek_offset() when called from functions_util().\n");
    return (off_t) 0;
  }


  /* get the current directory */
  MDS* destination_file_directory = get_MDS(fd, destination_directory_offset);
  if (destination_file_directory == NULL)
  {
    Stack_List_Destroy(&file_path_list);
    return (off_t) 0;
  }


  /* check if the file does not exist in the directory we want to place it */
  off_t destination_file_offset = directory_get_offset(fd, destination_file_directory, block_size, fns, last_entity_name);
  if (destination_file_offset == 0)
  {
    free(destination_file_directory);
    Stack_List_Destroy(&file_path_list);
    return (off_t) 0;
  }
  else if (destination_file_offset == (off_t) -1)
  {
    if (path[0] != 0)
    {
      printf("Error input: the file \"%s\" does not exist in the directory %s.\n", last_entity_name, path);
    }
    else
    {
      printf("Error input: the file \"%s\" does not exist in the current directory.\n", last_entity_name);
    }
    free(destination_file_directory);
    Stack_List_Destroy(&file_path_list);
    return (off_t) 0;
  }


  /* free the dynamic structures and variables used */
  free(destination_file_directory);
  Stack_List_Destroy(&file_path_list);


  return destination_file_offset;
}

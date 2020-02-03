#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "functions_util.h"
#include "functions.h"
#include "functions2.h"
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

    initialize_data_Block(new_block, block_size);
    insert_pair_into_block(new_block, insert_name, insert_offset, fns);

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


/* returns 1 if a name exists in a directory, -1 if it doesn't and 0 if an error
   occurs in the process */
int name_exists_in_directory(int fd, MDS* directory, size_t block_size, size_t fns, char* target_name)
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
        free(block);
        return 1;
      }

      /* if not, point to the next name */
      name = pointer_to_next_name(name, fns);
    }

    /* get the position of the next block */
    block_position = block->next_block;
    /* free the current block */
    free(block);
  }

  /* return -1 if no same name is found after scanning all the directory blocks */
  return -1;
}


/* return the offset of the last entity from a given path */
off_t get_offset_from_path(int fd, superblock* my_superblock, Stack_List* list, char original_path[])
{
  /* check if an empty path has been given; this means that we want the
     current directory */
  if (original_path[0] == 0)
  {
    /* get the offset of the current directory, in order to get the directory */
    off_t destination_directory_offset = Stack_List_Peek_offset(list);
    if (destination_directory_offset == (off_t) 0)
    {
      printf("Error in Stack_List_Peek_offset() when called from functions_util().\n");
      return (off_t) 0;
    }

    return destination_directory_offset;
  }

  /* get some important sizes */
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
      return (off_t) -1;
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
    return (off_t) -1;
  }


  /* free the dynamic structures and variables used */
  free(destination_file_directory);
  Stack_List_Destroy(&file_path_list);


  return destination_file_offset;
}


/* stores the actual name of a directory (not "." or "..") in the variables legit_name.
   Returns 1 upon success; else returns 0 */
int get_legit_name_from_path(int fd, superblock* my_superblock, Stack_List* list, char original_path[], char* legit_name)
{
  /* create a new list to use cfs_cd on it */
  Stack_List* directory_path_list = copy_List(list);
  if (directory_path_list == NULL)
  {
    return 0;
  }

  /* original_path will always be a directory, so go to that directory */
  int retval = cfs_cd(fd, my_superblock, directory_path_list, original_path);
  /* check if the operation failed */
  if (!retval)
  {
    Stack_List_Destroy(&directory_path_list);
    return 0;
  }

  /* get the actual name */
  off_t spam_offset = 0;
  retval = Stack_List_Peek(directory_path_list, &legit_name, &spam_offset);
  if (!retval)
  {
    Stack_List_Destroy(&directory_path_list);
    return 0;
  }


  /* free up the allocated memory */
  Stack_List_Destroy(&directory_path_list);

  /* we got the name inside the variable legit_name, so we are finished */
  return 1;
}


/* copy the information from a linux file to a cfs file */
int copy_from_linux_to_cfs(int fd, superblock* my_superblock, hole_map* holes, MDS* imported_file, int linux_file_fd, size_t linux_file_size)
{
  /* important sizes */
  size_t block_size = my_superblock->block_size;
  size_t size_for_data = block_size - sizeof(Block);
  /* variables used later */
  uint number_of_blocks = linux_file_size / size_for_data;
  size_t last_block_size = linux_file_size % size_for_data;
  if (last_block_size > 0)
  {
    number_of_blocks++;
  }

  /* find where to place the first block */
  off_t block_position = find_hole(holes, block_size);
  imported_file->first_block = block_position;

  /* write in the block where the next block will be placed */
  off_t new_block_position = block_position;

  int i = 0;
  /* copy the first n - 1 blocks, which all will be full */
  for (; i < number_of_blocks - 1; i++)
  {
    /* get a block */
    Block* block = NULL;
    MALLOC_OR_DIE_3(block, block_size);
    initialize_data_Block(block, block_size);

    /* assign its values */
    new_block_position = find_hole(holes, block_size);
    block->next_block = new_block_position;
    block->bytes_used = size_for_data;
    ssize_t read_value = read(linux_file_fd, block->data, size_for_data);
    /* check for errors */
    if (read_value == -1 || read_value != size_for_data)
    {
      perror("\nread() error in copy_from_linux_to_cfs()");
      printf("read_value = %ld, size that should have been read = %ld\n", read_value, size_for_data);
      free(block);
      return 0;
    }

    /* write the block in the cfs */
    int retval = set_Block(block, fd, block_size, block_position);
    /* free up the allocated memory */
    free(block);
    /* check for errors */
    if (!retval)
    {
      return 0;
    }

    /* save the position of the new block to be created */
    block_position = new_block_position;
  }

  /* copy the last block, which may or may not contain less information */
  Block* last_block = NULL;
  MALLOC_OR_DIE_3(last_block, block_size);
  initialize_data_Block(last_block, block_size);

  last_block->next_block = 0;

  /* if last block fits exactly */
  if (last_block_size == 0)
  {
    last_block_size = size_for_data;
  }

  last_block->bytes_used = last_block_size;
  ssize_t read_value = read(linux_file_fd, last_block->data, last_block_size);
  if (read_value == -1 || read_value != last_block_size)
  {
    perror("\nread() (last) error in copy_from_linux_to_cfs()");
    printf("read_value = %ld, size that should have been read (last) = %ld\n", read_value, last_block_size);
    free(last_block);
    return 0;
  }




  /* write the last block in the cfs file */
  int retval = set_Block(last_block, fd, block_size, block_position);
  /* free up the allocated memory */
  free(last_block);
  /* check for errors */
  if (!retval)
  {
    return 0;
  }


  /* inform the imported file */
  imported_file->size = linux_file_size;
  imported_file->blocks_using = number_of_blocks;

  /* inform the superblock */
  my_superblock->current_size += block_size * number_of_blocks;

  return 1;
}


/* copy the information of a cfs file to a linux file */
int copy_from_cfs_to_linux(int fd, superblock* my_superblock, MDS* source, int linux_file_fd)
{
  /* get an important size */
  size_t block_size = my_superblock->block_size;

  /* return ok if the cfs file is of size 0 */
  if (source->size == 0)
  {
    return 1;
  }

  /* position of the first block */
  off_t block_position = source->first_block;

  /* iterate through all the blocks */
  while (block_position != 0)
  {
    /* get the information block */
    Block* block = get_Block(fd, block_size, block_position);
    /* check for errors */
    if (block == NULL)
    {
      return 0;
    }

    /* determine how much size will be written */
    size_t size_to_write = block->bytes_used;

    /* write the block to the linux file */
    ssize_t size_written = write(linux_file_fd, (char *) block->data, size_to_write);

    /* check for errors */
    if (size_written != size_to_write)
    {
      if (size_written == -1)
      {
        perror("write() error when called from copy_from_cfs_to_linux()");
      }
      else
      {
        printf("write() error in copy_from_cfs_to_linux(). It was supposed to write %ld bytes, but it wrote %ld bytes.\n", size_to_write, size_written);
      }
      return 0;
    }

    /* get the position of the next block */
    block_position = block->next_block;
    /* free up the allocated struct */
    free(block);
  }

  /* return 1 if everything goes smoothly */
  return 1;
}


/* removes the data blocks of an entity from the cfs <=> creates holes */
int remove_MDS_blocks(int fd, superblock* my_superblock, hole_map* holes, MDS* remove_entity)
{
  /* get important sizes */
  size_t block_size = my_superblock->block_size;

  /* get the first block */
  off_t block_position = remove_entity->first_block;

  /* iterate through all the blocks to remove them and create new holes */
  while (block_position != 0)
  {
    /* create the hole that comes us from the deletion of the block */
    int retval = insert_hole(holes, block_position, block_position + block_size, fd);
    /* check if any error has occured, which should never actually occur */
    if (!retval)
    {
      printf("Problem: elouse h insert_hole() gia trypa: hole.start = %ld, hole.end = %ld.\n", block_position, block_position + block_size);
      return 0;
    }
    /* inform the superblock */
    my_superblock->current_size -= block_size;

    /* get the next block we just made a hole of, to get the next block if it exists */
    Block* block = get_Block(fd, block_size, block_position);
    /* check for errors */
    if (block == NULL)
    {
      return 0;
    }

    /* get the position of the next block */
    block_position = block->next_block;

    /* free up the current block because we don't need it anymore */
    free(block);
  }

  return 1;
}


/* removes a pair: <name, offset> from a directory data block, and replaces it
   with the last pair of the last block of the directory */
int remove_pair_from_directory(int fd, superblock* my_superblock, hole_map* holes, MDS* directory, off_t directory_offset, off_t remove_offset)
{
  /* important sizes */
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;
  size_t size_of_pair = fns + sizeof(off_t);

  /* variables to indicate key values */
  Block* block_that_contains_offset = NULL;
  off_t position_of_block_that_contains_offset = 0;
  int position_inside_block = -1;


  /* get the first data block of a directory */
  off_t block_position = directory->first_block;
  /* also get the previous block */
  off_t previous_block = block_position;

  /* loop to find the pair we want */
  while (block_position != 0)
  {
    /* get the block */
    Block* block = get_Block(fd, block_size, block_position);
    /* check for errors */
    DIE_IF_NULL(block);


    uint number_of_pairs = block->bytes_used / size_of_pair;
    char* name = (char *) block->data;


    /* iterate through all the pairs inside the block */
    int pair = 0;
    for (; pair < number_of_pairs; pair++)
    {
      /* get the offset */
      off_t* offset = pointer_to_offset(name, fns);

      /* if we find the offset we are searching */
      if (*offset == remove_offset)
      {
        /* free the space and inform the block */
        memset(name, 0, size_of_pair);
        block->bytes_used -= size_of_pair;

        /* save important variables */
        position_inside_block = pair;
        block_that_contains_offset = block;
        position_of_block_that_contains_offset = block_position;
        break;
      }

      /* point to next name */
      name = pointer_to_next_name(name, fns);
    }

    /* then this means the block was found */
    if (position_inside_block != -1)
    {
      break;
    }


    /* update the variables */
    previous_block = block_position;
    block_position = block->next_block;
    /* free up the allocated space */
    free(block);
  }

  /* check for an unlike error */
  if (block_position == 0)
  {
    printf("block_position == 0 in remove_pair(). This should have never printed. Congratulations, ELOUSES.\n");
    return 0;
  }

  /* inform the variable */
  position_of_block_that_contains_offset = block_position;



  /* if the entity to be removed is in the last block */
  if (block_that_contains_offset->next_block == 0)
  {
    /* if the block is now empty, we must remove it */
    if (block_that_contains_offset->bytes_used == 0)
    {
      /* get the previous, which for sure exists because the first block will
         never be emptied because it contains directories "." and ".." */
      Block *previous_of_last_block = get_Block(fd, block_size, previous_block);
      /* if get_Block() fails */
      if (previous_of_last_block == NULL)
      {
        free(block_that_contains_offset);
        return 0;
      }

      /* update the previous block */
      previous_of_last_block->next_block = 0;
      /* set the previous block */
      int retval = set_Block(previous_of_last_block, fd, block_size, previous_block);
      free(previous_of_last_block);
      /* if set_Block() fails */
      if (!retval)
      {
        free(block_that_contains_offset);
        return 0;
      }

      /* a new hole has been created with the removal of the block */
      insert_hole(holes, position_of_block_that_contains_offset, position_of_block_that_contains_offset + block_size, fd);
      my_superblock->current_size -= block_size;
      directory->blocks_using--;
    }
    /* else, if the block was not empty then we do not need to remove any block */
    else
    {
      /* how many pairs existed in the block before the removal */
      uint pairs_that_were_in_block = block_that_contains_offset->bytes_used / size_of_pair + 1;

      /* if the pair that we removed was not the last, swap it with the last */
      if (position_inside_block != pairs_that_were_in_block - 1)
      {
        size_t start_of_removed_pair = position_inside_block * size_of_pair;
        size_t start_of_last_pair = (pairs_that_were_in_block - 1) * size_of_pair;

        /* copy the last pair in its new place inside the same directory */
        memcpy(block_that_contains_offset->data + start_of_removed_pair, block_that_contains_offset->data + start_of_last_pair, size_of_pair);
        /* set the previous memory to 0 */
        memset(block_that_contains_offset->data + start_of_last_pair, 0, size_of_pair);
      }

      /* set the block */
      int retval = set_Block(block_that_contains_offset, fd, block_size, position_of_block_that_contains_offset);
      /* if set_Block() fails */
      if (!retval)
      {
        free(block_that_contains_offset);
        return 0;
      }

    }
  }
  /* if not, go to the last block, take the last pair and place it */
  else
  {
    /* iterate through the blocks to get the last block */
    while (3 > 0)
    {
      /* get the block */
      Block* block = get_Block(fd, block_size, block_position);
      /* if get_Block() fails */
      if (block == NULL)
      {
        free(block_that_contains_offset);
        return 0;
      }

      /* if the current block is the last, exit */
      if (block->next_block == 0)
      {
        /* free up the allocated space */
        free(block);
        break;
      }

      /* get the new positions */
      previous_block = block_position;
      block_position = block->next_block;
      /* free up the allocated memory */
      free(block);
    }

    /* inform the variables */
    off_t position_of_last_block = block_position;
    off_t position_of_previous_of_last_block = previous_block;

    /* for start, get the last block */
    Block* last_block = get_Block(fd, block_size, position_of_last_block);
    /* if get_Block() fails */
    if (last_block == NULL)
    {
      free(block_that_contains_offset);
      return 0;
    }

    /* how many pairs the last block contains */
    uint pairs_in_last_block = last_block->bytes_used / size_of_pair;

    /* get the positions of the pairs inside the data arrays */
    size_t start_of_removed_pair = position_inside_block * size_of_pair;
    size_t start_of_last_pair = (pairs_in_last_block - 1) * size_of_pair;

    /* copy the last pair in its new place inside the same directory */
    memcpy(block_that_contains_offset->data + start_of_removed_pair, last_block->data + start_of_last_pair, size_of_pair);
    /* fix its size */
    block_that_contains_offset->bytes_used += size_of_pair;
    /* set the previous memory to 0 */
    memset(last_block->data + start_of_last_pair, 0, size_of_pair);
    /* fix the size of the last block */
    last_block->bytes_used -= size_of_pair;


    /* set the block with the new pair */
    int retval = set_Block(block_that_contains_offset, fd, block_size, position_of_block_that_contains_offset);
    /* if set_Block() fails */
    if (!retval)
    {
      free(block_that_contains_offset);
      free(last_block);
      return 0;
    }

    /* if the last block contained only 1 pair and it got removed, remove the
       whole block, set the previous block accordingly and create a new hole */
    if (last_block->bytes_used == 0)
    {
      /* get the previous block in order to set to 0 its "next_block" attribute */
      Block *previous_of_last_block = get_Block(fd, block_size, position_of_previous_of_last_block);
      /* if get_Block() fails */
      if (previous_of_last_block == NULL)
      {
        free(block_that_contains_offset);
        free(last_block);
        return 0;
      }

      /* update the previous block */
      previous_of_last_block->next_block = 0;
      /* set the previous block */
      retval = set_Block(previous_of_last_block, fd, block_size, position_of_previous_of_last_block);
      free(previous_of_last_block);
      /* if set_Block() fails */
      if (!retval)
      {
        free(last_block);
        return 0;
      }

      /* a new hole has been created with the removal of the last block */
      insert_hole(holes, position_of_last_block, position_of_last_block + block_size, fd);
      my_superblock->current_size -= block_size;
      directory->blocks_using--;
    }
    /* else if the last pair of the last block got removed without causing any
       further deletion, just update the last block */
    else
    {
      /* set the previous block */
      retval = set_Block(last_block, fd, block_size, position_of_last_block);
      /* if set_Block() fails */
      if (!retval)
      {
        free(block_that_contains_offset);
        free(last_block);
        return 0;
      }
    }

    /* freep up the last block because we don't need it anymore */
    free(last_block);

  }


  /* free up the allocated space */
  free(block_that_contains_offset);

  /* inform the directory */
  directory->size -= size_of_pair;
  /* update the directory */
  int retval = set_MDS(directory, fd, directory_offset);
  if (!retval)
  {
    return 0;
  }

  return 1;
}

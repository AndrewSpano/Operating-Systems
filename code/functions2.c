#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "structs.h"
#include "functions2.h"
#include "util.h"


//pairnei to megethos tou struct pou theloume na apothikefsoume sto cfs_file
//kai epistrefei to offset pou xwraei(tripa i telos)
off_t find_hole(hole_map* holes, size_t my_size)
{
  off_t offset_to_return = 0;

  int i = 0;
  for (; i < MAX_HOLES; i++)
  {
    if (holes->holes_table[i].end == 0)
    {
      // printf("start = %lu, end = %lu\n", holes->holes_table[i].start, holes->holes_table[i].end);
      //end of current files
      printf("wanted offset %lu\n", holes->holes_table[i].start);
      offset_to_return = holes->holes_table[i].start;
      holes->holes_table[i].start += my_size;

      return offset_to_return;
    }
    // printf("start = %lu, end = %lu\n", holes->holes_table[i].start, holes->holes_table[i].end);
    off_t available_size = holes->holes_table[i].end - holes->holes_table[i].start;
    printf("available_size = %lu\n", available_size);

    if (available_size == my_size) //fits exactly
    {
      offset_to_return = holes->holes_table[i].start;
      shift_holes_to_the_left(holes, i);
      holes->current_hole_number--; //after shifting

      return offset_to_return;
    }
    else if (available_size > my_size) //leaves a hole
    {
      offset_to_return = holes->holes_table[i].start;
      holes->holes_table[i].start += my_size; // shrink hole

      return offset_to_return;
    }
  }


  return 0;
}



int cfs_mkdir(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, char* name)
{
  //need to check if #_of_entities< mdfn


  // find holes for mds and data block
  off_t offset_for_mds = find_hole(holes, sizeof(MDS));
  off_t offset_for_data_block = find_hole(holes, my_superblock->block_size);

  //initialize data block
  Block* data_block = NULL;
  MALLOC_OR_DIE(data_block, my_superblock->block_size, fd);
  /* initialize its values */

  //get current directory offset
  char * parent_name = malloc(sizeof(char) * my_superblock->filename_size);
  off_t parent_offset;
  Stack_List_Peek(list, &parent_name, &parent_offset);


  // void initialize_Directory_Data_Block(Block* block, size_t fns, off_t self_offset, off_t parent_offset);
  initialize_Directory_Data_Block(data_block, fns, offset_for_data_block, parent_offset);
  // write to the cfs file
  set_Block(data_block, fd, my_superblock->block_size, offset_for_data_block);

  //initialize mds
  //get superblock for id

  /* create the struct */
  MDS* mds = NULL;
  MALLOC_OR_DIE(mds, sizeof(MDS), fd);
  /* initialize its values */
  // void initialize_MDS(MDS* mds, uint id, uint type, uint number_of_hard_links, uint blocks_using, size_t size, off_t parent_offset, off_t first_block);
  initialize_MDS(mds, my_superblock->total_entities + 1, DIRECTORY, 1, 1, sizeof(MDS) + my_superblock->block_size, parent_offset, offset_for_data_block);
  /* write to the cfs file */
  set_MDS(mds, fd, offset_for_mds);


  //enimerosi parent directory
  off_t current_offset = parent_offset;
  MDS* mds_cur = get_MDS(fd, parent_offset);
  Block* last_block = get_Block(fd, my_superblock->block_size, mds_cur->first_block);
  while (last_block->next_block != 0)
  {
    last_block = get_Block(fd, my_superblock->block_size, last_block->next_block);
  }

  if(directory_data_block_Is_Full(last_block, my_superblock->block_size, fns) == 0) //there is space in the block
  {
    insert_pair(last_block, name, offset_for_mds, my_superblock->filename_size);
  }
  else //block is full
  { //allocate new block
    off_t new_block_hole = find_hole(holes, my_superblock->block_size);

    Block* new_block = NULL;
    new_block = malloc(my_superblock->block_size);

    new_block->next_block = 0;
    new_block->bytes_used = fns + sizeof(off_t);

    last_block->next_block = new_block_hole;

    insert_pair(block, name, new_block_hole, my_superblock->filename_size);
  }




  //enimerosi superblock
  my_superblock->total_entities += 1;
  my_superblock->current_size += sizeof(MDS) + my_superblock->block_size; //parent's block had space for the new entry

  return 0;
}

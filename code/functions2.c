#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "structs.h"
#include "functions2.h"
#include "util.h"
#include "functions_util.h"


int get_nth_pair(MDS* mds, char** name, off_t* offset, int fd, int n)
{ 
  superblock* my_superblock = get_superblock(fd);
  Block* my_block = get_Block(fd, my_superblock->block_size, mds->first_block);

  size_t size_of_struct_variables = sizeof(Block);
  size_t size_for_pairs = my_superblock->block_size - size_of_struct_variables;
  size_t size_of_pair = my_superblock->filename_size + sizeof(off_t);
  size_t pairs_in_block = my_block->bytes_used / size_of_pair;
  
  if (pairs_in_block < n)
  {
    printf("error: wrong n\n");
    return 0;
  }

  
  size_t max_pairs = size_for_pairs / size_of_pair;
  
  while(n > max_pairs)
  {
    Block* temp_block = my_block;
    my_block = get_Block(fd, my_superblock->block_size, my_block->next_block);
    free(temp_block);
    n = n - max_pairs; 
  } 

  char* ret_name = (char *) my_block->data;
  // printf("first name: %s\n", ret_name);
  off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
  // printf("offset for name: %lu\n", *ret_offset);
  
  int j = 1;
  for (; j < n ; j++)
  {
    ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
    ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
  }

  free(my_block);
  free(my_superblock);
  strcpy(*name,ret_name); 
  *offset = *ret_offset;
  /*failure*/
  return 0;
}

int get_type(int fd, off_t offset)
{
  MDS* my_mds = get_MDS(fd, offset);
  if (my_mds == NULL)
  {
    printf("get_MDS error\n");
    return -1;
  }
  int ret_type = my_mds->type;
  free(my_mds);
  return ret_type;
}


int cfs_ls(int fd, off_t offset)
{
  superblock* my_superblock = get_superblock(fd);
  MDS* my_mds = get_MDS(fd, offset);
  Block* my_block = get_Block(fd, my_superblock->block_size, my_mds->first_block);

  size_t size_of_struct_variables = sizeof(Block);
  size_t size_for_pairs = my_superblock->block_size - size_of_struct_variables;
  size_t size_of_pair = my_superblock->filename_size + sizeof(off_t);
  size_t pairs_in_block = my_block->bytes_used / size_of_pair;
  size_t max_pairs = size_for_pairs / size_of_pair;

  char* ret_name = (char *) my_block->data; //.
  // printf("first name: %s\n", ret_name);
  // off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
  // printf("offset for name: %lu\n", *ret_offset);
  ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size); //..
  // ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);


  int i = 2;
  for (; i < pairs_in_block; i++)
  {
    ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
    printf("%s ", ret_name);
    // ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
    // printf("%s\n", ret_offset);

  }

  //LOUZEI
  if (my_block->next_block != 0)
  {
    Block* temp_block = my_block;
    my_block = get_Block(fd, my_superblock->block_size, my_block->next_block);
    free(temp_block);

    /*if there is more than 1 blocks for data*/  
    while (my_block != 0) 
    {
      char* ret_name = (char *) my_block->data; //.
      printf("%s ", ret_name);
      // off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
      int i = 1;
      for (; i < pairs_in_block; i++)
      {
        ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
        printf("%s ", ret_name);
        // ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
        // printf("%s\n", ret_offset);
      } 

      /*get next block*/
      Block* temp_block = my_block;
      my_block = get_Block(fd, my_superblock->block_size, my_block->next_block);
      free(temp_block);
    }  
  }

  printf("\n");

  free(my_block);
  free(my_mds);
  free(my_superblock);

  return 0;
}
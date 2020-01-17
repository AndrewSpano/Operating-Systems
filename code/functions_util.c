#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functions_util.h"



void insert_pair(Block* block, char* insert_name, size_t insert_offset, size_t fns)
{
  char* name = (char *) block->data;
  name += block->bytes_used;

  /* this part may cause segmentation error, if we write past the block size */
  strcpy(name, insert_name);
  size_t* offset = pointer_to_offset(name, fns);
  *offset = insert_offset;

  size_t pair_size = fns + sizeof(size_t);
  block->bytes_used += pair_size;
}

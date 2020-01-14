#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"




/* Extracts the n-th string from a buffer with n or more strings.
   Returns 1 if it succeds, else returns 0. */
int get_nth_string(char* str, const char buf[], int n)
{
  /* check for valid parameters */
  if (n <= 0)
  {
    return 0;
  }


  int i = 0;
  int j = 1;
  /* use the loop to skip the first n - 1 strings, while also checking
     for errors */
  for (; j <= n - 1; j++)
  {

    while (i < MAX_BUFFER_SIZE && buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' && buf[i] != '\0')
    {
      i++;
    }

    if (i == MAX_BUFFER_SIZE || (buf[i] != ' ' && buf[i] != '\t'))
    {
      return 0;
    }

    while (i < MAX_BUFFER_SIZE && (buf[i] == ' ' || buf[i] == '\t'))
    {
      i++;
    }

  }

  /* check if we have already iterated through the buffer */
  if (i == MAX_BUFFER_SIZE || (buf[i] == '\n' || buf[i] == '\0'))
  {
    return 0;
  }


  int k = 0;
  /* extract the n-th string */
  while (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' && buf[i] != '\0')
  {
    str[k] = buf[i];
    i++;
    k++;

    if (i == 256)
    {
      memset(str, 0, k);
      return 0;
    }
  }
  str[k] = '\0';

  return 1;

}


/* If the buffer has a valid option, it returns the number of that option (1-14)
   Else, if there is an error, then -1 is returned */
int get_option(const char buffer[])
{
  /* string that contains the option */
  char option[MAX_BUFFER_SIZE];
  /* get the first string of the buffer to distinguish which option we have */
  int retval = get_nth_string(option, buffer, 1);

  if (retval == 0)
  {
    /* some error occured */
    return -1;
  }

  if (!strcmp(option, "cfs_workwith"))
  {
    return 1;
  }
  else if (!strcmp(option, "cfs_mkdir"))
  {
    return 2;
  }
  else if (!strcmp(option, "cfs_touch"))
  {
    return 3;
  }
  else if (!strcmp(option, "cfs_pwd"))
  {
    return 4;
  }
  else if (!strcmp(option, "cfs_cd"))
  {
    return 5;
  }
  else if (!strcmp(option, "cfs_ls"))
  {
    return 6;
  }
  else if (!strcmp(option, "cfs_cp"))
  {
    return 7;
  }
  else if (!strcmp(option, "cfs_cat"))
  {
    return 8;
  }
  else if (!strcmp(option, "cfs_ln"))
  {
    return 9;
  }
  else if (!strcmp(option, "cfs_mv"))
  {
    return 10;
  }
  else if (!strcmp(option, "cfs_rm"))
  {
    return 11;
  }
  else if (!strcmp(option, "cfs_import"))
  {
    return 12;
  }
  else if (!strcmp(option, "cfs_export"))
  {
    return 13;
  }
  else if (!strcmp(option, "cfs_create"))
  {
    return 14;
  }
  else
  {
    /* else no valid option has been found */
    return -1;
  }


}



void initialize_holes(hole_table* holes, int n)
{
  int i = 0;
  for (; i < n; i++)
  {
    holes->holes[i].start = 0;
    holes->holes[i].end = 0;
  }
}



void print_superblock(superblock* my_superblock)
{
  printf("\n\nSUPERBLOCK\n\n");
  printf("fd = %d\n", my_superblock->fd);
  printf("cfs_filename = %s\n", my_superblock->cfs_filename);
  printf("root_directory = %lu\n", my_superblock->root_directory);
  printf("current_size = %lu\n", my_superblock->current_size);
  printf("current_hole_number = %u\n", my_superblock->current_hole_number);
  printf("bs = %ld\n", my_superblock->block_size);
  printf("fns = %lu\n", my_superblock->filename_size);
  printf("cfs = %lu\n", my_superblock->max_file_size);
  printf("mdfn = %u\n", my_superblock->max_dir_file_number);
}



void print_hole_table(hole_table* holes_table)
{
  printf("\n\nHOLE TABLE\n\n");
  int i = 0;
  for (; i < MAX_HOLES; i++)
  {
    printf("start = %u, end = %u\n", holes_table->holes[i].start, holes_table->holes[i].end);

    if (holes_table->holes[i].end == 0)
    {
      return;
    }
  }
}



void print_MDS(MDS* mds)
{
  printf("\n\nMDS\n\n");
  printf("size = %u\n", mds->size);
  printf("type = %u\n", mds->type);
  printf("parent_offset = %u\n", mds->parent_offset);
  printf("creation_time = %u\n", mds->creation_time);
  printf("access_time = %u\n", mds->access_time);
  printf("modification_time = %u\n", mds->modification_time);
  printf("blocks_using = %u\n", mds->blocks_using);
  printf("first_block = %u\n", mds->first_block);
  printf("name = %s\n", mds->name);
}

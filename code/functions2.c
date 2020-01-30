#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

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

int print_characteristics(int fd, off_t offset)
{
  MDS* my_mds = get_MDS(fd, offset);
  if (my_mds == NULL)
  {
    printf("Error get_MDS\n");
    return 0;
  }
  if (offset == 0)
  {
    printf("problem with offset\n");
    return 0;
  }
  struct tm *info;
  // printf("%lu\n", (my_mds->creation_time));
  info = localtime(&(my_mds->creation_time));
  // printf("creation time: %s", asctime(info));

  printf("c:%d/%d/%d-%d:%d:%d ", info->tm_mday, info->tm_mon + 1, info->tm_year + 1900, info->tm_hour, info->tm_min, info->tm_sec);
  // printf("last access time: %s", asctime(info));
  info = localtime(&(my_mds->access_time));
  printf("a:%d/%d/%d-%d:%d:%d ", info->tm_mday, info->tm_mon + 1, info->tm_year + 1900, info->tm_hour, info->tm_min, info->tm_sec);
  info = localtime(&(my_mds->modification_time));
  printf("m:%d/%d/%d-%d:%d:%d ", info->tm_mday, info->tm_mon + 1, info->tm_year + 1900, info->tm_hour, info->tm_min, info->tm_sec);  
  // printf("modification time: %s", asctime(info));
  printf("%lu ", my_mds->size);

  free(my_mds);
  return 1;
}


// int get_size_of_directory(int fd, off_t offset)
// {
//   int size = 0;
//   superblock* my_superblock = get_superblock(fd);
//   MDS* my_mds = get_MDS(fd, offset);
//   Block* my_block = get_Block(fd, my_superblock->block_size, my_mds->first_block);

//   char* ret_name = (char *) my_block->data; //.
//   off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);



//   free(my_block);
//   free(my_mds);
//   free(my_superblock);

// }



int cfs_ls(int fd, off_t offset, int flag_a, int flag_r, int flag_l, int flag_u, int flag_d, int flag_h)
{
  superblock* my_superblock = get_superblock(fd);
  MDS* my_mds = get_MDS(fd, offset);
  Block* my_block = get_Block(fd, my_superblock->block_size, my_mds->first_block);

  size_t size_of_struct_variables = sizeof(Block);
  size_t size_for_pairs = my_superblock->block_size - size_of_struct_variables;
  size_t size_of_pair = my_superblock->filename_size + sizeof(off_t);
  size_t pairs_in_block = my_block->bytes_used / size_of_pair;
  size_t max_pairs = size_for_pairs / size_of_pair;
  if (max_pairs);

  char* ret_name = (char *) my_block->data; //.
  off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
  if (flag_a)
  {
    if (flag_l)
    {
      print_characteristics(fd, *ret_offset);
      printf("\033[1;34m");
      printf("%s \n", ret_name);
    }
    else
    {
      printf("\033[1;34m");
      printf("%s \n", ret_name);
    }    
  }
  printf("\033[0m");

  // printf("offset for name: %lu\n", *ret_offset);
  ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size); //..
  ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
  if (flag_a)
  {
    if (flag_l)
    {
      print_characteristics(fd, *ret_offset);
      printf("\033[1;34m");
      printf("%s \n", ret_name);
      printf("\033[0m");
    }
    else
    {
      printf("\033[1;34m");
      printf("%s \n", ret_name);
      printf("\033[0m");
    }    
  }
  // printf("\033[0m");

  int i = 2;
  for (; i < pairs_in_block; i++)
  {
    ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
    ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
    if (flag_d) //print only dirs
    {
      if (get_type(fd, *ret_offset) == 1)
      {
        if (flag_l)
        {
          print_characteristics(fd, *ret_offset);
          printf("\033[1;34m");
          printf("%s \n", ret_name);
          printf("\033[0m");
          if (flag_r)
          {
            cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
          }
        }
        else
        {
          printf("\033[1;34m");
          printf("%s \n", ret_name);
          printf("\033[0m");
          if (flag_r)
          {
            cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
          }
        }
      }
      // printf("%s\n", ret_offset);
      // printf("\033[0m");
    }
    else
    {
      if (get_type(fd, *ret_offset) == 1) //directory
      {
        if (flag_l)
        {
          print_characteristics(fd, *ret_offset);
          printf("\033[1;34m");
          printf("%s \n", ret_name);
          printf("\033[0m");
          if (flag_r)
          {
            cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
          }
        }
        else
        {
          printf("\033[1;34m");
          printf("%s \n", ret_name);
          printf("\033[0m");
          if (flag_r)
          {
            cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
          }
        }
        // printf("\033[0m");
      }
      else //file
      {
        if (flag_l)
        {
          print_characteristics(fd, *ret_offset);
          printf("%s \n", ret_name);
        }
        else
        {
          printf("%s \n", ret_name);
        } 
      }
      // printf("%s\n", ret_offset);
    }

  }

  /*if there is more than 1 blocks for data*/  
  while (my_block->next_block != 0)
  {
    Block* temp_block = my_block;
    my_block = get_Block(fd, my_superblock->block_size, my_block->next_block);
    free(temp_block);

    /*first entity*/
    char* ret_name = (char *) my_block->data; 
    off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
    if (flag_d) //print only dirs
    {
      if (get_type(fd, *ret_offset) == 1)
      {
        if (flag_l)
        {
          print_characteristics(fd, *ret_offset);
          printf("\033[1;34m");
          printf("%s \n", ret_name);
          printf("\033[0m");
          if (flag_r)
          {
            cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
          }
        }
        else
        {
          printf("\033[1;34m");
          printf("%s \n", ret_name);
          printf("\033[0m");
          if (flag_r) //xwris flag_l
          {
            cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
          }
        }
      }
      // printf("%s\n", ret_offset);
      // printf("\033[0m");
    }
    else //print all
    {
      if (get_type(fd, *ret_offset) == 1) //directory
      {
        if (flag_l)
        {
          print_characteristics(fd, *ret_offset);
          printf("\033[1;34m");
          printf("%s \n", ret_name);
          printf("\033[0m");
          if (flag_r)
          {
            cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
          }
        }
        else
        {
          printf("\033[1;34m");
          printf("%s \n", ret_name);
          printf("\033[0m");
          if (flag_r)//xwris flag_l
          {
            cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
          }
        }
        // printf("\033[0m");
      }
      else //file
      {
        if (flag_l)
        {
          print_characteristics(fd, *ret_offset);
          printf("%s \n", ret_name);
        }
        else
        {
          printf("%s \n", ret_name);
        } 
      }
      // printf("%s\n", ret_offset);
    }

    /*entities 2 and above*/
    size_t pairs_in_block = my_block->bytes_used / size_of_pair;
    int i = 1;
    for (; i < pairs_in_block; i++)
    {
      ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
      ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
      if (flag_d) //print only dirs
      {
        if (get_type(fd, *ret_offset) == 1)
        {
          if (flag_l)
          {
            print_characteristics(fd, *ret_offset);
            printf("\033[1;34m");
            printf("%s \n", ret_name);
            printf("\033[0m");
            if (flag_r)
            {
              cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
            }
          }
          else
          {
            printf("\033[1;34m");
            printf("%s \n", ret_name);
            printf("\033[0m");
            if (flag_r) //xwris flag_l
            {
              cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
            }
          }
        }
        // printf("%s\n", ret_offset);
        // printf("\033[0m");
      }
      else //print all
      {
        if (get_type(fd, *ret_offset) == 1) //directory
        {
          if (flag_l)
          {
            print_characteristics(fd, *ret_offset);
            printf("\033[1;34m");
            printf("%s \n", ret_name);
            printf("\033[0m");
            if (flag_r)
            {
              cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
            }
          }
          else
          {
            printf("\033[1;34m");
            printf("%s \n", ret_name);
            printf("\033[0m");
            if (flag_r)//xwris_flag_l
            {
              cfs_ls(fd, *ret_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
            }
          }
          // printf("\033[0m");
        }
        else //file
        {
          if (flag_l)
          {
            print_characteristics(fd, *ret_offset);
            printf("%s \n", ret_name);
          }
          else
          {
            printf("%s \n", ret_name);
          } 
        }
        // printf("%s\n", ret_offset);
      }

    }
  }

  // printf("\n");

  free(my_block);
  free(my_mds);
  free(my_superblock);

  return 0;
}

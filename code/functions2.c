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

int get_size_of_directory(int fd, off_t offset)
{
  int size = 0;
  superblock* my_superblock = get_superblock(fd);
  MDS* my_mds = get_MDS(fd, offset);
  Block* my_block = get_Block(fd, my_superblock->block_size, my_mds->first_block);


  // size_t size_of_struct_variables = sizeof(Block);
  // size_t size_for_pairs = my_superblock->block_size - size_of_struct_variables;
  size_t size_of_pair = my_superblock->filename_size + sizeof(off_t);
  size_t pairs_in_block = my_block->bytes_used / size_of_pair;
  // size_t max_pairs = size_for_pairs / size_of_pair;
  // if (max_pairs);


  char* ret_name = (char *) my_block->data; //.
  off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);

  ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size); //..
  ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);

  int i = 2;
  for (; i < pairs_in_block; i++)
  {
    ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
    ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);

    MDS* my_mds = get_MDS(fd, *ret_offset);
    if (my_mds->type == 2) //file 
    {
      size += my_mds->size;
    }
    else if (my_mds->type == 1) //directory 
    {
      size = size + get_size_of_directory(fd, *ret_offset);
    }

    free(my_mds);
  }


  while (my_block->next_block != 0)
  {
    Block* temp_block = my_block;
    my_block = get_Block(fd, my_superblock->block_size, my_block->next_block);
    free(temp_block);

    /*first entity*/
    char* ret_name = (char *) my_block->data; 
    off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
    MDS* my_mds = get_MDS(fd, *ret_offset);
    if (my_mds->type == 2) //file 
    {
      size += my_mds->size;
    }
    else if (my_mds->type == 1) //directory 
    {
      size = size + get_size_of_directory(fd, *ret_offset);
    }

    free(my_mds);

    /*entities 2 and above*/
    size_t pairs_in_block = my_block->bytes_used / size_of_pair;
    int i = 1;
    for (; i < pairs_in_block; i++)
    {
      ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
      ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
      MDS* my_mds = get_MDS(fd, *ret_offset);
      if (my_mds->type == 2) //file 
      {
        size += my_mds->size;
      }
      else if (my_mds->type == 1) //directory 
      {
        size = size + get_size_of_directory(fd, *ret_offset);
      }
      free(my_mds);
    }

  }  


  free(my_block);
  free(my_mds);
  free(my_superblock);
  return size;
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
  if (my_mds->type == 2) //file
  {
    printf("%lu ", my_mds->size);  
  }
  else if (my_mds->type == 1) //directory
  {   
    printf("%d ", get_size_of_directory(fd, offset));
  }

  free(my_mds);
  return 1;
}


int comparator(const void *p, const void *q) 
{ 
  return strcmp(((pair *)p)->name, ((pair*)q)->name); //-
} 



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

  if (flag_u)
  {
    /* code */
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

  } 
  else //!flag_u
  {
    uint size_of_table = 0;
    size_of_table = number_of_sub_entities_in_directory(my_mds, my_superblock->filename_size);
    // printf("size_of_table = %d\n", size_of_table);
    pair* table = malloc(sizeof(pair) * size_of_table);
    memset(table, 0, sizeof(pair) * size_of_table);

    /*first entity*/ 
    char* ret_name = (char *) my_block->data; 
    off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
    int j = 0;
    // table[j].name = (char*) malloc(strlen(ret_name) * sizeof(char) + 1);
    table[j].name = (char*) malloc(my_superblock->filename_size * sizeof(char));

    // printf("%p\n", table[j].name);
    strcpy(table[j].name, ret_name);
    table[j].offset = *ret_offset;
    j++;

    /*entities 2 and above*/
    size_t size_of_pair = my_superblock->filename_size + sizeof(off_t);
    size_t pairs_in_block = my_block->bytes_used / size_of_pair;
    int i = 1;
    for (; i < pairs_in_block; i++)
    {
      ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
      ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);

      table[j].name = malloc(my_superblock->filename_size * sizeof(char));

      strcpy(table[j].name, ret_name);
      table[j].offset = *ret_offset;
      j++;  
    } 

    /*if there are more than 1 blocks*/ 
    while (my_block->next_block != 0)
    {
      Block* temp_block = my_block;
      my_block = get_Block(fd, my_superblock->block_size, my_block->next_block);
      free(temp_block);

      /*first entity*/
      char* ret_name = (char *) my_block->data; 
      off_t* ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
      table[j].name = malloc(my_superblock->filename_size * sizeof(char));

      strcpy(table[j].name, ret_name);
      table[j].offset = *ret_offset;
      j++;

      /*entities 2 and above*/
      size_t pairs_in_block = my_block->bytes_used / size_of_pair;
      int i = 1;
      for (; i < pairs_in_block; i++)
      {
        ret_name = pointer_to_next_name(ret_name, my_superblock->filename_size);
        ret_offset = pointer_to_offset(ret_name, my_superblock->filename_size);
        table[j].name = malloc(my_superblock->filename_size * sizeof(char));

        strcpy(table[j].name, ret_name);
        table[j].offset = *ret_offset;
        j++;
      }   
    } 

    // int k = 0; 
    // for (; k < size_of_table; k++) 
    //   printf("%s %lu\n", table[k].name, table[k].offset);

    qsort((void*)table, size_of_table, sizeof(pair), comparator); 
    // printf("QSORT\n");
    
    //////same as flag_u//////
    if (flag_a)
    {
      if (flag_l)
      {
        print_characteristics(fd, table[0].offset); //.
        printf("\033[1;34m");
        printf("%s \n", table[0].name);
        printf("\033[0m");
        print_characteristics(fd, table[1].offset); //..
        // printf("\033[1;34m");
        printf("\033[1;34m");
        printf("%s \n", table[1].name);
      }
      else
      {
        printf("\033[1;34m");
        printf("%s \n", table[0].name); //.
        // printf("\033[1;34m");
        printf("%s \n", table[1].name); //..
      }    
    }
    printf("\033[0m");

    /*all other entitites expect /. /.. */ 
    int p = 2;
    for (; p < size_of_table; p++)
    {
      if (flag_d) //print only dirs
        {
          if (get_type(fd, table[p].offset) == 1)
          {
            if (flag_l)
            {
              print_characteristics(fd, table[p].offset);
              printf("\033[1;34m");
              printf("%s \n", table[p].name);
              printf("\033[0m");
              if (flag_r)
              {
                cfs_ls(fd, table[p].offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
              }
            }
            else
            {
              printf("\033[1;34m");
              printf("%s \n", table[p].name);
              printf("\033[0m");
              if (flag_r) //xwris flag_l
              {
                cfs_ls(fd, table[p].offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
              }
            }
          }
          // printf("%s\n", ret_offset);
          // printf("\033[0m");
        }
        else //print all
        {
          if (get_type(fd, table[p].offset) == 1) //directory
          {
            if (flag_l)
            {
              print_characteristics(fd, table[p].offset);
              printf("\033[1;34m");
              printf("%s \n", table[p].name);
              printf("\033[0m");
              if (flag_r)
              {
                cfs_ls(fd, table[p].offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
              }
            }
            else
            {
              printf("\033[1;34m");
              printf("%s \n", table[p].name);
              printf("\033[0m");
              if (flag_r)//xwris_flag_l
              {
                cfs_ls(fd, table[p].offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
              }
            }
            // printf("\033[0m");
          }
          else //file
          {
            if (flag_l)
            {
              print_characteristics(fd, table[p].offset);
              printf("%s \n", table[p].name);
            }
            else
            {
              printf("%s \n", table[p].name);
            } 
          }
          // printf("%s\n", ret_offset);
        }
    }
    ///////////

    // k = 0; 
    // for (; k < size_of_table; k++) 
    //   printf("%s %lu\n", table[k].name, table[k].offset);
  
    // printf("\n");
    int k = 0; 
    for (; k < size_of_table; k++){
      free(table[k].name);
    } 
    free(table);
  }

  free(my_block);
  free(my_mds);
  free(my_superblock);

  return 0;
}





int insert_hole(hole_map* holes, off_t my_start, off_t my_end, int fd)
{
  if (holes->current_hole_number == MAX_HOLES)
  {
    // printf("MAX HOLES\n");
    return 0;
  }

  if (my_start < holes->holes_table[0].start && my_end < holes->holes_table[0].start) //new hole at position 0
  {
    // printf("new hole at position 0\n");
    shift_holes_to_the_right(holes, 0 ); //hole position?
    holes->holes_table[0].start = my_start;
    holes->holes_table[0].end = my_end;
    holes->current_hole_number++;
    set_hole_map(holes, fd);
    return 1;
  }

  int i = 0;
  for (; i < holes->current_hole_number; i++)
  {    
    //i+1-> current hole number must be < MAX HOLES
    if (my_start > holes->holes_table[i].end && my_end < holes->holes_table[i+1].start) //new hole between holes
    {
      // printf("new hole between holes\n");
      shift_holes_to_the_right(holes, i+1); //hole position?
      holes->holes_table[i+1].start = my_start;
      holes->holes_table[i+1].end = my_end;
      holes->current_hole_number++;
      set_hole_map(holes, fd);
      return 1;
    }
    else if (holes->holes_table[i].end == my_start)
    {
      if (holes->holes_table[i+1].start == my_end) //merge holes
      {
        // printf("merge holes\n");
        holes->holes_table[i+1].start = holes->holes_table[i].start;
        shift_holes_to_the_left(holes, i); //i+1
        holes->current_hole_number--;
        set_hole_map(holes, fd);
        return 1;
      }
      else if (holes->holes_table[i+1].start > my_end) //extend hole
      {
        // printf("extend1\n");
        holes->holes_table[i].end = my_end;
        set_hole_map(holes, fd);
        return 1;
      }
      else
      {
        // printf("return 0 prwto\n");
        return 0;
      }
    }
    else if (holes->holes_table[i].start == my_end) // extend hole
    {
      // printf("extend2\n");
      holes->holes_table[i].start = my_start;
      set_hole_map(holes, fd);
      return 1;
    }


  }//for
  // printf("den bike pouthena sad\n");
  return 0;
}
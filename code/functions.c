#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "util.h"


int cfs_create(char* cfs_filename, uint bs, uint fns, uint cfs, uint mdfn)
{
  // check error already exists file cfs

  int fd = open(cfs_filename, O_CREAT | O_RDWR | O_APPEND , PERMS);

  if (fd == -1)
  {
    // error
  }





  size_t root_header_size = sizeof(MDS) + fns;
  MDS* root_header = malloc(root_header_size);


  root_header->size = root_header_size;
  root_header->type = DIRECTORY;
  root_header->parent_offset = 0;

  time_t my_time = time(NULL);
  root_header->creation_time = my_time;
  root_header->access_time = my_time;
  root_header->modification_time = my_time;

  root_header->blocks_using = 0;
  root_header->first_block = 0;

  strcpy(root_header->name, "/root");





  size_t superblock_size = sizeof(superblock);
  superblock* my_superblock = malloc(superblock_size);

  my_superblock->fd = fd;
  printf("fd = %d\n", fd);

  my_superblock->root_directory = superblock_size + sizeof(hole_table);
  my_superblock->current_size = superblock_size + sizeof(hole_table) + root_header_size;

  my_superblock->current_hole_number = 1;

  my_superblock->block_size = bs;
  my_superblock->filename_size = fns;
  my_superblock->max_file_size = cfs;
  my_superblock->max_dir_file_number = mdfn;
  strcpy(my_superblock->cfs_filename, cfs_filename);





  hole_table* holes = malloc(sizeof(hole_table));
  initialize_holes(holes, MAX_HOLES);

  holes->holes[0].start = superblock_size + sizeof(hole_table) + root_header_size;











  ssize_t retval = write(fd, my_superblock, superblock_size);
  // printf("write wrote %d bytes\n", retval);
  // check return retval

  // lseek(fd, 0, SEEK_END);

  retval = write(fd, holes, sizeof(hole_table));

  // lseek(fd, 0, SEEK_END);

  retval = write(fd, root_header, root_header_size);
  // printf("retval = %d\n", retval);


  free(root_header);
  free(my_superblock);
  free(holes);

  // close(fd);
  return fd;
}





int cfs_read(int fd)
{
  superblock* my_superblock = malloc(sizeof(superblock));
  hole_table* holes = malloc(sizeof(hole_table));
  MDS* my_root = malloc(94);



  lseek(fd, 0, SEEK_SET);

  int retval = read(fd, my_superblock, sizeof(superblock));

  retval = read(fd, holes, sizeof(hole_table));

  retval = read(fd, my_root, 94);


  print_superblock(my_superblock);
  print_hole_table(holes);
  print_MDS(my_root);


  close(fd);

  return fd;
}

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "functions.h"


int cfs_create(char* cfs_filename, uint bs, uint fns, uint cfs, uint mdfn)
{
  // check error already exists file cfs

  int fd = open(cfs_filename, O_CREAT | O_RDWR , PERMS);

  if (fd == -1)
  {
    // error
  }

  size_t superblock_size = sizeof(superblock) + strlen(cfs_filename) + 1;
  superblock* my_superblock = malloc(superblock_size);

  my_superblock->fd = fd;
  printf("fd = %d\n", fd);

  my_superblock->root_directory = 0;
  my_superblock->current_size = 0;

  my_superblock->current_hole_number = 1;

  my_superblock->block_size = bs;
  my_superblock->filename_size = fns;
  my_superblock->max_file_size = cfs;
  my_superblock->max_dir_file_number = mdfn;
  strcpy(my_superblock->cfs_filename, cfs_filename);

  ssize_t retval = write(fd, my_superblock, superblock_size);

  close(fd);

}

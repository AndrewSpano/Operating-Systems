#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "functions.h"
#include "functions2.h"
#include "util.h"
#include "list.h"



int main(int argc, char* argv[])
{
  size_t bs, fns, cfs;
  uint mdfn;
  char* cfs_filename = NULL;

  char* buffer = "cfs_create my_file.cfs";
  get_cfs_create_parameters(buffer, &bs, &fns, &cfs, &mdfn, &cfs_filename);


  superblock* my_superblock = NULL;
  hole_map* holes = NULL;
  Stack_List* list = NULL;

  int retval = cfs_create(cfs_filename, bs, fns, cfs, mdfn);
  int fd = cfs_workwith(cfs_filename, &my_superblock, &holes, &list);
  cfs_read(cfs_filename, fd);

  if (retval);

  printf("\n\n");

  /* MAY CAUSE SEGMENTATION ERROR */
  FREE_IF_NOT_NULL(my_superblock);
  FREE_IF_NOT_NULL(holes);
  FREE_IF_NOT_NULL(cfs_filename);
  Stack_List_Destroy(&list);

  close(fd);
  return EXIT_SUCCESS;
}

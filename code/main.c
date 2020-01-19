#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "functions.h"
#include "functions_util.h"
#include "functions2.h"
#include "util.h"
#include "list.h"



int main(int argc, char* argv[])
{

  hole_map* hole = malloc(sizeof(hole_map));
  initialize_holes(hole, MAX_HOLES, 1, 10);

  print_hole_table(hole);

  hole->holes_table[0].start = 10;
  hole->holes_table[0].end = 15;

  hole->holes_table[1].start = 20;
  hole->holes_table[1].end = 30;

  hole->holes_table[2].start = 40;
  hole->holes_table[2].end = 50;

  hole->holes_table[3].start = 60;
  hole->holes_table[3].end = 70;

  hole->holes_table[4].start = 80;
  hole->holes_table[4].end = 90;

  hole->holes_table[5].start = 200;
  hole->holes_table[5].end = 1000;

  hole->holes_table[6].start = 2000;
  hole->holes_table[6].end = 3000;

  hole->holes_table[7].start = 5000;
  hole->holes_table[7].end = 0;

  hole->current_hole_number = 8;

  print_hole_table(hole);

  shift_holes_to_the_right(hole, 4);

  print_hole_table(hole);


  printf("NOWWWW\n");

  hole->holes_table[4].start = 75;
  hole->holes_table[4].end = 85;

  print_hole_table(hole);


  shift_holes_to_the_left(hole, 4);

  print_hole_table(hole);


  free(hole);

  printf("\n");
  return 1;





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

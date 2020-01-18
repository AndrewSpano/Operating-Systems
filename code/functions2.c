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
int find_hole(char* cfs_filename, int fd, size_t my_size)
{

  fd = open(cfs_filename, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);

  // superblock* my_superblock = malloc(sizeof(superblock));
  hole_map* holes = malloc(sizeof(hole_map));
  // MDS* my_root = malloc(sizeof(MDS));



  lseek(fd, sizeof(superblock), SEEK_SET); 

  // int retval = read(fd, my_superblock, sizeof(superblock));

  int retval = read(fd, holes, sizeof(hole_map));

  print_hole_table(holes);

  // retval = read(fd, my_root, sizeof(MDS));

  printf("i need to find a spot for %lu size \n", my_size);
  int i = 0;
  for (; i < MAX_HOLES; i++)
  {
    if (holes->holes_table[i].end == 0)
    {
      printf("start = %lu, end = %lu\n", holes->holes_table[i].start, holes->holes_table[i].end);
      //end of current files
      printf("wanted offset %lu\n", holes->holes_table[i].start);
      return holes->holes_table[i].start;
    }
    printf("start = %lu, end = %lu\n", holes->holes_table[i].start, holes->holes_table[i].end);
    size_t available_size = holes->holes_table[i].end - holes->holes_table[i].start;
    printf("available_size = %lu\n", available_size);
    if (available_size == my_size) //fits exactly
    {
      holes->holes_table[i].start = holes->holes_table[i].end = 0;
      return holes->holes_table[i].start;
    } else if (available_size > my_size) //leaves a hole
    {
      holes->holes_table[i].start += my_size; // update hole_table
      return holes->holes_table[i].start;
    }
  }


  // free(my_root);
  // free(my_superblock);
  free(holes);

  return 0;

}

int cfs_mkdir(char * name, uint bs, uint fns, uint cfs, uint mdfn, int fd)
{
  //create mds 

  // /* create the struct */
  // MDS* mds = NULL;
  // MALLOC_OR_DIE(mds, sizeof(MDS), fd);

  // /* initialize its values */
  // initialize_MDS(mds, 3, DIRECTORY, 1, 1, sizeof(MDS) + bs, superblock_size + hole_map_size, superblock_size + hole_map_size + sizeof(MDS));

  // /* write to the cfs file */
  // WRITE_OR_DIE(fd, mds, sizeof(MDS));

  //evresi thesis sto hole table


  //enimerosi superblock
  return 0;
} 





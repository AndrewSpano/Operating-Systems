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
off_t find_hole(char* cfs_filename, int fd, size_t my_size)
{

  fd = open(cfs_filename, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);

  hole_map* holes = NULL;
  MALLOC_OR_DIE(holes, sizeof(hole_map), fd);

  holes = get_hole_map(fd);

  print_hole_table(holes);

  printf("i need to find a spot for %lu size \n", my_size);
  off_t offset_to_return = 0;
  int i = 0;
  for (; i < MAX_HOLES; i++)
  {
    if (holes->holes_table[i].start != 0 && holes->holes_table[i].end == 0)
    {
      // printf("start = %lu, end = %lu\n", holes->holes_table[i].start, holes->holes_table[i].end);
      //end of current files
      printf("wanted offset %lu\n", holes->holes_table[i].start);
      offset_to_return = holes->holes_table[i].start;
      holes->holes_table[i].start += my_size;
      // print_hole_table(holes);
      return offset_to_return;
    }
    // printf("start = %lu, end = %lu\n", holes->holes_table[i].start, holes->holes_table[i].end);
    off_t available_size = holes->holes_table[i].end - holes->holes_table[i].start;
    printf("available_size = %lu\n", available_size);

    if (available_size == my_size) //fits exactly
    {
      offset_to_return = holes->holes_table[i].start;
      holes->holes_table[i].start = holes->holes_table[i].end = 0; //clear hole
      free(holes);
      return offset_to_return;
    } else if (available_size > my_size) //leaves a hole
    {
      offset_to_return = holes->holes_table[i].start;
      holes->holes_table[i].start += my_size; // shrink hole
      free(holes);
      return offset_to_return;
    }
  }


  free(holes);

  return 0;

}

int cfs_mkdir(char * name, uint bs, uint fns, uint cfs, uint mdfn, int fd)
{
  //find holes for mds and data block 
  // off_t offset_for_mds = find_hole(name, fd, sizeof(MDS));
  // off_t offset_for_db = find_hole(name, fd, bs);

  // //initialize data block
  // Block* db = NULL;
  // MALLOC_OR_DIE(root_data, block_size, fd);
  // /* initialize its values */
  // void initialize_Directory_Data_Block(Block* block, size_t fns, off_t self_offset, off_t parent_offset);
  // initialize_Directory_Data_Block(db, fns, /*self_offset*/, /*parent_offset*/);
  // /* write to the cfs file */
  // WRITE_OR_DIE(fd, root_data, block_size);

  // //initialize mds
  // /* create the struct */
  // MDS* mds = NULL;
  // MALLOC_OR_DIE(mds, sizeof(MDS), fd);
  // /* initialize its values */
  // void initialize_MDS(MDS* mds, uint id, uint type, uint number_of_hard_links, uint blocks_using, size_t size, off_t parent_offset, off_t first_block);
  // initialize_MDS(mds, /*3*/, DIRECTORY, 1, 1, mds_size + bs, /*parent offset from list*/, offset_for_db);
  // /* write to the cfs file */
  // WRITE_OR_DIE(fd, mds, mds_size);




  //enimerosi parent directory

  //enimerosi superblock
  return 0;
} 





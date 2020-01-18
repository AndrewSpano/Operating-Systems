#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "functions.h"
#include "util.h"



int cfs_create(char* cfs_filename, size_t bs, size_t fns, size_t cfs, uint mdfn)
{

  int fd = open(cfs_filename, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);

  /* check is an error occured */
  if (fd < 0)
  {
    /* ERROR: the file already existed */
    if (errno == EEXIST)
    {
      perror("File already exists, give another name the for the cfs file.\n");
    }
    /* some other error occured */
    else
    {
      perror("Error occured");
    }

    return -1;
  }



  /* calculate the size of every struct */

  /* the size of the superblock is the size of its struct */
  size_t superblock_size = sizeof(superblock);

  /* the size of the hole map is the the size of its struct */
  size_t hole_map_size = sizeof(hole_map);

  /* the size of the /root is determined by the size of the structs MDS */
  size_t root_header_size = sizeof(MDS);

  /* the size of the data block used by the root directory */
  size_t block_size = bs;




  /* construct the superblock */

  /* create the struct */
  superblock* my_superblock = NULL;
  MALLOC_OR_DIE(my_superblock, superblock_size, fd);

  /* initialize its values */
  initialize_superblock(my_superblock, cfs_filename, fd, superblock_size + hole_map_size, superblock_size + hole_map_size + root_header_size + bs, bs, fns, cfs, mdfn);

  /* write to the cfs file */
  WRITE_OR_DIE(fd, my_superblock, superblock_size);





  /* construct the hole map */

  /* create the struct */
  hole_map* holes = NULL;
  MALLOC_OR_DIE(holes, hole_map_size, fd);

  /* initialize its values */
  initialize_holes(holes, MAX_HOLES, 1, superblock_size + hole_map_size + root_header_size + bs);

  /* write to the cfs file */
  WRITE_OR_DIE(fd, holes, hole_map_size);






  /* construct the header for the /root directory:
     the only directory that will have "infinite" space */

  /* create the struct */
  MDS* root_header = NULL;
  MALLOC_OR_DIE(root_header, root_header_size, fd);

  /* initialize its values */
  initialize_MDS(root_header, 2, DIRECTORY, 1, 1, root_header_size + bs, superblock_size + hole_map_size, superblock_size + hole_map_size + root_header_size);

  /* write to the cfs file */
  WRITE_OR_DIE(fd, root_header, root_header_size);





  /* construct the data block of the /root directory */

  /* create the struct */
  Block* root_data = NULL;
  MALLOC_OR_DIE(root_data, block_size, fd);

  /* initialize its values */
  initialize_Directory_Data_Block(root_data, fns, superblock_size + hole_map_size, superblock_size + hole_map_size);

  /* write to the cfs file */
  WRITE_OR_DIE(fd, root_data, block_size);



  /* free the allocated memory */
  free(root_data);
  free(root_header);
  free(my_superblock);
  free(holes);

  int ret = close(fd);
  if (ret == -1)
  {
    perror("close() error in cfs_create()");
    return -1;
  }

  return fd;
}



int cfs_touch(const char buffer[], int fd)
{
  // superblock* my_superblock = get_superblock(fd);
  //
  // size_t file_header_size = sizeof(MDS);
  //
  // /* create the struct */
  // MDS* file_header = NULL;
  // MALLOC_OR_DIE(file_header, file_header_size, fd);
  //
  // /* initialize its values */
  // initialize_MDS(file_header, 2, FILE, 1, 1, file_header_size + bs, superblock_size + hole_map_size, superblock_size + hole_map_size + file_header_size);
  //
  // /* write to the cfs file */
  // WRITE_OR_DIE(fd, file_header, file_header_size);
  //
  //
  //
  // free(file_header);
  // free(superblock);

  return 1;
}



int cfs_read(char* cfs_filename, int fd)
{
  fd = open(cfs_filename, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);

  superblock* my_superblock = get_superblock(fd);
  size_t bs = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;

  hole_map* holes = get_hole_map(fd);
  MDS* my_root = get_MDS(fd, sizeof(superblock) + sizeof(hole_map));
  Block* block = get_Block(fd, bs, sizeof(superblock) + sizeof(hole_map) + sizeof(MDS));

  print_superblock(my_superblock);
  print_hole_table(holes);
  print_MDS(my_root);
  print_Directory_Data_Block(block, fns);

  free(block);
  free(my_root);
  free(my_superblock);
  free(holes);


  close(fd);

  return fd;
}

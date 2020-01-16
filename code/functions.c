#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "functions.h"
#include "util.h"



int cfs_create(char* cfs_filename, uint bs, uint fns, uint cfs, uint mdfn)
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

  /* the size of the /root is determined by the size of the structs MDS, + the
     bytes that are needed to store its name */
  size_t root_header_size = sizeof(MDS);





  /* construct the superblock */

  /* create the struct */
  superblock* my_superblock = NULL;
  MALLOC_OR_DIE(my_superblock, superblock_size, fd);

  /* initialize its values */
  my_superblock->fd = fd;
  my_superblock->root_directory = superblock_size + hole_map_size;
  my_superblock->current_size = superblock_size + hole_map_size + root_header_size;
  my_superblock->block_size = bs;
  my_superblock->filename_size = fns;
  my_superblock->max_file_size = cfs;
  my_superblock->max_dir_file_number = mdfn;
  memset(my_superblock->cfs_filename, 0, MAX_CFS_FILENAME_SIZE);
  strcpy(my_superblock->cfs_filename, cfs_filename);

  /* write to the cfs file */
  WRITE_OR_DIE(fd, my_superblock, superblock_size);





  /* construct the hole map */

  /* create the struct */
  hole_map* holes = NULL;
  MALLOC_OR_DIE(holes, hole_map_size, fd);

  /* initialize its values */
  initialize_holes(holes, MAX_HOLES);
  holes->current_hole_number = 1;
  holes->holes_table[0].start = superblock_size + hole_map_size + root_header_size;

  /* write to the cfs file */
  WRITE_OR_DIE(fd, holes, hole_map_size);





  /* construct the header for the /root directory: the only directory that will
     have "infinite" space */

  /* create the struct */
  MDS* root_header = NULL;
  MALLOC_OR_DIE(root_header, root_header_size, fd);

  /* initialize its values */
  root_header->size = root_header_size;
  root_header->type = DIRECTORY;
  root_header->number_of_hard_links = 1;
  root_header->parent_offset = 0;

  time_t my_time = time(NULL);
  root_header->creation_time = my_time;
  root_header->access_time = my_time;
  root_header->modification_time = my_time;

  root_header->blocks_using = 0;
  root_header->first_block = 0;

  /* write to the cfs file */
  WRITE_OR_DIE(fd, root_header, root_header_size);





  /* free the allocated memory */
  free(root_header);
  free(my_superblock);
  free(holes);

  // int ret = close(fd);
  // if (ret == -1)
  // {
  //   perror("close() error in cfs_create()");
  //   return -1;
  // }

  return fd;
}





int cfs_read(int fd)
{
  superblock* my_superblock = malloc(sizeof(superblock));
  hole_map* holes = malloc(sizeof(hole_map));
  MDS* my_root = malloc(98);



  lseek(fd, 0, SEEK_SET);

  int retval = read(fd, my_superblock, sizeof(superblock));

  retval = read(fd, holes, sizeof(hole_map));

  retval = read(fd, my_root, 94);


  print_superblock(my_superblock);
  print_hole_table(holes);
  print_MDS(my_root);

  free(my_root);
  free(my_superblock);
  free(holes);

  close(fd);

  return fd;
}

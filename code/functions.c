#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "functions.h"
#include "util.h"



int cfs_create(char* cfs_filename, size_t bs, size_t fns, size_t cfs, uint mdfn)
{

  /* cfs directories have to able to at least contain the ./ and ../
     directories, therefore the data block size has to at least as big as the
     space that is needed to store the information for these 2 directories */
  if (bs < 2 * (fns + sizeof(off_t)))
  {
    printf("block size if too small: %lu, or filename_size is too big: %lu\n \
            A directory data block can't contain the basic information for the ./ and ../ directories.\n", bs, fns);
    return -1;
  }

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

  CLOSE_OR_DIE(fd);

  return 1;
}



int cfs_workwith(char* cfs_filename, Stack_List** list)
{
  /* open file just for reading to see if it exists */
  int fd = open(cfs_filename, O_RDONLY, READ_WRITE_USER_GROUP_PERMS);
  if (fd == -1)
  {
    if (errno == ENOENT)
    {
      printf("cfs file: %s does not exist.\n", cfs_filename);
    }
    else
    {
      perror("open() error in cfs_workwith()");
    }

    return -1;
  }

  /* close the file opened only for reading */
  CLOSE_OR_DIE(fd);


  /* now open it for both reading and writing, to work with it */
  fd = open(cfs_filename, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd == -1)
  {
    perror("open() error in correct opening of cfs_workwith()");
    return -1;
  }

  /* create the list that will be used to manage the paths */
  *list = create_List();
  /* check if the creation of the list fails */
  if (*list == NULL)
  {
    printf("Unexpected error in create_List().\n");
    CLOSE_OR_DIE(fd);

    return -1;
  }

  /* name of the root directory */
  char* root_name = NULL;
  MALLOC_OR_DIE(root_name, 5, fd);
  strcpy(root_name, "root");

  /* calculate the position of the root MDS */
  off_t root_position = sizeof(superblock) + sizeof(hole_map);

  /* push it into the stack list, while also checking for any failure */
  int ret = Stack_List_Push(*list, root_name, root_position);
  if (ret == -1)
  {
    /* pushing failed for some reason, so free() the allocated memory and exit */
    Stack_List_Destroy(list);
    perror("Unexpected error");
    CLOSE_OR_DIE(fd);

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

  return fd;
}

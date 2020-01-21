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
     space that is needed to store the information for these 2 directories error oc*/
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
      perror("File already exists, give another name for the cfs file.\n");
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
  initialize_MDS(root_header, 2, DIRECTORY, 1, 1, 2 * (fns + sizeof(off_t)), superblock_size + hole_map_size, superblock_size + hole_map_size + root_header_size);

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



int cfs_workwith(char* cfs_filename, superblock** my_superblock, hole_map** holes, Stack_List** list)
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



  /* get the superblock so that we don't have to read it from the file every
     time we use a cfs_function */
  *my_superblock = get_superblock(fd);
  if (*my_superblock == NULL)
  {
    perror("get_superblock() returned NULL in cfs_workwith()");
    CLOSE_OR_DIE(fd);

    return -1;
  }

  /* same as above, just for the holes map */
  *holes = get_hole_map(fd);
  if (*holes == NULL)
  {
    perror("get_hole_map() returned NULL in cfs_workwith()");
    free(my_superblock);
    CLOSE_OR_DIE(fd);

    return -1;
  }

  /* create the list that will be used to manage the paths */
  *list = create_List();
  /* check if the creation of the list fails */
  if (*list == NULL)
  {
    printf("Unexpected error in create_List().\n");
    free(my_superblock);
    free(holes);
    CLOSE_OR_DIE(fd);

    return -1;
  }

  /* name of the root directory */
  char* root_name = malloc(5 * sizeof(char));
  if (root_name == NULL)
  {
    printf("Unexpected malloc() error in create_List().\n");
    free(my_superblock);
    free(holes);
    Stack_List_Destroy(list);
    CLOSE_OR_DIE(fd);

    return -1;
  }
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



int cfs_mkdir(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, MDS* current_directory, char* insert_name)
{
  size_t block_size = my_superblock->block_size;

  off_t mds_position = find_hole(holes, sizeof(MDS));
  off_t block_position = find_hole(holes, block_size);

  if (mds_position == 0 || block_position == 0)
  {
    printf("No more holes are available. Make the hole maw bigger in the next cfs file you make\n");
    return 0;
  }

  // // find holes for mds and data block
  // off_t offset_for_mds = find_hole(holes, sizeof(MDS));
  // off_t offset_for_data_block = find_hole(holes, my_superblock->block_size);
  //
  // //initialize data block
  // Block* data_block = NULL;
  // MALLOC_OR_DIE(data_block, my_superblock->block_size, fd);
  // /* initialize its values */
  //
  // //get current directory offset
  // char * parent_name = malloc(sizeof(char) * my_superblock->filename_size);
  // off_t parent_offset;
  // Stack_List_Peek(list, &parent_name, &parent_offset);
  //
  //
  // // void initialize_Directory_Data_Block(Block* block, size_t fns, off_t self_offset, off_t parent_offset);
  // initialize_Directory_Data_Block(data_block, fns, offset_for_data_block, parent_offset);
  // // write to the cfs file
  // set_Block(data_block, fd, my_superblock->block_size, offset_for_data_block);
  //
  // //initialize mds
  // //get superblock for id
  //
  // /* create the struct */
  // MDS* mds = NULL;
  // MALLOC_OR_DIE(mds, sizeof(MDS), fd);
  // /* initialize its values */
  // // void initialize_MDS(MDS* mds, uint id, uint type, uint number_of_hard_links, uint blocks_using, size_t size, off_t parent_offset, off_t first_block);
  // initialize_MDS(mds, my_superblock->total_entities + 1, DIRECTORY, 1, 1, sizeof(MDS) + my_superblock->block_size, parent_offset, offset_for_data_block);
  // /* write to the cfs file */
  // set_MDS(mds, fd, offset_for_mds);
  //
  //
  // //enimerosi parent directory
  // off_t current_offset = parent_offset;
  // MDS* mds_cur = get_MDS(fd, parent_offset);
  // Block* last_block = get_Block(fd, my_superblock->block_size, mds_cur->first_block);
  // while (last_block->next_block != 0)
  // {
  //   last_block = get_Block(fd, my_superblock->block_size, last_block->next_block);
  // }
  //
  // if(directory_data_block_Is_Full(last_block, my_superblock->block_size, fns) == 0) //there is space in the block
  // {
  //   insert_pair(last_block, name, offset_for_mds, my_superblock->filename_size);
  // }
  // else //block is full
  // { //allocate new block
  //   off_t new_block_hole = find_hole(holes, my_superblock->block_size);
  //
  //   Block* new_block = NULL;
  //   new_block = malloc(my_superblock->block_size);
  //
  //   new_block->next_block = 0;
  //   new_block->bytes_used = fns + sizeof(off_t);
  //
  //   last_block->next_block = new_block_hole;
  //
  //   insert_pair(block, name, new_block_hole, my_superblock->filename_size);
  // }
  //
  //
  //
  //
  // //enimerosi superblock
  // my_superblock->total_entities += 1;
  // my_superblock->current_size += sizeof(MDS) + my_superblock->block_size; //parent's block had space for the new entry
  //
  return 0;
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

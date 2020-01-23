#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "functions.h"
#include "functions_util.h"
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
      printf("File already exists, give another name for the cfs file.\n");
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
  initialize_MDS(root_header, 0, DIRECTORY, 1, 1, 2 * (fns + sizeof(off_t)), superblock_size + hole_map_size, superblock_size + hole_map_size + root_header_size);

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



int cfs_mkdir(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, MDS* current_directory, off_t parent_offset, char* insert_name)
{
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;
  uint total_entities = my_superblock->total_entities;

  size_t size_of_mds = sizeof(MDS);
  off_t mds_position = find_hole(holes, size_of_mds);
  off_t block_position = find_hole(holes, block_size);

  if (mds_position == 0 || block_position == 0)
  {
    printf("No more holes are available. Make the hole map bigger in the next cfs file you make\n");
    return 0;
  }



  /* create the struct */
  MDS* new_mds = NULL;
  MALLOC_OR_DIE_3(new_mds, sizeof(MDS));
  /* initialize its values */
  initialize_MDS(new_mds, total_entities, DIRECTORY, 1, 1, 2 * (fns + sizeof(off_t)), parent_offset, block_position);

  /* write to the cfs file */
  int retval = set_MDS(new_mds, fd, mds_position);
  if (retval == 0)
  {
    perror("Error occured in set_MDS() when called from cfs_mkdir()");
    free(new_mds);

    return 0;
  }

  free(new_mds);


  /* initialize data block */
  Block* data_block = NULL;
  MALLOC_OR_DIE_3(data_block, block_size);
  /* initialize the new directory data block */
  initialize_Directory_Data_Block(data_block, fns, block_position, parent_offset);

  /* write the block in the cfs file */
  retval = set_Block(data_block, fd, block_size, block_position);
  if (retval == 0)
  {
    perror("Error occured in set_Block() when called from cfs_mkdir()");
    free(data_block);

    return 0;
  }

  free(data_block);

  /* insert the pair */
  retval = insert_pair(fd, holes, current_directory, insert_name, mds_position, block_size, fns);
  if (retval == 0)
  {
    printf("insert_pair() error when called from cfs_mkdir()\n");
    return 0;
  }
  /* if retval == 1, it means that we allocated a new block to place the pair <name, offset> */
  else if (retval == 1)
  {
    current_directory->blocks_using++;
    my_superblock->current_size += block_size;
  }


  /* calculate the attributes that will be added to the current directory */
  size_t size_of_pair = fns + sizeof(off_t);
  current_directory->size += size_of_pair;


  /* update the current_directory */
  retval = set_MDS(current_directory, fd, parent_offset);
  if (retval == 0)
  {
    perror("Error occured in set_MDS() when called from cfs_mkdir() before finishing the function");
    return 0;
  }


  /* update the superblock */
  my_superblock->total_entities += 1;
  /* the size of the new directory plus its first block */
  my_superblock->current_size += sizeof(MDS) + block_size;

  retval = set_superblock(my_superblock, fd);
  if (retval == 0)
  {
    perror("Error occured in set_superblock() when called from cfs_mkdir() before finishing the function");
    return 0;
  }

  return 1;
}



int cfs_touch(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, MDS* current_directory, off_t parent_offset, char* insert_name, off_t file_offset, int flag_a, int flag_m)
{

  /* first check if any flag is true, in order to perform its operation */
  if ((flag_a || flag_m) && file_offset != (off_t) -1)
  {

    /* get the file */
    MDS* target_file = get_MDS(fd, file_offset);
    if (target_file == NULL)
    {
      printf("get_MDS() error in cfs_touch().\n");
      return 0;
    }

    if (target_file->type != FILE)
    {
      printf("Error: entity with name %s is not a file.\n", insert_name);
      free(target_file);
      return -1;
    }

    time_t my_time = time(NULL);
    if (flag_a)
    {
      target_file->access_time = my_time;
    }
    else
    {
      target_file->modification_time = my_time;
    }

    /* update the file that was modified */
    int retval = set_MDS(target_file, fd, file_offset);
    if (retval == 0)
    {
      perror("Error occured in set_MDS()");
      FREE_IF_NOT_NULL(target_file);
      return 0;
    }

    free(target_file);
    return 1;
  }

  /* create new file */
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;
  uint total_entities = my_superblock->total_entities;

  size_t size_of_mds = sizeof(MDS);
  off_t mds_position = find_hole(holes, size_of_mds);

  if (mds_position == 0)
  {
    printf("No more holes are available. Make the hole map bigger in the next cfs file you make\n");
    return 0;
  }


  /* create the struct */
  MDS* new_mds = NULL;
  MALLOC_OR_DIE_3(new_mds, sizeof(MDS));
  /* initialize its values */
  initialize_MDS(new_mds, total_entities, FILE, 1, 0, 0, parent_offset, 0);

  /* write to the cfs file */
  int retval = set_MDS(new_mds, fd, mds_position);
  if (retval == 0)
  {
    perror("Error occured in set_MDS() when called from cfs_touch()");
    free(new_mds);

    return 0;
  }

  free(new_mds);


  /* insert the pair in the current directory */
  retval = insert_pair(fd, holes, current_directory, insert_name, mds_position, block_size, fns);
  if (retval == 0)
  {
    printf("insert_pair() error when called from cfs_touch()\n");
    return 0;
  }
  /* if retval == 1, it means that we allocated a new block to place the pair <name, offset> */
  else if (retval == 1)
  {
    current_directory->blocks_using++;
    my_superblock->current_size += block_size;
  }

  /* calculate the attributes that will be added to the current directory */
  size_t size_of_pair = fns + sizeof(off_t);
  current_directory->size += size_of_pair;


  /* update the current_directory */
  retval = set_MDS(current_directory, fd, parent_offset);
  if (retval == 0)
  {
    perror("Error occured in set_MDS() when called from cfs_touch() before finishing the function");
    return 0;
  }


  /* update the superblock */
  my_superblock->total_entities += 1;
  /* the size of the new file */
  my_superblock->current_size += sizeof(MDS);

  retval = set_superblock(my_superblock, fd);
  if (retval == 0)
  {
    perror("Error occured in set_superblock() when called from cfs_touch() before finishing the function");
    return 0;
  }

  return 1;
}



int cfs_cd(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, const char path[])
{
  /* this should never print */
  if (is_Empty(list))
  {
    printf("Bug: list is empty.\n");
    return -1;
  }

  /* if the command "cfs_cd" is given or the path is absolute, just go to the
     root directory */
  if (path[0] == 0 || path_is_absolute(path))
  {
    while (!is_in_Root(list))
    {
      Stack_List_Pop(list);
    }

    return 1;
  }


  size_t fns = my_superblock->filename_size;
  size_t block_size = my_superblock->block_size;
  /* temp variable to store the name of the current directory */
  char* temp_directory = NULL;
  MALLOC_OR_DIE_3(temp_directory, fns);


  int is_finished = 0;
  int path_index = 0;
  while (!is_finished)
  {
    /* while the character '/' is being read, ignore it */
    while (path[path_index] == '/')
    {
      path_index++;
    }

    /* if the string ends, break */
    if ((path[path_index] == 0) || (path[path_index] == '\n') || (path[path_index] == '\t'))
    {
      break;
    }

    int name_index = 0;
    /* get the next directory */
    while ((path[path_index] != 0) && (path[path_index] != '\n') && (path[path_index] != '\t') && (path[path_index] != '/'))
    {
      temp_directory[name_index] = path[path_index];
      name_index++;
      path_index++;

      if (path_index == MAX_BUFFER_SIZE)
      {
        printf("Error: the path exceeds the max number of availabe characters (%d) for a path. This should have been checked in main.\n", MAX_BUFFER_SIZE);
        free(temp_directory);
        return 0;
      }

      if (name_index == fns)
      {
        printf("The name of a directory is too big. The maximum number of characters that a name of an entity can have, is fns: %ld.\n", fns);
        free(temp_directory);
        return 0;
      }
    }



    /* get the info of the current directory that we are in */
    char* current_directory_name = malloc(fns * sizeof(char));
    if (current_directory_name == NULL)
    {
      perror("malloc() error");
      free(temp_directory);
      return 0;
    }
    off_t current_directory_offset = (off_t) 0;

    /* get the location of the MDS of the directory */
    Stack_List_Peek(list, &current_directory_name, &current_directory_offset);

    /* free the name of the current directory because we don't need it anymore */
    free(current_directory_name);

    /* get the current directory */
    MDS* current_directory = get_MDS(fd, current_directory_offset);
    if (current_directory == NULL)
    {
      printf("get_MDS() error in cfs_cd().\n");
      free(temp_directory);
      return 0;
    }



    off_t directory_offset = directory_get_offset(fd, current_directory, block_size, fns, temp_directory);
    if (directory_offset == (off_t) -1)
    {
      char wrong_directory[MAX_BUFFER_SIZE] = {0};
      memcpy(wrong_directory, path, path_index + 1);

      printf("Error: directory %s does not exist.\n", wrong_directory);
      free(current_directory);
      free(temp_directory);
      return 0;
    }
    else if (directory_offset == (off_t) 0)
    {
      printf("Unexpected directory_offset() error.\n");
      free(current_directory);
      free(temp_directory);
      return 0;
    }
    free(current_directory);


    char* address_for_list = malloc((strlen(temp_directory) + 1) * sizeof(char));
    if (address_for_list == NULL)
    {
      perror("malloc() error");
      free(temp_directory);
      return 0;
    }
    /* copy the name inside the address */
    strcpy(address_for_list, temp_directory);

    /* put the new pair inside the directory */
    Stack_List_Push(list, address_for_list, directory_offset);

    /* reset the memory for the temporary directory */
    memset(temp_directory, 0, fns);
  }


  free(temp_directory);
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

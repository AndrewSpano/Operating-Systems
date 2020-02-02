#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

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
  initialize_Directory_Data_Block(root_data, block_size, fns, superblock_size + hole_map_size, superblock_size + hole_map_size);

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



int cfs_mkdir(int fd, superblock* my_superblock, hole_map* holes, MDS* current_directory, off_t parent_offset, char* insert_name)
{
  /* get some important parameters */
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;
  uint total_entities = my_superblock->total_entities;

  /* calculate the size of the struct MDS */
  size_t size_of_mds = sizeof(MDS);
  /* find where to place the MDS and its first data block */
  off_t mds_position = find_hole(holes, size_of_mds);
  off_t block_position = find_hole(holes, block_size);

  /* if there are no more holes */
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

  /* free the new MDS because we don't need it anymore */
  free(new_mds);


  /* initialize data block */
  Block* data_block = NULL;
  MALLOC_OR_DIE_3(data_block, block_size);
  /* initialize the new directory data block */
  initialize_Directory_Data_Block(data_block, block_size, fns, mds_position, parent_offset);

  /* write the block in the cfs file */
  retval = set_Block(data_block, fd, block_size, block_position);
  if (retval == 0)
  {
    perror("Error occured in set_Block() when called from cfs_mkdir()");
    free(data_block);

    return 0;
  }

  /* free the data block because we don't need it anymore */
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


  /* inform the superblock */
  my_superblock->total_entities += 1;
  /* the size of the new directory plus its first block */
  my_superblock->current_size += sizeof(MDS) + block_size;

  /* update the superblock */
  retval = set_superblock(my_superblock, fd);
  if (retval == 0)
  {
    perror("Error occured in set_superblock() when called from cfs_mkdir() before finishing the function");
    return 0;
  }

  /* update the hole map */
  retval = set_hole_map(holes, fd);
  if (!retval)
  {
    return 0;
  }


  return 1;
}



int cfs_touch(int fd, superblock* my_superblock, hole_map* holes, MDS* current_directory, off_t parent_offset, char* insert_name, off_t file_offset, int flag_a, int flag_m)
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

  /* calculate some useful sizes and variables */
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;
  uint total_entities = my_superblock->total_entities;

  /* get info about the new file */
  size_t size_of_mds = sizeof(MDS);
  off_t mds_position = find_hole(holes, size_of_mds);

  /* if no hole is found */
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

  /* free up the used space */
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


  /* inform the superblock */
  my_superblock->total_entities += 1;
  my_superblock->current_size += sizeof(MDS);

  /* update the superblock */
  retval = set_superblock(my_superblock, fd);
  if (retval == 0)
  {
    perror("Error occured in set_superblock() when called from cfs_touch() before finishing the function");
    return 0;
  }

  /* update the hole map */
  retval = set_hole_map(holes, fd);
  if (!retval)
  {
    return 0;
  }


  /* return 1 if everything goes smoothly */
  return 1;
}



int cfs_pwd(int fd, superblock* my_superblock, Stack_List* list)
{
  Stack_List_Print_Path(list);
  return 1;
}



int cfs_cd(int fd, superblock* my_superblock, Stack_List* list, const char path[])
{
  /* this should never print */
  if (is_Empty(list))
  {
    printf("Bug: list is empty.\n");
    return 0;
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

    if (!strcmp(temp_directory, ".."))
    {
      if (!is_in_Root(list))
      {
        Stack_List_Pop(list);
      }
    }
    else if (!strcmp(temp_directory, "."))
    {
      /* do nothing */
    }
    else
    {

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
    }


    /* reset the memory for the temporary directory */
    memset(temp_directory, 0, fns);
  }


  free(temp_directory);
  return 1;
}



int cfs_cp(int fd, superblock* my_superblock, hole_map* holes, MDS* source, char* source_name, MDS* destination_directory, off_t destination_offset, int flag_R, int flag_i, int flag_r, char* source_path, char* destination_path)
{
  /* get some important sizes */
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;



  /* is the entity to be copied is a file */
  if (source->type == FILE)
  {

    MDS* new_file_to_be_copied = NULL;
    MALLOC_OR_DIE_3(new_file_to_be_copied, sizeof(MDS));

    /* find where to place the new file */
    off_t position_of_new_file = find_hole(holes, sizeof(MDS));

    /* find where to place the first block */
    off_t position_of_block = 0;
    if (source->size > 0)
    {
      /* find where to place the first block */
      position_of_block = find_hole(holes, block_size);
    }

    /* initialize the attribues of the copied file */
    initialize_MDS(new_file_to_be_copied, my_superblock->total_entities, FILE, 1, source->blocks_using, source->size, destination_offset, position_of_block);

    /* write the file header to the cfs */
    int retval = set_MDS(new_file_to_be_copied, fd, position_of_new_file);
    free(new_file_to_be_copied);
    if (!retval)
    {
      return 0;
    }


    /* if there are data blocks to be copied */
    if (source->size > 0)
    {
      off_t source_block_position = source->first_block;
      off_t next_block_position = position_of_block;

      /* copy all the data blocks one by one */
      while (69 * 0 == 0 * 69)
      {
        /* get the source block */
        Block* source_block = get_Block(fd, block_size, source_block_position);
        DIE_IF_NULL(source_block);

        /* get a block for the destination */
        Block* destination_block = malloc(block_size);
        if (destination_block == NULL)
        {
          free(source_block);
          return 0;
        }

        /* copy the data */
        memcpy(destination_block, source_block, block_size);
        /* if this is not the last data block, find where to place the next */
        if (source_block->next_block != 0)
        {
          /* where the next block will be placed */
          next_block_position = find_hole(holes, block_size);
          destination_block->next_block = next_block_position;
        }

        /* get the position of the next source block */
        source_block_position = source_block->next_block;
        /* free the current source block because we don't need it anymore */
        free(source_block);

        retval = set_Block(destination_block, fd, block_size, position_of_block);
        free(destination_block);
        if (!retval)
        {
          return 0;
        }

        /* break if all the blocks have been copied */
        if (source_block_position == 0)
        {
          break;
        }

        /* keep track of the position of the next block so that we know where
           to set it */
        position_of_block = next_block_position;
      }
    }



    /* insert pair <source_name, offset> to the destination directory */
    retval = insert_pair(fd, holes, destination_directory, source_name, position_of_new_file, block_size, fns);
    if (retval == 0)
    {
      return 0;
    }
    else if (retval == 1)
    {
      /* if insert_pair() returns 1, it means that we allocated a new block to insert the pair */
      my_superblock->current_size += block_size;
      destination_directory->blocks_using++;
    }


    /* inform the superblock */
    my_superblock->total_entities++;
    my_superblock->current_size += sizeof(MDS) + block_size * source->blocks_using;


  }
  /* if the entity to be copied is a directory */
  else if (source->type == DIRECTORY)
  {

    /* copy the directory */
    int retval = cfs_mkdir(fd, my_superblock, holes, destination_directory, destination_offset, source_name);
    if (!retval)
    {
      printf("Error in cfs_mkdir() when called from cfs_cp().\n");
      return 0;
    }

    /* get the position of the directory we just copied */
    off_t copy_directory_offset = directory_get_offset(fd, destination_directory, block_size, fns, source_name);
    /* check for errors */
    if (copy_directory_offset == (off_t) 0)
    {
      return 0;
    }
    else if (copy_directory_offset == (off_t) -1)
    {
      printf("Error in cfs_cp(). This message should have never been printed because the insertion above worked, therefore the entity must exist. Congratulations, ELOUSES.\n");
      return 0;
    }


    /* if we are to copy the directories recursively */
    if (flag_r)
    {
      /* initialize useful variables */
      size_t size_of_pair = fns + sizeof(off_t);

      /* get the copy directory */
      MDS* copy_directory = get_MDS(fd, copy_directory_offset);
      if (copy_directory == NULL)
      {
        return 0;
      }

      /* get the position of the data blocks of the source directory */
      off_t directory_data_block_position = source->first_block;
      /* iterate through all the directory data blocks */
      while (directory_data_block_position != (off_t) 0)
      {
        Block* directory_data_block = get_Block(fd, block_size, directory_data_block_position);
        if (directory_data_block == NULL)
        {
          free(copy_directory);
          return 0;
        }

        char* name = (char *) directory_data_block->data;

        uint number_of_pairs = directory_data_block->bytes_used / size_of_pair;
        int i = 0;
        for (; i < number_of_pairs; i++)
        {
          /* skip the hidden directories */
          if (!strcmp(name, ".") || !strcmp(name, ".."))
          {
            /* point to the next name */
            name = pointer_to_next_name(name, fns);
            continue;
          }

          /* fix the path for the source */
          char temp_source_buffer[MAX_BUFFER_SIZE] = {0};
          strcpy(temp_source_buffer, source_path);
          if (temp_source_buffer[strlen(temp_source_buffer) - 1] != '/')
          {
            strcat(temp_source_buffer, "/");
          }
          strcat(temp_source_buffer, name);

          /* fix the path for the destination */
          char temp_destination_buffer[MAX_BUFFER_SIZE] = {0};
          strcpy(temp_destination_buffer, destination_path);
          if (temp_destination_buffer[strlen(temp_destination_buffer) - 1] != '/')
          {
            strcat(temp_destination_buffer, "/");
          }
          strcat(temp_destination_buffer, source_name);

          /* ask for persmission if -i parameter has been given */
          if (flag_i)
          {
            if (!get_approval(temp_source_buffer, temp_destination_buffer, "copy"))
            {
              continue;
            }
          }

          /* find the new source that will be copied */
          off_t* entity_offset = pointer_to_offset(name, fns);
          /* get the new source that will be copied */
          MDS* new_source = get_MDS(fd, *entity_offset);
          if (new_source == NULL)
          {
            free(directory_data_block);
            free(copy_directory);
            return 0;
          }

          /* copy the entity recursively */
          retval = cfs_cp(fd, my_superblock, holes, new_source, name, copy_directory, copy_directory_offset, flag_R, flag_i, flag_r, temp_source_buffer, temp_destination_buffer);
          if (!retval)
          {
            free(directory_data_block);
            free(copy_directory);
            return 0;
          }

          /* free the allocated memory */
          free(new_source);

          /* point to the next name */
          name = pointer_to_next_name(name, fns);
        }

        /* get the position of the next directory data block */
        directory_data_block_position = directory_data_block->next_block;
        /* free the current block because we finished with it */
        free(directory_data_block);
      }

      /* free with the copy directory because we finished with it */
      /* update the destination directory */
      retval = set_MDS(copy_directory, fd, copy_directory_offset);
      free(copy_directory);
      if (!retval)
      {
        return 0;
      }
    }

  }



  /* update the superblock */
  int retval = set_superblock(my_superblock, fd);
  if (!retval)
  {
    return 0;
  }

  /* update the hole map */
  retval = set_hole_map(holes, fd);
  if (!retval)
  {
    return 0;
  }

  /* update the destination directory */
  retval = set_MDS(destination_directory, fd, destination_offset);
  if (!retval)
  {
    return 0;
  }

  return 1;
}



int cfs_cat(int fd, superblock* my_superblock, hole_map* holes, MDS* destination_file, off_t destination_file_offset, MDS* source_file)
{
  /* useful attributes */
  size_t block_size = my_superblock->block_size;

  /* how much data a block can contain */
  size_t bytes_for_data = block_size - sizeof(Block);

  /* counter used to update the different fields of the structs */
  uint number_of_new_blocks_created = 0;


  /* if this is the first time we write to this file */
  if (destination_file->blocks_using == 0)
  {
    /* allocate and initialize the new block */
    Block* new_block = NULL;
    MALLOC_OR_DIE_3(new_block, block_size);

    initialize_data_Block(new_block, block_size);

    /* find a spot for the new block */
    off_t block_position = find_hole(holes, block_size);

    /* write the block in the cfs file */
    int retval = set_Block(new_block, fd, block_size, block_position);
    if (!retval)
    {
      printf("set_Block() error.\n");
      free(new_block);
      return 0;
    }

    /* update the destination file */
    destination_file->first_block = block_position;

    free(new_block);
    number_of_new_blocks_created++;
  }

  /* get the position of the first block */
  off_t block_position = destination_file->first_block;
  Block* block = NULL;

  /* go to the last block in order to concatenate */
  while (6 + 9 != 69)
  {
    /* get the next block */
    block = get_Block(fd, block_size, block_position);
    DIE_IF_NULL(block);

    /* free when we reach the last block */
    if (block->next_block == 0)
    {
      break;
    }

    /* get the position of the next block */
    block_position = block->next_block;
    /* free up the used space to allocate the block */
    free(block);
  }


  /* check if the current block is full */
  if (block->bytes_used == bytes_for_data)
  {
    /* allocate and initialize the new block */
    Block* new_block = malloc(block_size);
    if (new_block == NULL)
    {
      free(block);
      perror("malloc() error");

      return 0;
    }
    new_block->next_block = 0;
    new_block->bytes_used = 0;
    memset(block->data, 0, bytes_for_data);

    off_t previous_block_position = block_position;
    block_position = find_hole(holes, block_size);
    block->next_block = block_position;

    /* update the block in the cfs file */
    int retval = set_Block(block, fd, block_size, previous_block_position);
    if (!retval)
    {
      printf("set_Block() error.\n");
      free(block);
      free(new_block);
      return 0;
    }

    free(block);
    block = new_block;
  }

  /* get the position of the first data block of the source file */
  off_t source_block_position = source_file->first_block;

  while (1 + 3 == 2 * 2)
  {
    /* get the block of the source file */
    Block* source_block = get_Block(fd, block_size, source_block_position);
    if (source_block == NULL)
    {
      free(block);
      return 0;
    }

    /* determine the sizes of the concatenation */
    size_t bytes_available_in_destination = bytes_for_data - block->bytes_used;
    size_t bytes_to_copy = bytes_available_in_destination;
    if (source_block->bytes_used < bytes_to_copy)
    {
      bytes_to_copy = source_block->bytes_used;
    }

    /* concatenate data */
    memcpy(block->data + block->bytes_used, source_block->data, bytes_to_copy);
    block->bytes_used += bytes_to_copy;

    /* if the below condition is true, then the concatenation is over */
    if (source_block->bytes_used <= bytes_to_copy && source_block->next_block == 0)
    {
      int retval = set_Block(block, fd, block_size, block_position);
      free(source_block);
      free(block);
      if (!retval)
      {
        return 0;
      }
      break;

    }

    /* concatenation is not over, so allocate a new block and copy the
       remainder of the source block */
    Block* new_block = malloc(block_size);
    if (new_block == NULL)
    {
      free(block);
      free(source_block);
      return 0;
    }
    initialize_data_Block(new_block, block_size);

    /* find a spot for the new block */
    off_t new_block_position = find_hole(holes, block_size);
    block->next_block = new_block_position;

    int retval = set_Block(block, fd, block_size, block_position);
    free(block);
    if (!retval)
    {
      free(new_block);
      free(source_block);
      return 0;
    }
    number_of_new_blocks_created++;

    /* update the new block */
    size_t bytes_left_in_source = source_block->bytes_used - bytes_to_copy;

    /* concatenate */
    memcpy(new_block->data, source_block->data + bytes_to_copy, bytes_left_in_source);
    new_block->bytes_used += bytes_left_in_source;

    /* get next source block */
    source_block_position = source_block->next_block;
    free(source_block);
    /* check if the concatenation has finished */
    if (source_block_position == 0)
    {
      int retval = set_Block(new_block, fd, block_size, new_block_position);
      free(new_block);
      if (!retval)
      {
        return 0;
      }
      break;
    }

    block = new_block;
  }


  /* update the structs */
  destination_file->blocks_using += number_of_new_blocks_created;
  destination_file->size += source_file->size;

  /* set the updates */
  int retval = set_MDS(destination_file, fd, destination_file_offset);
  if (!retval)
  {
    return 0;
  }

  return 1;
}



int cfs_ln(int fd, superblock* my_superblock, hole_map* holes, Stack_List* list, char source_file_path[], char output_file_path[])
{
  /* max dir file number */
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;
  uint mdfn = my_superblock->max_dir_file_number;


  /* get the offset of the entity that we want to create the hard link on */
  off_t source_offset = get_offset_from_path(fd, my_superblock, list, source_file_path);
  if (source_offset == (off_t) 0)
  {
    return -1;
  }



  /* get the source file */
  MDS* source = get_MDS(fd, source_offset);
  if (source == NULL)
  {
    return 0;
  }
  else if (source->type == DIRECTORY)
  {
    printf("Error input: hard links can't be created on directories.\n");
    free(source);
    return -1;
  }


  /* array used to store the name that the link will have */
  char name_of_link[MAX_BUFFER_SIZE] = {0};
  extract_last_entity_from_path(output_file_path, name_of_link);
  /* check that cfs limitations are met */
  if (strlen(name_of_link) > fns - 1)
  {
    printf("Error input: the output file name (name of the link) \"%s\" exceeds the max number of characters that a filename can have.\n", name_of_link);
    free(source);
    return -1;
  }


  /* get the offset of the directory that will contain the link */
  off_t output_directory_offset = get_offset_from_path(fd, my_superblock, list, output_file_path);
  if (output_directory_offset == (off_t) 0)
  {
    free(source);
    return 0;
  }


  /* get the destination directory */
  MDS* destination_directory = get_MDS(fd, output_directory_offset);
  /* check for errors */
  if (destination_directory == NULL)
  {
    free(source);
    return 0;
  }
  else if (destination_directory->type != DIRECTORY)
  {
    printf("Path given is wrong, the destination is not a directory. This should actually never print because get_offset() should catch the mistake.\n");
    free(source);
    free(destination_directory);
    return -1;
  }
  else if (!is_root_offset(my_superblock, output_directory_offset) && number_of_sub_entities_in_directory(destination_directory, fns) == mdfn)
  {
    printf("Destination directory for the output file has reached its max capacity, and therefore the hard link can't be added to it.\n");
    free(source);
    free(destination_directory);
    return -1;
  }

  /* again check for errors */
  int retval = name_exists_in_directory(fd, destination_directory, block_size, fns, name_of_link);
  if (retval == 0)
  {
    free(source);
    free(destination_directory);
    return 0;
  }
  else if (retval == 1)
  {
    if (output_file_path[0] != 0)
    {
      printf("Error input: the hard link \"%s\" can't be created in the directory \"%s\" because an entity with the same name already exists in that directory.\n", name_of_link, output_file_path);
    }
    else
    {
      printf("Error input: the hard link \"%s\" can't be created in the current directory because an entity with the same name already exists.\n", name_of_link);
    }
    free(source);
    free(destination_directory);
    return -1;
  }


  retval = insert_pair(fd, holes, destination_directory, name_of_link, source_offset, block_size, fns);
  if (retval == 0)
  {
    free(source);
    free(destination_directory);
    return 0;
  }
  else if (retval == 1)
  {
    /* if insert_pair() returned 1, it means that we allocated a new block to
       insert the pair, so inform the superblock and update the structs */
    my_superblock->current_size += block_size;

    /* update the superblock */
    retval = set_superblock(my_superblock, fd);
    if (retval == 0)
    {
      perror("Error occured in set_superblock() when called from cfs_ln() before finishing the function");
      free(source);
      free(destination_directory);
      return 0;
    }

    /* update the hole map */
    retval = set_hole_map(holes, fd);
    if (!retval)
    {
      perror("Error occured in set_hole_map() when called from cfs_ln() before finishing the function");
      free(source);
      free(destination_directory);
      return 0;
    }
  }


  /* increment the number of hard links */
  source->number_of_hard_links++;

  /* update the MDS in the cfs */
  retval = set_MDS(source, fd, source_offset);
  /* free up the allocated memory */
  free(source);
  free(destination_directory);

  /* last check for errors */
  if (!retval)
  {
    return 0;
  }


  /* return 1 if everything goes smoothly */
  return 1;
}



int cfs_import(int fd, superblock* my_superblock, hole_map* holes, MDS* destination_directory, off_t destination_offset, char* linux_path_name)
{
  /* get some important sizes */
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;
  size_t cfs = my_superblock->max_file_size;
  uint mdfn = my_superblock->max_dir_file_number;

  /* variable that will store file statistics */
  struct stat file_statistics = {0};
  /* get the file statistics */
  int retval = stat(linux_path_name, &file_statistics);
  /* check for errors */
  if (retval == -1)
  {
    if (errno == ENOENT)
    {
      printf("Error input: the given linux path \"%s\" does not exist.\n", linux_path_name);
      return 1;
    }
    else
    {
      perror("stat()");
      return 0;
    }
  }


  /* determine whether this entity to be imported is a file or a directory */
  switch (file_statistics.st_mode & S_IFMT)
  {

    /* if the entity is a directory */
    case S_IFDIR:
    {

      /* temp array used to store the path of the linux directory */
      char copy_linux_path_name[MAX_BUFFER_SIZE] = {0};
      strcpy(copy_linux_path_name, linux_path_name);
      /* array used to get the name if the linux directory */
      char linux_name[MAX_BUFFER_SIZE] = {0};
      extract_last_entity_from_path(copy_linux_path_name, linux_name);


      /* import the directory */
      int retval = cfs_mkdir(fd, my_superblock, holes, destination_directory, destination_offset, linux_name);
      if (!retval)
      {
        printf("Error in cfs_mkdir() when called from cfs_import().\n");
        return 0;
      }

      /* get the position of the directory we just imported */
      off_t imported_directory_offset = directory_get_offset(fd, destination_directory, block_size, fns, linux_name);
      /* check for errors */
      if (imported_directory_offset == (off_t) 0)
      {
        return 0;
      }
      else if (imported_directory_offset == (off_t) -1)
      {
        printf("Error in cfs_import(). This message should have never been printed because the insertion above worked, therefore the entity must exist. Congratulations, ELOUSES.\n");
        return 0;
      }

      /* get the imported directory */
      MDS* imported_directory = get_MDS(fd, imported_directory_offset);
      if (imported_directory == NULL)
      {
        return 0;
      }


      /* pointer to directory */
      DIR* linux_directory = NULL;
      /* pointer to struct dirent */
      struct dirent* entry = NULL;

      /* open the linux directory */
      linux_directory = opendir(linux_path_name);
      /* check for errors */
      if (linux_directory == NULL)
      {
        free(imported_directory);
        if (errno == EACCES)
        {
          printf("Error: the linux directory \"%s\" has no permission to access its contents.\n", linux_path_name);
          return 1;
        }
        else if (errno == ENOENT)
        {
          printf("Error: the linux directory \"%s\" does not exist, therefore it can't be imported.\n", linux_path_name);
          return 1;
        }

        perror("opendir() error in cfs_import()");
        return 0;
      }


      /* get the next entry of the directory */
      entry = readdir(linux_directory);

      /* while an entry exists */
      while (entry != NULL)
      {
        /* get the name of the sub-entities of the directory */
        char entry_name[MAX_BUFFER_SIZE] = {0};
        strcpy(entry_name, entry->d_name);
        /* contruct the path of the new entry from the current linux path */
        strcpy(copy_linux_path_name, linux_path_name);
        strcat(copy_linux_path_name, "/");
        strcat(copy_linux_path_name, entry_name);

        /* check for limitations of the cfs */
        if (strlen(entry_name) > fns - 1)
        {
          printf("The file \"%s\" has a name too big to fit in the cfs. The max characters for a file name is: fns = %ld\n", copy_linux_path_name, fns);
          /* get the next entry */
          entry = readdir(linux_directory);
          continue;
        }
        if (number_of_sub_entities_in_directory(imported_directory, fns) == mdfn)
        {
          printf("The cfs directory \"%s\" has reached its max capacity in sub-entities. No more files can be stored.\n", linux_name);
          break;
        }

        /* if the entry is not one of the directories "." and ".." */
        if (strcmp(entry_name, ".") && strcmp(entry_name, ".."))
        {
          /* import the sub-entities in the imported directory */
          retval = cfs_import(fd, my_superblock, holes, imported_directory, imported_directory_offset, copy_linux_path_name);
          /* check for errors */
          if (!retval)
          {
            free(imported_directory);
            retval = closedir(linux_directory);
            if (retval == -1)
            {
              perror("closedir() inside while loop in cfs_import()");
            }
            return 0;
          }
        }

        /* get the next entry */
        entry = readdir(linux_directory);
      }


      /* close the linux directory because we don't need it anymore */
      retval = closedir(linux_directory);
      /* check for errors */
      if (retval == -1)
      {
        free(imported_directory);
        perror("closedir() error in cfs_import()");
        return 0;
      }

      /* set the imported directory in the cfs */
      retval = set_MDS(imported_directory, fd, imported_directory_offset);
      /* free up the allocated space */
      free(imported_directory);
      /* check for errors */
      if (!retval)
      {
        return 0;
      }


      break;
    }

    /* if the entity if a file */
    case S_IFREG:
    {

      /* check the size of the file to be imported */
      size_t linux_file_size = file_statistics.st_size;
      if (linux_file_size > cfs)
      {
        printf("Linux file \"%s\" is too big to be imported in the cfs. Its size is %ld, and the max size that a file can have in the cfs is: %ld.\n", linux_path_name, linux_file_size, cfs);
        return 1;
      }


      /* open the linux FS file */
      int linux_file_fd = open(linux_path_name, O_RDONLY, READ_WRITE_USER_GROUP_PERMS);
      /* check for errors */
      if (linux_file_fd == -1)
      {
        if (errno == ENOENT)
        {
          printf("Error input: the linux file \"%s\" does not exist.\n", linux_path_name);
          return 1;
        }
        else
        {
          perror("open() error in cfs_import()");
          return 0;
        }
      }


      /* array of charact used to store the name of the file to be created */
      char linux_file_name[MAX_BUFFER_SIZE] = {0};
      extract_last_entity_from_path(linux_path_name, linux_file_name);


      /* create the file */
      retval = cfs_touch(fd, my_superblock, holes, destination_directory, destination_offset, linux_file_name,  (off_t) -1, 0, 0);
      if (!retval)
      {
        CLOSE_OR_DIE2(linux_file_fd);
        return 0;
      }


      /* get its offset */
      off_t imported_file_offset = directory_get_offset(fd, destination_directory, block_size, fns, linux_file_name);
      if (imported_file_offset == (off_t) 0)
      {
        CLOSE_OR_DIE2(linux_file_fd);
        return 0;
      }
      else if (imported_file_offset == (off_t) -1)
      {
        printf("Error, this message should have never printed, because you just a created a file, searched its offset and did not find it, in cfs_import (for files). Congratulations, ELOUSES.\n");
        CLOSE_OR_DIE2(linux_file_fd);
        return 0;
      }


      /* get the imported file */
      MDS* imported_file = get_MDS(fd, imported_file_offset);
      /* check for errors */
      if (imported_file == NULL)
      {
        CLOSE_OR_DIE2(linux_file_fd);
        return 0;
      }


      /* if there is anything to import <=> the file is not empty */
      if (linux_file_size > 0)
      {
        /* copy the contents of the linux file to the corresponding cfs file */
        retval = copy_from_linux_to_cfs(fd, my_superblock, holes, imported_file, linux_file_fd, linux_file_size);
        /* check for errors */
        if (!retval)
        {
          free(imported_file);
          return 0;
        }
      }


      /* write the newly imported file to the cfs */
      retval = set_MDS(imported_file, fd, imported_file_offset);
      free(imported_file);
      /* check for errors */
      if (!retval)
      {
        return 0;
      }


      /* inform the superblock */
      my_superblock->current_size += sizeof(MDS);


      /* close the linux file */
      CLOSE_OR_DIE2(linux_file_fd);

      break;
    }

    /* if the entity is something else */
    default:
    {
      printf("Linux entity \"%s\" is neither a directory nor a file, therefore it can't be imported in the cfs.\n", linux_path_name);
      break;
    }

  }



  /* update the superblock */
  retval = set_superblock(my_superblock, fd);
  if (!retval)
  {
    return 0;
  }

  /* update the hole map */
  retval = set_hole_map(holes, fd);
  if (!retval)
  {
    return 0;
  }

  /* update the destination directory */
  retval = set_MDS(destination_directory, fd, destination_offset);
  if (!retval)
  {
    return 0;
  }

  /* return 1 if everything goes smoothly */
  return 1;
}



int cfs_export(int fd, superblock* my_superblock, MDS* source, char* linux_path_name)
{
  /* get some important sizes */
  size_t block_size = my_superblock->block_size;
  size_t fns = my_superblock->filename_size;



  /* determine whether this entity to be exported is a file or a directory */
  switch (source->type)
  {

    /* if the entity to be exported is a directory */
    case DIRECTORY:
    {

      break;
    }

    /* if the entity to be exported is a file */
    case FILE:
    {
      /* create the linux file and get its fd */
      int linux_file_fd = open(linux_path_name, O_CREAT | O_WRONLY, PERMS);
      if (linux_file_fd == -1)
      {
        perror("open() when called from cfs_export()");
        return 0;
      }

      /* copy the contents of the cfs file to the corresponding linux file */
      int retval = copy_from_cfs_to_linux(fd, my_superblock, source, linux_file_fd);
      /* check for errors */
      if (!retval)
      {
        /* close the linux file */
        CLOSE_OR_DIE2(linux_file_fd);
        return 0;
      }

      /* close the linux file */
      CLOSE_OR_DIE2(linux_file_fd);

      break;
    }

    /* this should never happen */
    default:
    {

      break;
    }
  }



  /* return 1 if everything goes smoothly */
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

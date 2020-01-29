#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "functions.h"
#include "functions2.h"
#include "functions_util.h"
#include "util.h"
#include "list.h"



void free_mem(superblock** my_superblock, hole_map** holes, Stack_List** list)
{
  FREE_IF_NOT_NULL(*my_superblock);
  *my_superblock = NULL;

  FREE_IF_NOT_NULL(*holes);
  *holes = NULL;

  Stack_List_Destroy(list);
  *list = NULL;
}



int main(int argc, char* argv[])
{
  /* variable used to read input from the user */
  char buffer[MAX_BUFFER_SIZE] = {0};

  /* option given that will be derived from the buffer */
  int option = 0;

  /* file descriptor that will be used to store the currently open cfs file */
  int fd = -1;

  /* name of the current cfs file being used */
  char cfs_filename[MAX_CFS_FILENAME_SIZE] = {0};

  /* pointers used to fast access the data of the cfs file */
  superblock* my_superblock = NULL;
  hole_map* holes = NULL;
  Stack_List* list = NULL;

  /* useful information about the current cfs file that we are working with */
  size_t block_size = 0;
  size_t fns = 0;
  size_t max_entity_size = 0;
  uint max_number_of_files = 0;


  do {

    /* print the filename, if we are currently working with a file */
    if (cfs_filename[0] != 0)
    {
      // [0;32m	Green
      // [1;32m	Bold Green

      /* set print colour to Bold Green */
      printf("\033[1;32m");

      printf("%s", cfs_filename);

      /* set print colour back to normal */
      printf("\033[0m");
      printf(":");
    }

    Stack_List_Print_Directories(list, PRINT_HIERARCHY);
    printf("$ ");
    fgets(buffer, MAX_BUFFER_SIZE, stdin);
    option = get_option(buffer);



    switch (option)
    {
      case 1:
      {
        char new_cfs_filename[MAX_CFS_FILENAME_SIZE] = {0};
        if (!get_nth_string(new_cfs_filename, buffer, 2))
        {
          printf("Error input, the name of the cfs file to work with has to be given.\n");
          break;
        }

        superblock* new_superblock = NULL;
        hole_map* new_holes = NULL;
        Stack_List* new_list = NULL;

        int new_fd = cfs_workwith(new_cfs_filename, &new_superblock, &new_holes, &new_list);
        if (new_fd == -1)
        {
          printf("Error with the given cfs filename.\n");
          break;
        }

        free_mem(&my_superblock, &holes, &list);

        my_superblock = new_superblock;
        holes = new_holes;
        list = new_list;

        block_size = my_superblock->block_size;
        fns = my_superblock->filename_size;
        max_entity_size = my_superblock->max_file_size;
        max_number_of_files = my_superblock->max_dir_file_number;
        strcpy(cfs_filename, new_cfs_filename);

        /* close previous open file */
        if (fd != -1)
        {
          int ret = close(fd);
          if (ret == -1)
          {
            perror("Unexpected close() error when closing the previous cfs file. Exiting..");
            FREE_AND_CLOSE(my_superblock, holes, list, new_fd);

            return EXIT_FAILURE;
          }
        }

        fd = new_fd;

        break;
      }

      case 2:
      {

        BREAK_IF_NO_FILE_OPEN(fd);

        char* new_directory_name = malloc(fns * sizeof(char));
        if (new_directory_name == NULL)
        {
          perror("malloc() error");
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }

        if (!get_nth_string(new_directory_name, buffer, 2))
        {
          printf("Error input, at least 1 new directory has to be given.\n");
          free(new_directory_name);

          break;
        }

        /* get the info of the current directory that we are in */
        char* current_directory_name = malloc(fns * sizeof(char));
        if (current_directory_name == NULL)
        {
          perror("malloc() error");
          free(new_directory_name);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }
        off_t current_directory_offset = (off_t) 0;

        /* get the location of the MDS of the directory */
        int retval = Stack_List_Peek(list, &current_directory_name, &current_directory_offset);
        if (retval != 1)
        {
          printf("Stack_List_Peek() error in main() before calling cfs_mkdir().\n");
          free(new_directory_name);
          free(current_directory_name);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }

        /* free the name of the current directory because we don't need it anymore */
        free(current_directory_name);


        /* get the current directory */
        MDS* current_directory = get_MDS(fd, current_directory_offset);
        if (current_directory == NULL)
        {
          printf("get_MDS() error in main() before calling cfs_mkdir().\n");
          free(new_directory_name);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }


        int index = 2;
        /* for every directory name given */
        int directory_exists = get_nth_string(new_directory_name, buffer, index);
        while (directory_exists)
        {
          off_t directory_offset = directory_get_offset(fd, current_directory, block_size, fns, new_directory_name);
          if (directory_offset == (off_t) 0)
          {
            printf("Unexpected error occured in directory_get_offset(). Exiting..\n");
            free(current_directory);
            free(new_directory_name);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
          }
          else if (directory_offset != (off_t) -1)
          {
            printf("Cannot create directory '%s' because an entity with the same name already exists in the current directory.\n", new_directory_name);
            index++;
            directory_exists = get_nth_string(new_directory_name, buffer, index);
            if (directory_exists)
            {
              continue;
            }
            else
            {
              break;
            }
          }


          /* determine whether it can host a new sub-entity */
          if (!is_in_Root(list) && number_of_sub_entities_in_directory(current_directory, fns) == max_number_of_files)
          {
            printf("Can't create the directory '%s' because the current directory has already reached max number of sub-entites.\n", new_directory_name);
            break;
          }

          /* if we reach here it means that we can create a new directory inside
             the current one */
          retval = cfs_mkdir(fd, my_superblock, holes, current_directory, current_directory_offset, new_directory_name);
          if (!retval)
          {
            printf("Unexpected error occured in cfs_mkdir(). Exiting..\n");
            free(current_directory);
            free(new_directory_name);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);

            return EXIT_FAILURE;
          }

          index++;
          directory_exists = get_nth_string(new_directory_name, buffer, index);
        }

        free(current_directory);
        free(new_directory_name);

        break;
      }

      case 3:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        char* new_file_name = malloc(fns * sizeof(char));
        if (new_file_name == NULL)
        {
          perror("malloc() error");
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }


        if (!get_nth_string(new_file_name, buffer, 2))
        {
          printf("Error input, missing file operand.\n");
          free(new_file_name);

          break;
        }

        int flag_a = 0;
        int flag_m = 0;
        int valid_parameters = get_cfs_touch_parameters(buffer, &flag_a, &flag_m);
        if (!valid_parameters)
        {
          free(new_file_name);
          break;
        }
        else if (flag_a && flag_m)
        {
          printf("Error: parameters -a and -m cannot be given at the same time.");
          free(new_file_name);
          break;
        }


        /* get the info of the current directory that we are in */
        char* current_directory_name = malloc(fns * sizeof(char));
        if (current_directory_name == NULL)
        {
          perror("malloc() error");
          free(new_file_name);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }
        off_t current_directory_offset = (off_t) 0;

        /* get the location of the MDS of the directory */
        int retval = Stack_List_Peek(list, &current_directory_name, &current_directory_offset);
        if (retval != 1)
        {
          printf("Stack_List_Peek() error in main() before calling cfs_touch().\n");
          free(new_file_name);
          free(current_directory_name);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }

        /* free the name of the current directory because we don't need it anymore */
        free(current_directory_name);


        /* get the current directory */
        MDS* current_directory = get_MDS(fd, current_directory_offset);
        if (current_directory == NULL)
        {
          printf("get_MDS() error in main() before calling cfs_touch().\n");
          free(new_file_name);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }


        int index = 2;
        /* skip the parameter (options) strings */
        while (is_parameter(new_file_name))
        {
          index++;
          get_nth_string(new_file_name, buffer, index);
        }

        /* for every directory name given */
        int file_exists = get_nth_string(new_file_name, buffer, index);
        while (file_exists)
        {
          off_t file_offset = directory_get_offset(fd, current_directory, block_size, fns, new_file_name);
          if (file_offset == (off_t) 0)
          {
            printf("Unexpected error occured in directory_get_offset(). Exiting..\n");
            free(current_directory);
            free(new_file_name);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
          }
          else if (file_offset != (off_t) -1 && (!flag_a || !flag_m))
          {
            printf("Cannot create file '%s' because an entity with the same name already exists in the current directory.\n", new_file_name);
            index++;
            file_exists = get_nth_string(new_file_name, buffer, index);
            if (file_exists)
            {
              continue;
            }
            else
            {
              break;
            }
          }


          /* determine whether it can host a new sub-entity */
          if (!is_in_Root(list) && number_of_sub_entities_in_directory(current_directory, fns) == max_number_of_files)
          {
            printf("Can't create the file '%s' because the current directory has already reached max number of sub-entites.\n", new_file_name);
            break;
          }

          /* if we reach here it means that we can create/modify a file inside the current directory */
          retval = cfs_touch(fd, my_superblock, holes, current_directory, current_directory_offset, new_file_name, file_offset, flag_a, flag_m);
          if (!retval)
          {
            printf("Unexpected error occured in cfs_touch(). Exiting..\n");
            free(current_directory);
            free(new_file_name);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);

            return EXIT_FAILURE;
          }

          index++;
          file_exists = get_nth_string(new_file_name, buffer, index);
        }


        free(current_directory);
        free(new_file_name);

        break;
      }

      case 4:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        cfs_pwd(fd, my_superblock, list);

        break;
      }

      case 5:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        char spam[MAX_BUFFER_SIZE] = {0};
        if (get_nth_string(spam, buffer, 3))
        {
          printf("Error: too many arguments for cd.\n");
          break;
        }

        char path[MAX_BUFFER_SIZE] = {0};
        /* get the path */
        get_nth_string(path, buffer, 2);


        /* handle too big paths that overflow the path array */
        if (strlen(path) == MAX_BUFFER_SIZE && path[MAX_BUFFER_SIZE - 1] != 0)
        {
          printf("Path given is too big. Give a path that has totally less than %d characters.\n", MAX_BUFFER_SIZE);
          break;
        }

        /* create a copy of the list (of the current path) */
        Stack_List* copy_list = copy_List(list);
        if (copy_list == NULL)
        {
          printf("Error: copy_List() returned NULL.\n");
          break;
        }

        int retval = cfs_cd(fd, my_superblock, copy_list, path);
        /* check if the operation failed */
        if (retval == 0)
        {
          Stack_List_Destroy(&copy_list);
          break;
        }

        /* "destroy" the previous path */
        Stack_List_Destroy(&list);

        /* get the new path */
        list = copy_list;

        break;
      }

      case 6:
      {
        char* cur_name = malloc(MAX_CFS_FILENAME_SIZE * sizeof(char));
        off_t cur_offset;
        Stack_List_Peek(list, &cur_name, &cur_offset);

        cfs_ls(fd, cur_offset);
        free(cur_name);
        break;
      }

      case 7:
      {

        break;
      }

      case 8:
      {
        /* array used to read strings from the user input */
        char read_input[MAX_BUFFER_SIZE] = {0};

        int index = 2;
        int exists = get_nth_string(read_input, buffer, index);
        /* count and skip the source files */
        while (exists && strcmp(read_input, "-o"))
        {
          index++;
          exists = get_nth_string(read_input, buffer, index);
        }

        /* check for errors */
        if (index == 2)
        {
          printf("Error input: you must give at least one source file.\n");
          break;
        }
        if (!exists)
        {
          printf("Error input: you must give the flag \"-o\" to mark the destination file.\n");
          break;
        }

        /* how many source files are being added up */
        int total_sources = index - 2;

        /* array used to store the name of the output file */
        char destination_file_path[MAX_BUFFER_SIZE] = {0};
        index++;

        /* make sure that a destination is given */
        if (!get_nth_string(destination_file_path, buffer, index))
        {
          printf("Error input: you must give a destination file.\n");
          break;
        }

        /* make sure that no more than 1 destinations are given */
        if (get_nth_string(read_input, buffer, index + 1))
        {
          printf("Error input: only 1 destination file can be given.\n");
          break;
        }


        /* extract the name of the destination path, and check if a file with
           the same name already exists */
        char destination_file_name[MAX_BUFFER_SIZE] = {0};
        extract_last_entity_from_path(destination_file_path, destination_file_name);

        Stack_List* destination_path_list = copy_List(list);
        if (destination_path_list == NULL)
        {
          break;
        }

        /* check if the output file is to be placed in another directory. If so,
           then build the list to point at that directory */
        if (destination_file_path[0] != 0)
        {
          int retval = cfs_cd(fd, my_superblock, destination_path_list, destination_file_path);
          /* check if the operation failed */
          if (retval == 0)
          {
            Stack_List_Destroy(&destination_path_list);
            break;
          }
        }


        /* get the offset of the current directory, in order to get the directory */
        off_t destination_directory_offset = Stack_List_Peek_offset(destination_path_list);
        if (destination_directory_offset == (off_t) 0)
        {
          Stack_List_Destroy(&destination_path_list);
          printf("Error in Stack_List_Peek_offset() when called from main:: cfs_cat.\n");
          break;
        }


        /* get the current directory */
        MDS* destination_file_directory = get_MDS(fd, destination_directory_offset);
        if (destination_file_directory == NULL)
        {
          Stack_List_Destroy(&destination_path_list);
          break;
        }


        /* check if the file already exists in the directory we want to place it */
        off_t destination_file_offset = directory_get_offset(fd, destination_file_directory, block_size, fns, destination_file_name);
        if (destination_file_offset == 0)
        {
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          break;
        }
        else if (destination_file_offset != (off_t) -1)
        {
          if (destination_file_path[0] == 0)
          {
            printf("Error input: a file with the same name as the output file already exists in the current directory.\n");
          }
          else
          {
            printf("Error input: a file with the same name as the output file already exists in the %s directory.\n", destination_file_path);
          }
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          break;
        }

        /* create the output file, in order to concatenate to it the source files */
        int retval = cfs_touch(fd, my_superblock, holes, destination_file_directory, destination_directory_offset, destination_file_name, -1, 0, 0);
        if (!retval)
        {
          printf("Unexpected error occured in cfs_cat(). Exiting..\n");
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }


        /* now actually get the offset of the destination file inside its directory */
        destination_file_offset = directory_get_offset(fd, destination_file_directory, block_size, fns, destination_file_name);
        if (destination_file_offset == 0)
        {
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          break;
        }

        /* we do not need the MDS of the directory anymore */
        free(destination_file_directory);

        /* get the file to change its attributes */
        MDS* destination_file = get_MDS(fd, destination_file_offset);
        if (destination_file_directory == NULL)
        {
          Stack_List_Destroy(&destination_path_list);
          break;
        }


        int i = 0;
        size_t destination_file_size = 0;
        for (; i < total_sources; i++)
        {
          /* get the path of the file */
          get_nth_string(read_input, buffer, i + 2);

          /* get the offset of the source file */
          off_t source_file_offset = get_offset_from_path(fd, my_superblock, list, read_input);
          if (source_file_offset == (off_t) 0)
          {
            continue;
          }

          /* get the source file */
          MDS* source_file = get_MDS(fd, source_file_offset);
          /* check for errors */
          if (source_file == 0)
          {
            continue;
          }
          else if (source_file->type != FILE)
          {
            char last_entity_name[MAX_BUFFER_SIZE] = {0};
            extract_last_entity_from_path(read_input, last_entity_name);
            printf("Error input: entity named %s is not a file.\n", last_entity_name);
            free(source_file);

            continue;
          }
          else if (destination_file_size + source_file->size > max_entity_size)
          {
            char last_entity_name[MAX_BUFFER_SIZE] = {0};
            extract_last_entity_from_path(read_input, last_entity_name);
            printf("Error input: entity named %s can't be concatenated to file %s because if would exceed max capacity of file.\n", last_entity_name, destination_file_name);
            free(source_file);

            continue;
          }

          /* if source file is not empty, concatenate */
          if (source_file->size > 0)
          {
            cfs_cat(fd, my_superblock, holes, destination_file, destination_file_offset, source_file);
          }

          /* increment size */
          destination_file_size += source_file->size;
          /* free source file */
          free(source_file);

          memset(read_input, 0, MAX_BUFFER_SIZE);
        }


        /* update the superblock */
        my_superblock->current_size += sizeof(MDS) + block_size * destination_file->blocks_using;
        retval = set_superblock(my_superblock, fd);
        if (!retval)
        {
          free(destination_file);
          Stack_List_Destroy(&destination_path_list);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }


        /* free up the used space */
        free(destination_file);
        Stack_List_Destroy(&destination_path_list);

        break;
      }

      case 9:
      {

        break;
      }

      case 10:
      {

        break;
      }

      case 11:
      {

        break;
      }

      case 12:
      {

        break;
      }

      case 13:
      {

        break;
      }

      case 14:
      {
        size_t bs = 0;
        size_t fns = 0;
        size_t cfs = 0;
        uint mdfn = 0;

        char new_cfs_filename[MAX_CFS_FILENAME_SIZE] = {0};

        int retval = get_cfs_create_parameters(buffer, &bs, &fns, &cfs, &mdfn, new_cfs_filename);
        if (!retval)
        {
          // printf("Error while reading the parameters of cfs_create.\n");
          break;
        }

        retval = cfs_create(new_cfs_filename, bs, fns, cfs, mdfn);
        if (retval == -1)
        {
          printf("Error in cfs_create().\n");
          break;
        }

        break;
      }

      case 15:
      {
        printf("\nTerminating the program..\n");

        break;
      }

      default:
      {
        char wrong_input[MAX_BUFFER_SIZE] = {0};
        if (get_nth_string(wrong_input, buffer, 1))
        {
          printf("Unknown command: '%s'\n", wrong_input);
        }

        break;
      }
    }

  } while (option != 15);

  if (max_entity_size || block_size);

  free_mem(&my_superblock, &holes, &list);

  if (fd > 0)
  {
    close(fd);
  }
  return EXIT_SUCCESS;
}
